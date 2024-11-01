#!/bin/bash

# # Common configuration values
ROBSize=128
numSIQEntries=128
numWIQEntries=20
LQEntries=72
SQEntries=68
numPhysFloatRegs=280
numPhysVecRegs=280
numPhysIntRegs=280

# ROBSize=512
# numSIQEntries=512
# numWIQEntries=20
# LQEntries=144
# SQEntries=136
# numPhysFloatRegs=264
# numPhysVecRegs=264
# numPhysIntRegs=264

# Set your custom parameters here
OUTPUT_DIR="Scale_everything"       # Set your custom directory name
MATRIX_DIMENSIONS="2000 2000 2000 2000"              # Set your custom matrix dimensions
FILE_SUFFIX="2000_90Per_sparse_scaled"                           # Set your custom file suffix for names

# Create necessary directories if they don't exist
mkdir -p sparse_matrix/$OUTPUT_DIR/txt_files

# Run simulations with different configurations
time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 1" \
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
    > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}.txt \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_error.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 2" \
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
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_error.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 3" \
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
    -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_error.txt &

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 4" \
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
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}_error.txt &


# EXTRA THREADS RUN
time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S4W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 5" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 6 -WThreads 4 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S4W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S4W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 322 -numPhysVecRegs 324 -numPhysIntRegs 322 \

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S5W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 6" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 7 -WThreads 5 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S5W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S5W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 364 -numPhysVecRegs 375 -numPhysIntRegs 375 \


time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S6W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 7" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 8 -WThreads 6 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S6W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S6W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 406 -numPhysVecRegs 420 -numPhysIntRegs 420 \

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S7W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 8" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 9 -WThreads 7 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S7W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S7W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 448 -numPhysVecRegs 456 -numPhysIntRegs 448 \

time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S8W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 90 9" \
    --threadTypes S \
    --cpu="o3_grace_test" \
    --mem-type=DDR4_2400_16x4 \
    --mem-size=64GB \
    --mem-channels=2 \
    --num-cpus=1 \
    --smt -t 10 -WThreads 8 -SThreads 2 \
    -FirstThreadSOtherW=True \
    -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    -LQEntries $LQEntries -SQEntries $SQEntries \
    -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
    > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S8W_${FILE_SUFFIX}.txt  \
    2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S8W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 490 -numPhysVecRegs 500 -numPhysIntRegs 490 \


