/*
   bhaskar_rao_design.c

   Usage: ./bhaskar_rao_design v b r k L seed [t_support] [alpha_support] [t_sign] [alpha_sign]

   Constructs a BRD(v,b,r,k,L) matrix (with entries in {-1,0,1}) by a two–phase approach:
   1. Find a {0,1} incidence matrix (support) with each row having r ones, each column having k ones,
      and for each pair of distinct rows the number of common ones is exactly L.
   2. Assign signs (±1) to the ones so that for every two rows the products in the L common columns 
      are balanced: exactly L/2 are +1 and L/2 are –1.

   The extra parameters (if provided) are hyperparameters that control the simulated annealing:
      t_support      : initial temperature for support phase (default 1.0)
      alpha_support  : cooling factor for support phase (default 0.999)
      t_sign         : initial temperature for sign–assignment phase (default 1.0)
      alpha_sign     : cooling factor for sign–assignment phase (default 0.999)

   This program runs indefinitely until a valid design is found.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Define macros for min and max and for accessing symmetric matrices.
   For matrices "common" and "plus" we store only a complete v x v array
   but only the entries with i<j are used in the potential computation.
*/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ACCESS(mat, i, j) ((i) < (j) ? mat[i][j] : mat[j][i])

/* Main */
int main(int argc, char **argv) {
    if(argc < 7){
        fprintf(stderr, "Usage: %s v b r k L seed [t_support] [alpha_support] [t_sign] [alpha_sign]\n", argv[0]);
        return 1;
    }
    
    int v = atoi(argv[1]);
    int b = atoi(argv[2]);
    int r = atoi(argv[3]);
    int k = atoi(argv[4]);
    int L = atoi(argv[5]);
    unsigned int seed = (unsigned int) atoi(argv[6]);
    srand(seed);
    
    /* Hyperparameters for simulated annealing moves.
       Default values are used if not provided.
    */
    double t_support = 1.0;
    double alpha_support = 0.999;
    double t_sign = 1.0;
    double alpha_sign = 0.999;
    if(argc > 7)
        t_support = atof(argv[7]);
    if(argc > 8)
        alpha_support = atof(argv[8]);
    if(argc > 9)
        t_sign = atof(argv[9]);
    if(argc > 10)
        alpha_sign = atof(argv[10]);
    
    /* Allocate support matrix (v x b) as int**.
       support[i][j] = 1 if row i has a nonzero in column j, 0 otherwise.
    */
    int **support = malloc(v * sizeof(int *));
    for (int i = 0; i < v; i++){
        support[i] = calloc(b, sizeof(int));
    }
    
    /* Allocate symmetric matrix "common" for the number of common ones in each pair of rows.
       We allocate a full v x v and use ACCESS(common, i, j) for i and j.
    */
    int **common = malloc(v * sizeof(int *));
    for (int i = 0; i < v; i++){
        common[i] = calloc(v, sizeof(int));
    }
    
    /* Phase 1: Build an initial support matrix with row sum = r and column sum = k.
       We use a column–based greedy randomized assignment. If a column cannot be assigned,
       then we restart.
    */
    int *row_sum = calloc(v, sizeof(int));
    int success = 0;
    while(!success) {
        /* Reset support and row_sum */
        for (int i = 0; i < v; i++){
            memset(support[i], 0, b * sizeof(int));
            row_sum[i] = 0;
        }
        /* Create an ordering of columns to process in random order */
        int *col_order = malloc(b * sizeof(int));
        for (int j = 0; j < b; j++) 
            col_order[j] = j;
        for (int j = 0; j < b; j++){
            int swap_index = j + rand() % (b - j);
            int temp = col_order[j];
            col_order[j] = col_order[swap_index];
            col_order[swap_index] = temp;
        }
        int valid = 1;
        for (int idx = 0; idx < b; idx++){
            int col = col_order[idx];
            /* List all available rows (those with row_sum < r) */
            int available_count = 0;
            int *available_rows = malloc(v * sizeof(int));
            for (int i = 0; i < v; i++){
                if(row_sum[i] < r){
                    available_rows[available_count] = i;
                    available_count++;
                }
            }
            if(available_count < k) {
                valid = 0;
                free(available_rows);
                break;
            }
            /* Shuffle available_rows */
            for (int i = 0; i < available_count; i++){
                int swap_index = i + rand() % (available_count - i);
                int temp = available_rows[i];
                available_rows[i] = available_rows[swap_index];
                available_rows[swap_index] = temp;
            }
            for (int t = 0; t < k; t++){
                int row = available_rows[t];
                support[row][col] = 1;
                row_sum[row]++;
            }
            free(available_rows);
        }
        free(col_order);
        for (int i = 0; i < v; i++){
            if(row_sum[i] != r) { valid = 0; break; }
        }
        if(valid) {
            success = 1;
        }
    }
    free(row_sum);
    
    /* Compute the initial pairwise common counts */
    for (int i = 0; i < v; i++){
        for (int j = i+1; j < v; j++){
            int sum = 0;
            for (int col = 0; col < b; col++){
                if(support[i][col] && support[j][col])
                    sum++;
            }
            common[i][j] = sum;
            common[j][i] = sum; // keep symmetric copy for ease
        }
    }
    
    /* Compute support potential P = sum_{i<j} |common(i,j) - L| */
    int P = 0;
    for (int i = 0; i < v; i++){
        for (int j = i+1; j < v; j++){
            P += abs(common[i][j] - L);
        }
    }
    
    /* Phase 1 (Support Improvement): Apply switch moves until P==0.
       A “switch” move chooses two distinct rows i and j and two distinct columns a and b
       such that the 2x2 submatrix is
           [1  0]
           [0  1]
       and then switches to
           [0  1]
           [1  0].
       This move preserves row and column sums. Its effect on the common counts for the pairs 
       (i,s) and (j,s) (for s different from i,j) is computed and the move is accepted if it 
       reduces the overall potential or with a probability of exp(-delta/t_support).
    */
    long long support_iter = 0;
    while(P > 0) {
        support_iter++;
        int i = rand() % v;
        int j = rand() % v;
        while(j == i)
            j = rand() % v;
        int col_a = rand() % b;
        int col_b = rand() % b;
        while(col_b == col_a)
            col_b = rand() % b;
        if (!(support[i][col_a] == 1 && support[j][col_b] == 1 &&
              support[i][col_b] == 0 && support[j][col_a] == 0))
            continue;
        int delta = 0;
        /* For every row s different from i and j, compute the change in potential
           for pairs (i, s) and (j, s). */
        for (int s = 0; s < v; s++){
            if(s == i || s == j)
                continue;
            /* For pair (i,s) : */
            int old_val = (i < s ? common[i][s] : common[s][i]);
            int new_val = old_val;
            if(support[s][col_a] == 1)
                new_val -= 1;  /* row i loses 1 in col_a */
            if(support[s][col_b] == 1)
                new_val += 1;  /* row i gains 1 in col_b */
            delta += (abs(new_val - L) - abs(old_val - L));
            
            /* For pair (j,s) : */
            old_val = (j < s ? common[j][s] : common[s][j]);
            new_val = old_val;
            if(support[s][col_b] == 1)
                new_val -= 1;  /* row j loses 1 in col_b */
            if(support[s][col_a] == 1)
                new_val += 1;  /* row j gains 1 in col_a */
            delta += (abs(new_val - L) - abs(old_val - L));
        }
        if(delta <= 0 || ((double)rand()/RAND_MAX) < exp(-delta / t_support)) {
            /* Accept move: update support entries */
            support[i][col_a] = 0;
            support[i][col_b] = 1;
            support[j][col_b] = 0;
            support[j][col_a] = 1;
            /* Update the common matrix for affected pairs. */
            for (int s = 0; s < v; s++){
                if(s == i || s == j)
                    continue;
                /* Update pair (i,s): */
                if(support[s][col_a] == 1) {
                    if(i < s)
                        common[i][s] -= 1;
                    else
                        common[s][i] -= 1;
                }
                if(support[s][col_b] == 1) {
                    if(i < s)
                        common[i][s] += 1;
                    else
                        common[s][i] += 1;
                }
                /* Update pair (j,s): */
                if(support[s][col_b] == 1) {
                    if(j < s)
                        common[j][s] -= 1;
                    else
                        common[s][j] -= 1;
                }
                if(support[s][col_a] == 1) {
                    if(j < s)
                        common[j][s] += 1;
                    else
                        common[s][j] += 1;
                }
            }
            /* Recompute overall potential P */
            int newP = 0;
            for (int u = 0; u < v; u++)
                for (int w = u+1; w < v; w++)
                    newP += abs(common[u][w] - L);
            P = newP;
        }
        t_support *= alpha_support;
        /* (Optionally one could print progress every so often.) */
    }
    /* At this point the support matrix satisfies:
         - Each row has r ones.
         - Each column has k ones.
         - For every two distinct rows the number of common ones equals L.
    */
    
    /* Phase 2: Sign Assignment.
       Allocate a sign matrix B (v x b) with entries in {-1, 0, +1}; for j with support[i][j]==1,
       assign a random sign. (Zeros remain zeros.)
    */
    int **B = malloc(v * sizeof(int *));
    for (int i = 0; i < v; i++){
        B[i] = malloc(b * sizeof(int));
        for (int j = 0; j < b; j++){
            if(support[i][j] == 1)
                B[i][j] = (rand() % 2) * 2 - 1; // Either -1 or +1.
            else
                B[i][j] = 0;
        }
    }
    
    /* Allocate plus matrix "plus" for each pair (i,j): counts in common columns where product is +1.
       We use the similar symmetric full v x v array.
    */
    int **plus = malloc(v * sizeof(int *));
    for (int i = 0; i < v; i++){
        plus[i] = calloc(v, sizeof(int));
    }
    
    /* Compute the initial plus counts for each pair (i,j) with i<j */
    for (int i = 0; i < v; i++){
        for (int j = i+1; j < v; j++){
            int count = 0;
            for (int col = 0; col < b; col++){
                if(support[i][col] && support[j][col]){
                    int prod = B[i][col] * B[j][col];
                    if(prod == 1)
                        count++;
                }
            }
            plus[i][j] = count;
            plus[j][i] = count;
        }
    }
    
    /* The target for each pair is that exactly L/2 of the products are +1.
       Let halfL = L/2.
       Define sign potential Q = sum_{i<j} |plus(i,j) - halfL|
    */
    int halfL = L / 2;  // We assume L is even.
    int Q = 0;
    for (int i = 0; i < v; i++){
        for (int j = i+1; j < v; j++){
            Q += abs(plus[i][j] - halfL);
        }
    }
    
    long long sign_iter = 0;
    while(Q > 0) {
        sign_iter++;
        /* Randomly choose a nonzero entry from some row */
        int row = rand() % v;
        int *nonzero_cols = malloc(r * sizeof(int));
        int count_nz = 0;
        for (int j = 0; j < b; j++){
            if(support[row][j] == 1){
                nonzero_cols[count_nz] = j;
                count_nz++;
            }
        }
        if(count_nz == 0) {
            free(nonzero_cols);
            continue;
        }
        int col = nonzero_cols[rand() % count_nz];
        free(nonzero_cols);
        
        int deltaQ = 0;
        /* Flipping the sign at (row, col) will affect every pair (row, s) for s with support[s][col]==1 */
        for (int s = 0; s < v; s++){
            if(s == row)
                continue;
            if(support[s][col] == 1) {
                int old_val = (row < s ? plus[row][s] : plus[s][row]);
                int new_val;
                int prod = B[row][col] * B[s][col];
                if(prod == 1)
                    new_val = old_val - 1;
                else
                    new_val = old_val + 1;
                deltaQ += (abs(new_val - halfL) - abs(old_val - halfL));
            }
        }
        if(deltaQ <= 0 || ((double)rand()/RAND_MAX) < exp(-deltaQ / t_sign)) {
            /* Accept move: update plus array for every affected pair */
            for (int s = 0; s < v; s++){
                if(s == row)
                    continue;
                if(support[s][col] == 1) {
                    int *pval = (row < s ? &plus[row][s] : &plus[s][row]);
                    int prod = B[row][col] * B[s][col];
                    if(prod == 1)
                        *pval = *pval - 1;
                    else
                        *pval = *pval + 1;
                }
            }
            /* Flip the sign */
            B[row][col] = -B[row][col];
            /* Recompute overall Q */
            int newQ = 0;
            for (int i = 0; i < v; i++){
                for (int j = i+1; j < v; j++){
                    newQ += abs(plus[i][j] - halfL);
                }
            }
            Q = newQ;
        }
        t_sign *= alpha_sign;
        /* (Optionally, one may print progress info, but here nothing is printed.) */
    }
    
    /* At this point the design is complete.
       Print final Bhaskar Rao Design: the matrix B (v rows, b columns).
       (Entries will be -1, 0, or +1.)
    */
    for (int i = 0; i < v; i++){
        for (int j = 0; j < b; j++){
            printf("%2d ", B[i][j]);
        }
        printf("\n");
    }
    
    /* Free memory */
    for (int i = 0; i < v; i++){
        free(support[i]);
        free(B[i]);
        free(common[i]);
        free(plus[i]);
    }
    free(support);
    free(B);
    free(common);
    free(plus);
    
    return 0;
}