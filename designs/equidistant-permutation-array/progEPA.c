#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

// Function to initialize an m by n matrix with permutations of 0 to n-1
void initialize_permutations(int **array, int n, int m) {
    int i, j, k, t;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++)
            array[i][j] = j;
        for (j = 0; j < n-1; j++) {
            k = j + rand() / (RAND_MAX / (n - j) + 1);
            t = array[i][j];
            array[i][j] = array[i][k];
            array[i][k] = t;
        }
    }
}

// Function to compute Hamming distance between two permutations
int hamming_distance(int *perm1, int *perm2, int n) {
    int i, distance = 0;
    for (i = 0; i < n; i++) {
        if (perm1[i] != perm2[i])
            distance++;
    }
    return distance;
}

// Function to calculate the cost of the current state 
int calculate_cost(int **array, int n, int m, int d) {
    int cost = 0;
    for (int i = 0; i < m - 1; i++) {
        for (int j = i + 1; j < m; j++) {
            cost += abs(hamming_distance(array[i], array[j], n) - d);
        }
    }
    return cost;
}

// Function to make a small modification to the matrix (swap elements in a row)
void perturb(int **array, int n, int m, int *backup_row) {
    int row = rand() % m;
    int i = rand() % n;
    int j = rand() % n;
    while (i == j) {
        j = rand() % n;
    }
    
    // Backup the row before perturbation for potential rollback
    memcpy(backup_row, array[row], n * sizeof(int));

    int temp = array[row][i];
    array[row][i] = array[row][j];
    array[row][j] = temp;

    // Store the changed row index for future reference
    backup_row[n] = row; 
}

// Function to print the matrix
void print_matrix(int **array, int n, int m) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", array[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s n d m seed [hyperparameters...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    int d = atoi(argv[2]);
    int m = atoi(argv[3]);
    unsigned int seed = (unsigned int)atoi(argv[4]);

    // Set the random seed
    srand(seed);

    // Hyperparameters
    double alpha = argc >= 6 ? atof(argv[5]) : 0.99; // Cooling rate (default 0.99)

    // Allocate memory for the matrix
    int **array = malloc(m * sizeof(int *));
    for (int i = 0; i < m; i++) {
        array[i] = malloc(n * sizeof(int));
    }

    // Initialize permutations
    initialize_permutations(array, n, m);

    // Backup row storage (size n + 1: last element stores the row index)
    int *backup_row = malloc((n + 1) * sizeof(int));
    
    int current_cost = calculate_cost(array, n, m, d);

    while (current_cost != 0) {
        // Perturb the matrix and back up the changed row
        perturb(array, n, m, backup_row);
        int new_cost = calculate_cost(array, n, m, d);

        // Decide whether to accept the new state
        if (new_cost < current_cost || (exp((current_cost - new_cost) / alpha) > ((double) rand() / (double) RAND_MAX))) {
            current_cost = new_cost;
        } else {
            // Rollback the change if not accepted
            int row = backup_row[n];
            memcpy(array[row], backup_row, n * sizeof(int));
        }
    }

    // Print the final valid solution
    print_matrix(array, n, m);

    // Free allocated memory
    for (int i = 0; i < m; i++) {
        free(array[i]);
    }
    free(array);
    free(backup_row);

    return 0;
}