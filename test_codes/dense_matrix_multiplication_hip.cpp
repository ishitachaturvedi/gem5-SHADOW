#include <stdio.h>
#include <stdlib.h>
#include <hip/hip_runtime.h>

#define NUM_THREADS 256 // Number of threads per block

// Structure to represent a dense matrix
typedef struct {
    int rows;
    int cols;
    float *values;
} DenseMatrix;

#define CHECK(cmd) \
{\
    hipError_t error  = cmd;\
    if (error != hipSuccess) { \
        fprintf(stderr, "error: '%s'(%d) at %s:%d\n", hipGetErrorString(error), error,__FILE__, __LINE__); \
        exit(EXIT_FAILURE);\
    }\
}

// Function to create a dense matrix
DenseMatrix createDenseMatrix(int rows, int cols) {
    DenseMatrix mat;
    mat.rows = rows;
    mat.cols = cols;
    mat.values = (float *)malloc(rows * cols * sizeof(float));
    return mat;
}

// Function to free the memory allocated for a dense matrix
void freeDenseMatrix(DenseMatrix *mat) {
    free(mat->values);
}

// Device kernel for dense matrix multiplication
__global__ void denseMatMulKernel(DenseMatrix A, DenseMatrix B, float *C, int B_cols) {
    int row = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < A.rows) {
        for (int col = 0; col < B.cols; col++) {
            float sum = 0;

            for (int k = 0; k < A.cols; k++) {
                float A_value = A.values[row * A.cols + k];
                float B_value = B.values[k * B.cols + col];

                float rowSum = 0;
                float randomValue = 111 % 100 + 1;  // Pseudo-random number

                // More complex bitwise and arithmetic operations
                A_value = ((A_value * 30) / (A_value * 20)) + randomValue;
                A_value = (A_value * 3 - 5 + rowSum) * 1000;
                rowSum += A_value;

                // Introduce more complex dependencies
                B_value = (B_value * rowSum + randomValue) / (A_value + 1);
                B_value = ((B_value * 0xFF) + 8) / ((B_value * 8) + 0xFF);

                // Complex arithmetic operations with conditional logic
                int tempResult = (rowSum * B_value + A_value) * 500;

                if (tempResult > 250) {
                    B_value = tempResult + A_value;
                } else {
                    B_value = tempResult + (A_value * 10);
                }

                sum += A_value * B_value;
            }

            C[row * B.cols + col] = sum;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <ROWS_A> <COLS_A> <ROWS_B> <COLS_B>\n", argv[0]);
        return -1;
    }

    int ROWS_A = atoi(argv[1]);
    int COLS_A = atoi(argv[2]);
    int ROWS_B = atoi(argv[3]);
    int COLS_B = atoi(argv[4]);

    if (COLS_A != ROWS_B) {
        printf("Invalid matrix dimensions! Number of columns in A must match the number of rows in B.\n");
        return -1;
    }

    hipDeviceProp_t props;
    CHECK(hipGetDeviceProperties(&props, 0/*deviceID*/));
    printf("info: running on device %s\n", props.name);

    DenseMatrix A = createDenseMatrix(ROWS_A, COLS_A);
    DenseMatrix B = createDenseMatrix(ROWS_B, COLS_B);

    // Initialize matrices A and B with random values
    for (int i = 0; i < ROWS_A * COLS_A; i++) {
        A.values[i] = (float)(rand() % 100 + 1);
    }

    for (int i = 0; i < ROWS_B * COLS_B; i++) {
        B.values[i] = (float)(rand() % 100 + 1);
    }

    float *C = (float *)calloc(ROWS_A * COLS_B, sizeof(float));

    DenseMatrix d_A, d_B;
    float *d_C;
    CHECK(hipMalloc(&d_A.values, ROWS_A * COLS_A * sizeof(float)));
    CHECK(hipMalloc(&d_B.values, ROWS_B * COLS_B * sizeof(float)));
    CHECK(hipMalloc(&d_C, ROWS_A * COLS_B * sizeof(float)));

    CHECK(hipMemcpy(d_A.values, A.values, ROWS_A * COLS_A * sizeof(float), hipMemcpyHostToDevice));
    CHECK(hipMemcpy(d_B.values, B.values, ROWS_B * COLS_B * sizeof(float), hipMemcpyHostToDevice));
    CHECK(hipMemset(d_C, 0, ROWS_A * COLS_B * sizeof(float)));

    d_A.rows = ROWS_A;
    d_A.cols = COLS_A;
    d_B.rows = ROWS_B;
    d_B.cols = COLS_B;

    int blocksPerGrid = (ROWS_A + NUM_THREADS - 1) / NUM_THREADS;
    hipLaunchKernelGGL(denseMatMulKernel, dim3(blocksPerGrid), dim3(NUM_THREADS), 0, 0, d_A, d_B, d_C, COLS_B);

    CHECK(hipMemcpy(C, d_C, ROWS_A * COLS_B * sizeof(float), hipMemcpyDeviceToHost));

    // Print part of the resulting matrix for verification
    printf("Resulting matrix C (partial view):\n");
    for (int i = 0; i < 5 && i < ROWS_A; i++) {
        for (int j = 0; j < 5 && j < COLS_B; j++) {
            printf("%f ", C[i * COLS_B + j]);
        }
        printf("\n");
    }

    CHECK(hipFree(d_A.values));
    CHECK(hipFree(d_B.values));
    CHECK(hipFree(d_C));
    freeDenseMatrix(&A);
    freeDenseMatrix(&B);
    free(C);

    return 0;
}
