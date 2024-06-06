#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

// Function to allocate memory for a matrix
int **allocateMatrix(int rows, int cols) {
    int **matrix = (int **)malloc(rows * sizeof(int *));
    if (matrix == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rows; ++i) {
        matrix[i] = (int *)malloc(cols * sizeof(int));
        if (matrix[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    return matrix;
}

// Function to free memory allocated for a matrix
void freeMatrix(int **matrix, int rows) {
    for (int i = 0; i < rows; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

// Function to generate a random matrix
void generateRandomMatrix(int **matrix, int rows, int cols, int seed) {
    srand(seed);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = rand() % 100;  // Generate random integer between 0 and 99
        }
    }
}

// Function to perform matrix multiplication
void matmul(int **A, int **B, int **C, int rowsA, int colsA, int colsB) {
    // Transpose matrix B for better cache utilization
    int **BT = allocateMatrix(colsB, rowsA);
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            BT[j][i] = B[i][j];
        }
    }

    // Define blocking parameters
    const int BLOCK_SIZE = 64;

    // Perform matrix multiplication with loop unrolling and blocking
    for (int i = 0; i < rowsA; i += BLOCK_SIZE) {
        for (int j = 0; j < colsB; j += BLOCK_SIZE) {
            for (int k = 0; k < colsA; ++k) {
                for (int ii = i; ii < i + BLOCK_SIZE && ii < rowsA; ++ii) {
                    for (int jj = j; jj < j + BLOCK_SIZE && jj < colsB; ++jj) {
                        for (int kk = 0; kk < rowsA; ++kk) {
                            C[ii][jj] += A[ii][kk] * BT[jj][kk];
                        }
                    }
                }
            }
        }
    }

    // Free memory allocated for transposed matrix B
    freeMatrix(BT, colsB);
}

int main(int argc, char *argv[]) {
    // Check if the number of command-line arguments is correct
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <rowsA> <colsA> <colsB> <seed>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse command-line arguments
    int rowsA = atoi(argv[1]);
    int colsA = atoi(argv[2]);
    int colsB = atoi(argv[3]);
    int seed = atoi(argv[4]);

    // Allocate memory for matrices A, B, and C
    int **A = allocateMatrix(rowsA, colsA);
    int **B = allocateMatrix(colsA, colsB);
    int **C = allocateMatrix(rowsA, colsB);

    // Generate random matrices A and B
    generateRandomMatrix(A, rowsA, colsA, seed);
    generateRandomMatrix(B, colsA, colsB, seed);

    // Perform matrix multiplication
    matmul(A, B, C, rowsA, colsA, colsB);

    // Display the result matrix
    printf("Result matrix (C):\n");
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            printf("%d ", C[i][j]);
        }
        printf("\n");
    }

    // Free memory allocated for matrices
    freeMatrix(A, rowsA);
    freeMatrix(B, colsA);
    freeMatrix(C, rowsA);

    return 0;
}
