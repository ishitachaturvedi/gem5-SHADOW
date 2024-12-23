# parsec.blackscholes
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/parsec_FINAL/blackscholes 1 bmrk_binaries/parsec_FINAL/in_16.txt prices.txt' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280

# CRONO apsp
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/apsp_work_stealing 1 128 16' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280 > out

# CRONO bc 
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/bc_work_stealing 1 128 16' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280


# CRONO bfs 
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/bfs_work_stealing 0 1 128 16' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280


# CRONO tsp
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/tsp_work_stealing_test 2 4' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280

# Rodinia backprop
# <num of input elements> <num threads>
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/backprop 10 2' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280

# Rodinia heartwall
# heartwall <inputfile> <num of frames> <num of threads>
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/heartwall bmrk_binaries/data/rodinia-data/heartwall/test.avi 1 1' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280

# Rodinia nn
# nn <filelist> <num> <target latitude> <target longitude> <num_threads>
time build/ARM/gem5.opt  -d test/test configs/example/se_SMT_ARM.py 'bmrk_binaries/CRONO/nn filelist.txt 20 30 10 2' --threadTypes S --cpu=o3_grace_test --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 2 -WThreads 0 -SThreads 2 -FirstThreadSOtherW=True -ROBSize 256 -numSIQEntries 256 -numWIQEntries 10 -numIQEntries 256 -LQEntries 72 -SQEntries 68 -numPhysFloatRegs 280 -numPhysVecRegs 280 -numPhysIntRegs 280

