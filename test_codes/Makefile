# Makefile for hip code
HIP_PATH?= /opt/rocm/hip
HIPCC=$(HIP_PATH)/bin/hipcc

BIN_DIR?= ./bin

sparse_matrix_multiplication_hip: $(BIN_DIR)/sparse_matrix_multiplication_hip

dense_matrix_multiplication_hip: $(BIN_DIR)/dense_matrix_multiplication_hip

$(BIN_DIR)/sparse_matrix_multiplication_hip: sparse_matrix_multiplication_hip.cpp $(BIN_DIR)
	$(HIPCC) $(CXXFLAGS) --amdgpu-target=gfx902 sparse_matrix_multiplication_hip.cpp -o $(BIN_DIR)/sparse_matrix_multiplication_hip

$(BIN_DIR)/dense_matrix_multiplication_hip: dense_matrix_multiplication_hip.cpp $(BIN_DIR)
	$(HIPCC) $(CXXFLAGS) --amdgpu-target=gfx902 dense_matrix_multiplication_hip.cpp -o $(BIN_DIR)/dense_matrix_multiplication_hip

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: sparse_matrix_multiplication_hip dense_matrix_multiplication_hip clean