#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

/* Return a random double in [0,1). */
static inline double rand01(void) {
    return (double) rand() / ((double) RAND_MAX + 1.0);
}

/* Return a random sign: +1 or -1 */
static inline int random_sign(void) {
    return (rand() % 2 == 0) ? 1 : -1;
}

/*
 * Initialize the v×b matrix (stored in 1D array of length v*b, row–major) so that
 * each row gets exactly r nonzero entries and each column gets exactly k nonzero entries.
 * Nonzero entries are assigned a random sign (±1).
 */
bool init_matrix_valid(int *matrix, int v, int b, int r, int k) {
    int attempt;
    for (attempt = 0; attempt < 1000; attempt++) {
        // Reset matrix to 0.
        for (int i = 0; i < v * b; i++) {
            matrix[i] = 0;
        }
        int *row_remaining = malloc(v * sizeof(int));
        int *col_remaining = malloc(b * sizeof(int));
        if (!row_remaining || !col_remaining) {
            fprintf(stderr, "Memory allocation failure.\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < v; i++)
            row_remaining[i] = r;
        for (int j = 0; j < b; j++)
            col_remaining[j] = k;
        int total_pairs = v * b;
        int *order = malloc(total_pairs * sizeof(int));
        if (!order) {
            fprintf(stderr, "Memory allocation failure.\n");
            exit(EXIT_FAILURE);
        }
        for (int idx = 0; idx < total_pairs; idx++) {
            order[idx] = idx;
        }
        // Shuffle using Fisher–Yates.
        for (int i = total_pairs - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = order[i];
            order[i] = order[j];
            order[j] = temp;
        }
        // Greedily assign nonzeros.
        for (int idx = 0; idx < total_pairs; idx++) {
            int pos = order[idx];
            int i = pos / b;
            int j = pos % b;
            if (row_remaining[i] > 0 && col_remaining[j] > 0) {
                matrix[i * b + j] = random_sign();
                row_remaining[i]--;
                col_remaining[j]--;
            }
        }
        free(order);
        bool valid_incidence = true;
        for (int i = 0; i < v; i++) {
            if (row_remaining[i] != 0) {
                valid_incidence = false;
                break;
            }
        }
        for (int j = 0; j < b; j++) {
            if (col_remaining[j] != 0) {
                valid_incidence = false;
                break;
            }
        }
        free(row_remaining);
        free(col_remaining);
        if (valid_incidence)
            return true;
    }
    return false;
}

/*
 * Check whether the entire design is valid.
 * That is, for every row the number of nonzeros equals r,
 * for every column equals k, and for every pair of distinct rows the
 * common nonzeros count equals L with half of those having product +1.
 */
bool is_valid(int *matrix, int v, int b, int r, int k, int L) {
    for (int i = 0; i < v; i++) {
        int count = 0;
        for (int j = 0; j < b; j++) {
            if (matrix[i * b + j] != 0)
                count++;
        }
        if (count != r)
            return false;
    }
    for (int j = 0; j < b; j++) {
        int count = 0;
        for (int i = 0; i < v; i++) {
            if (matrix[i * b + j] != 0)
                count++;
        }
        if (count != k)
            return false;
    }
    for (int i = 0; i < v; i++) {
        for (int j = i+1; j < v; j++) {
            int common = 0, plus = 0, minus = 0;
            for (int col = 0; col < b; col++) {
                int a = matrix[i * b + col];
                int ap = matrix[j * b + col];
                if (a != 0 && ap != 0) {
                    common++;
                    if (a * ap > 0)
                        plus++;
                    else
                        minus++;
                }
            }
            if (common != L || plus != L/2 || minus != L/2)
                return false;
        }
    }
    return true;
}

/*
 * Pick (i,j) uniformly at random among cells that are nonzero.
 */
bool pick_random_nonzero(int *matrix, int v, int b, int *pi, int *pj) {
    int count = 0;
    for (int i = 0; i < v * b; i++) {
        if (matrix[i] != 0)
            count++;
    }
    if (count == 0)
        return false;
    int target = rand() % count;
    count = 0;
    for (int i = 0; i < v; i++) {
        for (int j = 0; j < b; j++) {
            if (matrix[i * b + j] != 0) {
                if (count == target) {
                    *pi = i;
                    *pj = j;
                    return true;
                }
                count++;
            }
        }
    }
    return false;
}

/*
 * A sign-flip move simply flips the sign at a cell (i,j).
 * This move preserves the row and column incidence.
 */
void flip_sign(int *matrix, int i, int j, int b) {
    matrix[i * b + j] = -matrix[i * b + j];
}

/*
 * Attempt a swap move between two rows.
 *
 * For two distinct rows i and p, we look for a pair of columns (colA, colB) such that:
 *   - In row i, colA is nonzero and colB is zero.
 *   - In row p, colA is zero and colB is nonzero.
 *
 * Then we “swap” the nonzeros between the rows.
 *
 * Returns true if a valid swap candidate was found.
 */
bool attempt_swap_move(int *matrix, int v, int b, int *row1, int *row2, int *col1, int *col2) {
    for (int attempt = 0; attempt < 50; attempt++) {
        int i = rand() % v;
        int p = rand() % v;
        if (i == p)
            continue;
        if (i > p) { int temp = i; i = p; p = temp; }
        int *listA = malloc(b * sizeof(int));
        int *listB = malloc(b * sizeof(int));
        if (!listA || !listB) {
            fprintf(stderr, "Memory allocation failure.\n");
            exit(EXIT_FAILURE);
        }
        int lenA = 0, lenB = 0;
        for (int col = 0; col < b; col++) {
            int a1 = matrix[i * b + col];
            int a2 = matrix[p * b + col];
            if (a1 != 0 && a2 == 0)
                listA[lenA++] = col;
            if (a1 == 0 && a2 != 0)
                listB[lenB++] = col;
        }
        if (lenA > 0 && lenB > 0) {
            int colA = listA[rand() % lenA];
            int colB = listB[rand() % lenB];
            free(listA);
            free(listB);
            *row1 = i; *row2 = p;
            *col1 = colA; *col2 = colB;
            return true;
        }
        free(listA);
        free(listB);
    }
    return false;
}

/* 
 * Structure to hold an update for a pair (i,j). For each unordered pair,
 * we want to update (or cache) the common and plus counts. (For a sign–flip,
 * the common count remains unchanged.)
 */
struct PairUpdate {
    int i;
    int j;
    int new_common;
    int new_plus;
    double old_pen;  // old penalty value
    double new_pen;  // new penalty value
};

int main(int argc, char *argv[]) {
    if (argc < 7) {
        fprintf(stderr, "Usage: %s v b r k L seed [flip_prob beta]\n", argv[0]);
        return 1;
    }
    int v = atoi(argv[1]);
    int b = atoi(argv[2]);
    int r = atoi(argv[3]);
    int k = atoi(argv[4]);
    int L = atoi(argv[5]);
    int seed = atoi(argv[6]);
    srand(seed);
    
    // Hyperparameters
    double flip_prob = 0.5;  // default
    double beta = 1.0;       // default
    if (argc >= 8) {
        flip_prob = atof(argv[7]);
    }
    if (argc >= 9) {
        beta = atof(argv[8]);
    }
    
    // Allocate matrix (v×b).
    int *matrix = malloc(v * b * sizeof(int));
    if (!matrix) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    
    if (!init_matrix_valid(matrix, v, b, r, k)) {
        fprintf(stderr, "Failed to initialize a matrix with the given row/column constraints.\n");
        free(matrix);
        return 1;
    }
    
    // Allocate the cache for pair penalties (only for i<j)
    double **pair_penalty = malloc(v * sizeof(double *));
    if (!pair_penalty) {
        fprintf(stderr, "Memory allocation failed for pair_penalty.\n");
        free(matrix);
        return 1;
    }
    // Also allocate caches for the "common" and "plus" counts.
    int **common_cache = malloc(v * sizeof(int *));
    int **plus_cache   = malloc(v * sizeof(int *));
    if (!common_cache || !plus_cache) {
        fprintf(stderr, "Memory allocation failed for common/plus caches.\n");
        free(matrix);
        free(pair_penalty);
        return 1;
    }
    for (int i = 0; i < v; i++) {
        pair_penalty[i] = malloc(v * sizeof(double));
        common_cache[i] = malloc(v * sizeof(int));
        plus_cache[i]   = malloc(v * sizeof(int));
        if (!pair_penalty[i] || !common_cache[i] || !plus_cache[i]) {
            fprintf(stderr, "Memory allocation failed for row %d caches.\n", i);
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < v; j++) {
            pair_penalty[i][j] = 0.0;
            // (common_cache and plus_cache are used only for i<j, but we initialize all entries to 0)
            common_cache[i][j] = 0;
            plus_cache[i][j] = 0;
        }
    }
    
    // Initialize the caches for each unordered pair (i,j) with i<j
    double current_penalty = 0.0;
    for (int i = 0; i < v; i++) {
        for (int j = i+1; j < v; j++) {
            int common = 0, plus = 0;
            for (int col = 0; col < b; col++) {
                int a = matrix[i * b + col];
                int ap = matrix[j * b + col];
                if (a != 0 && ap != 0) {
                    common++;
                    if (a * ap > 0)
                        plus++;
                }
            }
            common_cache[i][j] = common;
            plus_cache[i][j] = plus;
            double pen = fabs(common - L) + fabs(plus - (L/2.0));
            pair_penalty[i][j] = pen;
            current_penalty += pen;
        }
    }
    
    // Main loop: run until a valid design is found.
    while (true) {
        if (current_penalty == 0.0 && is_valid(matrix, v, b, r, k, L)) {
            // Print the final design.
            for (int i = 0; i < v; i++) {
                for (int j = 0; j < b; j++) {
                    printf("%d ", matrix[i * b + j]);
                }
                printf("\n");
            }
            break;
        }
        
        double p = rand01();
        if (p < flip_prob) {
            // ---------- Sign flip move ----------
            int i, j;
            if (!pick_random_nonzero(matrix, v, b, &i, &j))
                continue;
            
            int old_val = matrix[i * b + j];
            int new_val = -old_val;
            // Tentatively flip the sign.
            matrix[i * b + j] = new_val;
            
            // We update only those pairs that share column j with row i.
            // (For a sign flip the "common" count does not change, only the "plus" count.)
            struct PairUpdate updates[v]; // worst-case: v-1 affected pairs.
            int updates_count = 0;
            double delta = 0.0;
            
            for (int p_row = 0; p_row < v; p_row++) {
                if (p_row == i)
                    continue;
                if (matrix[p_row * b + j] == 0)
                    continue;
                int i1 = (i < p_row) ? i : p_row;
                int i2 = (i < p_row) ? p_row : i;
                int old_common = common_cache[i1][i2]; // unchanged
                int old_plus = plus_cache[i1][i2];
                int new_plus = old_plus;
                // Only the cell in column j is affected.
                // Before the flip the contribution was:
                //   +1 if old_val * (matrix[p_row][j]) > 0,
                //   0 if negative.
                // After the flip, the sign reverses.
                if (old_val * matrix[p_row * b + j] > 0)
                    new_plus--;
                else
                    new_plus++;
                
                int old_pen = abs(old_common - L) + abs(old_plus - (L/2));
                int new_pen = abs(old_common - L) + abs(new_plus - (L/2));
                delta += (new_pen - old_pen);
                
                updates[updates_count].i = i1;
                updates[updates_count].j = i2;
                updates[updates_count].new_common = old_common; // unchanged
                updates[updates_count].new_plus = new_plus;
                updates[updates_count].old_pen = old_pen;
                updates[updates_count].new_pen = new_pen;
                updates_count++;
            }
            
            double accept_prob = (delta <= 0.0) ? 1.0 : exp(-beta * delta);
            if (rand01() < accept_prob) {
                // Accept the move: update the plus cache and penalty cache.
                for (int u = 0; u < updates_count; u++) {
                    int ui = updates[u].i;
                    int uj = updates[u].j;
                    plus_cache[ui][uj] = updates[u].new_plus;
                    pair_penalty[ui][uj] = fabs(common_cache[ui][uj] - L) + fabs(plus_cache[ui][uj] - (L/2.0));
                }
                current_penalty += delta;
            } else {
                // Reject: revert the change.
                matrix[i * b + j] = old_val;
            }
        } else {
            // ---------- Swap move ----------
            int row1, row2, col1, col2;
            if (!attempt_swap_move(matrix, v, b, &row1, &row2, &col1, &col2))
                continue;
            // Save the old values at the swap positions.
            int old_val_r1_c1 = matrix[row1 * b + col1]; // nonzero
            int old_val_r2_c2 = matrix[row2 * b + col2]; // nonzero
            int new_val_r1_c2 = random_sign();
            int new_val_r2_c1 = random_sign();
            
            // Apply the swap move:
            // row1: remove nonzero at col1, add nonzero at col2.
            // row2: remove nonzero at col2, add nonzero at col1.
            matrix[row1 * b + col1] = 0;
            matrix[row1 * b + col2] = new_val_r1_c2;
            matrix[row2 * b + col1] = new_val_r2_c1;
            matrix[row2 * b + col2] = 0;
            
            // For a swap move the affected pairs are those involving row1 or row2.
            // We will update the caches for pairs (row1, p) and (row2, p) for all p except when p is the other.
            struct PairUpdate updates[2 * v]; // enough for affected pairs.
            int updates_count = 0;
            double delta = 0.0;
            
            // Update all pairs with row1.
            for (int p = 0; p < v; p++) {
                if (p == row1 || p == row2)
                    continue;
                int i1 = (row1 < p) ? row1 : p;
                int i2 = (row1 < p) ? p : row1;
                int old_common = common_cache[i1][i2];
                int old_plus = plus_cache[i1][i2];
                int d_common = 0, d_plus = 0;
                // For row1:
                // Column col1: originally row1 had old_val_r1_c1, now becomes 0.
                if (matrix[p * b + col1] != 0) {
                    d_common -= 1;
                    if (old_val_r1_c1 * matrix[p * b + col1] > 0)
                        d_plus -= 1;
                }
                // Column col2: originally row1 had 0, now gets new_val_r1_c2.
                if (matrix[p * b + col2] != 0) {
                    d_common += 1;
                    if (new_val_r1_c2 * matrix[p * b + col2] > 0)
                        d_plus += 1;
                }
                int new_common = old_common + d_common;
                int new_plus = old_plus + d_plus;
                int old_pen = abs(old_common - L) + abs(old_plus - (L/2));
                int new_pen = abs(new_common - L) + abs(new_plus - (L/2));
                delta += (new_pen - old_pen);
                
                updates[updates_count].i = i1;
                updates[updates_count].j = i2;
                updates[updates_count].new_common = new_common;
                updates[updates_count].new_plus = new_plus;
                updates[updates_count].old_pen = old_pen;
                updates[updates_count].new_pen = new_pen;
                updates_count++;
            }
            
            // Update all pairs with row2.
            for (int p = 0; p < v; p++) {
                if (p == row1 || p == row2)
                    continue;
                int i1 = (row2 < p) ? row2 : p;
                int i2 = (row2 < p) ? p : row2;
                int old_common = common_cache[i1][i2];
                int old_plus = plus_cache[i1][i2];
                int d_common = 0, d_plus = 0;
                // For row2:
                // Column col2: originally row2 had old_val_r2_c2, now becomes 0.
                if (matrix[p * b + col2] != 0) {
                    d_common -= 1;
                    if (old_val_r2_c2 * matrix[p * b + col2] > 0)
                        d_plus -= 1;
                }
                // Column col1: originally row2 had 0, now gets new_val_r2_c1.
                if (matrix[p * b + col1] != 0) {
                    d_common += 1;
                    if (new_val_r2_c1 * matrix[p * b + col1] > 0)
                        d_plus += 1;
                }
                int new_common = old_common + d_common;
                int new_plus = old_plus + d_plus;
                int old_pen = abs(old_common - L) + abs(old_plus - (L/2));
                int new_pen = abs(new_common - L) + abs(new_plus - (L/2));
                delta += (new_pen - old_pen);
                
                updates[updates_count].i = i1;
                updates[updates_count].j = i2;
                updates[updates_count].new_common = new_common;
                updates[updates_count].new_plus = new_plus;
                updates[updates_count].old_pen = old_pen;
                updates[updates_count].new_pen = new_pen;
                updates_count++;
            }
            
            double accept_prob = (delta <= 0.0) ? 1.0 : exp(-beta * delta);
            if (rand01() < accept_prob) {
                // Accept the move: update the caches for all affected pairs.
                for (int u = 0; u < updates_count; u++) {
                    int ui = updates[u].i;
                    int uj = updates[u].j;
                    common_cache[ui][uj] = updates[u].new_common;
                    plus_cache[ui][uj] = updates[u].new_plus;
                    pair_penalty[ui][uj] = fabs(common_cache[ui][uj] - L) + fabs(plus_cache[ui][uj] - (L/2.0));
                }
                current_penalty += delta;
            } else {
                // Reject: revert the swap move.
                matrix[row1 * b + col1] = old_val_r1_c1;
                matrix[row1 * b + col2] = 0;
                matrix[row2 * b + col1] = 0;
                matrix[row2 * b + col2] = old_val_r2_c2;
            }
        }
    }
    
    // Free allocated memory.
    for (int i = 0; i < v; i++) {
        free(pair_penalty[i]);
        free(common_cache[i]);
        free(plus_cache[i]);
    }
    free(pair_penalty);
    free(common_cache);
    free(plus_cache);
    free(matrix);
    return 0;
}