/*
 * unsqs.c
 *
 * This program builds a Uniform Nested Steiner Quadruple System UNSQS(v,p).
 *
 * The approach is in two phases:
 *
 * 1. SQS construction by exact cover.
 *    We list all 4–subsets (candidate blocks) of V={0,...,v-1} and use a
 *    recursive backtracking (Algorithm X–style) method to select a family of
 *    blocks so that every triple is contained in exactly one block.
 *
 * 2. Tabu search optimization for ND–pair splitting.
 *    Each block (of 4 numbers) may be split as {a,b}+{c,d}, {a,c}+{b,d} or {a,d}+{b,c}.
 *    We choose one option per block so that among all blocks (each contributing two ND–pairs)
 *    exactly p distinct pairs appear and each appears exactly F_target times,
 *       where F_target = (v*(v-1)*(v-2))/(12*p).
 *
 * The program reads command–line arguments:
 *    v p seed [tabu_tenure] [penalty_weight] [random_move_prob]
 *
 * When a valid UNSQS is found the program prints out one block per line (four space separated numbers);
 * the first two numbers form the first ND–pair and the last two the second ND–pair.
 *
 * Compile with: gcc unsqs.c -O2 -o unsqs -lm
 *
 * Note: For performance and brevity this implementation is “all in one file”.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Maximum v (points) */
#define MAX_V 60

/*-----------------------------------------------------------
  Global variables used in the exact--cover part.
-----------------------------------------------------------*/

/* triple_index[i][j][k] for i<j<k, mapping the triple (i,j,k) to a unique index. */
int triple_index[MAX_V][MAX_V][MAX_V];

/* Structure for a candidate SQS block (a 4–subset) */
typedef struct {
    int vertices[4];   /* points in increasing order */
    int triples[4];    /* indices of the 4 triples in the block:
                          (v0,v1,v2), (v0,v1,v3), (v0,v2,v3), (v1,v2,v3) */
} Block;

/* Global variables for exact cover search */
int V;                   // current order v (set in main)
int T;                   // total number of triples = C(v,3)
int required_blocks;     // number of blocks in an SQS = v*(v-1)*(v-2)/24
Block *candidates = NULL;  // Array (of size candidate_count) of candidate blocks (all 4–subsets)
int candidate_count = 0;     // total number of candidate blocks

bool *triple_covered = NULL;  // Array (size T) indicating whether a given triple is covered
int **triple_candidates = NULL;  // For each triple (index 0..T-1), a list (array) of candidate block indices that cover it.
int *triple_candidate_total = NULL; // For each triple, the total number of candidate blocks covering it.
int *solution = NULL;   // Array (of length required_blocks) to hold a candidate block index for each block in the SQS.
 
/* Recursive backtracking search (Algorithm X style) for an exact cover for all triples.
   Returns true if a full cover is found (and the chosen candidate indices are stored in solution). */
bool search_exact_cover(int level) {
    if (level == required_blocks) {
        /* All blocks chosen. Verify that every triple is covered. */
        for (int t = 0; t < T; t++) {
            if (!triple_covered[t])
                return false;
        }
        return true;
    }
    /* Choose an uncovered triple with smallest candidate total (heuristic) */
    int min_count = 1000000;
    int chosen_t = -1;
    for (int t = 0; t < T; t++) {
        if (!triple_covered[t]) {
            if (triple_candidate_total[t] < min_count) {
                min_count = triple_candidate_total[t];
                chosen_t = t;
                if (min_count == 0)
                    break; /* no candidate covers this triple */
            }
        }
    }
    if (chosen_t == -1)
        return false;  /* All covered? Should not happen here. */

    /* For each candidate block that covers triple chosen_t, try it */
    int total_options = triple_candidate_total[chosen_t];
    for (int i = 0; i < total_options; i++) {
        int block_idx = triple_candidates[chosen_t][i];
        /* Check if candidate block is available (all its quadruple’s triples are uncovered) */
        bool available = true;
        for (int j = 0; j < 4; j++) {
            int t_idx = candidates[block_idx].triples[j];
            if (triple_covered[t_idx]) { available = false; break; }
        }
        if (!available)
            continue;
        /* Choose candidate block: mark its 4 triples as covered */
        int marked[4];
        for (int j = 0; j < 4; j++) {
            int t_idx = candidates[block_idx].triples[j];
            triple_covered[t_idx] = true;
            marked[j] = t_idx;
        }
        solution[level] = block_idx;
        if (search_exact_cover(level + 1))
            return true;
        /* Backtrack: unmark the 4 triples */
        for (int j = 0; j < 4; j++) {
            triple_covered[marked[j]] = false;
        }
    }
    return false;
}

/* This function generates an SQS(v) by solving the exact cover instance.
   It returns an array (of length required_blocks) of Blocks.
   (If no solution is found the program aborts.) */
Block* generate_SQS(int v) {
    V = v;
    /* Compute T = C(v,3) and fill triple_index */
    T = 0;
    for (int i = 0; i < v; i++) {
        for (int j = i+1; j < v; j++) {
            for (int k = j+1; k < v; k++) {
                triple_index[i][j][k] = T;
                T++;
            }
        }
    }
    required_blocks = (v * (v-1) * (v-2)) / 24;
    
    /* Generate all candidate blocks = all 4–subsets (C(v,4)) */
    int cand_estimate = (v*(v-1)*(v-2)*(v-3))/24;
    candidates = (Block*) malloc(cand_estimate * sizeof(Block));
    candidate_count = 0;
    for (int a = 0; a < v-3; a++) {
        for (int b = a+1; b < v-2; b++) {
            for (int c = b+1; c < v-1; c++) {
                for (int d = c+1; d < v; d++) {
                    candidates[candidate_count].vertices[0] = a;
                    candidates[candidate_count].vertices[1] = b;
                    candidates[candidate_count].vertices[2] = c;
                    candidates[candidate_count].vertices[3] = d;
                    /* The 4 triples in this block */
                    candidates[candidate_count].triples[0] = triple_index[a][b][c];
                    candidates[candidate_count].triples[1] = triple_index[a][b][d];
                    candidates[candidate_count].triples[2] = triple_index[a][c][d];
                    candidates[candidate_count].triples[3] = triple_index[b][c][d];
                    candidate_count++;
                }
            }
        }
    }
    
    /* For each triple (by index 0..T-1) build the list of candidate blocks that cover it. */
    triple_candidates = (int**) malloc(T * sizeof(int*));
    triple_candidate_total = (int*) malloc(T * sizeof(int));
    for (int t = 0; t < T; t++) {
        triple_candidate_total[t] = 0;
    }
    /* First pass: count */
    for (int i = 0; i < candidate_count; i++) {
        for (int j = 0; j < 4; j++) {
            int t_idx = candidates[i].triples[j];
            triple_candidate_total[t_idx]++;
        }
    }
    /* Allocate arrays */
    for (int t = 0; t < T; t++) {
        triple_candidates[t] = (int*) malloc(triple_candidate_total[t] * sizeof(int));
        /* reset the count to 0 so we can use it as an index */
        triple_candidate_total[t] = 0;
    }
    /* Second pass: fill the arrays */
    for (int i = 0; i < candidate_count; i++) {
        for (int j = 0; j < 4; j++) {
            int t_idx = candidates[i].triples[j];
            triple_candidates[t_idx][triple_candidate_total[t_idx]++] = i;
        }
    }
    
    /* Allocate and initialize triple_covered array */
    triple_covered = (bool*) malloc(T * sizeof(bool));
    for (int t = 0; t < T; t++)
        triple_covered[t] = false;
    
    solution = (int*) malloc(required_blocks * sizeof(int));
    
    if (!search_exact_cover(0)) {
        fprintf(stderr, "Failed to construct SQS for v=%d\n", v);
        exit(1);
    }
    
    /* Build the SQS blocks from the solution */
    Block *sqs = (Block*) malloc(required_blocks * sizeof(Block));
    for (int i = 0; i < required_blocks; i++) {
        int idx = solution[i];
        sqs[i] = candidates[idx];
    }
    
    /* Free allocated memory for the exact–cover part */
    for (int t = 0; t < T; t++) {
        free(triple_candidates[t]);
    }
    free(triple_candidates);
    free(triple_candidate_total);
    free(triple_covered);
    free(solution);
    free(candidates);
    
    return sqs;
}

/*-----------------------------------------------------------
  Phase 2: Tabu Search for ND–pair splitting.
-----------------------------------------------------------*/

/* Given a Block and a choice (option) in {0,1,2}, this function returns the two ND–pairs
   as an array “pair[2][2]”. (Each ND–pair is an unordered pair; here we always return them
   as (min, max).)
   Option 0: pair1 = (v0,v1), pair2 = (v2,v3)
   Option 1: pair1 = (v0,v2), pair2 = (v1,v3)
   Option 2: pair1 = (v0,v3), pair2 = (v1,v2)
*/
void get_split_pairs(Block block, int option, int pair[2][2]) {
    int a = block.vertices[0];
    int b = block.vertices[1];
    int c = block.vertices[2];
    int d = block.vertices[3];
    if (option == 0) {
        pair[0][0] = a; pair[0][1] = b;
        pair[1][0] = c; pair[1][1] = d;
    } else if (option == 1) {
        pair[0][0] = a; pair[0][1] = c;
        pair[1][0] = b; pair[1][1] = d;
    } else if (option == 2) {
        pair[0][0] = a; pair[0][1] = d;
        pair[1][0] = b; pair[1][1] = c;
    }
}

/* 
   Global tabu search variables:
   - sqs_blocks: the list of SQS blocks (size = required_blocks)
   - current_split: for each block, the current split (an integer in {0,1,2}).
   - pair_count_arr[i][j]: the current number of occurrences for the pair {i,j} (for i<j).
   - currentDistinct: number of pairs that currently occur at least once.
   - current_cost: the objective cost.
*/

Block *sqs_blocks = NULL;
int *current_split = NULL;   // (dimension: num_blocks = required_blocks)
int num_blocks;              // equal to required_blocks
int pair_count_arr[MAX_V][MAX_V];  // only entries with i<j are used.
int currentDistinct = 0;
double current_cost = 0.0;
long long iteration_counter = 0;
double best_cost = 1e9;

/* Hyperparameters (taken from command–line or defaults) */
int tabu_tenure;           // integer; default 7.
double penalty_weight;     // double; default 1000.0.
double random_move_prob;   // double; default 0.1.

/* F_target is computed as
      (v*(v-1)*(v-2))/(12*p)
   (and the input parameters are checked to guarantee that F_target is an integer) */
int F_target;
int P;   // the user–provided p value.

 
/* Compute the current cost.
   Cost = (penalty_weight)*|currentDistinct – P| + sum_{i<j, pair_count_arr[i][j]>0} (pair_count_arr[i][j]-F_target)^2
*/
double compute_cost() {
    double cost = 0.0;
    cost += penalty_weight * fabs(currentDistinct - P);
    for (int i = 0; i < V; i++) {
        for (int j = i+1; j < V; j++) {
            if (pair_count_arr[i][j] > 0) {
                int diff = pair_count_arr[i][j] - F_target;
                cost += diff * diff;
            }
        }
    }
    return cost;
}

/* Update the global pair_count_arr by processing the block's ND–pairs for split option “option”
   and adding (delta=+1) or removing (delta=-1) the counts. */
void update_pair_count(Block block, int option, int delta) {
    int pairs[2][2];
    get_split_pairs(block, option, pairs);
    for (int k = 0; k < 2; k++) {
        int a = pairs[k][0], b = pairs[k][1];
        if (a > b) { int tmp = a; a = b; b = tmp; }
        pair_count_arr[a][b] += delta;
    }
}

/* Compute the change (delta cost) if we change block i from its current split to new_option.
   This looks at the two “old” pairs being removed and the two “new” pairs being added.
   It computes the change in the quadratic penalty plus the penalty for distinct ND–pairs.
*/
double delta_cost_for_move(int i, int new_option) {
    Block block = sqs_blocks[i];
    int old_option = current_split[i];
    int old_pairs[2][2], new_pairs[2][2];
    get_split_pairs(block, old_option, old_pairs);
    get_split_pairs(block, new_option, new_pairs);
    
    double d_cost = 0.0;
    int d_distinct = 0;  // change in the number of distinct pairs.
    
    /* For each pair removed */
    for (int k = 0; k < 2; k++) {
        int a = old_pairs[k][0], b = old_pairs[k][1];
        if (a > b) { int tmp = a; a = b; b = tmp; }
        int old_count = pair_count_arr[a][b];
        int new_count = old_count - 1;
        double old_contrib = (old_count > 0) ? ((old_count - F_target) * (old_count - F_target)) : 0;
        double new_contrib = (new_count > 0) ? ((new_count - F_target) * (new_count - F_target)) : 0;
        d_cost += (new_contrib - old_contrib);
        if (old_count == 1)
            d_distinct -= 1;
    }
    /* For each pair added */
    for (int k = 0; k < 2; k++) {
        int a = new_pairs[k][0], b = new_pairs[k][1];
        if (a > b) { int tmp = a; a = b; b = tmp; }
        int old_count = pair_count_arr[a][b];
        int new_count = old_count + 1;
        double old_contrib = (old_count > 0) ? ((old_count - F_target) * (old_count - F_target)) : 0;
        double new_contrib = ((new_count - F_target) * (new_count - F_target));
        d_cost += (new_contrib - old_contrib);
        if (old_count == 0)
            d_distinct += 1;
    }
    int old_distinct = currentDistinct;
    int new_distinct = currentDistinct + d_distinct;
    double old_penalty = penalty_weight * fabs(old_distinct - P);
    double new_penalty = penalty_weight * fabs(new_distinct - P);
    d_cost += (new_penalty - old_penalty);
    return d_cost;
}

/* 
   Tabu search: globally, we try local moves: for each block, try switching its splitting option.
   Moves that reverse recent changes are “tabu” for a number of iterations given by tabu_tenure.
   With a small probability (random_move_prob) we pick a random move to help escape local minima.
   The search continues until the cost becomes 0.
*/
void tabu_search() {
    num_blocks = required_blocks;
    /* Allocate and initialize current_split: start with option 0 for every block */
    current_split = (int*) malloc(num_blocks * sizeof(int));
    for (int i = 0; i < num_blocks; i++) {
        current_split[i] = 0;
    }
    /* Initialize pair_count_arr to 0 */
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++)
            pair_count_arr[i][j] = 0;
    /* For each block, add its ND–pairs (using current_split) */
    for (int i = 0; i < num_blocks; i++) {
        update_pair_count(sqs_blocks[i], current_split[i], 1);
    }
    /* Compute currentDistinct */
    currentDistinct = 0;
    for (int i = 0; i < V; i++) {
        for (int j = i+1; j < V; j++) {
            if (pair_count_arr[i][j] > 0)
                currentDistinct++;
        }
    }
    current_cost = compute_cost();
    best_cost = current_cost;
    
    /* Allocate a tabu list: for each block i and each option 0,1,2,
       tabu[i][option] = iteration number until which changing block i to that option is forbidden.
    */
    int **tabu = (int**) malloc(num_blocks * sizeof(int*));
    for (int i = 0; i < num_blocks; i++) {
        tabu[i] = (int*) malloc(3 * sizeof(int));
        for (int j = 0; j < 3; j++) {
            tabu[i][j] = 0;
        }
    }
    
    /* Main search loop */
    while (true) {
        if (current_cost == 0.0)
            break;  /* Found a valid uniform solution */
        iteration_counter++;
        double best_move_improvement = 1e9;
        int best_i = -1;
        int best_new_option = -1;
        /* Examine all blocks and all moves */
        for (int i = 0; i < num_blocks; i++) {
            int curr_option = current_split[i];
            for (int option = 0; option < 3; option++) {
                if (option == curr_option)
                    continue;
                double d_cost = delta_cost_for_move(i, option);
                double candidate_cost = current_cost + d_cost;
                /* Disallow moves that are tabu unless they yield an aspiration improvement */
                if (iteration_counter < tabu[i][option] && candidate_cost >= best_cost)
                    continue;
                if (d_cost < best_move_improvement) {
                    best_move_improvement = d_cost;
                    best_i = i;
                    best_new_option = option;
                }
            }
        }
        /* If no move found, choose a random move */
        if (best_i == -1) {
            best_i = rand() % num_blocks;
            int curr = current_split[best_i];
            best_new_option = (curr + 1 + rand() % 2) % 3;
        } else {
            /* With probability random_move_prob, do a random move */
            double r = ((double) rand()) / RAND_MAX;
            if (r < random_move_prob) {
                best_i = rand() % num_blocks;
                int curr = current_split[best_i];
                best_new_option = (curr + 1 + rand() % 2) % 3;
            }
        }
        /* Apply the chosen move on block best_i: update pair_count_arr, current_stamp, distinct count, etc. */
        Block blk = sqs_blocks[best_i];
        int old_option = current_split[best_i];
        int old_pairs[2][2], new_pairs[2][2];
        get_split_pairs(blk, old_option, old_pairs);
        get_split_pairs(blk, best_new_option, new_pairs);
        for (int k = 0; k < 2; k++) {
            int a = old_pairs[k][0], b = old_pairs[k][1];
            if (a > b) { int tmp = a; a = b; b = tmp; }
            int oldc = pair_count_arr[a][b];
            pair_count_arr[a][b] = oldc - 1;
            if (oldc == 1)
                currentDistinct--;
        }
        for (int k = 0; k < 2; k++) {
            int a = new_pairs[k][0], b = new_pairs[k][1];
            if (a > b) { int tmp = a; a = b; b = tmp; }
            int oldc = pair_count_arr[a][b];
            pair_count_arr[a][b] = oldc + 1;
            if (oldc == 0)
                currentDistinct++;
        }
        current_split[best_i] = best_new_option;
        current_cost = compute_cost();
        if (current_cost < best_cost)
            best_cost = current_cost;
        /* Mark reverse move as tabu */
        tabu[best_i][old_option] = iteration_counter + tabu_tenure;
    }
    /* Free the tabu list */
    for (int i = 0; i < num_blocks; i++) {
        free(tabu[i]);
    }
    free(tabu);
    /* Do NOT free current_split because we want to use it later in output. */
}

/*-----------------------------------------------------------
  Main program.
-----------------------------------------------------------*/
 
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s v p seed [tabu_tenure] [penalty_weight] [random_move_prob]\n", argv[0]);
        return 1;
    }
    int v = atoi(argv[1]);
    P = atoi(argv[2]);
    int seed_input = atoi(argv[3]);
    srand(seed_input);
    V = v;
    
    /* Read hyperparameters (if provided) else use defaults */
    if (argc >= 5)
        tabu_tenure = atoi(argv[4]);
    else
        tabu_tenure = 7;
    if (argc >= 6)
        penalty_weight = atof(argv[5]);
    else
        penalty_weight = 1000.0;
    if (argc >= 7)
        random_move_prob = atof(argv[6]);
    else
        random_move_prob = 0.1;
    
    /* Check that total ND–pair occurrences divides by p.
       Total ND pair occurrences = (v*(v-1)*(v-2))/12
       F_target must be an integer. */
    int total_nd = (v * (v-1) * (v-2)) / 12;
    if (total_nd % P != 0) {
        fprintf(stderr, "Invalid parameters: v=%d, p=%d => (v*(v-1)*(v-2))/(12*p) is not an integer.\n", v, P);
        return 1;
    }
    F_target = total_nd / P;
    
    /* Phase 1: Construct a Steiner Quadruple System SQS(v) */
    sqs_blocks = generate_SQS(v);
    required_blocks = (v * (v-1) * (v-2)) / 24;
    
    /* Phase 2: Adjust ND–pair splits by tabu search until each distinct ND–pair appears F_target times
       and exactly p distinct pairs are used. */
    tabu_search();
    
    /* Finally, output the UNSQS(v,p): one block per line.
       In each block, the first two numbers give the first ND–pair and the last two give the second ND–pair.
    */
    for (int i = 0; i < required_blocks; i++) {
        int option = current_split[i];
        int pairs[2][2];
        get_split_pairs(sqs_blocks[i], option, pairs);
        printf("%d %d %d %d\n", pairs[0][0], pairs[0][1], pairs[1][0], pairs[1][1]);
    }
    
    free(sqs_blocks);
    free(current_split);
    
    return 0;
}