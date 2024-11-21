#!/bin/bash

# # Common configuration values
ROBSize=256
numSIQEntries=256
numWIQEntries=10
LQEntries=72
SQEntries=68
numPhysFloatRegs=270
numPhysVecRegs=270
numPhysIntRegs=270

# ROBSize=256
# numSIQEntries=256
# numWIQEntries=20
# LQEntries=144
# SQEntries=136
# numPhysFloatRegs=264
# numPhysVecRegs=264
# numPhysIntRegs=264

OUTPUT_DIR="test_sweep_configuration"

# Create necessary directories if they don't exist
mkdir -p sparse_matrix/$OUTPUT_DIR/txt_files
# fixing the size of the PRF for gem5, in-order threads are non-speculative and do not use ARF, so we dont need to do double assignment. They dont move forward unless TLB hit. 
# How do we manage other stalls? CPUs support other stall types as well. WB is OoO. 

# Loop over percentages from 10 to 90
#for PERCENTAGE in 50 60 70 80 85 90 95 98 99 99.9; do
#for PERCENTAGE in 50 70 85 90 98 99 99.9; do
#for PERCENTAGE in 70 80 85 90 95 99; do
#for PERCENTAGE in 80 90 95; do
for PERCENTAGE in 70 80 85; do
    MATRIX_DIMENSIONS="400 400 400 400 $PERCENTAGE"     # Update the fifth dimension
    FILE_SUFFIX="400_${PERCENTAGE}Per_sparse_ROB256"           # Update the file suffix

    # # Run simulations with different configurations

    # OoO Configs
    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 1" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 1 -WThreads 0 -SThreads 1 \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 226 -numPhysVecRegs 226 -numPhysIntRegs 226 -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned"  \
        > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}.txt \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_2pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 2" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 2 -WThreads 0 -SThreads 2 \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned" \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 182 -numPhysVecRegs 182 -numPhysIntRegs 182 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_2pThread_${FILE_SUFFIX}.txt \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_2pThread_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_3pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 3" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 3 -WThreads 0 -SThreads 3 \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned"\
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 138 -numPhysVecRegs 138 -numPhysIntRegs 138 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_3pThread_${FILE_SUFFIX}.txt \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_3pThread_${FILE_SUFFIX}_error.txt &

    # 1 OoO with many in order 
    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_1S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 2" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 2 -WThreads 1 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 226 -numPhysVecRegs 226 -numPhysIntRegs 226 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S1W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S1W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_1S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 3" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 3 -WThreads 2 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 226 -numPhysVecRegs 226 -numPhysIntRegs 226 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S2W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S2W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_1S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 4 -WThreads 3 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 226 -numPhysVecRegs 226 -numPhysIntRegs 226\
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S3W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S3W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_1S4W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 5" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 5 -WThreads 4 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 226 -numPhysVecRegs 226 -numPhysIntRegs 226 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S4W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_1S4W_${FILE_SUFFIX}_error.txt &


    # 2 OoO with many in order 
    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 3" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 3 -WThreads 1 -SThreads 2 \
        -FirstThreadSOtherW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 182 -numPhysVecRegs 182 -numPhysIntRegs 182 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 4 -WThreads 2 -SThreads 2 \
        -FirstThreadSOtherW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries -numIQEntries $numSIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs 182 -numPhysVecRegs 182 -numPhysIntRegs 182 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_error.txt &

    # in order threads
    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_AllW6_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 6" \
        --threadTypes W \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 6 -WThreads 6 -SThreads 0 \
        -ROBSize $ROBSize -numSIQEntries 60 -numWIQEntries 0 -numIQEntries 60 \
        -LQEntries 30 -SQEntries 30 -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned" \
        -numPhysFloatRegs 270 -numPhysVecRegs 270 -numPhysIntRegs 270 \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}_error.txt &

done