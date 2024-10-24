#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define CHUNK_SIZE 10  // Define the chunk size for work stealing

// #define GEM5

#ifdef GEM5
#include "gem5/m5ops.h"
#endif


// Structure to represent a sparse matrix in CSR format
typedef struct {
    int rows;
    int cols;
    int *rowPtr;
    int *colIdx;
    double *values;  // Change type to double
} SparseMatrix;

// Structure to pass arguments to threads
typedef struct {
    SparseMatrix *A;
    SparseMatrix *B;
    SparseMatrix *C;
    int thread_id;
    int num_threads;
} ThreadArgs;

// Global shared index to keep track of rows being processed
int currentRow = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to multiply two sparse matrices using CSR format
void *multiplySparseMatrices(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    SparseMatrix *A = threadArgs->A;
    SparseMatrix *B = threadArgs->B;
    SparseMatrix *C = threadArgs->C;

#ifdef GEM5
    if(threadArgs->thread_id == 0) {
        m5_dump_reset_stats(0,0);
    }
#endif

    while (1) {
        int startRow;

        // Critical section: get the next chunk of rows to process
        #ifdef GEM5
        m5_start_mutex(threadArgs->thread_id);
        #endif
        pthread_mutex_lock(&mutex);
        #ifdef GEM5
        m5_end_mutex(threadArgs->thread_id);
        #endif
        startRow = currentRow;
        currentRow += CHUNK_SIZE;
        pthread_mutex_unlock(&mutex);

        // Break the loop if all rows are processed
        if (startRow >= A->rows) {
            break;
        }

        // Process the assigned chunk of rows
        int endRow = startRow + CHUNK_SIZE;
        if (endRow > A->rows) {
            endRow = A->rows;
        }

        #ifdef GEM5
        m5_numiter(threadArgs->thread_id);
        #endif

        for (int row = startRow; row < endRow; row++) {
            for (int j = A->rowPtr[row]; j < A->rowPtr[row + 1]; j++) {
                int colA = A->colIdx[j];
                int valueA = A->values[j];

                for (int k = B->rowPtr[colA]; k < B->rowPtr[colA + 1]; k++) {
                    int colB = B->colIdx[k];
                    int valueB = B->values[k];

                    // Accumulate the result for C[row][colB]
                    C->values[row * C->cols + colB] += valueA * valueB;
                }
            }
        }
    }

    #ifdef GEM5
    if(threadArgs->thread_id == 0) {
        m5_dump_reset_stats(0,0);
    }
    #endif

    return NULL;
}

// Function to multiply two sparse matrices using CSR format (single-threaded)
void multiplySparseMatricesSingle(SparseMatrix *A, SparseMatrix *B, SparseMatrix *C) {
    for (int row = 0; row < A->rows; row++) {
        for (int j = A->rowPtr[row]; j < A->rowPtr[row + 1]; j++) {
            int colA = A->colIdx[j];
            double valueA = A->values[j];  // Use double for values

            for (int k = B->rowPtr[colA]; k < B->rowPtr[colA + 1]; k++) {
                int colB = B->colIdx[k];
                double valueB = B->values[k];

                // Accumulate the result for C[row][colB]
                C->values[row * C->cols + colB] += valueA * valueB;
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
    matrix.values = (double *)malloc(numNonZero * sizeof(double)); // Change type to double
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
            printf("%f ", matrix->values[i * matrix->cols + j]); // Print as double
        }
        printf("\n");
    }
}

// Function to compare two matrices
int compareMatrices(SparseMatrix *A, SparseMatrix *B) {
    if (A->rows != B->rows || A->cols != B->cols) {
        return 0;  // Different dimensions
    }

    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            if (A->values[i * A->cols + j] != B->values[i * B->cols + j]) {
                return 0;  // Different values
            }
        }
    }
    return 1;  // Matrices are equal
}

int main(int argc, char *argv[]) {
    // Validate the number of command-line arguments
    if (argc != 7) {
        printf("Usage: %s <ROWS_A> <COLS_A> <ROWS_B> <COLS_B> <val_non_zero> <num_threads>\n", argv[0]);
        return -1;
    }

    // Read matrix dimensions and sparsity parameter from command-line arguments
    int ROWS_A = atoi(argv[1]);
    int COLS_A = atoi(argv[2]);
    int ROWS_B = atoi(argv[3]);
    int COLS_B = atoi(argv[4]);
    int val_non_zero = atoi(argv[5]);
    int num_threads = atoi(argv[6]);  // Number of threads

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

    // Create row pointer array for matrix A
    for (int i = 0; i < ROWS_A; i++) {
        A.rowPtr[i] = i * (numNonZeroA / ROWS_A); // Simplistic distribution
    }
    A.rowPtr[ROWS_A] = numNonZeroA; // Last element points to the end of the non-zero values

    // Randomly initialize matrix B (sparse data)
    for (int i = 0; i < numNonZeroB; i++) {
        B.colIdx[i] = rand() % COLS_B; // Random column indices
        B.values[i] = rand() % 10 + 1; // Random values between 1 and 10
    }

    // Create row pointer array for matrix B
    for (int i = 0; i < ROWS_B; i++) {
        B.rowPtr[i] = i * (numNonZeroB / ROWS_B); // Simplistic distribution
    }
    B.rowPtr[ROWS_B] = numNonZeroB; // Last element points to the end of the non-zero values

    // Create the result matrix C (dense format for simplicity)
    SparseMatrix C = createSparseMatrix(ROWS_A, COLS_B, ROWS_A * COLS_B); // Result matrix size is ROWS_A x COLS_B

    // Measure execution time for thread creation and joining
    // clock_t thread_start = clock();

    // // Create and start threads for multi-threaded multiplication
    pthread_t threads[num_threads];
    ThreadArgs threadArgs[num_threads];

    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].A = &A;
        threadArgs[i].B = &B;
        threadArgs[i].C = &C;
        threadArgs[i].thread_id = i;
        threadArgs[i].num_threads = num_threads;

        if (pthread_create(&threads[i], NULL, multiplySparseMatrices, (void *)&threadArgs[i]) != 0) {
            perror("Failed to create thread");
            return -1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // clock_t thread_end = clock();
    // double thread_time_spent = (double)(thread_end - thread_start) / CLOCKS_PER_SEC;

    // Measure execution time for single-threaded multiplication
    // clock_t single_start = clock();
    // SparseMatrix C_single = createSparseMatrix(ROWS_A, COLS_B, ROWS_A * COLS_B); // Result matrix for single-threaded
    // multiplySparseMatricesSingle(&A, &B, &C_single);
    // clock_t single_end = clock();
    // double single_time_spent = (double)(single_end - single_start) / CLOCKS_PER_SEC;

    // // Compare results
    // if (compareMatrices(&C_single, &C)) {
    //     printf("Result is correct!\n");
    // } else {
    //     printf("Result is incorrect!\n");
    // }
    
    // // Print execution times
    // printf("Single-threaded execution time: %f seconds\n", single_time_spent);
    // printf("Multi-threaded execution time: %f seconds\n", thread_time_spent);

    // Clean up memory
    freeSparseMatrix(&A);
    freeSparseMatrix(&B);
    freeSparseMatrix(&C);
    // freeSparseMatrix(&C_single);
    return 0;
}
