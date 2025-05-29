#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Structure for a clique.  For a clique of type 0, S is a k-1–subset;
   for type 1, S is a k+1–subset.  (S is stored as a bitmask using bits 0..N-1.)
*/
typedef struct {
    int type; // 0 or 1.
    int S;    // The integer bitmask representing the selected subset S.
} Clique;

/* Function prototypes */
int binom(int n, int k);
void gen_vertices(int start, int chosen, int k, int N, int current, int *vertices, int *index, int *vertexMapping);
void get_clique_coverage(Clique cl, int N, int k, int *vertexMapping, int *out, int *out_size);
int random_subset(int N, int r);
Clique generate_random_clique(int N, int k);
int compute_delta(Clique old, Clique new, int N, int k, int *cover_count, int *vertexMapping);
void add_clique(Clique cl, int N, int k, int *cover_count, int *vertexMapping);
void remove_clique(Clique cl, int N, int k, int *cover_count, int *vertexMapping);

int main(int argc, char *argv[]) {
    if(argc < 5) {
         fprintf(stderr, "Usage: %s N k C seed [p_random]\n", argv[0]);
         return 1;
    }
    int N = atoi(argv[1]);
    int k = atoi(argv[2]);
    int C = atoi(argv[3]);
    int seed = atoi(argv[4]);
    srand(seed);

    // Hyperparameter: probability with which we take a totally random move.
    double p_random = 0.1;
    if(argc >= 6) {
         p_random = atof(argv[5]);
    }
    
    /* Precompute the vertices of J(N,k).  That is, list all k–element subsets
       (each represented as an N–bit bitmask with exactly k bits on). We also set up an array
       vertexMapping[mask] giving (if mask has exactly k bits) its index in vertices.
    */
    int nVertices = binom(N, k);
    int *vertices = (int*)malloc(nVertices * sizeof(int));
    int *vertexMapping = (int*)malloc((1 << N) * sizeof(int));
    for (int i = 0; i < (1 << N); i++) {
         vertexMapping[i] = -1;
    }
    int index = 0;
    gen_vertices(0, 0, k, N, 0, vertices, &index, vertexMapping);
    
    /* cover_count[i] will count how many cliques in our candidate cover currently cover
       the vertex with index i (i.e. the i–th k–subset).
    */
    int *cover_count = (int*)calloc(nVertices, sizeof(int)); // all zero initially.
    
    /* Build an initial candidate solution consisting of C random cliques.
       (Each clique is maximal – that is, of type 0 or type 1 – and is stored in our array "solution".)
    */
    Clique *solution = (Clique*)malloc(C * sizeof(Clique));
    for (int i = 0; i < C; i++) {
         solution[i] = generate_random_clique(N, k);
         add_clique(solution[i], N, k, cover_count, vertexMapping);
    }
    
    // Compute the initial objective: number of vertices (k–subsets) not covered.
    int uncovered = 0;
    for (int i = 0; i < nVertices; i++) {
         if (cover_count[i] == 0)
             uncovered++;
    }
    
    long long iterations = 0;
    /* Main local–search loop: keep modifying the candidate cover until every vertex is covered.
       Two types of moves are attempted on every iteration:
         (a) With probability p_random, a totally random move – a randomly chosen clique is replaced
             by a randomly generated clique.
         (b) Otherwise, we choose an uncovered vertex and then (from the fact that every vertex is contained
             in many cliques) randomly generate a candidate clique that would cover that vertex; we then
             replace a randomly chosen clique with that candidate clique if doing so does not worsen coverage.
    */
    while (uncovered > 0) {
         iterations++;
         // Occasionally recalc the number of uncovered vertices exactly (every 10000 iterations).
         if (iterations % 10000 == 0) {
             int tot = 0;
             for (int i = 0; i < nVertices; i++) {
                   if (cover_count[i] == 0)
                         tot++;
             }
             uncovered = tot;
         }
         double r = (double)rand() / (RAND_MAX + 1.0);
         if (r < p_random) {
              /* Random move: select a random candidate clique in our cover and replace it with
                 a completely random clique (of type 0 or 1).
              */
              int idx = rand() % C;
              Clique new_cl = generate_random_clique(N, k);
              int delta = compute_delta(solution[idx], new_cl, N, k, cover_count, vertexMapping);
              remove_clique(solution[idx], N, k, cover_count, vertexMapping);
              add_clique(new_cl, N, k, cover_count, vertexMapping);
              solution[idx] = new_cl;
              uncovered += delta;
         } else {
              /* Heuristic move: choose one vertex that is not covered and generate cliques that would cover it.
                 Every k–subset v is contained in exactly k cliques of type 0 (for each x in v, using S = v\{x})
                 and in (N–k) cliques of type 1 (for each x not in v, using S = v ∪ {x}). Thus there are exactly N moves.
              */
              int num_uncov = 0;
              int *uncov_indices = (int*)malloc(nVertices * sizeof(int));
              for (int i = 0; i < nVertices; i++) {
                   if (cover_count[i] == 0) {
                         uncov_indices[num_uncov++] = i;
                   }
              }
              if (num_uncov == 0) {
                  free(uncov_indices);
                  continue;
              }
              int chosen_uncov = uncov_indices[rand() % num_uncov];
              free(uncov_indices);
              int vmask = vertices[chosen_uncov]; // the bitmask for the uncovered vertex
              
              /* Generate the candidate moves covering vmask.
                 For type 0: for each bit in vmask (each element in vmask) let S = (vmask without that element).
                 For type 1: for each element not in vmask let S = (vmask plus that element).
              */
              int candidate_count = 0;
              Clique candidate_moves[32];  // N is at most 15 so 32 is safe.
              for (int x = 0; x < N; x++) {
                   if (vmask & (1 << x)) {
                         Clique cand;
                         cand.type = 0;
                         cand.S = vmask & ~(1 << x);
                         candidate_moves[candidate_count++] = cand;
                   }
              }
              for (int x = 0; x < N; x++) {
                   if (!(vmask & (1 << x))) {
                         Clique cand;
                         cand.type = 1;
                         cand.S = vmask | (1 << x);
                         candidate_moves[candidate_count++] = cand;
                   }
              }
              // There are candidate_count == N moves.
              Clique move = candidate_moves[rand() % candidate_count];
              
              /* Also pick a candidate clique in the current cover (chosen at random) to replace.
              */
              int idx = rand() % C;
              int delta = compute_delta(solution[idx], move, N, k, cover_count, vertexMapping);
              if (delta <= 0) {  // Accept moves that do not worsen (or improve) coverage.
                   remove_clique(solution[idx], N, k, cover_count, vertexMapping);
                   add_clique(move, N, k, cover_count, vertexMapping);
                   solution[idx] = move;
                   uncovered += delta;
              }
         }
    }
    
    /* Once a valid clique cover is found (every vertex is covered), output the cover.
       Each clique is output on one line as:
         <type> <s_1> <s_2> ... 
       Where the s_i (taken from S) are printed in increasing order (and adjusted to be 1-indexed).
    */
    for (int i = 0; i < C; i++) {
         printf("%d", solution[i].type);
         int set = solution[i].S;
         // For type 0 we have |S| = k-1; for type 1, |S| = k+1.
         for (int x = 0; x < N; x++) {
              if (set & (1 << x)) {
                  printf(" %d", x + 1);
              }
         }
         printf("\n");
    }
    
    /* Clean up */
    free(vertices);
    free(vertexMapping);
    free(cover_count);
    free(solution);
    
    return 0;
}

/* Compute binomial coefficient "n choose k" */
int binom(int n, int k) {
    if (k > n)
         return 0;
    if (k == 0 || k == n)
         return 1;
    int res = 1;
    if (k > n - k)
         k = n - k;
    for (int i = 1; i <= k; i++) {
         res = res * (n - i + 1) / i;
    }
    return res;
}

/* Recursively generate all k–element subsets of {0,1,...,N-1} (represented as bitmasks)
   and store them in the array "vertices". Also set vertexMapping[mask] = index.
*/
void gen_vertices(int start, int chosen, int k, int N, int current, int *vertices, int *index, int *vertexMapping) {
    if (chosen == k) {
         vertices[*index] = current;
         vertexMapping[current] = *index;
         (*index)++;
         return;
    }
    for (int i = start; i < N; i++) {
         gen_vertices(i+1, chosen+1, k, N, current | (1 << i), vertices, index, vertexMapping);
    }
}

/* For a given clique cl, compute the list of vertices (by their indices in the vertices array)
   that cl covers.  For type 0, S is a (k-1)–subset and the clique consists of all vertices S ∪ {x} 
   for each x not in S.  For type 1, S is a (k+1)–subset and the clique consists of all vertices S\{x}
   for each x in S.
*/
void get_clique_coverage(Clique cl, int N, int k, int *vertexMapping, int *out, int *out_size) {
    int count = 0;
    if (cl.type == 0) {
         for (int x = 0; x < N; x++) {
              if (!(cl.S & (1 << x))) {
                   int vertex = cl.S | (1 << x); // This bitmask has k bits.
                   int idx = vertexMapping[vertex];
                   out[count++] = idx;
              }
         }
    } else { // cl.type == 1.
         for (int x = 0; x < N; x++) {
              if (cl.S & (1 << x)) {
                   int vertex = cl.S & ~(1 << x);
                   int idx = vertexMapping[vertex];
                   out[count++] = idx;
              }
         }
    }
    *out_size = count;
}

/* Generate a random r–subset of {0, …, N-1} (returned as a bitmask). */
int random_subset(int N, int r) {
    int subset = 0;
    int count = 0;
    while (count < r) {
         int candidate = rand() % N;
         if (!(subset & (1 << candidate))) {
              subset |= (1 << candidate);
              count++;
         }
    }
    return subset;
}

/* Generate a clique at random.  (If possible we choose type 0 or type 1 with equal probability.)
   For type 0 the defining subset S has k-1 elements; for type 1 it has k+1 elements.
*/
Clique generate_random_clique(int N, int k) {
    Clique cl;
    if (k - 1 < 0) { 
         cl.type = 1; 
         cl.S = random_subset(N, k + 1);
         return cl;
    }
    if (k + 1 > N) { 
         cl.type = 0; 
         cl.S = random_subset(N, k - 1);
         return cl;
    }
    if (rand() % 2 == 0) {
         cl.type = 0;
         cl.S = random_subset(N, k - 1);
    } else {
         cl.type = 1;
         cl.S = random_subset(N, k + 1);
    }
    return cl;
}

/* Compute the change (delta) in the objective (number of uncovered vertices) if we
   were to remove clique "old" and replace it with clique "new".  (The cover_count array
   contains the counts before removal.)
   For each vertex u that is covered by old but not by new, if cover_count[u]==1 then removal will cause u to become uncovered (+1).
   For each vertex u that is covered by new but not by old, if cover_count[u]==0 then addition will cover u (-1).
   Therefore delta = (# uncovered gained) - (# uncovered lost).
*/
int compute_delta(Clique old, Clique new, int N, int k, int *cover_count, int *vertexMapping) {
    int old_cov[32], new_cov[32];
    int old_size, new_size;
    get_clique_coverage(old, N, k, vertexMapping, old_cov, &old_size);
    get_clique_coverage(new, N, k, vertexMapping, new_cov, &new_size);
    int delta = 0;
    for (int i = 0; i < old_size; i++) {
         int found = 0;
         for (int j = 0; j < new_size; j++) {
              if (old_cov[i] == new_cov[j]) { found = 1; break; }
         }
         if (!found) {
              if (cover_count[old_cov[i]] == 1)
                   delta++;
         }
    }
    for (int i = 0; i < new_size; i++) {
         int found = 0;
         for (int j = 0; j < old_size; j++) {
              if (new_cov[i] == old_cov[j]) { found = 1; break; }
         }
         if (!found) {
              if (cover_count[new_cov[i]] == 0)
                   delta--;
         }
    }
    return delta;
}

/* Add the clique cl to the cover_count array (i.e. increment the count for every vertex that cl covers). */
void add_clique(Clique cl, int N, int k, int *cover_count, int *vertexMapping) {
    int temp[32], size;
    get_clique_coverage(cl, N, k, vertexMapping, temp, &size);
    for (int i = 0; i < size; i++) {
         cover_count[temp[i]]++;
    }
}

/* Remove the clique cl from the cover_count array (i.e. decrement the count for every vertex that cl covers). */
void remove_clique(Clique cl, int N, int k, int *cover_count, int *vertexMapping) {
    int temp[32], size;
    get_clique_coverage(cl, N, k, vertexMapping, temp, &size);
    for (int i = 0; i < size; i++) {
         cover_count[temp[i]]--;
    }
}