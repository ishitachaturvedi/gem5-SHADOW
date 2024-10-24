#!/bin/bash

# Common configuration values
#ROBSize=128
ROBSize=64
numSIQEntries=128
numWIQEntries=20
LQEntries=72
SQEntries=68
numPhysFloatRegs=264
numPhysVecRegs=264
numPhysIntRegs=264

# Set your custom parameters here
OUTPUT_DIR="ROB_Scaling"       # Set your custom directory name
MATRIX_DIMENSIONS="1000 1000 1000 1000"              # Set your custom matrix dimensions
FILE_SUFFIX="1000_ROB64"                           # Set your custom file suffix for names

# Create necessary directories if they don't exist
mkdir -p sparse_matrix/$OUTPUT_DIR/txt_files

# Run simulations with different configurations
time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 10 1" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 2 -WThreads 0 -SThreads 2 \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
    &> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_no_pred_runahead.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 10 2" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 3 -WThreads 1 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
    &> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_no_pred_ratio_runahead.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 10 3" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 4 -WThreads 2 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
    &> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_no_pred_ratio_runahead.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 10 4" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 5 -WThreads 3 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
    &> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}_no_pred_ratio_runahead.txt &
