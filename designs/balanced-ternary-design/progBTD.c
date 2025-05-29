#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Structure for moves in the search */
struct Move {
    int row;
    int col1;
    int col2;
    int delta;  // change in penalty if this move is applied
};

int main(int argc, char **argv) {
    if (argc < 9) {
        fprintf(stderr, "Usage: %s V B p1 p2 R K L seed [tabu_tenure] [random_move_probability]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Parse required command-line parameters */
    int V = atoi(argv[1]);
    int B = atoi(argv[2]);
    int p1 = atoi(argv[3]);
    int p2 = atoi(argv[4]);
    int R  = atoi(argv[5]);  // must equal p1 + 2*p2
    int K  = atoi(argv[6]);
    int L  = atoi(argv[7]);
    int seed = atoi(argv[8]);
    srand(seed);

    /* Extra hyperparameters – if not provided, use defaults:
         tabu_tenure: integer, default 10.
         random_move_probability: double, default 0.05.
    */
    int tabu_tenure = 10; 
    double random_move_probability = 0.05;
    if (argc > 9) {
        tabu_tenure = atoi(argv[9]);
    }
    if (argc > 10) {
        random_move_probability = atof(argv[10]);
    }

    /* Check basic consistency: total row sum must equal total column sum, i.e. V*R == B*K */
    if (V * R != B * K) {
        fprintf(stderr, "Inconsistent parameters: V * R (%d) != B * K (%d)\n", V*R, B*K);
        exit(EXIT_FAILURE);
    }

    /* Allocate and initialize the incidence matrix M[V][B].
       For each row we randomly assign exactly p1 ones and p2 twos (the rest are 0).
    */
    int **M = malloc(V * sizeof(int *));
    if (!M) { perror("malloc M"); exit(EXIT_FAILURE); }
    for (int v = 0; v < V; v++) {
        M[v] = malloc(B * sizeof(int));
        if (!M[v]) { perror("malloc M[v]"); exit(EXIT_FAILURE); }
        /* initialize to all zeros */
        for (int b = 0; b < B; b++)
            M[v][b] = 0;
        /* Create an index array for columns to shuffle */
        int *indices = malloc(B * sizeof(int));
        if (!indices) { perror("malloc indices"); exit(EXIT_FAILURE); }
        for (int b = 0; b < B; b++)
            indices[b] = b;
        /* Fisher-Yates shuffle */
        for (int b = 0; b < B; b++) {
            int r = b + rand() % (B - b);
            int tmp = indices[b];
            indices[b] = indices[r];
            indices[r] = tmp;
        }
        /* Set first p1 positions to 1, next p2 positions to 2 */
        for (int j = 0; j < p1; j++) {
            M[v][indices[j]] = 1;
        }
        for (int j = p1; j < p1 + p2; j++) {
            M[v][indices[j]] = 2;
        }
        free(indices);
    }

    /* Build the column sum array colSum[B] */
    int *colSum = malloc(B * sizeof(int));
    if (!colSum) { perror("malloc colSum"); exit(EXIT_FAILURE); }
    for (int b = 0; b < B; b++) {
        colSum[b] = 0;
        for (int v = 0; v < V; v++)
            colSum[b] += M[v][b];
    }

    /* Allocate and compute pairwise dot products into matrix P[V][V]. 
       For each pair (v,w) we set P[v][w] = sum_{b} M[v][b]*M[w][b]. We maintain full symmetry.
    */
    int **P = malloc(V * sizeof(int *));
    if (!P) { perror("malloc P"); exit(EXIT_FAILURE); }
    for (int v = 0; v < V; v++) {
        P[v] = malloc(V * sizeof(int));
        if (!P[v]) { perror("malloc P[v]"); exit(EXIT_FAILURE); }
        for (int w = 0; w < V; w++) {
            P[v][w] = 0;
        }
    }
    for (int v = 0; v < V; v++) {
        for (int w = v+1; w < V; w++) {
            int dot = 0;
            for (int b = 0; b < B; b++)
                dot += M[v][b] * M[w][b];
            P[v][w] = dot;
            P[w][v] = dot;
        }
    }

    /* Allocate the 3D tabu list array tabu[V][B][B].
       For each row v, and each unordered pair of columns (b,c) we record an iteration number until which that swap is disallowed.
       (We use both tabu[v][b][c] and tabu[v][c][b] to simplify lookups.)
    */
    int ***tabu = malloc(V * sizeof(int **));
    if (!tabu) { perror("malloc tabu"); exit(EXIT_FAILURE); }
    for (int v = 0; v < V; v++) {
        tabu[v] = malloc(B * sizeof(int *));
        if (!tabu[v]) { perror("malloc tabu[v]"); exit(EXIT_FAILURE); }
        for (int b = 0; b < B; b++) {
            tabu[v][b] = malloc(B * sizeof(int));
            if (!tabu[v][b]) { perror("malloc tabu[v][b]"); exit(EXIT_FAILURE); }
            for (int c = 0; c < B; c++)
                tabu[v][b][c] = 0;  // initial value zero means not tabu
        }
    }

    /* Compute the initial overall penalty.
       Column penalty: for each column b, add |colSum[b] - K|.
       Pair penalty: for each pair (v,w) with v<w, add |P[v][w] - L|.
    */
    int current_penalty = 0;
    for (int b = 0; b < B; b++)
        current_penalty += abs(colSum[b] - K);
    for (int v = 0; v < V; v++)
        for (int w = v+1; w < V; w++)
            current_penalty += abs(P[v][w] - L);

    /* Begin the main tabu search loop. We keep running until a solution (penalty==0) is found. */
    long long iter = 0;  // iteration counter
    while (1) {
        iter++;

        /* We'll scan all allowed swap–moves.
           A move is defined by selecting one row v and two distinct columns b and c (with b < c) such that M[v][b] != M[v][c].
           Swapping these two entries leaves the row multiset unchanged.
           We compute the change (delta) in penalty from swapping M[v][b] and M[v][c].
           (The affected parts: columns b and c and all pairs that involve row v.)
        */
        int candidate_count = 0;
        int candidate_capacity = V * B * B; // upper bound on number of moves
        struct Move *candidates = malloc(candidate_capacity * sizeof(struct Move));
        if (!candidates) { perror("malloc candidates"); exit(EXIT_FAILURE); }
        
        struct Move best_allowed_move;
        best_allowed_move.delta = 1000000000;
        struct Move best_tabu_move;
        best_tabu_move.delta = 1000000000;
        int tabu_found = 0;

        for (int v = 0; v < V; v++) {
            for (int b = 0; b < B; b++) {
                for (int c = b+1; c < B; c++) {
                    if (M[v][b] == M[v][c])
                        continue;  // swapping equal entries would change nothing.

                    int a = M[v][b]; // value at column b in row v
                    int d = M[v][c]; // value at column c in row v

                    /* ---- Column penalty change ---- */
                    int old_penalty_col_b = abs(colSum[b] - K);
                    int old_penalty_col_c = abs(colSum[c] - K);
                    int new_col_b = colSum[b] - a + d;
                    int new_col_c = colSum[c] - d + a;
                    int new_penalty_col_b = abs(new_col_b - K);
                    int new_penalty_col_c = abs(new_col_c - K);
                    int delta_col = (new_penalty_col_b - old_penalty_col_b) + (new_penalty_col_c - old_penalty_col_c);

                    /* ---- Pair penalty change for pairs involving row v ---- */
                    int delta_pair = 0;
                    for (int w = 0; w < V; w++) {
                        if (w == v)
                            continue;
                        int old_pair = (v < w) ? P[v][w] : P[w][v];
                        /* In row v, the contribution from column b changes from 'a' to 'd',
                           and from column c changes from 'd' to 'a'. For row w the contributions are M[w][b] and M[w][c].
                           Hence the change is: (d - a)*(M[w][b] - M[w][c]). */
                        int change = (d - a) * (M[w][b] - M[w][c]);
                        int new_pair = old_pair + change;
                        delta_pair += abs(new_pair - L) - abs(old_pair - L);
                    }

                    int delta = delta_col + delta_pair;
                    int new_penalty = current_penalty + delta;

                    /* Check the tabu condition.
                       We use tabu[v][b][c] (and tabu[v][c][b]) to store the iteration number until which a move is forbidden.
                       Here a move is allowed if the current iteration is greater than or equal to tabu[v][b][c],
                       or if the move yields a valid solution (new_penalty==0, aspiration criterion). */
                    int move_is_tabu = (iter < tabu[v][b][c]);
                    int allowed = (!move_is_tabu) || (new_penalty == 0);

                    struct Move move;
                    move.row = v;
                    move.col1 = b;
                    move.col2 = c;
                    move.delta = delta;

                    if (allowed) {
                        candidates[candidate_count++] = move;
                        if (delta < best_allowed_move.delta)
                            best_allowed_move = move;
                    } else {
                        if (delta < best_tabu_move.delta) {
                            best_tabu_move = move;
                            tabu_found = 1;
                        }
                    }
                }
            }
        }
        
        /* Decide which move to make:
           If there is at least one non-tabu (allowed) move, then with probability random_move_probability pick one at random;
           otherwise choose the best allowed move.
           If there are no allowed moves, then choose the best tabu move (backup measure). */
        struct Move chosen_move;
        if (candidate_count > 0) {
            double r = (double)rand() / RAND_MAX;
            if (r < random_move_probability) {
                int idx = rand() % candidate_count;
                chosen_move = candidates[idx];
            } else {
                chosen_move = best_allowed_move;
            }
        } else if (tabu_found) {
            chosen_move = best_tabu_move;
        } else {
            free(candidates);
            continue;
        }
        free(candidates);

        /* ---- Execute the chosen move ---- */
        int v = chosen_move.row;
        int b = chosen_move.col1;
        int c = chosen_move.col2;
        int a = M[v][b];
        int d = M[v][c];

        /* Swap the two entries in row v */
        M[v][b] = d;
        M[v][c] = a;

        /* Update the column sums for columns b and c */
        colSum[b] = colSum[b] - a + d;
        colSum[c] = colSum[c] - d + a;

        /* Update all pair dot products that involve the changed row v.
           For any other row w (w != v), the updated dot product changes by
              delta = (d - a) * (M[w][b] - M[w][c]).
           We update both P[v][w] (or P[w][v]) accordingly.
        */
        for (int w = 0; w < V; w++) {
            if (w == v)
                continue;
            int change = (d - a) * (M[w][b] - M[w][c]);
            if (v < w) {
                P[v][w] += change;
                P[w][v] = P[v][w];
            } else {
                P[w][v] += change;
                P[v][w] = P[w][v];
            }
        }

        /* Update the overall current penalty.
           For robustness (and to avoid accumulation of rounding mistakes in delta),
           we recalc the complete penalty each iteration.
        */
        int recalculated_penalty = 0;
        for (int b = 0; b < B; b++)
            recalculated_penalty += abs(colSum[b] - K);
        for (int i = 0; i < V; i++)
            for (int j = i+1; j < V; j++)
                recalculated_penalty += abs(P[i][j] - L);
        current_penalty = recalculated_penalty;

        /* Mark the executed move in the tabu list. Note we mark both orders.
           The move (v,{b,c}) is forbidden until iteration (current_iter + tabu_tenure).
        */
        tabu[v][b][c] = iter + tabu_tenure;
        tabu[v][c][b] = iter + tabu_tenure;

        /* If a valid design has been reached, print the final incidence matrix and exit */
        if (current_penalty == 0) {
            for (int i = 0; i < V; i++) {
                for (int j = 0; j < B; j++) {
                    printf("%d ", M[i][j]);
                }
                printf("\n");
            }
            /* Clean up */
            for (int i = 0; i < V; i++)
                free(M[i]);
            free(M);
            free(colSum);
            for (int i = 0; i < V; i++)
                free(P[i]);
            free(P);
            for (int v = 0; v < V; v++) {
                for (int b = 0; b < B; b++)
                    free(tabu[v][b]);
                free(tabu[v]);
            }
            free(tabu);
            return 0;
        }
        /* (No status messages are printed; the loop will continue indefinitely until a solution is found.) */
    }

    return 0;
}