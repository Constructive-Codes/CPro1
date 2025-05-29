/*
  Johnson Clique Cover solver for J(N,k) using local search.
  
  Usage:  ./jcc N k C seed [penalty_weight] [temperature]

  The program builds an initial cover (via a greedy method) and then
  iteratively applies local modifications (addition, removal, swap moves)
  guided by a cost function:
 
      cost = (# uncovered vertices) + penalty_weight * |(# cliques) - C|.
 
  A vertex is a k‐subset of [N]. A clique is a maximal clique of type 0 or type 1.
 
  Type 0 clique: Given a (k-1)-subset S (encoded as a bit mask), the clique
  covers each vertex of the form S ∪ {x} for each x not in S.
 
  Type 1 clique: Given a (k+1)-subset S, the clique covers each vertex S \ {x},
  for x in S.
 
  The program runs indefinitely until a valid cover (complete coverage and
  exactly C cliques) is found, then prints to stdout a list of C lines. Each line
  starts with 0 or 1 (the clique type) followed by the (1-indexed) elements in S.
 
  (N<=15, C<800.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Structure to represent a clique.
typedef struct {
    int type; // 0 for type0, 1 for type1.
    int base; // Bit mask representing the set S.
} Clique;

// Global variables.
int N, k, targetC, num_vertices;
int *vertices = NULL;      // Array of all vertices (each is a bit mask with exactly k bits set).
int *coverage_count = NULL; // Array of length num_vertices: how many cliques currently cover vertex i.
int *vertex_index = NULL;   // Array of length (1 << N): for each mask, its index in vertices (or -1 if not a vertex).
int uncovered_count = 0;    // Number of vertices for which coverage_count is zero.

Clique *solution = NULL;   // Dynamic array of cliques in the current cover.
int num_solution = 0;      // Current number of cliques in the cover.
int solution_capacity = 0; // Allocated capacity for the solution array.

double penalty_weight = 1.0; // Hyperparameter: weight for difference between current clique count and targetC.
double temperature = 1.0;    // Hyperparameter: temperature used for simulated annealing.
double current_cost = 0.0;   // Current cost = uncovered_count + penalty_weight * |num_solution - targetC|.

// Computes the cost given uncovered vertices and the current number of cliques.
double compute_cost(int uncovered, int sol_count) {
    return uncovered + penalty_weight * fabs(sol_count - targetC);
}

// Update the global current_cost.
void update_current_cost() {
    current_cost = compute_cost(uncovered_count, num_solution);
}

// Add clique 'clique' to the current solution and update coverage information.
void add_clique_to_solution(Clique clique) {
    int x, idx, vertex_mask;
    if (clique.type == 0) {
        // Type 0: for each x not in the base, the vertex v = base U {x} is covered.
        for (x = 0; x < N; x++) {
            if (!(clique.base & (1 << x))) {
                vertex_mask = clique.base | (1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    if (coverage_count[idx] == 0)
                        uncovered_count--;
                    coverage_count[idx]++;
                }
            }
        }
    } else {
        // Type 1: for each x in the base, the vertex v = base \ {x} is covered.
        for (x = 0; x < N; x++) {
            if (clique.base & (1 << x)) {
                vertex_mask = clique.base & ~(1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    if (coverage_count[idx] == 0)
                        uncovered_count--;
                    coverage_count[idx]++;
                }
            }
        }
    }
    // Add the clique to the solution array.
    if (num_solution == solution_capacity) {
        solution_capacity = (solution_capacity == 0 ? 1024 : solution_capacity * 2);
        solution = realloc(solution, solution_capacity * sizeof(Clique));
        if (!solution) {
            fprintf(stderr, "Memory allocation error in solution array.\n");
            exit(1);
        }
    }
    solution[num_solution++] = clique;
    update_current_cost();
}

// Remove the clique at index 'index' from the solution and update coverage.
void remove_clique_from_solution(int index) {
    int x, idx, vertex_mask;
    Clique clique = solution[index];
    if (clique.type == 0) {
        for (x = 0; x < N; x++) {
            if (!(clique.base & (1 << x))) {
                vertex_mask = clique.base | (1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    coverage_count[idx]--;
                    if (coverage_count[idx] == 0)
                        uncovered_count++;
                }
            }
        }
    } else {
        for (x = 0; x < N; x++) {
            if (clique.base & (1 << x)) {
                vertex_mask = clique.base & ~(1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    coverage_count[idx]--;
                    if (coverage_count[idx] == 0)
                        uncovered_count++;
                }
            }
        }
    }
    // Remove by replacing with the last clique.
    solution[index] = solution[num_solution - 1];
    num_solution--;
    update_current_cost();
}

// Given an arbitrary coverage array "cov" (of length num_vertices),
// compute the gain of adding clique "clique" (i.e. number of vertices that are newly covered).
int candidate_gain(Clique clique, int *cov) {
    int gain = 0, x, idx, vertex_mask;
    if (clique.type == 0) {
        for (x = 0; x < N; x++) {
            if (!(clique.base & (1 << x))) {
                vertex_mask = clique.base | (1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1 && cov[idx] == 0)
                    gain++;
            }
        }
    } else {
        for (x = 0; x < N; x++) {
            if (clique.base & (1 << x)) {
                vertex_mask = clique.base & ~(1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1 && cov[idx] == 0)
                    gain++;
            }
        }
    }
    return gain;
}

// Given an uncovered vertex u (represented as a bit mask with k bits),
// choose the candidate clique (of type0 or type1) that would cover u and yield maximum gain.
// The current coverage array (of length num_vertices) is passed in "cov".
Clique choose_best_candidate_for_vertex(int u, int *cov) {
    Clique best_candidate;
    best_candidate.type = -1;
    best_candidate.base = 0;
    int best_gain = -1, current_gain;
    int x;
    // Type 0 candidates: for each element in u (remove one element).
    for (x = 0; x < N; x++) {
        if (u & (1 << x)) {
            int candidate_base = u & ~(1 << x);
            Clique candidate;
            candidate.type = 0;
            candidate.base = candidate_base;
            current_gain = candidate_gain(candidate, cov);
            if (current_gain > best_gain) {
                best_gain = current_gain;
                best_candidate = candidate;
            }
        }
    }
    // Type 1 candidates: for each element not in u (add one element).
    int y;
    for (y = 0; y < N; y++) {
        if (!(u & (1 << y))) {
            int candidate_base = u | (1 << y);
            Clique candidate;
            candidate.type = 1;
            candidate.base = candidate_base;
            current_gain = candidate_gain(candidate, cov);
            if (current_gain > best_gain) {
                best_gain = current_gain;
                best_candidate = candidate;
            }
        }
    }
    return best_candidate;
}

// Simulated annealing acceptance test: accept if delta <= 0 or with probability exp(-delta/temperature) otherwise.
int accept_move(double delta) {
    if (delta <= 0)
        return 1;
    double prob = exp(-delta / temperature);
    double r = (double)rand() / RAND_MAX;
    return (r < prob);
}

// Try an "addition" move: choose an uncovered vertex (or arbitrary if all are covered but num_solution < targetC),
// generate candidate cliques for it, and add the one with highest gain.
// REVISED: Instead of malloc/free for building a list of uncovered vertices, we simply use uncovered_count.
int try_add_move() {
    int chosen_index;
    int u;
    if (uncovered_count > 0) {
        int r = rand() % uncovered_count;
        int count = 0;
        for (int i = 0; i < num_vertices; i++) {
            if (coverage_count[i] == 0) {
                if (count == r) {
                    chosen_index = i;
                    break;
                }
                count++;
            }
        }
        u = vertices[chosen_index];
    } else {
        chosen_index = rand() % num_vertices;
        u = vertices[chosen_index];
    }
    // Choose the best candidate clique for vertex u (using the current coverage_count array).
    Clique candidate = choose_best_candidate_for_vertex(u, coverage_count);
    int add_gain = candidate_gain(candidate, coverage_count);
    int new_uncovered = uncovered_count - add_gain;
    int new_solution_count = num_solution + 1;
    double new_cost = new_uncovered + penalty_weight * fabs(new_solution_count - targetC);
    double delta = new_cost - current_cost;
    if (accept_move(delta)) {
        add_clique_to_solution(candidate);
        return 1;
    }
    return 0;
}

// Try a "removal" move: randomly select an existing clique (if there are excess cliques)
// and simulate its removal (i.e. count how many vertices would become uncovered), then remove it if accepted.
int try_removal_move() {
    if (num_solution == 0)
        return 0;
    if (num_solution <= targetC)
        return 0; // Only try removal if we have more than targetC cliques.
    int attempts = 10;
    for (int a = 0; a < attempts; a++) {
        int idx = rand() % num_solution;
        Clique candidate = solution[idx];
        int loss = 0, x, idx_v, vertex_mask;
        if (candidate.type == 0) {
            for (x = 0; x < N; x++) {
                if (!(candidate.base & (1 << x))) {
                    vertex_mask = candidate.base | (1 << x);
                    idx_v = vertex_index[vertex_mask];
                    if (idx_v != -1 && coverage_count[idx_v] == 1)
                        loss++;
                }
            }
        } else {
            for (x = 0; x < N; x++) {
                if (candidate.base & (1 << x)) {
                    vertex_mask = candidate.base & ~(1 << x);
                    idx_v = vertex_index[vertex_mask];
                    if (idx_v != -1 && coverage_count[idx_v] == 1)
                        loss++;
                }
            }
        }
        int new_uncovered = uncovered_count + loss;
        int new_solution_count = num_solution - 1;
        double new_cost = new_uncovered + penalty_weight * fabs(new_solution_count - targetC);
        double delta = new_cost - current_cost;
        if (accept_move(delta)) {
            remove_clique_from_solution(idx);
            return 1;
        }
    }
    return 0;
}

// Helper: given a clique, simulate its removal by subtracting its contributions from temp_cov.
void simulate_removal(Clique clique, int *temp_cov) {
    int x, idx, vertex_mask;
    if (clique.type == 0) {
        for (x = 0; x < N; x++) {
            if (!(clique.base & (1 << x))) {
                vertex_mask = clique.base | (1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    temp_cov[idx]--;
                }
            }
        }
    } else {
        for (x = 0; x < N; x++) {
            if (clique.base & (1 << x)) {
                vertex_mask = clique.base & ~(1 << x);
                idx = vertex_index[vertex_mask];
                if (idx != -1) {
                    temp_cov[idx]--;
                }
            }
        }
    }
}

// Try a "swap" move: remove one clique and then add a new clique.
int try_swap_move() {
    if (num_solution == 0)
        return 0;
    int idx = rand() % num_solution;
    Clique removed = solution[idx];
    // Create a temporary copy of coverage_count to simulate removal.
    int *temp_cov = malloc(num_vertices * sizeof(int));
    if (!temp_cov) { fprintf(stderr, "Memory allocation error\n"); exit(1); }
    memcpy(temp_cov, coverage_count, num_vertices * sizeof(int));
    simulate_removal(removed, temp_cov);
    int temp_uncovered = 0;
    for (int i = 0; i < num_vertices; i++) {
        if (temp_cov[i] == 0)
            temp_uncovered++;
    }
    if (temp_uncovered == 0) {
        free(temp_cov);
        return 0;
    }
    // Instead of allocating an array for uncovered vertices, pick one by a linear scan.
    int r = rand() % temp_uncovered;
    int count = 0;
    int chosen_index = -1;
    for (int i = 0; i < num_vertices; i++) {
        if (temp_cov[i] == 0) {
            if (count == r) {
                chosen_index = i;
                break;
            }
            count++;
        }
    }
    int u = vertices[chosen_index];
    // Now choose the best candidate clique for u using the simulated coverage (temp_cov).
    Clique candidate = choose_best_candidate_for_vertex(u, temp_cov);
    int add_gain = candidate_gain(candidate, temp_cov);
    int new_uncovered = temp_uncovered - add_gain;
    int new_solution_count = num_solution; // Swap preserves the number of cliques.
    double new_cost = new_uncovered + penalty_weight * fabs(new_solution_count - targetC);
    double delta = new_cost - current_cost;
    int accepted = accept_move(delta);
    if (accepted) {
        // Commit the swap: remove the chosen clique and add the candidate.
        remove_clique_from_solution(idx);
        add_clique_to_solution(candidate);
        free(temp_cov);
        return 1;
    }
    free(temp_cov);
    return 0;
}

// Print out the final Johnson Clique Cover in the required format.
// For each clique, print one line: the type (0 or 1) followed by the (1-indexed) elements of its base in increasing order.
void print_solution() {
    for (int i = 0; i < num_solution; i++) {
        Clique c = solution[i];
        if (c.type == 0) {
            printf("0");
            for (int x = 0; x < N; x++) {
                if (c.base & (1 << x))
                    printf(" %d", x + 1);
            }
            printf("\n");
        } else {
            printf("1");
            for (int x = 0; x < N; x++) {
                if (c.base & (1 << x))
                    printf(" %d", x + 1);
            }
            printf("\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s N k C seed [penalty_weight] [temperature]\n", argv[0]);
        return 1;
    }
    N = atoi(argv[1]);
    k = atoi(argv[2]);
    targetC = atoi(argv[3]);
    int seed = atoi(argv[4]);
    srand(seed);

    // Parse extra hyperparameters if provided.
    if (argc >= 6)
        penalty_weight = atof(argv[5]);
    if (argc >= 7)
        temperature = atof(argv[6]);

    // Allocate vertex_index array for all masks from 0 to (1 << N) - 1.
    int total_masks = 1 << N;
    vertex_index = malloc(total_masks * sizeof(int));
    if (!vertex_index) { fprintf(stderr, "Memory allocation error\n"); exit(1); }
    for (int i = 0; i < total_masks; i++)
        vertex_index[i] = -1;

    // Generate all vertices (bit masks with exactly k ones) and store them in the vertices array.
    vertices = malloc(total_masks * sizeof(int)); // Worst-case allocation.
    if (!vertices) { fprintf(stderr, "Memory allocation error\n"); exit(1); }
    num_vertices = 0;
    for (int mask = 0; mask < total_masks; mask++) {
        if (__builtin_popcount(mask) == k) {
            vertices[num_vertices] = mask;
            vertex_index[mask] = num_vertices;
            num_vertices++;
        }
    }

    // Allocate and initialize coverage_count array.
    coverage_count = calloc(num_vertices, sizeof(int));
    if (!coverage_count) { fprintf(stderr, "Memory allocation error\n"); exit(1); }
    uncovered_count = num_vertices;

    // Initialize solution array.
    solution_capacity = 1024;
    solution = malloc(solution_capacity * sizeof(Clique));
    if (!solution) { fprintf(stderr, "Memory allocation error\n"); exit(1); }
    num_solution = 0;

    // Greedy initial cover: while there is at least one uncovered vertex,
    // pick the first uncovered vertex and add the candidate clique (with maximum gain) covering it.
    while (uncovered_count > 0) {
        int u_idx = -1;
        for (int i = 0; i < num_vertices; i++) {
            if (coverage_count[i] == 0) { u_idx = i; break; }
        }
        if (u_idx == -1)
            break; // Should not happen if uncovered_count > 0.
        int u = vertices[u_idx];
        Clique candidate = choose_best_candidate_for_vertex(u, coverage_count);
        add_clique_to_solution(candidate);
    }

    // If the greedy solution has fewer than targetC cliques, add arbitrary (redundant) cliques until count equals targetC.
    while (num_solution < targetC) {
        Clique candidate;
        if (rand() % 2 == 0) {
            // Type 0: generate a random (k-1)-subset.
            candidate.type = 0;
            candidate.base = 0;
            int count = (k - 1);
            int arr[N];
            for (int i = 0; i < N; i++)
                arr[i] = i;
            for (int i = 0; i < count; i++) {
                int j = i + rand() % (N - i);
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
            for (int i = 0; i < count; i++)
                candidate.base |= (1 << arr[i]);
        } else {
            // Type 1: generate a random (k+1)-subset.
            candidate.type = 1;
            candidate.base = 0;
            int count = (k + 1);
            int arr[N];
            for (int i = 0; i < N; i++)
                arr[i] = i;
            for (int i = 0; i < count; i++) {
                int j = i + rand() % (N - i);
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
            for (int i = 0; i < count; i++)
                candidate.base |= (1 << arr[i]);
        }
        add_clique_to_solution(candidate);
    }
    update_current_cost();

    // Local search loop.  Keep trying moves until the cost becomes 0, i.e.,
    // until every vertex is covered AND the number of cliques equals targetC.
    while (current_cost > 0) {
        int move_choice;
        // Determine allowed moves.
        int allow_add = ((uncovered_count > 0) || (num_solution < targetC)) ? 1 : 0;
        int allow_removal = (num_solution > targetC) ? 1 : 0;
        int allow_swap = 1; // Always allowed.
        int move_options[3], num_options = 0;
        if (allow_add)
            move_options[num_options++] = 0;
        if (allow_removal)
            move_options[num_options++] = 1;
        if (allow_swap)
            move_options[num_options++] = 2;
        move_choice = move_options[rand() % num_options];

        if (move_choice == 0) {
            try_add_move();
        } else if (move_choice == 1) {
            try_removal_move();
        } else if (move_choice == 2) {
            try_swap_move();
        }
        // Loop indefinitely until current_cost == 0.
    }

    // When a valid cover is found (current_cost == 0), print the cover.
    print_solution();

    // Free allocated memory.
    free(vertices);
    free(coverage_count);
    free(vertex_index);
    free(solution);

    return 0;
}