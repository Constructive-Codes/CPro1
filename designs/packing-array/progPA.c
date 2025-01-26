#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

void initializeArray(int** array, int N, int k, int v);
int calculateInitialCost(int** array, int N, int k, int v, int*** count);
int updateCostAndFrequency(int** array, int N, int k, int v, int row, int col, int newValue, int*** count);
void copyArray(int** source, int** destination, int N, int k);
void printArray(int** array, int N, int k);
double getAcceptanceProbability(int cost, int newCost, double temperature);
void simulatedAnnealing(int** array, int N, int k, int v, double initialTemp, double alpha);

int main(int argc, char* argv[]) {
    if (argc < 7) {
        printf("Usage: %s N k v seed initialTemp alpha\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int k = atoi(argv[2]);
    int v = atoi(argv[3]);
    int seed = atoi(argv[4]);
    double initialTemp = atof(argv[5]);
    double alpha = atof(argv[6]);

    srand(seed);

    // Allocate memory for the array
    int** array = malloc(N * sizeof(int*));
    for (int i = 0; i < N; i++) {
        array[i] = malloc(k * sizeof(int));
    }

    // Initialize the array
    initializeArray(array, N, k, v);

    // Perform simulated annealing to find the Packing Array
    simulatedAnnealing(array, N, k, v, initialTemp, alpha);

    // Print the final Packing Array
    printArray(array, N, k);

    // Free allocated memory
    for (int i = 0; i < N; i++) {
        free(array[i]);
    }
    free(array);

    return 0;
}

void initializeArray(int** array, int N, int k, int v) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            array[i][j] = rand() % v;
        }
    }
}

int calculateInitialCost(int** array, int N, int k, int v, int*** count) {
    int cost = 0;
    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < k; j++) {
            for (int m = 0; m < v; m++) {
                for (int n = 0; n < v; n++) {
                    count[i][j][m * v + n] = 0;
                }
            }
            for (int row = 0; row < N; row++) {
                int x = array[row][i];
                int y = array[row][j];
                count[i][j][x * v + y]++;
                if (count[i][j][x * v + y] > 1) {
                    cost++;
                }
            }
        }
    }
    return cost;
}

int updateCostAndFrequency(int** array, int N, int k, int v, int row, int col, int newValue, int*** count) {
    int oldValue = array[row][col];
    int cost = 0;

    for (int j = 0; j < k; j++) {
        if (j == col) continue;

        int a = (col < j) ? col : j;
        int b = (col > j) ? col : j;
        int xOld = (col < j) ? oldValue : array[row][j];
        int yOld = (col > j) ? oldValue : array[row][j];
        int xNew = (col < j) ? newValue : array[row][j];
        int yNew = (col > j) ? newValue : array[row][j];

        if (count[a][b][xOld * v + yOld] > 1) {
            cost--;
        }
        count[a][b][xOld * v + yOld]--;
        if (count[a][b][xNew * v + yNew] >= 1) {
            cost++;
        }
        count[a][b][xNew * v + yNew]++;
    }

    array[row][col] = newValue;
    return cost;
}

void copyArray(int** source, int** destination, int N, int k) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            destination[i][j] = source[i][j];
        }
    }
}

void printArray(int** array, int N, int k) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            printf("%d ", array[i][j]);
        }
        printf("\n");
    }
}

double getAcceptanceProbability(int cost, int newCost, double temperature) {
    if (newCost < cost) {
        return 1.0;
    }
    return exp((cost - newCost) / temperature);
}

void simulatedAnnealing(int** array, int N, int k, int v, double initialTemp, double alpha) {
    double temperature = initialTemp;

    // Allocate memory for the frequency counts
    int*** count = malloc(k * sizeof(int**));
    for (int i = 0; i < k; i++) {
        count[i] = malloc(k * sizeof(int*));
        for (int j = i+1; j < k; j++) {
            count[i][j] = malloc(v * v * sizeof(int));
        }
    }

    int cost = calculateInitialCost(array, N, k, v, count);
    while (cost > 0) {
        // Generate a neighboring solution
        int row = rand() % N;
        int col = rand() % k;
        int oldValue = array[row][col];
        int newValue = rand() % v;

        if (newValue == oldValue) continue;

        int newCost = cost + updateCostAndFrequency(array, N, k, v, row, col, newValue, count);
        double acceptanceProb = getAcceptanceProbability(cost, newCost, temperature);
        if (acceptanceProb > ((double) rand() / RAND_MAX)) {
            cost = newCost;
        } else {
            // revert change
            updateCostAndFrequency(array, N, k, v, row, col, oldValue, count);
        }
        temperature *= alpha;
    }

    for (int i = 0; i < k; i++) {
        for (int j = i+1; j < k; j++) {
            free(count[i][j]);
        }
        free(count[i]);
    }
    free(count);
}