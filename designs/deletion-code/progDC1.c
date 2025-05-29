#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* Data structure for caching the deletion outcomes of a word.
   For a given word (an n‐bit integer), get_outcomes(word) returns a sorted array
   (of length “count”) of the distinct (n-s)-bit values that arise when deleting s bits.
*/
typedef struct {
    int count;      // number of outcomes (after duplicate removal)
    int *outcomes;  // pointer to array (of length count)
} OutcomeSet;

/* Global parameters (set after command‐line parsing) */
int n, s, m;           // word length, number of deletions, number of codewords
int num_comb;          // number of deletion operations = C(n, n-s) = C(n,s)
int **keep_combinations; // global list of all combinations of (n-s) indices (from 0 to n-1)
int comb_index;         // used during combination generation

/* Outcome cache data structures.
   We allocate an array of size (1 << n) so that for each n–bit word we can cache its OutcomeSet.
   The outcome_computed array flags whether a word’s OutcomeSet has been computed.
*/
OutcomeSet *outcome_cache;
char *outcome_computed; // 0 = not computed, 1 = computed

/* Compare function for qsort (for integers) */
int compare_int(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

/* Recursive function to generate all combinations (of the indices to keep).
   We choose k = n-s indices from {0, 1, ..., n-1} (in increasing order).
 */
void generate_combinations_recursive(int start, int index, int k, int n, int *combo) {
    if(index == k) {
        int *combination = malloc(k * sizeof(int));
        for (int i = 0; i < k; i++) {
            combination[i] = combo[i];
        }
        keep_combinations[comb_index++] = combination;
        return;
    }
    for (int i = start; i <= n - (k - index); i++) {
        combo[index] = i;
        generate_combinations_recursive(i + 1, index + 1, k, n, combo);
    }
}

/* Precompute all keep combinations.
   (There are num_comb = C(n, n-s) of them.)
*/
void precompute_combinations() {
    int k = n - s;
    // Compute num_comb = binom(n, k)
    num_comb = 1;
    for (int i = 0; i < k; i++) {
        num_comb = num_comb * (n - i) / (i + 1);
    }
    keep_combinations = malloc(num_comb * sizeof(int *));
    comb_index = 0;
    int *combo = malloc(k * sizeof(int));
    generate_combinations_recursive(0, 0, k, n, combo);
    free(combo);
}

/* Given a word (an integer with n bits), return a pointer to its cached OutcomeSet.
   If not already computed, this function computes the (n-s)-bit deletion outcomes by:
     – For each of the precomputed keep combinations, extract the bits in order.
     – Then sort the list and remove any duplicate outcomes.
*/
OutcomeSet *get_outcomes(int word) {
    if(outcome_computed[word])
        return &outcome_cache[word];

    OutcomeSet *os = &outcome_cache[word];
    os->count = num_comb;  // initially, one outcome per combination (duplicates may occur)
    os->outcomes = malloc(num_comb * sizeof(int));
    int k = n - s;
    for (int i = 0; i < num_comb; i++) {
        int outcome = 0;
        for (int j = 0; j < k; j++) {
            int pos = keep_combinations[i][j]; 
            /* Bit extraction: we interpret the n‐bit word so that bit position 0 is the leftmost.
               Hence the bit at “pos” (0 ≤ pos ≤ n-1) is obtained as:
                     bit = (word >> (n-1-pos)) & 1.
            */
            int bit = (word >> (n - 1 - pos)) & 1;
            outcome = (outcome << 1) | bit;
        }
        os->outcomes[i] = outcome;
    }
    qsort(os->outcomes, num_comb, sizeof(int), compare_int);
    // Remove duplicates (so that we treat D(x) as a set)
    int unique_count = 0;
    for (int i = 0; i < num_comb; i++) {
        if(i == 0 || os->outcomes[i] != os->outcomes[i - 1]) {
            os->outcomes[unique_count++] = os->outcomes[i];
        }
    }
    os->count = unique_count;
    outcome_computed[word] = 1;
    return os;
}

/* The current candidate solution is stored in the global array "codewords".
   Each entry is an integer representing an n–bit word.
*/
int *codewords;   

/* Global deletion_counts array.
   This array (of size 2^(n-s)) tallies the number of codewords that produce
   a given (n-s)-bit deletion outcome (over all deletion procedures).
*/
int *deletion_counts;  // size = 1 << (n-s)

/* Returns the overall conflict cost.
   For each deletion outcome r, if deletion_counts[r] > 1 then we “pay” (deletion_counts[r] – 1).
   Thus a valid (conflict–free) deletion code will have cost 0.
*/
int compute_global_cost() {
    int total = 0;
    int size = 1 << (n - s);
    for (int r = 0; r < size; r++) {
        if(deletion_counts[r] > 1)
            total += (deletion_counts[r] - 1);
    }
    return total;
}

/* Tabu search “memory” structures.
   – tabu_flip is a 2D array for individual bit–flip moves (dimensions: m x n).
   – tabu_replace is an array for replacement moves (one per codeword).
   A move is considered tabu if the stored iteration number is greater than the current iteration.
*/
int **tabu_flip;    // m rows, each with n integers
int *tabu_replace;  // m integers

/* Hyperparameters (beyond the required four parameters).
   – tabu_tenure: number of iterations for which a move remains tabu (default 7).
   – num_replacement_samples: when trying a replacement move on a codeword,
       how many random candidate words to try (default 5).
*/
int tabu_tenure;
int num_replacement_samples;

/* 
   Compute the change (delta) in global conflict cost if we change codeword i from its current value
   to candidate_val.
   Only the deletion outcomes produced by the codeword i are affected.
   We compute the difference by doing a “merge” of the sorted outcome sets:
      For every outcome r that is in candidate but not in the old set, if deletion_counts[r] >= 1 then add +1.
      For every outcome r that is in the old set but not in candidate, if deletion_counts[r] >= 2 then subtract 1.
   (Outcomes in both sets produce no net change.)
   
   Our small change: while merging we compute a lower bound on the final delta—
   namely, current delta minus (the number of old outcomes remaining).
   If that lower bound is already no better than the best delta found so far (best_threshold),
   we abort early (returning that lower bound). This avoids spending time on candidate moves that
   we can tell early on will not beat the current best.
*/
int compute_delta(int i, int candidate_val, int best_threshold) {
    OutcomeSet *old_set = get_outcomes(codewords[i]);
    OutcomeSet *cand_set = get_outcomes(candidate_val);
    int delta = 0;
    int ia = 0, ib = 0;
    while(ia < old_set->count && ib < cand_set->count) {
        if(old_set->outcomes[ia] < cand_set->outcomes[ib]) {
            int r = old_set->outcomes[ia];
            int base = deletion_counts[r];
            if(base >= 2)
                delta -= 1;
            ia++;
        } else if(old_set->outcomes[ia] > cand_set->outcomes[ib]) {
            int r = cand_set->outcomes[ib];
            int base = deletion_counts[r];
            if(base >= 1)
                delta += 1;
            ib++;
        } else {
            // r appears in both sets: no change.
            ia++; 
            ib++;
        }
        // Early termination:
        // In the best case, every remaining outcome in the old_set would subtract 1.
        int remainingOld = old_set->count - ia;
        int lower_bound = delta - remainingOld;
        if(lower_bound >= best_threshold) {
            // It is impossible for this candidate move to beat the best move so far.
            return lower_bound;
        }
    }
    while(ia < old_set->count) {
        int r = old_set->outcomes[ia];
        int base = deletion_counts[r];
        if(base >= 2)
            delta -= 1;
        ia++;
    }
    while(ib < cand_set->count) {
        int r = cand_set->outcomes[ib];
        int base = deletion_counts[r];
        if(base >= 1)
            delta += 1;
        ib++;
    }
    return delta;
}

/* When a move is applied to codeword i (either by a bit–flip or replacement),
   we must update the deletion_counts array.
   For each outcome produced by the old value we decrement deletion_counts;
   then for each outcome produced by the candidate (new) value we increment deletion_counts.
*/
void apply_move(int i, int candidate_val) {
    OutcomeSet *old_set = get_outcomes(codewords[i]);
    OutcomeSet *cand_set = get_outcomes(candidate_val);
    for (int j = 0; j < old_set->count; j++) {
        int r = old_set->outcomes[j];
        deletion_counts[r]--;
    }
    for (int j = 0; j < cand_set->count; j++) {
        int r = cand_set->outcomes[j];
        deletion_counts[r]++;
    }
    codewords[i] = candidate_val;
}

/* Returns true if codeword i (i.e. current codewords[i]) is involved in any conflict.
   That is, if any deletion outcome produced by codeword i has deletion_counts[r] > 1.
*/
bool is_conflicted(int i) {
    OutcomeSet *os = get_outcomes(codewords[i]);
    for (int j = 0; j < os->count; j++) {
        int r = os->outcomes[j];
        if(deletion_counts[r] > 1)
            return true;
    }
    return false;
}

/* Helper function to generate a random n–bit word.
   (This is used both during initialization and in candidate replacement moves.)
*/
int random_word() {
    int word = 0;
    for (int j = 0; j < n; j++) {
        int bit = rand() % 2;
        word = (word << 1) | bit;
    }
    return word;
}

int main(int argc, char *argv[]) {
    if(argc < 5) {
        fprintf(stderr, "Usage: %s n s m seed [tabu_tenure] [num_replacement_samples]\n", argv[0]);
        exit(1);
    }
    /* Read required parameters: n, s, m, and seed.
       Then read two extra hyperparameters (if provided) in order:
         tabu_tenure and num_replacement_samples.
    */
    n = atoi(argv[1]);
    s = atoi(argv[2]);
    m = atoi(argv[3]);
    int seed_val = atoi(argv[4]);
    srand(seed_val);
    
    tabu_tenure = (argc >= 6) ? atoi(argv[5]) : 7;
    num_replacement_samples = (argc >= 7) ? atoi(argv[6]) : 5;
    
    if(n < 7 || n > 16 || (s != 2 && s != 3)) {
        fprintf(stderr, "Invalid parameters: n must be between 7 and 16 and s must be 2 or 3.\n");
        exit(1);
    }
    
    /* Allocate and initialize the candidate solution.
       Each codeword is an n–bit integer generated at random.
    */
    codewords = malloc(m * sizeof(int));
    for (int i = 0; i < m; i++) {
        codewords[i] = random_word();
    }
    
    /* Precompute the combinations of positions to keep.
       (There are num_comb = C(n, n-s) such combinations.)
    */
    precompute_combinations();
    
    /* Allocate the outcome cache for all n–bit words.
       (We allocate an array of size 2^n.)
    */
    int total_words = 1 << n;
    outcome_cache = malloc(total_words * sizeof(OutcomeSet));
    outcome_computed = calloc(total_words, sizeof(char));
    
    /* Allocate and initialize the deletion_counts array.
       Its size is 2^(n-s): one entry for each possible (n-s)-bit outcome.
    */
    int dsize = 1 << (n - s);
    deletion_counts = calloc(dsize, sizeof(int));
    for (int i = 0; i < m; i++) {
        OutcomeSet *os = get_outcomes(codewords[i]);
        for (int j = 0; j < os->count; j++) {
            int r = os->outcomes[j];
            deletion_counts[r]++;
        }
    }
    
    int current_cost = compute_global_cost();
    int best_cost = current_cost;
    int iter = 0;
    
    /* Allocate and initialize the tabu structures.
       tabu_flip[i][j] is the iteration until which a bit–flip on bit j of codeword i is forbidden.
       tabu_replace[i] plays the analogous role for full replacement moves.
    */
    tabu_flip = malloc(m * sizeof(int *));
    for (int i = 0; i < m; i++) {
        tabu_flip[i] = calloc(n, sizeof(int));
    }
    tabu_replace = calloc(m, sizeof(int));
    
    /* Main Tabu Search loop.
       In each iteration we:
         – Identify the codewords involved in conflicts.
         – Enumerate candidate moves (bit–flip moves for every bit and a few full replacement moves).
         – Evaluate each move’s effect (delta in conflict cost) and check the tabu conditions.
         – Choose the best allowable move and update the candidate solution.
         – Update the tabu list and conflict cost.
         – Continue until a conflict–free deletion code (global cost zero) is found.
    */
    while(current_cost > 0) {
        int best_delta = 100000000; // arbitrarily large positive value
        int best_move_type = -1;     // 0: bit flip, 1: replacement
        int best_i = -1;
        int best_bit = -1;
        int best_candidate_val = 0;
        bool move_found = false;
        
        for (int i = 0; i < m; i++) {
            if (!is_conflicted(i))
                continue;
            
            /* Bit–flip moves: try flipping each bit of codeword i */
            for (int j = 0; j < n; j++) {
                int candidate_val = codewords[i] ^ (1 << (n - 1 - j));
                if (candidate_val == codewords[i])
                    continue; // no change
                int delta = compute_delta(i, candidate_val, best_delta);
                bool tabu = (tabu_flip[i][j] > iter);
                bool aspiration = (current_cost + delta < best_cost);
                if(tabu && !aspiration)
                    continue;
                if(delta < best_delta) {
                    best_delta = delta;
                    best_move_type = 0;
                    best_i = i;
                    best_bit = j;
                    best_candidate_val = candidate_val;
                    move_found = true;
                }
            }
            
            /* Replacement moves: try a few random full replacements */
            for (int sample = 0; sample < num_replacement_samples; sample++) {
                int candidate_val = random_word();
                if(candidate_val == codewords[i])
                    continue;
                int delta = compute_delta(i, candidate_val, best_delta);
                bool tabu = (tabu_replace[i] > iter);
                bool aspiration = (current_cost + delta < best_cost);
                if(tabu && !aspiration)
                    continue;
                if(delta < best_delta) {
                    best_delta = delta;
                    best_move_type = 1;
                    best_i = i;
                    best_candidate_val = candidate_val;
                    move_found = true;
                }
            }
        }
        
        /* If no move was found (which can happen in a plateau), pick a random conflicted codeword
           and perform a random bit–flip move.
        */
        if (!move_found) {
            int rand_i = rand() % m;
            while (!is_conflicted(rand_i)) {
                rand_i = rand() % m;
            }
            int rand_bit = rand() % n;
            best_move_type = 0;
            best_i = rand_i;
            best_bit = rand_bit;
            best_candidate_val = codewords[rand_i] ^ (1 << (n - 1 - rand_bit));
            best_delta = compute_delta(rand_i, best_candidate_val, best_delta);
        }
        
        /* Apply the chosen move */
        apply_move(best_i, best_candidate_val);
        if(best_move_type == 0) {
            tabu_flip[best_i][best_bit] = iter + tabu_tenure;
        } else {
            tabu_replace[best_i] = iter + tabu_tenure;
        }
        current_cost += best_delta;
        if(current_cost < best_cost)
            best_cost = current_cost;
        iter++;
    }
    
    /* When the loop terminates, the candidate solution has no conflicts.
       Print the final deletion code: an m×n array (each row is a space–separated list of n bits).
    */
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            int bit = (codewords[i] >> (n - 1 - j)) & 1;
            printf("%d", bit);
            if(j < n - 1)
                printf(" ");
        }
        printf("\n");
    }
    
    /* Optional cleanup – free allocated memory.
       (For brevity not all allocated memory is freed here.)
    */
    return 0;
}