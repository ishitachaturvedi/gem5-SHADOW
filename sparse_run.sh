#!/bin/bash

# # Common configuration values
ROBSize=256
numSIQEntries=256
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

OUTPUT_DIR="changing_sparsity"

# Create necessary directories if they don't exist
mkdir -p sparse_matrix/$OUTPUT_DIR/txt_files


# Loop over percentages from 10 to 90
for PERCENTAGE in {10..90..10}; do
#for PERCENTAGE in 90; do
    MATRIX_DIMENSIONS="200 200 200 200 $PERCENTAGE"     # Update the fifth dimension
    FILE_SUFFIX="200_${PERCENTAGE}Per_sparse_ROB256_test1"           # Update the file suffix

    # Run simulations with different configurations
        time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 1" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 1 -WThreads 0 -SThreads 1 \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}.txt \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 2" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 2 -WThreads 1 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 3" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 3 -WThreads 2 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
        --threadTypes S \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 4 -WThreads 3 -SThreads 1 \
        -MainSAllPW=True \
        -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
        -LQEntries $LQEntries -SQEntries $SQEntries \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_AllW4_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
        --threadTypes W \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 4 -WThreads 4 -SThreads 0 \
        -ROBSize $ROBSize -numSIQEntries 40 -numWIQEntries 0 \
        -LQEntries 100 -SQEntries 100 -smtIQPolicy="Partitioned" -SQEntries 40 -smtLSQPolicy="Partitioned" \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW4_${FILE_SUFFIX}.txt  \
        2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW4_${FILE_SUFFIX}_error.txt &
done


# for PERCENTAGE in 90; do
#     MATRIX_DIMENSIONS="400 400 400 400 $PERCENTAGE"     # Update the fifth dimension
#     FILE_SUFFIX="400_${PERCENTAGE}Per_sparse_ROB256_test1"           # Update the file suffix

#     # Run simulations with different configurations
#     time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#         "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 1" \
#         --threadTypes S \
#         --cpu="o3_grace_test" \
#         --mem-type=DDR4_2400_16x4 \
#         --mem-size=64GB \
#         --mem-channels=2 \
#         --num-cpus=1 \
#         --smt -t 2 -WThreads 0 -SThreads 2 \
#         -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#         -LQEntries $LQEntries -SQEntries $SQEntries \
#         -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
#         > sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}.txt \
#         2> sparse_matrix/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_error.txt &

#     time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#         "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 2" \
#         --threadTypes S \
#         --cpu="o3_grace_test" \
#         --mem-type=DDR4_2400_16x4 \
#         --mem-size=64GB \
#         --mem-channels=2 \
#         --num-cpus=1 \
#         --smt -t 3 -WThreads 1 -SThreads 2 \
#         -FirstThreadSOtherW=True \
#         -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#         -LQEntries $LQEntries -SQEntries $SQEntries \
#         -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
#         > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}.txt  \
#         2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_error.txt &

#     time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#         "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 3" \
#         --threadTypes S \
#         --cpu="o3_grace_test" \
#         --mem-type=DDR4_2400_16x4 \
#         --mem-size=64GB \
#         --mem-channels=2 \
#         --num-cpus=1 \
#         --smt -t 4 -WThreads 2 -SThreads 2 \
#         -FirstThreadSOtherW=True \
#         -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#         -LQEntries $LQEntries -SQEntries $SQEntries \
#         -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
#         > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}.txt  \
#         2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_error.txt &

#     time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#         "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
#         --threadTypes S \
#         --cpu="o3_grace_test" \
#         --mem-type=DDR4_2400_16x4 \
#         --mem-size=64GB \
#         --mem-channels=2 \
#         --num-cpus=1 \
#         --smt -t 5 -WThreads 3 -SThreads 2 \
#         -FirstThreadSOtherW=True \
#         -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#         -LQEntries $LQEntries -SQEntries $SQEntries \
#         -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
#         > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}.txt  \
#         2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S3W_${FILE_SUFFIX}_error.txt &

#     time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_AllW4_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#         "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 4" \
#         --threadTypes S \
#         --cpu="o3_grace_test" \
#         --mem-type=DDR4_2400_16x4 \
#         --mem-size=64GB \
#         --mem-channels=2 \
#         --num-cpus=1 \
#         --smt -t 5 -WThreads 4 -SThreads 1 \
#         -MainSAllPW=True \
#         -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#         -LQEntries $LQEntries -SQEntries $SQEntries \
#         -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
#         > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW4_${FILE_SUFFIX}.txt  \
#         2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW4_${FILE_SUFFIX}_error.txt &
# done


# # EXTRA THREADS RUN

    # time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S4W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
    #     "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 5" \
    #     --threadTypes S \
    #     --cpu="o3_grace_test" \
    #     --mem-type=DDR4_2400_16x4 \
    #     --mem-size=64GB \
    #     --mem-channels=2 \
    #     --num-cpus=1 \
    #     --smt -t 6 -WThreads 4 -SThreads 2 \
    #     -FirstThreadSOtherW=True \
    #     -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
    #     -LQEntries $LQEntries -SQEntries $SQEntries \
    #     -numPhysFloatRegs 1000 -numPhysVecRegs 1000 -numPhysIntRegs 1000 \
    #     > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S4W_${FILE_SUFFIX}.txt  \
    #     2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S4W_${FILE_SUFFIX}_error.txt &

#     #-numPhysFloatRegs 322 -numPhysVecRegs 324 -numPhysIntRegs 322 \

# time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S5W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#     "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 6" \
#     --threadTypes S \
#     --cpu="o3_grace_test" \
#     --mem-type=DDR4_2400_16x4 \
#     --mem-size=64GB \
#     --mem-channels=2 \
#     --num-cpus=1 \
#     --smt -t 7 -WThreads 5 -SThreads 2 \
#     -FirstThreadSOtherW=True \
#     -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#     -LQEntries $LQEntries -SQEntries $SQEntries \
#     -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
#     > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S5W_${FILE_SUFFIX}.txt  \
#     2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S5W_${FILE_SUFFIX}_error.txt &

#     #-numPhysFloatRegs 364 -numPhysVecRegs 375 -numPhysIntRegs 375 \


# time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S6W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#     "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 7" \
#     --threadTypes S \
#     --cpu="o3_grace_test" \
#     --mem-type=DDR4_2400_16x4 \
#     --mem-size=64GB \
#     --mem-channels=2 \
#     --num-cpus=1 \
#     --smt -t 8 -WThreads 6 -SThreads 2 \
#     -FirstThreadSOtherW=True \
#     -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#     -LQEntries $LQEntries -SQEntries $SQEntries \
#     -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
#     > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S6W_${FILE_SUFFIX}.txt  \
#     2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S6W_${FILE_SUFFIX}_error.txt &

#     #-numPhysFloatRegs 406 -numPhysVecRegs 420 -numPhysIntRegs 420 \

# time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S7W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#     "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 8" \
#     --threadTypes S \
#     --cpu="o3_grace_test" \
#     --mem-type=DDR4_2400_16x4 \
#     --mem-size=64GB \
#     --mem-channels=2 \
#     --num-cpus=1 \
#     --smt -t 9 -WThreads 7 -SThreads 2 \
#     -FirstThreadSOtherW=True \
#     -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#     -LQEntries $LQEntries -SQEntries $SQEntries \
#     -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
#     > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S7W_${FILE_SUFFIX}.txt  \
#     2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S7W_${FILE_SUFFIX}_error.txt &

#     #-numPhysFloatRegs 448 -numPhysVecRegs 456 -numPhysIntRegs 448 \

# time build/ARM/gem5.opt -d sparse_matrix/$OUTPUT_DIR/pthreadDynamic_2S8W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
#     "test_codes/sparce_matrix_dynamic_row $MATRIX_DIMENSIONS 9" \
#     --threadTypes S \
#     --cpu="o3_grace_test" \
#     --mem-type=DDR4_2400_16x4 \
#     --mem-size=64GB \
#     --mem-channels=2 \
#     --num-cpus=1 \
#     --smt -t 10 -WThreads 8 -SThreads 2 \
#     -FirstThreadSOtherW=True \
#     -ROBSize $ROBSize -numSIQEntries $numSIQEntries -numWIQEntries $numWIQEntries \
#     -LQEntries $LQEntries -SQEntries $SQEntries \
#     -numPhysFloatRegs 700 -numPhysVecRegs 700 -numPhysIntRegs 700 \
#     > sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S8W_${FILE_SUFFIX}.txt  \
#     2> sparse_matrix/$OUTPUT_DIR/txt_files/pthreadDynamic_2S8W_${FILE_SUFFIX}_error.txt &

    #-numPhysFloatRegs 490 -numPhysVecRegs 500 -numPhysIntRegs 490 \


