/*
 * Deletion Code (DC) Generator
 * Usage: ./dc n s m seed
 *
 * Given parameters:
 *   n: length of words (7 <= n <= 16)
 *   s: deletion parameter (2 or 3)
 *   m: size of the deletion code (< 250)
 *   seed: random seed parameter (accepted but not used for randomization)
 *
 * This implementation uses a precomputed candidate deletion mask (using bitmask DP)
 * and performs a DFS/backtracking search that checks candidate compatibility via fast
 * bit‐wise tests. The candidate list is first sorted by “popcount” (i.e. number of 
 * distinct deletion outcomes) to help prune the search.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Structure to hold each candidate word and its precomputed deletion mask.
typedef struct {
    uint32_t word;     // The candidate word, stored as an integer (0 .. 2^n - 1)
    uint32_t popcount; // Number of bits set in the candidate's deletion mask.
    uint64_t *mask;    // Pointer to an array of uint64_t; the deletion outcomes bitmask.
} Candidate;

// Global variables (set in main, then used by DFS)
static int n, s, m;
static int candidateCount;  // Number of candidate words = 2^n.
static int mask_size;       // Number of 64‐bit words needed to represent all deletion outcomes.
static Candidate *candidates = NULL;  // Array of candidate words.
static uint32_t *solution = NULL;       // Array to hold chosen codewords (of length m).

/*
 * compute_candidate_mask
 * Computes the bitmask representation for candidate word x.
 *
 * For each way to delete s bits from the n–bit word (by choosing s distinct positions),
 * the resulting (n–s)–bit number is computed (treating the bits in order) and the
 * bit in the mask corresponding to that value is set.
 *
 * The parameter "mask" is an array of length mask_size (already zeroed).
 */
void compute_candidate_mask(uint32_t x, int n, int s, int mask_size, uint64_t *mask) {
    if (s == 2) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                uint32_t outcome = 0;
                for (int pos = 0; pos < n; pos++) {
                    if (pos == i || pos == j)
                        continue;
                    int bit = (x >> (n - 1 - pos)) & 1;
                    outcome = (outcome << 1) | bit;
                }
                int arrIndex = outcome / 64;
                int bitIndex = outcome % 64;
                mask[arrIndex] |= ((uint64_t)1 << bitIndex);
            }
        }
    } else if (s == 3) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                for (int k = j + 1; k < n; k++) {
                    uint32_t outcome = 0;
                    for (int pos = 0; pos < n; pos++) {
                        if (pos == i || pos == j || pos == k)
                            continue;
                        int bit = (x >> (n - 1 - pos)) & 1;
                        outcome = (outcome << 1) | bit;
                    }
                    int arrIndex = outcome / 64;
                    int bitIndex = outcome % 64;
                    mask[arrIndex] |= ((uint64_t)1 << bitIndex);
                }
            }
        }
    }
}

/*
 * count_mask_popcount
 * Returns the sum of bits set in the given mask array.
 */
uint32_t count_mask_popcount(uint64_t *mask, int mask_size) {
    uint32_t count = 0;
    for (int i = 0; i < mask_size; i++) {
        count += __builtin_popcountll(mask[i]);
    }
    return count;
}

/*
 * compare_candidate
 * Comparison routine for qsort: sort by increasing popcount, and if equal then by word.
 */
int compare_candidate(const void *a, const void *b) {
    const Candidate *ca = (const Candidate *)a;
    const Candidate *cb = (const Candidate *)b;
    if (ca->popcount < cb->popcount)
        return -1;
    else if (ca->popcount > cb->popcount)
        return 1;
    else {
        return (ca->word < cb->word) ? -1 : (ca->word > cb->word) ? 1 : 0;
    }
}

/*
 * dfs
 * Recursive backtracking procedure.
 *
 * start - the starting index in the sorted candidate list for this recursion level.
 * count - the number of words already chosen in the partial solution.
 * global_mask - the current 64–bit mask array (of length mask_size) that is the bitwise union
 *               of the deletion masks of the words already chosen.
 *
 * When count reaches m, a valid deletion code has been found and it is printed.
 */
void dfs(int start, int count, uint64_t *global_mask) {
    if ((candidateCount - start) < (m - count))
        return;
    if (count == m) {
        // Print the solution: m rows each with n space‐separated bits.
        for (int i = 0; i < m; i++) {
            uint32_t word = solution[i];
            for (int bit = 0; bit < n; bit++) {
                int b = (word >> (n - 1 - bit)) & 1;
                printf("%d", b);
                if (bit < n - 1)
                    printf(" ");
            }
            printf("\n");
        }
        exit(0); // Terminate as soon as a valid solution is output.
    }
    for (int i = start; i < candidateCount; i++) {
        bool conflict = false;
        for (int j = 0; j < mask_size; j++) {
            if (global_mask[j] & candidates[i].mask[j]) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            uint64_t new_mask[mask_size];
            for (int j = 0; j < mask_size; j++) {
                new_mask[j] = global_mask[j] | candidates[i].mask[j];
            }
            solution[count] = candidates[i].word;
            dfs(i + 1, count + 1, new_mask);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s n s m seed\n", argv[0]);
        return 1;
    }
    n = atoi(argv[1]);
    s = atoi(argv[2]);
    m = atoi(argv[3]);
    int seed = atoi(argv[4]);
    srand(seed); // The seed is accepted though not used for randomness here.

    if (n < 7 || n > 16) {
        fprintf(stderr, "Error: n must be between 7 and 16.\n");
        return 1;
    }
    if (s != 2 && s != 3) {
        fprintf(stderr, "Error: s must be 2 or 3.\n");
        return 1;
    }
    if (m < 1 || m >= 250) {
        fprintf(stderr, "Error: m must be positive and less than 250.\n");
        return 1;
    }

    candidateCount = 1 << n;  // Total number of candidate words.
    // The deletion outcomes are (n-s)-bit numbers, so total outcomes = 2^(n-s)
    int total_outcomes = 1 << (n - s);
    mask_size = (total_outcomes + 63) / 64;  // How many 64-bit words cover all outcomes.

    // Allocate candidate array.
    candidates = malloc(candidateCount * sizeof(Candidate));
    if (!candidates) {
        fprintf(stderr, "Memory allocation error for candidates.\n");
        return 1;
    }
    
    // Allocate one contiguous block for all candidate masks.
    uint64_t *allMasks = calloc(candidateCount * mask_size, sizeof(uint64_t));
    if (!allMasks) {
        fprintf(stderr, "Memory allocation error for candidate masks.\n");
        return 1;
    }
    
    // Precompute deletion masks (and popcounts) for all candidate words.
    for (uint32_t x = 0; x < (uint32_t)candidateCount; x++) {
        candidates[x].word = x;
        candidates[x].mask = allMasks + x * mask_size; // Each candidate gets its subarray.
        compute_candidate_mask(x, n, s, mask_size, candidates[x].mask);
        candidates[x].popcount = count_mask_popcount(candidates[x].mask, mask_size);
    }
    
    // Sort candidates by (increasing) popcount, then by word.
    qsort(candidates, candidateCount, sizeof(Candidate), compare_candidate);

    // Allocate array to store the solution (m chosen words).
    solution = malloc(m * sizeof(uint32_t));
    if (!solution) {
        fprintf(stderr, "Memory allocation error for solution.\n");
        return 1;
    }
    
    // Start DFS with an initial (all-zero) global mask.
    uint64_t initial_mask[mask_size];
    for (int i = 0; i < mask_size; i++) {
        initial_mask[i] = 0;
    }
    
    dfs(0, 0, initial_mask);
    
    // If DFS returns, no valid deletion code was found (the program will not normally get here).
    fprintf(stderr, "No valid deletion code found.\n");
    return 1;
}