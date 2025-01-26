#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_ITER 1000000  // For safety - limits the number of iterations if stuck
#define NO_IMPROVEMENT_LIMIT 10000  // Limit of iterations without improvement

typedef struct {
    char* name;
    double min;
    double max;
    double default_value;
} Hyperparameter;

// Utility to print the matrix
void printMatrix(int** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%2d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Improved initialization with balanced random values
void initializeMatrix(int** W, int n, int w) {
    int pos_entries = w / 2;
    int neg_entries = w / 2;
    
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            if (i == j) {
                W[i][j] = 0;
                continue;
            }

            int choices[3] = {1, -1, 0};
            int pos = pos_entries;
            int neg = neg_entries;
            int zero = (n - 1) - pos - neg;
            
            int chosen_value;
            do {
                chosen_value = choices[rand() % 3];
            } while ((chosen_value == 1 && pos <= 0) || 
                     (chosen_value == -1 && neg <= 0) ||
                     (chosen_value == 0 && zero <= 0));
            
            W[i][j] = chosen_value;
            W[j][i] = chosen_value;
            pos_entries -= (chosen_value == 1);
            neg_entries -= (chosen_value == -1);
        }
    }
}

// Objective function to measure how close W is to satisfying WW^T = wI
double objectiveFunction(int** W, int n, int w) {
    double obj = 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            int sum = 0;
            for (int k = 0; k < n; k++) {
                sum += W[i][k] * W[j][k];
            }
            int delta = (i == j ? w : 0) - sum;
            obj += delta * delta;
        }
    }
    return obj;
}

// Copy matrix
void copyMatrix(int** dest, int** src, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            dest[i][j] = src[i][j];
        }
    }
}

// Simulated Annealing step
void simulatedAnnealing(int** W, int n, int w, double alpha, double initial_temp) {
    int iterations = 0;
    int no_improvement_iterations = 0;
    double temperature = initial_temp;
    int** newW = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        newW[i] = (int*)malloc(n * sizeof(int));
    }
    
    double current_obj = objectiveFunction(W, n, w);
    while (1) {
        for (int ite = 0; ite < MAX_ITER; ite++) {
            copyMatrix(newW, W, n);

            // Randomly select a symmetric position to mutate
            int i = rand() % n;
            int j = rand() % n;
            if (i > j) {
                int tmp = i;
                i = j;
                j = tmp;
            }

            // Ensure new value is different - mutation step and balance w entries
            int oldValue = W[i][j];
            int newValue;
            do {
                newValue = (rand() % 3) - 1;
            } while (newValue == oldValue);

            newW[i][j] = newValue;
            newW[j][i] = newValue;

            double new_obj = objectiveFunction(newW, n, w);

            // Metropolis criterion
            double delta_obj = new_obj - current_obj;
            if (delta_obj < 0 || (exp(-delta_obj / temperature) > (rand() / (double)RAND_MAX))) {
                copyMatrix(W, newW, n);
                current_obj = new_obj;
                no_improvement_iterations = 0;  // Reset the counter
            } else {
                no_improvement_iterations++;
            }

            // Successful termination check
            if (current_obj == 0) {
                for (int i = 0; i < n; i++) {
                    free(newW[i]);
                }
                free(newW);
                return;
            }

            // Re-randomize if stuck in local minima
            if (no_improvement_iterations > NO_IMPROVEMENT_LIMIT) {
                initializeMatrix(W, n, w);
                current_obj = objectiveFunction(W, n, w);
                no_improvement_iterations = 0;
                temperature = initial_temp;  // Reset temperature
            }
        }
        // Cooling step
        temperature *= alpha;
        iterations++;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Usage: %s n w seed alpha initial_temp\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int w = atoi(argv[2]);
    unsigned int seed = atoi(argv[3]);
    double alpha = atof(argv[4]);
    double initial_temp = atof(argv[5]);

    srand(seed);

    int** W = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        W[i] = (int*)malloc(n * sizeof(int));
    }

    initializeMatrix(W, n, w);

    simulatedAnnealing(W, n, w, alpha, initial_temp);

    printMatrix(W, n);

    for (int i = 0; i < n; i++) {
        free(W[i]);
    }
    free(W);

    return 0;
}