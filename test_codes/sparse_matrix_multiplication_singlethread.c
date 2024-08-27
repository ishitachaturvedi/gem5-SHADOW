#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Structure to represent a sparse matrix in CSR format
typedef struct {
    int rows;
    int cols;
    int *rowPtr;
    int *colIdx;
    int *values;
} SparseMatrix;

// Function to multiply two sparse matrices using CSR format in a single thread
void multiplySparseMatrices(SparseMatrix *A, SparseMatrix *B, SparseMatrix *C) {
    for (int i = 0; i < A->rows; i++) {
        for (int j = A->rowPtr[i]; j < A->rowPtr[i + 1]; j++) {
            int colA = A->colIdx[j];
            int valueA = A->values[j];

            for (int k = B->rowPtr[colA]; k < B->rowPtr[colA + 1]; k++) {
                int colB = B->colIdx[k];
                int valueB = B->values[k];

                // Accumulate the result for C[i][colB]
                C->values[i * C->cols + colB] += valueA * valueB;
            }
        }
    }
}

// Function to create a sparse matrix in CSR format
SparseMatrix createSparseMatrix(int rows, int cols, int numNonZero) {
    SparseMatrix matrix;
    matrix.rows = rows;
    matrix.cols = cols;
    matrix.rowPtr = (int *)calloc(rows + 1, sizeof(int)); // Extra space for rowPtr
    matrix.colIdx = (int *)malloc(numNonZero * sizeof(int));
    matrix.values = (int *)malloc(numNonZero * sizeof(int));
    return matrix;
}

// Function to free a sparse matrix
void freeSparseMatrix(SparseMatrix *matrix) {
    free(matrix->rowPtr);
    free(matrix->colIdx);
    free(matrix->values);
}

// Function to print a sparse matrix (dense representation)
void printDenseMatrix(SparseMatrix *matrix) {
    for (int i = 0; i < matrix->rows; i++) {
        for (int j = 0; j < matrix->cols; j++) {
            printf("%d ", matrix->values[i * matrix->cols + j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    // Validate the number of command-line arguments
    if (argc != 6) {
        printf("Usage: %s <ROWS_A> <COLS_A> <ROWS_B> <COLS_B> <val_non_zero>\n", argv[0]);
        return -1;
    }

    // Read matrix dimensions and sparsity parameter from command-line arguments
    int ROWS_A = atoi(argv[1]);
    int COLS_A = atoi(argv[2]);
    int ROWS_B = atoi(argv[3]);
    int COLS_B = atoi(argv[4]);
    int val_non_zero = atoi(argv[5]);

    // Validate matrix dimensions
    if (COLS_A != ROWS_B) {
        printf("Invalid matrix dimensions! Number of columns in A must match the number of rows in B.\n");
        return -1;
    }

    // Estimate the number of non-zero elements (can be adjusted based on sparsity)
    int numNonZeroA = ROWS_A * COLS_A / val_non_zero; // Roughly non-zero elements in A
    int numNonZeroB = ROWS_B * COLS_B / val_non_zero; // Roughly non-zero elements in B

    // Create sparse matrices A and B
    SparseMatrix A = createSparseMatrix(ROWS_A, COLS_A, numNonZeroA);
    SparseMatrix B = createSparseMatrix(ROWS_B, COLS_B, numNonZeroB);

    // Randomly initialize matrix A (sparse data)
    for (int i = 0; i < numNonZeroA; i++) {
        A.colIdx[i] = rand() % COLS_A; // Random column indices
        A.values[i] = rand() % 10 + 1; // Random values between 1 and 10
    }

    for (int i = 0; i <= ROWS_A; i++) {
        A.rowPtr[i] = (i * 2) % numNonZeroA; // Example row pointer distribution
    }

    // Randomly initialize matrix B (sparse data)
    for (int i = 0; i < numNonZeroB; i++) {
        B.colIdx[i] = rand() % COLS_B; // Random column indices
        B.values[i] = rand() % 10 + 1; // Random values between 1 and 10
    }

    for (int i = 0; i <= ROWS_B; i++) {
        B.rowPtr[i] = (i * 5) % numNonZeroB; // Example row pointer distribution
    }

    // Create the result matrix C (dense format for simplicity)
    SparseMatrix C = createSparseMatrix(ROWS_A, COLS_B, ROWS_A * COLS_B); // Result matrix size is ROWS_A x COLS_B

    // Perform matrix multiplication in a single thread
    multiplySparseMatrices(&A, &B, &C);

    // Print the resulting matrix (dense format for simplicity)
    // printf("Resulting Dense Matrix C (%d x %d):\n", ROWS_A, COLS_B);
    // printDenseMatrix(&C);

    // Free allocated memory
    freeSparseMatrix(&A);
    freeSparseMatrix(&B);
    freeSparseMatrix(&C);

    return 0;
}
