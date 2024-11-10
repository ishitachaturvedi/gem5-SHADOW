#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#define TILE_SIZE 50  // Size of the tile (10x10)
#define CHUNK_SIZE 5  // Size of the chunk for work stealing

// #define TILE_SIZE 50  // Size of the tile (10x10)
// #define CHUNK_SIZE 5  // Size of the chunk for work stealing

#define GEM5

#define RUNAHEAD_TILES 100

// Global variables
int N;               // Size of the matrix
int num_threads;     // Number of threads
int **A, **B, **C, **C_single; // Matrices
int *rows_processed; // Array to count processed rows by each thread
pthread_mutex_t mutex; // Mutex for synchronizing access to rows
pthread_mutex_t mutex_cond;
int next_row[RUNAHEAD_TILES] = {0,0,0,0,0,0,0,0,0,0}; // Index of the next row to process in the tile
int base_row[RUNAHEAD_TILES] = {0,0,0,0,0,0,0,0,0,0}; // starting row of this tile
int base_col[RUNAHEAD_TILES] = {0,0,0,0,0,0,0,0,0,0}; // starting col of this tile
int base_k[RUNAHEAD_TILES] = {0,0,0,0,0,0,0,0,0,0}; // starting k of this tile

int next_row_idx = -1;

pthread_cond_t cond_var;

bool start_setup_next_tile = true;

pthread_barrier_t barrier;
pthread_barrier_t barrier_done;

// Structure for passing data to threads
typedef struct {
    int thread_id;
    int tile_row;
    int tile_col;
    int tile_k;
    bool tile_done;
    int next_row_idx;
} thread_data_t;

int tile_row = 0;
int tile_col = 0; 
int tile_k = 0; 
int reset = 0; // make sure that the 0th column iteration works.
int all_rows_done = 0;

void *thread_func(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    data->tile_done = true;
    data->next_row_idx = -1; 

    // if(data->thread_id == 0) {
    //     #ifdef GEM5
    //     m5_dump_reset_stats(0,0);
    //     #endif 
    // }   

    while(1) {
        // go to the next (row,tile) combination once you have covered all columns for this tile.
        // use tid 0 for this
        if(data->thread_id == 0) {
            if(reset!=0) {
                tile_k = tile_k + TILE_SIZE;
            }
            if(tile_k == N) {
                tile_col = tile_col + TILE_SIZE;
                tile_k = 0;
            }
            reset = 1;
            if(tile_col == N) {
                tile_row = tile_row + TILE_SIZE;
                // if we have exceeded the number of rows, we are done
                if(tile_row >= N) {
                    all_rows_done = 1;
                }
                tile_col = 0;
            }
            next_row_idx = (next_row_idx + 1) % RUNAHEAD_TILES;
            next_row[next_row_idx] = tile_row;
            base_row[next_row_idx] = tile_row;
            base_col[next_row_idx] = tile_col;
            base_k[next_row_idx] = tile_k;
            start_setup_next_tile = false;
            pthread_cond_broadcast(&cond_var);
        }

        #ifdef GEM5
        m5_start_barrier(data->thread_id);
        #endif
        pthread_mutex_lock(&mutex_cond);
        while ((data->next_row_idx == next_row_idx) && !all_rows_done) {
            pthread_cond_wait(&cond_var, &mutex_cond);
        }
        pthread_mutex_unlock(&mutex_cond);
        #ifdef GEM5
        m5_end_barrier(data->thread_id);
        #endif

        // all threads exit
        if(all_rows_done) {
            break;
        }

        data->tile_done = false;
        data->next_row_idx = next_row_idx;

        // start executing the  matrix multiplication for this tile
        while (true) {
            // Lock the mutex to fetch the next available chunk of rows
            #ifdef GEM5
            m5_start_mutex(data->thread_id);
            #endif
            pthread_mutex_lock(&mutex);
            #ifdef GEM5
            m5_end_mutex(data->thread_id);
            #endif
            int start_row = next_row[data->next_row_idx];
            int col_run = base_col[data->next_row_idx];
            int tile_run = base_k[data->next_row_idx];
            
            next_row[data->next_row_idx] += CHUNK_SIZE; // Increment to the next chunk

            // Check if we exceed the tile boundaries
            if (start_row >= base_row[data->next_row_idx] + TILE_SIZE) {

                // mark thread as done
                data->tile_done = true;

                if(data->thread_id == 0) {
                    start_setup_next_tile = true;
                }

                pthread_mutex_unlock(&mutex);
                break;  // No more rows in the tile
            }
            int end_row = start_row + CHUNK_SIZE;
            if (end_row > base_row[data->next_row_idx] + TILE_SIZE) {
                end_row = base_row[data->next_row_idx] + TILE_SIZE; // Adjust to tile boundary
            }

            pthread_mutex_unlock(&mutex);

            #ifdef GEM5
            m5_numiter(data->thread_id);
            #endif

            // Process the assigned chunk of rows
            for (int i = start_row; i < end_row; i++) {
                for (int j = col_run; j < col_run + TILE_SIZE && j < N; j++) {
                    int sum = 0;
                    for (int k = tile_run; k < tile_run + TILE_SIZE; k++) {
                        sum += A[i][k] * B[k][j];
                    }
                    C[i][j] += sum;
                }
                // rows_processed[data->thread_id]++;
            }
        } 
    } 

    // if(data->thread_id == 0) {
    //     #ifdef GEM5
    //     m5_dump_reset_stats(0,0);
    //     #endif 
    // }   

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
    for(int i = 0; i < 1; i++) {

        // Initialize matrices
        initialize_matrices();

        // Allocate memory for row counters
        rows_processed = (int *)calloc(num_threads, sizeof(int));

        // Create a mutex
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&mutex_cond, NULL);

        pthread_barrier_init(&barrier, NULL, num_threads);
        pthread_barrier_init(&barrier_done, NULL, num_threads);

        // Create threads
        pthread_t threads[num_threads];
        thread_data_t thread_data[num_threads];

        #ifdef GEM5
        m5_dump_reset_stats(0,0);
        #endif 


        for (int t = 0; t < num_threads; t++) {
            thread_data[t].thread_id = t;
            thread_data[t].tile_row = 0; // Starting row of the tile
            thread_data[t].tile_col = 0; // Starting column of the tile
            rows_processed[t] = 0;
            pthread_create(&threads[t], NULL, thread_func, (void *)&thread_data[t]);
        }

        // Join threads after processing the tile
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        #ifdef GEM5
        m5_dump_reset_stats(0,0);
        #endif 

        // Perform single-threaded multiplication for validation
        // single_threaded_matrix_multiply();

        // // Validate the multi-threaded result against the single-threaded result
        // if (validate_result()) {
        //     printf("Validation successful: multi-threaded result matches single-threaded result.\n");
        //     fprintf(stderr, "Validation successful: multi-threaded result matches single-threaded result.\n");
        // } else {
        //     printf("Validation failed: multi-threaded result does not match single-threaded result.\n");
        //     fprintf(stderr, "Validation failed: multi-threaded result does not match single-threaded result.\n");
        // }

        // // // Print how many rows each thread processed
        // for (int t = 0; t < num_threads; t++) {
        //     printf("Thread %d processed %d rows.\n", t, rows_processed[t]);
        // }

        // Clean up resources
        cleanup();
        free(rows_processed);
        pthread_mutex_destroy(&mutex);
        pthread_mutex_destroy(&mutex_cond);
    }

    printf("Matrix multiplication completed.\n");

    return 0;
}
