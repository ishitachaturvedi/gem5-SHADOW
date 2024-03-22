#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int **matrixA;
int **matrixB;
int **result;
int matrix_size;
int num_threads;

// Structure to pass arguments to thread function
typedef struct {
    int row_start;
    int row_end;
} ThreadArgs;

// Function to multiply a row of matrix A with matrix B
void *multiply(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = args->row_start; i < args->row_end; i++) {
        for (int j = 0; j < matrix_size; j++) {
            result[i][j] = 0;
            for (int k = 0; k < matrix_size; k++) {
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return 1;
    }

    matrix_size = atoi(argv[1]);
    num_threads = atoi(argv[2]);

    if (matrix_size <= 0 || num_threads <= 0) {
        printf("Invalid matrix size or number of threads.\n");
        return 1;
    }

    // Allocate memory for matrices
    matrixA = (int **)malloc(matrix_size * sizeof(int *));
    matrixB = (int **)malloc(matrix_size * sizeof(int *));
    result = (int **)malloc(matrix_size * sizeof(int *));
    for (int i = 0; i < matrix_size; i++) {
        matrixA[i] = (int *)malloc(matrix_size * sizeof(int));
        matrixB[i] = (int *)malloc(matrix_size * sizeof(int));
        result[i] = (int *)malloc(matrix_size * sizeof(int));
    }

    // Initialize matrices
    printf("Matrix A:\n");
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            matrixA[i][j] = rand() % 10;
            printf("%d ", matrixA[i][j]);
        }
        printf("\n");
    }

    printf("\nMatrix B:\n");
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            matrixB[i][j] = rand() % 10;
            printf("%d ", matrixB[i][j]);
        }
        printf("\n");
    }

    pthread_t threads[num_threads];
    ThreadArgs threadArgs[num_threads];
    int thread_step = matrix_size / num_threads;

    // Create threads for matrix multiplication
    for (int i = 0; i < num_threads; i++) {
        threadArgs[i].row_start = i * thread_step;
        threadArgs[i].row_end = (i + 1) * thread_step;
        if (i == num_threads - 1) {
            threadArgs[i].row_end = matrix_size; // Handle last thread
        }
        pthread_create(&threads[i], NULL, multiply, &threadArgs[i]);
    }

    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Print result matrix
    printf("\nResult Matrix:\n");
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            printf("%d ", result[i][j]);
        }
        printf("\n");
    }

    // Free allocated memory
    for (int i = 0; i < matrix_size; i++) {
        free(matrixA[i]);
        free(matrixB[i]);
        free(result[i]);
    }
    free(matrixA);
    free(matrixB);
    free(result);

    return 0;
}
