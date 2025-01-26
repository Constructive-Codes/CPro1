#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Function prototypes
void initialize_population(int** population, int V, int B, int R, int K, int population_size);
void evaluate_fitness(int** population, int* fitness, int V, int B, int p1, int p2, int R, int K, int L, int population_size);
void select_and_crossover(int** population, int* fitness, int V, int B, int population_size, float crossover_prob);
void mutate(int** population, int V, int B, float mutation_rate, int population_size);
int is_valid_solution(int* individual, int V, int B, int p1, int p2, int R, int K, int L);
void print_solution(int* individual, int V, int B);

int main(int argc, char** argv) {
    if (argc < 9) {
        printf("Usage: %s V B p1 p2 R K L seed [population_size] [crossover_prob] [mutation_rate]\n", argv[0]);
        return 1;
    }

    // Parse command-line arguments
    int V = atoi(argv[1]);
    int B = atoi(argv[2]);
    int p1 = atoi(argv[3]);
    int p2 = atoi(argv[4]);
    int R = atoi(argv[5]);
    int K = atoi(argv[6]);
    int L = atoi(argv[7]);
    int seed = atoi(argv[8]);

    // Default hyperparameters
    int population_size = 100;
    float crossover_prob = 0.8;
    float mutation_rate = 0.01;

    // Override hyperparameters if provided
    if (argc > 9) population_size = atoi(argv[9]);
    if (argc > 10) crossover_prob = atof(argv[10]);
    if (argc > 11) mutation_rate = atof(argv[11]);

    // Seed the random number generator
    srand(seed);

    // Initialize population
    int** population = (int**)malloc(population_size * sizeof(int*));
    for (int i = 0; i < population_size; ++i) {
        population[i] = (int*)malloc(V * B * sizeof(int));
    }
    initialize_population(population, V, B, R, K, population_size);

    // Fitness array
    int* fitness = (int*)malloc(population_size * sizeof(int));

    // Evolve population
    while (1) {
        evaluate_fitness(population, fitness, V, B, p1, p2, R, K, L, population_size);

        for (int i = 0; i < population_size; ++i) {
            if (is_valid_solution(population[i], V, B, p1, p2, R, K, L)) {
                print_solution(population[i], V, B);
                goto cleanup;
            }
        }

        select_and_crossover(population, fitness, V, B, population_size, crossover_prob);
        mutate(population, V, B, mutation_rate, population_size);
    }

cleanup:
    for (int i = 0; i < population_size; ++i) {
        free(population[i]);
    }
    free(population);
    free(fitness);

    return 0;
}

// Function to initialize the population with random incidence matrices
void initialize_population(int** population, int V, int B, int R, int K, int population_size) {
    for (int i = 0; i < population_size; ++i) {
        for (int v = 0; v < V; ++v) {
            int sum = 0;
            for (int b = 0; b < B; ++b) {
                int value = rand() % 3;
                population[i][v * B + b] = value;
                sum += value;
            }

            // Adjust the sum to ensure row sum equals R
            while (sum != R) {
                int b = rand() % B;
                int delta = (sum < R && population[i][v * B + b] < 2) ? 1 : -1;
                if (population[i][v * B + b] + delta >= 0 && population[i][v * B + b] + delta <= 2) {
                    population[i][v * B + b] += delta;
                    sum += delta;
                }
            }
        }

        for (int b = 0; b < B; ++b) {
            int sum = 0;
            for (int v = 0; v < V; ++v) {
                sum += population[i][v * B + b];
            }

            // Adjust the sum to ensure column sum equals K
            while (sum != K) {
                int v = rand() % V;
                int delta = (sum < K && population[i][v * B + b] < 2) ? 1 : -1;
                if (population[i][v * B + b] + delta >= 0 && population[i][v * B + b] + delta <= 2) {
                    population[i][v * B + b] += delta;
                    sum += delta;
                }
            }
        }
    }
}

// Function to evaluate the fitness of each individual in the population
void evaluate_fitness(int** population, int* fitness, int V, int B, int p1, int p2, int R, int K, int L, int population_size) {
    for (int i = 0; i < population_size; ++i) {
        int penalty = 0;

        for (int v = 0; v < V; ++v) {
            int row_sum = 0;
            int ones = 0;
            int twos = 0;

            for (int b = 0; b < B; ++b) {
                int value = population[i][v * B + b];
                row_sum += value;
                if (value == 1) ones++;
                if (value == 2) twos++;
            }

            // Penalize deviations from R and desired multiplicities
            penalty += abs(row_sum - R);
            penalty += abs(ones - p1);
            penalty += abs(twos - p2);
        }

        for (int b = 0; b < B; ++b) {
            int col_sum = 0;
            for (int v = 0; v < V; ++v) {
                col_sum += population[i][v * B + b];
            }
            // Penalize deviations from K
            penalty += abs(col_sum - K);
        }

        // Penalize deviations from pairwise appearance constraints
        for (int v = 0; v < V; ++v) {
            for (int w = v + 1; w < V; ++w) {
                int pairwise_appearance = 0;
                for (int b = 0; b < B; ++b) {
                    pairwise_appearance += population[i][v * B + b] * population[i][w * B + b];
                }
                penalty += abs(pairwise_appearance - L);
            }
        }

        fitness[i] = penalty;
    }
}

// Function to select parents and perform crossover to generate new population
void select_and_crossover(int** population, int* fitness, int V, int B, int population_size, float crossover_prob) {
    int** new_population = (int**)malloc(population_size * sizeof(int*));
    for (int i = 0; i < population_size; ++i) {
        new_population[i] = (int*)malloc(V * B * sizeof(int));
    }

    // Selection via tournament
    for (int i = 0; i < population_size; i += 2) {
        int idx1 = rand() % population_size;
        int idx2 = rand() % population_size;
        int idx3 = rand() % population_size;
        int idx4 = rand() % population_size;

        int parent1 = (fitness[idx1] < fitness[idx2]) ? idx1 : idx2;
        int parent2 = (fitness[idx3] < fitness[idx4]) ? idx3 : idx4;

        // Crossover
        if ((float)rand() / RAND_MAX < crossover_prob) {
            int crossover_point = rand() % (V * B);
            for (int j = 0; j < crossover_point; ++j) {
                new_population[i][j] = population[parent1][j];
                new_population[i + 1][j] = population[parent2][j];
            }
            for (int j = crossover_point; j < V * B; ++j) {
                new_population[i][j] = population[parent2][j];
                new_population[i + 1][j] = population[parent1][j];
            }
        } else {
            // No crossover, just copy parents
            for (int j = 0; j < V * B; ++j) {
                new_population[i][j] = population[parent1][j];
                new_population[i + 1][j] = population[parent2][j];
            }
        }
    }

    // Replace old population with new population
    for (int i = 0; i < population_size; ++i) {
        free(population[i]);
        population[i] = new_population[i];
    }
    free(new_population);
}

// Function to mutate individuals in the population
void mutate(int** population, int V, int B, float mutation_rate, int population_size) {
    for (int i = 0; i < population_size; ++i) {
        if ((float)rand() / RAND_MAX < mutation_rate) {
            int v = rand() % V;
            int b = rand() % B;
            population[i][v * B + b] = rand() % 3;
        }
    }
}

// Function to check if an individual is a valid solution
int is_valid_solution(int* individual, int V, int B, int p1, int p2, int R, int K, int L) {
    for (int v = 0; v < V; ++v) {
        int row_sum = 0;
        int ones = 0;
        int twos = 0;
        for (int b = 0; b < B; ++b) {
            int value = individual[v * B + b];
            row_sum += value;
            if (value == 1) ones++;
            if (value == 2) twos++;
        }
        if (row_sum != R || ones != p1 || twos != p2) return 0;
    }

    for (int b = 0; b < B; ++b) {
        int col_sum = 0;
        for (int v = 0; v < V; ++v) {
            col_sum += individual[v * B + b];
        }
        if (col_sum != K) return 0;
    }

    for (int v = 0; v < V; ++v) {
        for (int w = v + 1; w < V; ++w) {
            int pairwise_appearance = 0;
            for (int b = 0; b < B; ++b) {
                pairwise_appearance += individual[v * B + b] * individual[w * B + b];
            }
            if (pairwise_appearance != L) return 0;
        }
    }
    return 1;
}

// Function to print a valid solution
void print_solution(int* individual, int V, int B) {
    for (int v = 0; v < V; ++v) {
        for (int b = 0; b < B; ++b) {
            printf("%d ", individual[v * B + b]);
        }
        printf("\n");
    }
}