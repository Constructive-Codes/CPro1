#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Global variables.
int n, w;
int **W;      // Global n x n matrix (values in {-1,0,1}).
int *count;   // For each row, the number of nonzero entries (should equal w when complete).
int hyper_shuffle = 1; // Hyperparameter: if nonzero then try candidate values in random order.

// Forward declarations.
int assign_row_segment(int r, int pos, int L, int need, int curr, int *dp, int *row_assign, int *base);
int solve_row(int r);

/*
  assign_row_segment is a recursive routine that “fills in” the free segment in row r.
  The free segment covers columns r through n–1 (length L = n–r). The routine is called with:
    • r       : current row being filled.
    • pos     : the current index (0 ≤ pos ≤ L) into the free segment (with col = r+pos).
    • need    : the number of additional (nonzero) entries still required in row r,
                i.e. need = w – (number already in row r from columns 0..r–1).
    • curr    : the number of nonzero entries chosen so far in this segment.
    • dp[]    : an array (length r) for each previous row i (0 ≤ i < r) holding the cumulative contribution
                from the free-segment positions already assigned. In the end for each i we need
                base[i] + dp[i] == 0.
    • row_assign[] : an array (length L) that will hold the chosen values for row r (columns r..n–1).
    • base[]  : an array (length r) with base[i] = sum_{j=0}^{r–1} W[r][j]*W[i][j] (the contribution from already fixed cells).
    
  At the leaf (pos == L) the routine returns success if (a) the number of added nonzeros equals need and 
  (b) for each 0≤i<r, (base[i] + dp[i]) equals zero.
  
  When pos < L, the routine loops over the candidate value choices in {-1,0,1} for the free cell.
  If the candidate is used in an off‐diagonal position (col = r+pos with col > r), then our global count
  for row col is incremented—and if that would cause count[col] > w the candidate is rejected.
  (When hyper_shuffle is set the candidate order is randomized.)
*/
int assign_row_segment(int r, int pos, int L, int need, int curr, int *dp, int *row_assign, int *base) {
    if (pos == L) {
        if (curr != need) return 0;
        for (int i = 0; i < r; i++) {
            if (base[i] + dp[i] != 0) return 0;
        }
        return 1;
    }
    // Feasibility check: for each earlier row i, count the number of remaining positions that can “adjust” the dot sum.
    for (int i = 0; i < r; i++) {
        int rem = 0;
        for (int j = pos; j < L; j++) {
            int col = r + j;
            if (W[i][col] != 0) rem++;  // row i is fully assigned so far.
        }
        if (abs(base[i] + dp[i]) > rem) return 0;
    }
    if (curr > need || curr + (L - pos) < need) return 0;

    int candidates[3] = { -1, 0, 1 };
    if (hyper_shuffle) {
        // Simple Fisher–Yates shuffle for the three candidates.
        for (int i = 0; i < 3; i++) {
            int j = i + rand() % (3 - i);
            int temp = candidates[i];
            candidates[i] = candidates[j];
            candidates[j] = temp;
        }
    }
    for (int k = 0; k < 3; k++) {
        int v = candidates[k];
        row_assign[pos] = v;
        int new_curr = curr + (v != 0 ? 1 : 0);
        int col = r + pos;
        int old_count = 0;
        if (col > r) {
            old_count = count[col];
            if (v != 0) {
                count[col]++; 
                if (count[col] > w) {
                    count[col] = old_count;
                    continue;
                }
            }
        }
        int new_dp[r];  // VLA: new_dp[i] = dp[i] + v * W[i][col] for each i in 0..r–1.
        for (int i = 0; i < r; i++) {
            new_dp[i] = dp[i] + v * W[i][col];
        }
        if (assign_row_segment(r, pos + 1, L, need, new_curr, new_dp, row_assign, base)) {
            return 1;
        }
        if (col > r && v != 0) {
            count[col] = old_count; // backtrack count update.
        }
    }
    return 0;
}

/*
  solve_row is the primary recursive backtracking routine that “builds” the symmetric matrix one row at a time.
  For row r the entries in columns 0..r–1 are already set; we now assign the free segment (columns r..n–1)
  using assign_row_segment.
  
  (We store the current nonzero count per row in the global array “count”. When an off‐diagonal value is set in row r,
   the symmetric cell W[j][r] is set at the same time and count[j] is updated.)
  
  If a complete assignment for row r is found (and the dot products between row r and each earlier row are verified to be 0)
  then we update W[r][*] accordingly and call solve_row(r+1); if we eventually succeed all the way to r==n,
  solve_row returns success.
*/
int solve_row(int r) {
    if (r == n) return 1;  // all rows finished.
    int old_count_r = count[r];
    // Save counts for future rows (rows r+1..n–1) for backtracking.
    int *old_count_future = (int*)malloc((n - r - 1) * sizeof(int));
    for (int j = r + 1; j < n; j++) {
        old_count_future[j - (r + 1)] = count[j];
    }
    // Compute the base vector (of length r) for row r. For each i < r,
    // base[i] = sum_{j=0}^{r-1} W[r][j] * W[i][j].
    int *base = NULL;
    if (r > 0) {
        base = (int*)malloc(r * sizeof(int));
        for (int i = 0; i < r; i++) {
            base[i] = 0;
            for (int j = 0; j < r; j++) {
                base[i] += W[r][j] * W[i][j];
            }
        }
    }
    int L = n - r;         // number of cells to assign for row r (columns r..n–1).
    int need = w - count[r]; // additional nonzero values needed in row r.
    if (need < 0 || need > L) {
        if (base) free(base);
        free(old_count_future);
        return 0;
    }
    int *row_assign = (int*)malloc(L * sizeof(int));
    int *dp = NULL;
    if (r > 0) {
        dp = (int*)malloc(r * sizeof(int));
        for (int i = 0; i < r; i++) dp[i] = 0;
    }

    int success = assign_row_segment(r, 0, L, need, 0, (r > 0 ? dp : NULL), row_assign, (r > 0 ? base : NULL));
    if (!success) {
        if (dp) free(dp);
        if (base) free(base);
        free(row_assign);
        for (int j = r + 1; j < n; j++) {
            count[j] = old_count_future[j - (r + 1)];
        }
        count[r] = old_count_r;
        free(old_count_future);
        return 0;
    }
    // Having found a valid free-segment for row r, copy row_assign into row r.
    for (int pos = 0; pos < L; pos++) {
        int col = r + pos;
        W[r][col] = row_assign[pos];
        if (col > r) {
            W[col][r] = row_assign[pos];  // enforce symmetry
        }
    }
    // Now add into count[r] the number of newly placed nonzeros.
    int added = 0;
    for (int pos = 0; pos < L; pos++) {
        if (row_assign[pos] != 0)
            added++;
    }
    count[r] += added;
    if (count[r] != w) { // should equal w now
        free(row_assign);
        if (dp) free(dp);
        if (base) free(base);
        for (int j = r + 1; j < n; j++) {
            count[j] = old_count_future[j - (r + 1)];
        }
        count[r] = old_count_r;
        free(old_count_future);
        return 0;
    }
    free(row_assign);
    if (dp) free(dp);
    if (base) free(base);
    // Check dot products of row r with each earlier row.
    for (int i = 0; i < r; i++) {
        int dot = 0;
        for (int j = 0; j < n; j++) {
            dot += W[r][j] * W[i][j];
        }
        if (dot != 0) {
            for (int j = r + 1; j < n; j++) {
                count[j] = old_count_future[j - (r + 1)];
            }
            count[r] = old_count_r;
            free(old_count_future);
            return 0;
        }
    }
    free(old_count_future);
    if (solve_row(r + 1)) return 1;
    // Backtrack: revert assignments for row r.
    for (int j = r; j < n; j++) {
        if (j > r && W[r][j] != 0) {
            count[j]--; // revert off‐diagonal update.
        }
        if (W[r][j] != 0) {
            if (j == r)
                count[r]--;
            W[r][j] = 0;
        }
        if (j > r)
            W[j][r] = 0;
    }
    count[r] = old_count_r;
    return 0;
}

int main(int argc, char *argv[]){
    if (argc < 4) {
        fprintf(stderr, "Usage: %s n w seed [shuffle]\n", argv[0]);
        return 1;
    }
    n = atoi(argv[1]);
    w = atoi(argv[2]);
    int seed = atoi(argv[3]);
    srand(seed);
    if (argc >= 5) {
        hyper_shuffle = atoi(argv[4]);
    }
    // Dynamically allocate global matrix W and count array.
    W = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        W[i] = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) {
            W[i][j] = 0;
        }
    }
    count = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        count[i] = 0;
    }
    // We “retry” by reinitializing the global state if needed.
    while (1) {
        // Reinitialize matrix W and count array.
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                W[i][j] = 0;
            }
            count[i] = 0;
        }
        if (solve_row(0)) {
            // (Optional extra check: Verify that every row has exactly w nonzeros, and that every pair of distinct rows is orthogonal.)
            int valid = 1;
            for (int i = 0; i < n; i++) {
                if (count[i] != w) { valid = 0; break; }
            }
            for (int i = 0; i < n && valid; i++) {
                for (int j = i + 1; j < n && valid; j++) {
                    int dot = 0;
                    for (int k = 0; k < n; k++) {
                        dot += W[i][k] * W[j][k];
                    }
                    if (dot != 0) valid = 0;
                }
            }
            if (valid) {
                // Print the final symmetric weighing matrix.
                for (int i = 0; i < n; i++){
                    for (int j = 0; j < n; j++){
                        printf("%d ", W[i][j]);
                    }
                    printf("\n");
                }
                break;
            }
        }
        // Otherwise, the search tree was exhausted for this pass; retry.
    }
    // Clean up:
    for (int i = 0; i < n; i++){
        free(W[i]);
    }
    free(W);
    free(count);
    return 0;
}