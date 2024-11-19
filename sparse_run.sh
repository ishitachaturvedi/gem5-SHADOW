#!/bin/bash

# # Common configuration values
ROBSize=256
numSIQEntries=256
numWIQEntries=20
LQEntries=72
SQEntries=68
numPhysFloatRegs=300
numPhysVecRegs=300
numPhysIntRegs=300

# ROBSize=256
# numSIQEntries=256
# numWIQEntries=20
# LQEntries=144
# SQEntries=136
# numPhysFloatRegs=264
# numPhysVecRegs=264
# numPhysIntRegs=264

OUTPUT_DIR="sweep_configuration"

# Create necessary directories if they don't exist
mkdir -p sparse_matrix/$OUTPUT_DIR/txt_files


# Loop over percentages from 10 to 90
#for PERCENTAGE in 50 60 70 80 85 90 95 98 99 99.9; do
#for PERCENTAGE in 50 60 70 80 85 90 98 99 99.9; do
for PERCENTAGE in 95; do
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned"  \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
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
        -ROBSize $ROBSize -numSIQEntries 40 -numWIQEntries 0 -numIQEntries $numSIQEntries \
         -LQEntries 72 -SQEntries 68 -smtIQPolicy="Partitioned" -SQEntries 40 -smtLSQPolicy="Partitioned" \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}_error.txt &

done