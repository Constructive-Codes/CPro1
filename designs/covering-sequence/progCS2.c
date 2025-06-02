/*
   Covering Sequence Constructor using Local Search (Hill Climbing)
   Constructs a cyclic (n,R)-covering sequence (over {0,1}) of length L.
   Command-line parameters (in order):
      n R L seed perturbation_size group_attempts
   where:
      n, R, L, seed are required.
      perturbation_size is the size of a multi–bit “group move” (default 2),
      group_attempts is the number of random multi–bit moves attempted when the search is stuck (default 10).
      
   The program runs indefinitely until it finds a valid covering sequence.
   
   Compile with e.g.: gcc -O2 -std=c99 covering_sequence.c -o covering_sequence
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Global variables (set in main after reading command-line args)
static int n_global;      // word length
static int R_global;      // maximum Hamming distance allowed
static int L_global;      // length of cyclic candidate sequence
static int global_uncovered; // number of binary words (0..2^n-1) not covered

// Candidate sequence: array of length L_global (each element is 0 or 1)
static int *candidate = NULL;
// For each cyclic window (starting at index j=0,...,L_global-1), store the n-bit value (with bit0 = candidate[j])
static int *window_val = NULL;
// Coverage array: for each binary word y (0 <= y < (1<<n_global)),
// coverage[y] = how many candidate windows cover y (i.e. are within Hamming distance <= R_global)
static int *coverage = NULL;


/**************************************
 *  Helper: Shuffle an integer array
 **************************************/
void shuffle_int_array(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/*************************************************************
 *  update_delta_for_value:
 *  For a given n-bit window value “val” update the temporary 
 *  delta array with contribution "contrib" for every binary word
 *  within Hamming distance <= R_global of val.
 *  Also, using the array "visited" (of size (1<<n_global)) and union_arr,
 *  we add indices (y) that we update (if not already added) so that later we
 *  can iterate only over the union.
 *************************************************************/
static inline void update_delta_for_value(int val, int contrib, int n, int R,
                                           int *delta_array, char *visited,
                                           int *union_arr, int *union_count) {
    int y;
    // d = 0 (always do the original value)
    y = val;
    delta_array[y] += contrib;
    if (!visited[y]) { visited[y] = 1; union_arr[(*union_count)++] = y; }
    if (R >= 1) {
        for (int i = 0; i < n; i++) {
            y = val ^ (1 << i);
            delta_array[y] += contrib;
            if (!visited[y]) { visited[y] = 1; union_arr[(*union_count)++] = y; }
        }
    }
    if (R >= 2) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                y = val ^ (1 << i) ^ (1 << j);
                delta_array[y] += contrib;
                if (!visited[y]) { visited[y] = 1; union_arr[(*union_count)++] = y; }
            }
        }
    }
    if (R >= 3) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                for (int k = j + 1; k < n; k++) {
                    y = val ^ (1 << i) ^ (1 << j) ^ (1 << k);
                    delta_array[y] += contrib;
                    if (!visited[y]) { visited[y] = 1; union_arr[(*union_count)++] = y; }
                }
            }
        }
    }
}

/*************************************************************
 *  update_coverage_for_value:
 *  Similar to update_delta_for_value but directly updates the
 *  global coverage array.
 *************************************************************/
static inline void update_coverage_for_value(int val, int contrib, int n, int R) {
    int y;
    y = val;
    coverage[y] += contrib;
    if (R >= 1) {
        for (int i = 0; i < n; i++) {
            y = val ^ (1 << i);
            coverage[y] += contrib;
        }
    }
    if (R >= 2) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                y = val ^ (1 << i) ^ (1 << j);
                coverage[y] += contrib;
            }
        }
    }
    if (R >= 3) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                for (int k = j + 1; k < n; k++) {
                    y = val ^ (1 << i) ^ (1 << j) ^ (1 << k);
                    coverage[y] += contrib;
                }
            }
        }
    }
}

/*************************************************************
 * simulate_move:
 *  Try a move that flips the bits at positions given in the
 *  array "flip_indices" (of length flip_count).  
 * 
 *  This function computes (via an incremental update) the effect 
 *  that the move would have on the global coverage (i.e. on the number 
 *  of uncovered words), and returns delta_uncovered = new_global_uncovered - global_uncovered.
 *
 *  If apply is nonzero then the candidate is updated (i.e. the move is committed):
 *    - candidate bits are flipped,
 *    - the corresponding windows (those that contain any flipped bit) are updated,
 *    - the global "coverage" array is updated, and
 *    - global_uncovered is changed accordingly.
 *
 *  (If apply==0 then the move is only simulated and global state is not modified.)
 *************************************************************/
int simulate_move(int *flip_indices, int flip_count, int apply) {
    int sizeDelta = 1 << n_global;  // size of the delta and visited arrays
    int *delta_array = (int *) calloc(sizeDelta, sizeof(int));
    char *visited = (char *) calloc(sizeDelta, sizeof(char));
    int *union_arr = (int *) malloc(sizeDelta * sizeof(int)); 
    int union_count = 0;

    // For affected windows, we allocate arrays.
    int *aff_list = (int *) malloc(L_global * sizeof(int)); // worst-case: all windows
    int aff_count = 0;
    int *new_vals = (int *) malloc(L_global * sizeof(int)); // new window values for the affected windows
    char *affected_flag = (char *) calloc(L_global, sizeof(char));

    // For every candidate bit position to be flipped, all windows that contain that bit are affected.
    for (int k = 0; k < flip_count; k++) {
        int pos = flip_indices[k];
        for (int i = 0; i < n_global; i++) {
            int j = (pos - i + L_global) % L_global;
            if (!affected_flag[j]) {
                affected_flag[j] = 1;
                aff_list[aff_count++] = j;
            }
        }
    }

    // For every affected window j, compute its new window value.
    for (int t = 0; t < aff_count; t++) {
        int j = aff_list[t];
        int old_val = window_val[j];
        int new_val = old_val;
        // For each candidate position p in flip_indices, if p lies in window j then update the bit accordingly.
        for (int k = 0; k < flip_count; k++) {
            int pos = flip_indices[k];
            /* In cyclic window starting at j, the candidate at position pos is at offset:
               offset = ( (pos - j + L_global) mod L_global )
               and the window covers offsets 0..n_global-1. */
            int offset = (pos - j + L_global) % L_global;
            if (offset < n_global) {
                int new_bit = 1 - candidate[pos];  // flip candidate[pos]
                if (new_bit == 1)
                    new_val |= (1 << offset);
                else
                    new_val &= ~(1 << offset);
            }
        }
        new_vals[t] = new_val;
        // For simulation of coverage: for the affected window, remove old contribution then add new contribution.
        update_delta_for_value(old_val, -1, n_global, R_global, delta_array, visited, union_arr, &union_count);
        update_delta_for_value(new_val, +1, n_global, R_global, delta_array, visited, union_arr, &union_count);
    }

    // Compute the net change in the number of uncovered words.
    int delta_uncovered = 0;
    for (int i = 0; i < union_count; i++) {
        int y = union_arr[i];
        int old_cov = coverage[y];
        int new_cov = old_cov + delta_array[y];
        int f_old = (old_cov == 0) ? 1 : 0;
        int f_new = (new_cov == 0) ? 1 : 0;
        delta_uncovered += (f_new - f_old);
        // Reset visited marker (for cleanliness)
        visited[y] = 0;
        // Not strictly necessary to zero delta_array[y] here.
    }

    // If we are to apply the move, update the global state.
    if (apply) {
        // For every affected window, update global coverage and then update window_val.
        for (int t = 0; t < aff_count; t++) {
            int j = aff_list[t];
            int old_val = window_val[j];
            int new_val = new_vals[t];
            // Remove the contribution for the old window value.
            update_coverage_for_value(old_val, -1, n_global, R_global);
            // Add the contribution for the new window value.
            update_coverage_for_value(new_val, +1, n_global, R_global);
            // Commit the new window value.
            window_val[j] = new_val;
        }
        // Flip the candidate bits.
        for (int k = 0; k < flip_count; k++) {
            int pos = flip_indices[k];
            candidate[pos] = 1 - candidate[pos];
        }
        global_uncovered += delta_uncovered;
    }
    
    free(delta_array);
    free(visited);
    free(union_arr);
    free(aff_list);
    free(new_vals);
    free(affected_flag);

    return delta_uncovered;
}


/*************************************************************
 *  Initialize global variables: candidate, window_val, coverage.
 *  (The candidate sequence is initialized randomly.)
 *************************************************************/
void initialize_globals() {
    // Allocate candidate sequence, window_val, and coverage.
    candidate = (int *) malloc(L_global * sizeof(int));
    window_val = (int *) malloc(L_global * sizeof(int));
    int covSize = 1 << n_global;
    coverage = (int *) calloc(covSize, sizeof(int));

    // Random initial candidate (each bit 0 or 1 uniformly)
    for (int i = 0; i < L_global; i++) {
        candidate[i] = rand() % 2;
    }

    // Build the window values.
    // For each window starting at index j, let window_val[j] be the n-bit number with:
    // bit0 = candidate[j], bit1 = candidate[(j+1)%L_global], ... , bit(n-1) = candidate[(j+n-1)%L_global].
    for (int j = 0; j < L_global; j++) {
        int val = 0;
        for (int k = 0; k < n_global; k++) {
            int bit = candidate[(j + k) % L_global];
            if (bit)
                val |= (1 << k);
        }
        window_val[j] = val;
    }

    // Update coverage: for every window, for every word within Hamming distance <= R_global,
    // increment coverage.
    for (int j = 0; j < L_global; j++) {
        int val = window_val[j];
        // Use the same enumeration as in update_delta_for_value (with contrib = +1)
        update_coverage_for_value(val, +1, n_global, R_global);
    }
    // Count how many words (from 0 to (1<<n_global)-1) have coverage 0.
    global_uncovered = 0;
    int covSizeInt = 1 << n_global;
    for (int y = 0; y < covSizeInt; y++) {
        if (coverage[y] == 0)
            global_uncovered++;
    }
}


/*************************************************************
 *  Main local‐search loop.
 *************************************************************/
int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s n R L seed [perturbation_size] [group_attempts]\n", argv[0]);
        return 1;
    }
    n_global = atoi(argv[1]);
    R_global = atoi(argv[2]);
    L_global = atoi(argv[3]);
    int seed = atoi(argv[4]);
    srand(seed);

    // Extra hyperparameters:
    int perturbation_size = 2;  // default value
    int group_attempts = 10;    // default value
    if (argc >= 6) {
        perturbation_size = atoi(argv[5]);
        if (perturbation_size < 1)
            perturbation_size = 2;
    }
    if (argc >= 7) {
        group_attempts = atoi(argv[6]);
        if (group_attempts < 1)
            group_attempts = 10;
    }

    // Initialize globals.
    initialize_globals();

    // Main local search loop: continue until all 2^n words are covered.
    while (global_uncovered > 0) {
        bool move_made = false;
        /* Try single–bit moves in a random order.
           For each candidate bit position, simulate flipping it. 
           If the move strictly reduces the number of uncovered words, accept it.
        */
        int *perm = (int *) malloc(L_global * sizeof(int));
        for (int i = 0; i < L_global; i++) {
            perm[i] = i;
        }
        shuffle_int_array(perm, L_global);
        for (int i = 0; i < L_global; i++) {
            int pos = perm[i];
            int flip[1] = { pos };
            int delta = simulate_move(flip, 1, 0); // simulate only (do not apply)
            if (delta < 0) {
                // Accept this move.
                simulate_move(flip, 1, 1);
                move_made = true;
                break;
            }
        }
        free(perm);

        if (!move_made) {
            /* If no single–bit move improves the solution,
               try a multi–bit group move (a perturbation) using perturbation_size.
               Try several random moves (group_attempts) and choose the one
               that minimizes the number of uncovered words (even if not improving).
            */
            int best_delta = 1000000;
            int *best_group = (int *) malloc(perturbation_size * sizeof(int));
            int *current_group = (int *) malloc(perturbation_size * sizeof(int));
            for (int attempt = 0; attempt < group_attempts; attempt++) {
                // Pick a set of distinct random indices.
                for (int k = 0; k < perturbation_size; k++) {
                    bool unique = false;
                    int idx;
                    while (!unique) {
                        idx = rand() % L_global;
                        unique = true;
                        for (int j = 0; j < k; j++) {
                            if (current_group[j] == idx) { unique = false; break; }
                        }
                    }
                    current_group[k] = idx;
                }
                int delta = simulate_move(current_group, perturbation_size, 0); // simulate only
                if (delta < best_delta) {
                    best_delta = delta;
                    memcpy(best_group, current_group, perturbation_size * sizeof(int));
                }
            }
            // Commit the best group move (even if not improving)
            simulate_move(best_group, perturbation_size, 1);
            free(best_group);
            free(current_group);
        }
        // (Optionally you could print progress here.)
    }

    // If we exit the loop, global_uncovered == 0 and the candidate is a valid covering sequence.
    // Print the candidate sequence as a list of values separated by spaces on one line.
    for (int i = 0; i < L_global; i++) {
        printf("%d ", candidate[i]);
    }
    printf("\n");

    // Free globals.
    free(candidate);
    free(window_val);
    free(coverage);
    return 0;
}