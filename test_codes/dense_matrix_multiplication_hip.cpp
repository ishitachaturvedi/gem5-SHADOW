#include <hip/hip_runtime.h>
#include <iostream>
#include <vector>
#include <cmath>

#define TILE_SIZE 16

// HIP kernel for tiled matrix multiplication
__global__ void tiledMatrixMultiply(const float* A, const float* B, float* C, int width) {
    __shared__ float tileA[TILE_SIZE][TILE_SIZE];
    __shared__ float tileB[TILE_SIZE][TILE_SIZE];

    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;

    float temp = 0.0f;

    for (int i = 0; i < width / TILE_SIZE; ++i) {
        tileA[threadIdx.y][threadIdx.x] = A[row * width + (i * TILE_SIZE + threadIdx.x)];
        tileB[threadIdx.y][threadIdx.x] = B[(i * TILE_SIZE + threadIdx.y) * width + col];

        __syncthreads();

        for (int j = 0; j < TILE_SIZE; ++j) {
            temp += tileA[threadIdx.y][j] * tileB[j][threadIdx.x];
        }

        __syncthreads();
    }

    C[row * width + col] = temp;
}

// Single-threaded CPU matrix multiplication for verification
void matrixMultiplyCPU(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int width) {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < width; ++k) {
                sum += A[i * width + k] * B[k * width + j];
            }
            C[i * width + j] = sum;
        }
    }
}

// Function to verify GPU results against CPU results
bool verifyResults(const std::vector<float>& C1, const std::vector<float>& C2, int width) {
    const float epsilon = 1e-5;
    for (int i = 0; i < width * width; ++i) {
        if (std::fabs(C1[i] - C2[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

int main() {
    const int width = 1024;
    size_t size = width * width * sizeof(float);

    // Unified memory allocation using hipMallocManaged
    float* A;
    float* B;
    float* C;

    hipError_t err;

    err = hipMallocManaged(&A, size);
    if (err != hipSuccess) {
        std::cerr << "Failed to allocate unified memory for A: " << hipGetErrorString(err) << std::endl;
        return -1;
    }

    err = hipMallocManaged(&B, size);
    if (err != hipSuccess) {
        std::cerr << "Failed to allocate unified memory for B: " << hipGetErrorString(err) << std::endl;
        hipFree(A); // Free previously allocated memory
        return -1;
    }

    err = hipMallocManaged(&C, size);
    if (err != hipSuccess) {
        std::cerr << "Failed to allocate unified memory for C: " << hipGetErrorString(err) << std::endl;
        hipFree(A); // Free previously allocated memory
        hipFree(B);
        return -1;
    }

    // Initialize matrices A and B
    for (int i = 0; i < width * width; ++i) {
        A[i] = 1;
        B[i] = 1;
    }

    dim3 dimBlock(TILE_SIZE, TILE_SIZE);
    dim3 dimGrid(width / TILE_SIZE, width / TILE_SIZE);

    // Launch kernel
    hipLaunchKernelGGL(tiledMatrixMultiply, dimGrid, dimBlock, 0, 0, A, B, C, width);

    // Synchronize and check for errors
    err = hipDeviceSynchronize();
    if (err != hipSuccess) {
        std::cerr << "Kernel execution failed: " << hipGetErrorString(err) << std::endl;
        return -1;
    }

    // Perform matrix multiplication on CPU for verification
    std::vector<float> C_cpu(width * width);
    matrixMultiplyCPU(std::vector<float>(A, A + width * width), std::vector<float>(B, B + width * width), C_cpu, width);

    // Verify GPU results against CPU results
    if (verifyResults(std::vector<float>(C, C + width * width), C_cpu, width)) {
        std::cout << "Matrix multiplication is correct." << std::endl;
    } else {
        std::cout << "Matrix multiplication is incorrect." << std::endl;
    }

   // Free unified memory
   err = hipFree(A);
   if (err != hipSuccess) {
       std::cerr << "Failed to free unified memory for A: " << hipGetErrorString(err) << std::endl;
       return -1;
   }

   err = hipFree(B);
   if (err != hipSuccess) {
       std::cerr << "Failed to free unified memory for B: " << hipGetErrorString(err) << std::endl;
       return -1;
   }

   err = hipFree(C);
   if (err != hipSuccess) {
       std::cerr << "Failed to free unified memory for C: " << hipGetErrorString(err) << std::endl;
       return -1;
   }

   return 0;
}