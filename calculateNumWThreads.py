# This script is used to calculate the number of W threads (in-order threads) which can run in this system. We look at how many regs were free on an average when 1 OoO thread was run. We assign the rest of the regs to W threads

def process_file(filename):
    # Initialize variables to store the values
    free_int_reg_class_value = None
    free_vec_reg_class_value = None

    result_int = 0
    result_vec = 0

    # Open the file and read it line by line
    with open(filename, 'r') as file:
        for line in file:
            # Check if the line contains FreeIntRegClass
            if 'system.cpu_cluster.cpus.rename.FreeIntRegClass' in line:
                # Extract the value using split and strip
                free_int_reg_class_value = float(line.split()[1])
            
            # Check if the line contains FreeVecRegClass
            if 'system.cpu_cluster.cpus.rename.FreeVecRegClass' in line:
                # Extract the value using split and strip
                free_vec_reg_class_value = float(line.split()[1])

    # If the values were found, perform the division and print the results
    if free_int_reg_class_value is not None:
        result_int = free_int_reg_class_value / 42
        print(f'Result of FreeIntRegClass value ',free_int_reg_class_value ,' divided by 42 (num arch regs): ',result_int)
    else:
        print('FreeIntRegClass not found in the file.')

    if free_vec_reg_class_value is not None:
        result_vec = free_vec_reg_class_value / 44
        print(f'Result of FreeVecRegClass value ', free_vec_reg_class_value ,' divided by 44 (num arch regs): ',result_vec)
    else:
        print('FreeVecRegClass not found in the file.')

    min_val = min(result_int, result_vec)
    print('You can have upto ',min_val,' W Theads')

# Example usage:
process_file('sparse_matrix/sparce_matrix_dynamic_1pThread_parallelBottleneck_small_80_9_40/stats.txt')