#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct {
    int n;
    int w;
    unsigned int seed;
    double initial_temp;
    double cooling_rate;
} Params;

void initialize_matrix(int **W, int n, int seed);
void print_matrix(int **W, int n);
void copy_matrix(int **source, int **dest, int n);
void tweak_matrix(int **W, int n);
double objective_function(int **W, int n, int w);
int is_valid_skew_weighing_matrix(int **W, int n, int w);

int main(int argc, char *argv[]) {
    if (argc < 6) {
        printf("Usage: %s <n> <w> <seed> <initial_temp> <cooling_rate>\n", argv[0]);
        return 1;
    }

    Params params;
    params.n = atoi(argv[1]);
    params.w = atoi(argv[2]);
    params.seed = atoi(argv[3]);
    params.initial_temp = atof(argv[4]);
    params.cooling_rate = atof(argv[5]);

    srand(params.seed);

    int **W = (int **)malloc(params.n * sizeof(int *));
    int **best_W = (int **)malloc(params.n * sizeof(int *));
    for (int i = 0; i < params.n; i++) {
        W[i] = (int *)malloc(params.n * sizeof(int));
        best_W[i] = (int *)malloc(params.n * sizeof(int));
    }

    double temperature = params.initial_temp;
    initialize_matrix(W, params.n, params.seed);
    copy_matrix(W, best_W, params.n);

    double best_cost = objective_function(W, params.n, params.w);
    double current_cost = best_cost;

    while (1) {
        int **new_W = (int **)malloc(params.n * sizeof(int *));
        for (int i = 0; i < params.n; i++) {
            new_W[i] = (int *)malloc(params.n * sizeof(int));
        }

        copy_matrix(W, new_W, params.n);
        tweak_matrix(new_W, params.n);

        double new_cost = objective_function(new_W, params.n, params.w);
        if (new_cost < best_cost) {
            copy_matrix(new_W, best_W, params.n);
            best_cost = new_cost;
        }

        if (new_cost < current_cost || exp((current_cost - new_cost) / temperature) > ((double)rand() / RAND_MAX)) {
            copy_matrix(new_W, W, params.n);
            current_cost = new_cost;
        }

        temperature *= params.cooling_rate;

        for (int i = 0; i < params.n; i++) {
            free(new_W[i]);
        }
        free(new_W);

        if (is_valid_skew_weighing_matrix(best_W, params.n, params.w)) {
            break;
        }
    }

    print_matrix(best_W, params.n);

    for (int i = 0; i < params.n; i++) {
        free(W[i]);
        free(best_W[i]);
    }
    free(W);
    free(best_W);

    return 0;
}

void initialize_matrix(int **W, int n, int seed) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            int value = rand() % 3 - 1; // Randomly choose from {-1, 0, 1}
            W[i][j] = value;
            W[j][i] = -value;
        }
        W[i][i] = 0;
    }
}

void print_matrix(int **W, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%2d ", W[i][j]);
        }
        printf("\n");
    }
}

void copy_matrix(int **source, int **dest, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            dest[i][j] = source[i][j];
        }
    }
}

#define TWEAK_OPTIONS {-1, 0, 1}

// Optimized tweak_matrix function
void tweak_matrix(int **W, int n) {
    static const int options[] = TWEAK_OPTIONS;
    int i = rand() % n;
    int j = rand() % n;

    if (i != j) {
        int old_value = W[i][j];
        int new_value = options[(rand() % 3)]; // Pick a random value from {-1, 0, 1}
        while (new_value == old_value) {
            new_value = options[(rand() % 3)];
        }
        W[i][j] = new_value;
        W[j][i] = -new_value;
    }
}

double objective_function(int **W, int n, int w) {
    double cost = 0.0;
    for (int i = 0; i < n; i++) {
        int row_sum = 0;
        for (int j = 0; j < n; j++) {
            row_sum += (W[i][j] != 0);
        }
        cost += fabs(w - row_sum);
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int dot_product = 0;
            for (int k = 0; k < n; k++) {
                dot_product += W[i][k] * W[j][k];
            }
            if (i == j) {
                cost += fabs(dot_product - w);
            } else if (dot_product != 0) {
                cost += fabs(dot_product);
            }
        }
    }

    return cost;
}

int is_valid_skew_weighing_matrix(int **W, int n, int w) {
    for (int i = 0; i < n; i++) {
        int row_sum = 0;
        for (int j = 0; j < n; j++) {
            row_sum += (W[i][j] != 0);
        }
        if (row_sum != w) {
            return 0;
        }
    }

    int **product = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        product[i] = (int *)malloc(n * sizeof(int));
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            product[i][j] = 0;
            for (int k = 0; k < n; k++) {
                product[i][j] += W[i][k] * W[j][k];
            }
            if ((i == j && product[i][j] != w) || (i != j && product[i][j] != 0)) {
                for (int m = 0; m < n; m++) {
                    free(product[m]);
                }
                free(product);
                return 0;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        free(product[i]);
    }
    free(product);

    return 1;
}