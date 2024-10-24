#include <hip/hip_runtime.h>
#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>  // For atoi and atof
#include <time.h>

__global__ void sparse_matmul_hip_kernel(int* row_ptr_A, int* col_ind_A, double* val_A, 
                                         int* row_ptr_B, int* col_ind_B, double* val_B, 
                                         double* result, int A_rows, int B_cols) {
    int row = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < A_rows) {
        for (int k = row_ptr_A[row]; k < row_ptr_A[row + 1]; k++) {
            int a_col = col_ind_A[k];
            double a_val = val_A[k];

            for (int j = row_ptr_B[a_col]; j < row_ptr_B[a_col + 1]; j++) {
                int b_col = col_ind_B[j];
                double b_val = val_B[j];

                atomicAdd(&result[row * B_cols + b_col], a_val * b_val);
            }
        }
    }
}

void sparse_matmul_hip(int* row_ptr_A, int* col_ind_A, double* val_A, int A_rows, 
                       int col_ind_A_size, int val_A_size, 
                       int* row_ptr_B, int* col_ind_B, double* val_B, int B_cols, 
                       int col_ind_B_size, int val_B_size, 
                       double* result) {
    // Launch kernel
    int threads_per_block = 256;
    int blocks = (A_rows + threads_per_block - 1) / threads_per_block;
    sparse_matmul_hip_kernel<<<blocks, threads_per_block>>>(row_ptr_A, col_ind_A, val_A, 
                                                            row_ptr_B, col_ind_B, val_B, 
                                                            result, A_rows, B_cols);
    hipDeviceSynchronize();
}

void sparse_matmul_single_thread(const std::vector<int>& row_ptr_A, const std::vector<int>& col_ind_A, const std::vector<double>& val_A,
                                  const std::vector<int>& row_ptr_B, const std::vector<int>& col_ind_B, const std::vector<double>& val_B,
                                  std::vector<double>& result, int A_rows, int B_cols) {
    std::fill(result.begin(), result.end(), 0.0); // Initialize result matrix to zero

    for (int row = 0; row < A_rows; ++row) {
        for (int k = row_ptr_A[row]; k < row_ptr_A[row + 1]; ++k) {
            int a_col = col_ind_A[k];
            double a_val = val_A[k];

            for (int j = row_ptr_B[a_col]; j < row_ptr_B[a_col + 1]; ++j) {
                int b_col = col_ind_B[j];
                double b_val = val_B[j];

                result[row * B_cols + b_col] += a_val * b_val; // Standard multiplication
            }
        }
    }
}

void generate_sparse_matrix(int rows, int cols, double sparsity, 
                            std::vector<int>& row_ptr, std::vector<int>& col_ind, std::vector<double>& val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_val(0.0, 10.0);
    std::uniform_real_distribution<> dis_spar(0.0, 1.0);

    row_ptr.resize(rows + 1);
    int nnz = 0; 

    for (int i = 0; i < rows; i++) {
        row_ptr[i] = nnz;
        for (int j = 0; j < cols; j++) {
            if (dis_spar(gen) >= sparsity) {
                col_ind.push_back(j);
                val.push_back(1);
                nnz++;
            }
        }
    }
    row_ptr[rows] = nnz;
}

void print_result_matrix(const std::vector<double>& result, int A_rows, int B_cols) {
    std::cout << "Result Matrix:\n";
    for (int i = 0; i < A_rows; i++) {
        for (int j = 0; j < B_cols; j++) {
            std::cout << result[i * B_cols + j] << " ";
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <A_rows> <B_cols> <sparsity_A> <sparsity_B>\n";
        return 1;
    }

    int A_rows = atoi(argv[1]);
    int B_cols = atoi(argv[2]);
    double sparsity_A = atof(argv[3]);
    double sparsity_B = atof(argv[4]);

    std::vector<int> row_ptr_A, col_ind_A, row_ptr_B, col_ind_B;
    std::vector<double> val_A, val_B;

    // Generate random sparse matrices A and B
    generate_sparse_matrix(A_rows, B_cols, sparsity_A, row_ptr_A, col_ind_A, val_A);
    generate_sparse_matrix(B_cols, A_rows, sparsity_B, row_ptr_B, col_ind_B, val_B);

    // Allocate device memory for A and B
    int *d_row_ptr_A, *d_col_ind_A, *d_row_ptr_B, *d_col_ind_B;
    double *d_val_A, *d_val_B, *d_result;

    hipMalloc(&d_row_ptr_A, (A_rows + 1) * sizeof(int));
    hipMalloc(&d_col_ind_A, col_ind_A.size() * sizeof(int));
    hipMalloc(&d_val_A, val_A.size() * sizeof(double));

    hipMalloc(&d_row_ptr_B, (B_cols + 1) * sizeof(int));
    hipMalloc(&d_col_ind_B, col_ind_B.size() * sizeof(int));
    hipMalloc(&d_val_B, val_B.size() * sizeof(double));

    // Allocate device memory for the result
    hipMalloc(&d_result, A_rows * B_cols * sizeof(double));

    // Copy data from host to device
    hipMemcpy(d_row_ptr_A, row_ptr_A.data(), (A_rows + 1) * sizeof(int), hipMemcpyHostToDevice);
    hipMemcpy(d_col_ind_A, col_ind_A.data(), col_ind_A.size() * sizeof(int), hipMemcpyHostToDevice);
    hipMemcpy(d_val_A, val_A.data(), val_A.size() * sizeof(double), hipMemcpyHostToDevice);
    
    hipMemcpy(d_row_ptr_B, row_ptr_B.data(), (B_cols + 1) * sizeof(int), hipMemcpyHostToDevice);
    hipMemcpy(d_col_ind_B, col_ind_B.data(), col_ind_B.size() * sizeof(int), hipMemcpyHostToDevice);
    hipMemcpy(d_val_B, val_B.data(), val_B.size() * sizeof(double), hipMemcpyHostToDevice);

    // Initialize result to zero on device
    hipMemset(d_result, 0, A_rows * B_cols * sizeof(double));

    clock_t thread_start = clock();
    // Perform sparse matrix multiplication with HIP
    sparse_matmul_hip(d_row_ptr_A, d_col_ind_A, d_val_A, A_rows, 
                      col_ind_A.size() * sizeof(int), val_A.size() * sizeof(double), 
                      d_row_ptr_B, d_col_ind_B, d_val_B, B_cols, 
                      col_ind_B.size() * sizeof(int), val_B.size() * sizeof(double), 
                      d_result);
    clock_t thread_end = clock();
    double thread_time_spent = (double)(thread_end - thread_start) / CLOCKS_PER_SEC;
    printf("Multi-threaded execution time: %f seconds\n", thread_time_spent);

    // Perform sparse matrix multiplication in single-threaded mode for error checking
    // std::vector<double> result_single_thread(A_rows * B_cols);
    // sparse_matmul_single_thread(row_ptr_A, col_ind_A, val_A, row_ptr_B, col_ind_B, val_B, result_single_thread, A_rows, B_cols);

    // // Copy results from GPU to CPU
    // std::vector<double> result_hip(A_rows * B_cols);
    // hipMemcpy(result_hip.data(), d_result, A_rows * B_cols * sizeof(double), hipMemcpyDeviceToHost);

    // // Validate results
    // bool equal = true;
    // for (size_t i = 0; i < result_single_thread.size(); i++) {
    //     if (fabs(result_hip[i] - result_single_thread[i]) > 1e-6) { // Use a tolerance for floating-point comparison
    //         equal = false;
    //         break;
    //     }
    // }
    // std::cout << "Results are " << (equal ? "equal" : "not equal") << std::endl;

    // print_result_matrix(result_hip, A_rows, B_cols);

    // Free allocated device memory
    hipFree(d_row_ptr_A);
    hipFree(d_col_ind_A);
    hipFree(d_val_A);
    hipFree(d_row_ptr_B);
    hipFree(d_col_ind_B);
    hipFree(d_val_B);
    hipFree(d_result);

    return 0;
}
