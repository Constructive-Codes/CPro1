/*
   Covering Sequence Construction via Iterated Local Improvement with Bitmasking

   Command-line parameters:
     n             : length of window (e.g., 5)
     R             : allowed Hamming radius (e.g., 1)
     L             : total length of the cyclic sequence (e.g., 8)
     seed          : random seed (an integer)
     random_prob   : (optional) probability [0.0, 1.0] to take a random move if no candidate improves coverage (default 0.1)
     sample_size   : (optional) number of candidate bit positions (up to L) to evaluate each iteration (default = min(10,L))
     
   The output is a single line containing L bits (0’s or 1’s separated by spaces)
   forming an (n,R)-covering sequence.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global parameters (set from command-line)
int n, R, L;
unsigned int seed;

// Global arrays:
//   seq         : the candidate sequence of L bits (each element is 0 or 1)
//   window_vals : for each starting index j (0 <= j < L), the n–bit window (packed into an int)
//   freq        : an array of size (1 << n) counting how many windows (after applying the Hamming‐ball)
                   // cover each n–bit word; a word is “covered” if freq[word] > 0.
int *seq = NULL;
int *window_vals = NULL;
int *freq = NULL;   // frequency array; size = 1<<n

// mask_deltas will hold the list of all bit masks (of length n) with at most R ones. 
// In other words, for any n–bit window value w, every word within Hamming distance ≤ R from w is w XOR mask,
// for some mask in mask_deltas.
int *mask_deltas = NULL;
int num_deltas = 0;

// Hyperparameters (in addition to the required 4 arguments)
double random_prob = 0.1; // probability to take a random move when no improvement is found
int sample_size = 0;      // number of candidate bit positions to sample each iteration (default: min(10, L))

// -------------------------------------------------------------------------
// Recursive routine to generate all bit masks of length n having at most R ones.
// We build up a value "mask" by choosing positions (starting at index "start").
// "chosen" counts how many bits we have chosen so far.
void gen_deltas(int n, int R, int start, int chosen, int mask, int *deltas, int *count) {
    // Add the current mask (this adds the mask for each combination of size = chosen, and we allow all sizes up to R)
    deltas[*count] = mask;
    (*count)++;
    if (chosen == R)
        return;
    for (int i = start; i < n; i++) {
        gen_deltas(n, R, i+1, chosen+1, mask | (1 << i), deltas, count);
    }
}

// This routine allocates (with an upper bound size) and fills the global mask_deltas array.
void generate_mask_deltas(int n, int R) {
    // As an upper bound, we allocate space for 1000 integers.
    mask_deltas = (int *)malloc(1000 * sizeof(int));
    if (!mask_deltas) {
        fprintf(stderr, "Memory allocation failed for mask_deltas\n");
        exit(1);
    }
    num_deltas = 0;
    gen_deltas(n, R, 0, 0, 0, mask_deltas, &num_deltas);
}

// -------------------------------------------------------------------------
// Compute the current number of uncovered words. A word is uncovered if freq[word]==0.
int compute_uncovered_count(int freq_size) {
    int count = 0;
    for (int i = 0; i < freq_size; i++) {
        if (freq[i] == 0)
            count++;
    }
    return count;
}

// -------------------------------------------------------------------------
// Simulate the effect of flipping bit at index "candidate" in the sequence.
// (This is done without modifying the global state.)
//
// The effect of a flip is felt in every window that includes position candidate.
// Because our windows are of length n and the sequence is cyclic, exactly n windows are affected:
// For each r from 0 to n-1, let j = (candidate - r + L) mod L be an affected window.
// In window j, the candidate bit is in position (n-1 - r) (because we pack the window with the 
// leftmost bit as the most significant).
//
// For each affected window we “remove” the contribution of its old value and “add” the new value.
// For each window value w, every word covered (within Hamming radius R) is of the form: w XOR delta,
// for each delta in mask_deltas.
// We accumulate the net change to freq[word] in a temporary array
// (using “touched” bookkeeping so that we only iterate on entries that changed).
//
// The net “improvement” is computed as follows:
//  • For each word where the old freq was 0 and the new freq becomes > 0, we have “covered” one more word.
//  • For each word where the old freq was > 0 and the new freq becomes 0, we have “lost” a cover.
// The candidate improvement is the number of words that go from uncovered to covered MINUS the number that go from covered to uncovered.
int simulate_flip(int candidate, int freq_size, int *temp_change, char *touched, int *touched_list, int *touched_count) {
    int improvement = 0;
    *touched_count = 0;  // reset count
    
    // For each affected window (there are n such windows):
    // (For each r, j = (candidate - r + L) mod L is affected.)
    for (int r = 0; r < n; r++) {
        int j = (candidate - r + L) % L;
        int old_val = window_vals[j];
        int new_val = old_val ^ (1 << (n - 1 - r));  // flip the bit at position (n-1-r) in the window
        for (int d = 0; d < num_deltas; d++) {
            int remove_word = old_val ^ mask_deltas[d];
            int add_word    = new_val ^ mask_deltas[d];
            // For the word that will lose a cover:
            if (!touched[remove_word]) {
                touched[remove_word] = 1;
                touched_list[(*touched_count)++] = remove_word;
                temp_change[remove_word] = 0;
            }
            temp_change[remove_word] -= 1;
            // For the word that will gain a cover:
            if (!touched[add_word]) {
                touched[add_word] = 1;
                touched_list[(*touched_count)++] = add_word;
                temp_change[add_word] = 0;
            }
            temp_change[add_word] += 1;
        }
    }
    // Now sum over all affected words.
    for (int i = 0; i < *touched_count; i++) {
        int word = touched_list[i];
        int new_freq = freq[word] + temp_change[word];
        if (freq[word] == 0 && new_freq > 0)
            improvement++;       // word becomes covered
        else if (freq[word] > 0 && new_freq == 0)
            improvement--;       // word becomes uncovered
    }
    
    // Clear the touched flags and temp_change entries.
    for (int i = 0; i < *touched_count; i++) {
        int word = touched_list[i];
        touched[word] = 0;
        temp_change[word] = 0;
    }
    *touched_count = 0;
    return improvement;
}

// -------------------------------------------------------------------------
// Apply the flip of bit at position "candidate" to the global state.
// (This will update the sequence, the corresponding window_vals for the n windows
// that include candidate, and update freq accordingly.)
void apply_flip(int candidate, int freq_size) {
    // For every window that includes the candidate bit:
    for (int r = 0; r < n; r++) {
        int j = (candidate - r + L) % L;
        int old_val = window_vals[j];
        int new_val = old_val ^ (1 << (n - 1 - r));
        // For each delta, update frequency: remove the old contribution and add the new.
        for (int d = 0; d < num_deltas; d++) {
            int remove_word = old_val ^ mask_deltas[d];
            int add_word    = new_val ^ mask_deltas[d];
            freq[remove_word]--;
            freq[add_word]++;
        }
        window_vals[j] = new_val;
    }
    // Flip the candidate bit in the sequence.
    seq[candidate] = 1 - seq[candidate];
}

// -------------------------------------------------------------------------
// Main routine.
int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s n R L seed [random_prob] [sample_size]\n", argv[0]);
        exit(1);
    }
    n = atoi(argv[1]);
    R = atoi(argv[2]);
    L = atoi(argv[3]);
    seed = (unsigned int)atoi(argv[4]);
    srand(seed);
    
    // Read optional hyperparameters.
    if (argc >= 6) {
        random_prob = atof(argv[5]);
    } else {
        random_prob = 0.1;
    }
    if (argc >= 7) {
        sample_size = atoi(argv[6]);
        if (sample_size < 1)
            sample_size = 1;
        if (sample_size > L)
            sample_size = L;
    } else {
        sample_size = (L < 10 ? L : 10);
    }

    int freq_size = 1 << n;  // total number of n-bit words

    // Allocate arrays.
    seq = (int *)malloc(L * sizeof(int));
    if (!seq) { fprintf(stderr, "Memory allocation error for seq\n"); exit(1); }
    
    window_vals = (int *)malloc(L * sizeof(int));
    if (!window_vals) { fprintf(stderr, "Memory allocation error for window_vals\n"); exit(1); }
    
    freq = (int *)calloc(freq_size, sizeof(int));
    if (!freq) { fprintf(stderr, "Memory allocation error for freq\n"); exit(1); }
    
    // Create an initial candidate sequence (random 0s and 1s).
    for (int i = 0; i < L; i++) {
        seq[i] = rand() % 2;
    }
    
    // Precompute the n–bit windows. Each window starting at j (mod L) is formed by
    // taking n consecutive bits (wrapping around) with the leftmost bit being the most significant.
    for (int j = 0; j < L; j++) {
        int w = 0;
        for (int k = 0; k < n; k++) {
            int pos = (j + k) % L;
            w = (w << 1) | seq[pos];
        }
        window_vals[j] = w;
    }
    
    // Precompute mask_deltas for Hamming balls of radius R.
    generate_mask_deltas(n, R);
    
    // Update the frequency array by “adding” the Hamming ball of every window.
    for (int j = 0; j < L; j++) {
        int w = window_vals[j];
        for (int d = 0; d < num_deltas; d++) {
            int word = w ^ mask_deltas[d];
            freq[word]++;
        }
    }
    
    int global_uncovered = compute_uncovered_count(freq_size);
    
    // Allocate temporary arrays for candidate simulation.
    // temp_change: an array (of size freq_size = 1<<n) used to accumulate the net change.
    int *temp_change = (int *)calloc(freq_size, sizeof(int));
    if (!temp_change) { fprintf(stderr, "Allocation error for temp_change\n"); exit(1); }
    // touched is a flag array for words (0 if not touched, 1 if touched).
    char *touched = (char *)calloc(freq_size, sizeof(char));
    if (!touched) { fprintf(stderr, "Allocation error for touched\n"); exit(1); }
    // touched_list will contain the indices (words) that were modified.
    int *touched_list = (int *)malloc(freq_size * sizeof(int));
    if (!touched_list) { fprintf(stderr, "Allocation error for touched_list\n"); exit(1); }
    int touched_count = 0;
    
    // Iterated local improvement loop.
    // (We iterate indefinitely until every n–bit word is covered.)
    long long iterations = 0;
    while (global_uncovered > 0) {
        iterations++;
        int best_candidate = -1;
        int best_improvement = -1000000000;  // a very low starting value
        
        // Instead of exhausting all L bit positions each iteration,
        // we sample "sample_size" candidate indices uniformly at random.
        for (int s = 0; s < sample_size; s++) {
            int candidate = rand() % L;
            int improvement = simulate_flip(candidate, freq_size, temp_change, touched, touched_list, &touched_count);
            if (improvement > best_improvement) {
                best_improvement = improvement;
                best_candidate = candidate;
            }
        }
        
        // Decide which candidate move to make.
        // If the best candidate yields an improvement (i.e. covers more previously uncovered words),
        // take that move. Otherwise, with probability random_prob, choose a random flip;
        // if not, still take the (least-bad) candidate from our sample.
        int chosen_candidate;
        double coin = (double)rand() / (double)RAND_MAX;
        if (best_improvement > 0 || coin >= random_prob)
            chosen_candidate = best_candidate;
        else
            chosen_candidate = rand() % L;
        
        // (Before applying the move, one could print debug information.)
        // Apply the chosen candidate move: update the frequency table incrementally,
        // update the affected window_vals, and flip the bit in seq.
        apply_flip(chosen_candidate, freq_size);
        
        // Update the global uncovered count.
        if (best_improvement > 0) {
            global_uncovered -= best_improvement;
            if (global_uncovered < 0)
                global_uncovered = 0;
        } else {
            // Otherwise, recompute from scratch.
            global_uncovered = compute_uncovered_count(freq_size);
        }
        
        // Optionally, every once in a while print progress to stderr.
        if (iterations % 10000 == 0) {
            fprintf(stderr, "Iteration %lld, global uncovered words = %d\n", iterations, global_uncovered);
        }
    }
    
    // The candidate sequence now covers every n-bit word (within Hamming radius R).
    // Print the final covering sequence as a single line (L numbers separated by spaces).
    for (int i = 0; i < L; i++) {
        printf("%d ", seq[i]);
    }
    printf("\n");
    
    // Free all dynamically allocated memory.
    free(seq);
    free(window_vals);
    free(freq);
    free(mask_deltas);
    free(temp_change);
    free(touched);
    free(touched_list);
    
    return 0;
}