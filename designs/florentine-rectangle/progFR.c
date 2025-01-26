#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_N 100
#define MAX_R 100

int grid[MAX_R][MAX_N];
int used[MAX_R][MAX_N];
int m_step_constraints[MAX_N][MAX_N][MAX_N];

uint32_t state[4];

// xoshiro128+ PRNG initialization
void init_xoshiro128(uint32_t seed) {
    state[0] = seed;
    state[1] = seed * 1812433253U + 1;
    state[2] = state[1] * 1812433253U + 1;
    state[3] = state[2] * 1812433253U + 1;
}

// xoshiro128+ PRNG implementation
uint32_t xoshiro128(void) {
    uint32_t result = state[0] + state[3];
    uint32_t t = state[1] << 9;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;
    state[3] = (state[3] << 11) | (state[3] >> (32 - 11));

    return result;
}

void initialize(int r, int n) {
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < n; ++j) {
            grid[i][j] = -1; // -1 indicates an empty cell
            used[i][j] = 0;
        }
    }
    for (int a = 0; a < n; ++a) {
        for (int b = 0; b < n; ++b) {
            for (int m = 1; m < n; ++m) {
                m_step_constraints[a][b][m] = 0;
            }
        }
    }
}

int is_valid(int row, int col, int num, int n) {
    // Check if num can be placed at grid[row][col]
    for (int m = 1; m < n; ++m) {
        if (col - m >= 0 && m_step_constraints[grid[row][col - m]][num][m] > 0) {
            return 0;
        }
    }
    return 1;
}

void update_constraints(int row, int col, int num, int n, int add) {
    for (int m = 1; m < n; ++m) {
        if (col - m >= 0) {
            m_step_constraints[grid[row][col - m]][num][m] += add;
        }
    }
}

void shuffle(int *array, int n) {
    for (int i = n - 1; i > 0; --i) {
        int j = xoshiro128() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

int solve(int r, int n, int row, int col) {
    if (row == r) {
        return 1; // All rows are filled
    }

    if (col == n) {
        return solve(r, n, row + 1, 0); // Move to the next row
    }

    int numbers[MAX_N];
    for (int i = 0; i < n; ++i) {
        numbers[i] = i;
    }
    shuffle(numbers, n);

    for (int i = 0; i < n; ++i) {
        int num = numbers[i];
        if (!used[row][num]) {
            if (is_valid(row, col, num, n)) {
                grid[row][col] = num;
                used[row][num] = 1;
                update_constraints(row, col, num, n, 1);

                if (solve(r, n, row, col + 1)) {
                    return 1;
                }

                update_constraints(row, col, num, n, -1);
                used[row][num] = 0;
                grid[row][col] = -1;
            }
        }
    }
    return 0;
}

void print_grid(int r, int n) {
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < n; ++j) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s r n seed\n", argv[0]);
        return 1;
    }

    int r = atoi(argv[1]);
    int n = atoi(argv[2]);
    int seed = atoi(argv[3]);

    if (r > MAX_R || n > MAX_N) {
        printf("Max r is %d and max n is %d\n", MAX_R, MAX_N);
        return 1;
    }

    init_xoshiro128(seed);
    initialize(r, n);

    if (solve(r, n, 0, 0)) {
        print_grid(r, n);
    } else {
        printf("No solution found.\n");
    }

    return 0;
}