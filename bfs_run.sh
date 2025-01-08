#!/bin/bash

# # Common configuration values
ROBSize=128
numSIQEntries=128
numWIQEntries=10
LQEntries=72
SQEntries=68
numPhysFloatRegs=300
numPhysVecRegs=300
numPhysIntRegs=300

# 270 280 290 _fixed

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
mkdir -p CRONO_results/bfs/$OUTPUT_DIR/txt_files
# fixing the size of the PRF for gem5, in-order threads are non-speculative and do not use ARF, so we dont need to do double assignment. They dont move forward unless TLB hit. 
# How do we manage other stalls? CPUs support other stall types as well. WB is OoO. 

PER="graph4096.txt"

VALS="graph4096.txt"

for MATRIX_DIMENSIONS in 1; do
    FILE_SUFFIX="${PER}_ROB128_REG300_8wide"           # Update the file suffix

    # # Run simulations with different configurations
    # OoO Configs
    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/Dynamic_1pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 1 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}.txt \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_1pThread_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/Dynamic_2pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 2 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_2pThread_${FILE_SUFFIX}.txt \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_2pThread_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/Dynamic_3pThread_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 3 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_3pThread_${FILE_SUFFIX}.txt \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/Dynamic_3pThread_${FILE_SUFFIX}_error.txt &

    # 1 OoO with many in order 
    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_1S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 2 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S1W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S1W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_1S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 3 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S2W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S2W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_1S3W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 4 $VALS" \
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
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs\
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S3W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S3W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_1S4W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 5 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S4W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_1S4W_${FILE_SUFFIX}_error.txt &


    # # 2 OoO with many in order 
    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_2S1W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 3 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_2S1W_${FILE_SUFFIX}_error.txt &

    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_2S2W_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 4 $VALS" \
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
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_2S2W_${FILE_SUFFIX}_error.txt &

    # in order threads
    time build/ARM/gem5.opt -d CRONO_results/bfs/$OUTPUT_DIR/pthreadDynamic_AllW6_$FILE_SUFFIX configs/example/se_SMT_ARM.py \
        "bmrk_binaries/CRONO/bfs 6 $VALS" \
        --threadTypes W \
        --cpu="o3_grace_test" \
        --mem-type=DDR4_2400_16x4 \
        --mem-size=64GB \
        --mem-channels=2 \
        --num-cpus=1 \
        --smt -t 6 -WThreads 6 -SThreads 0 \
        -ROBSize $ROBSize -numSIQEntries 60 -numWIQEntries 0 -numIQEntries 60 \
        -LQEntries 30 -SQEntries 30 -smtIQPolicy="Partitioned" -smtLSQPolicy="Partitioned" \
        -numPhysFloatRegs $numPhysFloatRegs -numPhysVecRegs $numPhysVecRegs -numPhysIntRegs $numPhysIntRegs \
        > CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}.txt  \
        2> CRONO_results/bfs/$OUTPUT_DIR/txt_files/pthreadDynamic_AllW6_${FILE_SUFFIX}_error.txt &

done