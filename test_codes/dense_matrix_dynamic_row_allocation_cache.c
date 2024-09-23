#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define TILE_SIZE 50  // Size of the tile (10x10)
#define CHUNK_SIZE 10  // Size of the chunk for work stealing

// Global variables
int N;               // Size of the matrix
int num_threads;     // Number of threads
int **A, **B, **C, **C_single; // Matrices
int *rows_processed; // Array to count processed rows by each thread
pthread_mutex_t mutex; // Mutex for synchronizing access to rows
int next_row; // Index of the next row to process in the tile

// Structure for passing data to threads
typedef struct {
    int thread_id;
    int tile_row;
    int tile_col;
} thread_data_t;

// Function for each thread
void *thread_func(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    while (true) {
        // Lock the mutex to fetch the next available chunk of rows
        pthread_mutex_lock(&mutex);
        
        int start_row = next_row;
        next_row += CHUNK_SIZE; // Increment to the next chunk

        // Check if we exceed the tile boundaries
        if (start_row >= data->tile_row + TILE_SIZE) {
            pthread_mutex_unlock(&mutex);
            break;  // No more rows in the tile
        }
        int end_row = start_row + CHUNK_SIZE;
        if (end_row > data->tile_row + TILE_SIZE) {
            end_row = data->tile_row + TILE_SIZE; // Adjust to tile boundary
        }

        pthread_mutex_unlock(&mutex);

        // Process the assigned chunk of rows
        for (int i = start_row; i < end_row; i++) {
            for (int j = data->tile_col; j < data->tile_col + TILE_SIZE && j < N; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] += sum;
            }
            // Increment the row counter for this thread
            rows_processed[data->thread_id]++;
        }
    }
}

// Function for single-threaded matrix multiplication
void single_threaded_matrix_multiply() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C_single[i][j] = 0;  // Initialize to zero
            for (int k = 0; k < N; k++) {
                C_single[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Function to validate the multi-threaded result against the single-threaded result
int validate_result() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (C[i][j] != C_single[i][j]) {
                printf("Validation failed at C[%d][%d]: multi-threaded = %d, single-threaded = %d\n",
                       i, j, C[i][j], C_single[i][j]);
                return 0;  // Mismatch found
            }
        }
    }
    return 1;  // Matrices match
}

// Function to initialize matrices
void initialize_matrices() {
    A = (int **)malloc(N * sizeof(int *));
    B = (int **)malloc(N * sizeof(int *));
    C = (int **)malloc(N * sizeof(int *));
    C_single = (int **)malloc(N * sizeof(int *));
    
    for (int i = 0; i < N; i++) {
        A[i] = (int *)malloc(N * sizeof(int));
        B[i] = (int *)malloc(N * sizeof(int));
        C[i] = (int *)calloc(N, sizeof(int)); // Initialize C to zero
        C_single[i] = (int *)calloc(N, sizeof(int)); // Initialize C_single to zero
        for (int j = 0; j < N; j++) {
            A[i][j] = 1;
            B[i][j] = 1;
        }
    }
}

// Function to clean up memory
void cleanup() {
    for (int i = 0; i < N; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
        free(C_single[i]);
    }
    free(A);
    free(B);
    free(C);
    free(C_single);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <matrix_size> <num_threads>\n", argv[0]);
        return -1;
    }

    N = atoi(argv[1]);
    num_threads = atoi(argv[2]);

    if (N % TILE_SIZE != 0) {
        printf("Matrix size must be divisible by %d\n", TILE_SIZE);
        return -1;
    }

    // Initialize matrices
    initialize_matrices();

    // Allocate memory for row counters
    rows_processed = (int *)calloc(num_threads, sizeof(int));
    next_row = 0; // Initialize the next row index

    // Create a mutex
    pthread_mutex_init(&mutex, NULL);

    // Create threads
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    // Create tiles and start each thread on a tile
    for (int tile_row = 0; tile_row < N; tile_row += TILE_SIZE) {
        for (int tile_col = 0; tile_col < N; tile_col += TILE_SIZE) {
            next_row = tile_row;
            // Assign thread data for each thread
            for (int t = 0; t < num_threads; t++) {
                thread_data[t].thread_id = t;
                thread_data[t].tile_row = tile_row; // Starting row of the tile
                thread_data[t].tile_col = tile_col; // Starting column of the tile
                pthread_create(&threads[t], NULL, thread_func, (void *)&thread_data[t]);
            }

            // Join threads after processing the tile
            for (int t = 0; t < num_threads; t++) {
                pthread_join(threads[t], NULL);
            }
        }
    }

    // Perform single-threaded multiplication for validation
    single_threaded_matrix_multiply();

    // Validate the multi-threaded result against the single-threaded result
    if (validate_result()) {
        printf("Validation successful: multi-threaded result matches single-threaded result.\n");
    } else {
        printf("Validation failed: multi-threaded result does not match single-threaded result.\n");
    }

    // Print how many rows each thread processed
    for (int t = 0; t < num_threads; t++) {
        printf("Thread %d processed %d rows.\n", t, rows_processed[t]);
    }

    // Clean up resources
    cleanup();
    free(rows_processed);
    pthread_mutex_destroy(&mutex);

    printf("Matrix multiplication completed.\n");

    return 0;
}
