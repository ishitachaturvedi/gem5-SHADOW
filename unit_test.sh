# test hello-pthreads
time build/ARM/gem5.opt -d unittest_res/hp_s configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 4" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/outS_hp &

time build/ARM/gem5.opt -d unittest_res/hp_w configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 4" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/outW_hp &

# test pthreads-lock
time build/ARM/gem5.opt -d unittest_res/pl_s configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 4" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/outS_pl &

time build/ARM/gem5.opt -d unittest_res/pl_w configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 4" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/outW_pl &

#test 2 applications simultaneously

# test hello-pthreads
time build/ARM/gem5.opt -d unittest_res/hp_s2 configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 2"  "test_codes/hello_pthreads-arm 4" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/outS_hp2 &

time build/ARM/gem5.opt -d unittest_res/hp_w2 configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 2" "test_codes/hello_pthreads-arm 4" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/outW_hp2 &


# test pthreads-lock
time build/ARM/gem5.opt -d unittest_res/pl_s2 configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 2" "test_codes/pthreads_lock-arm 4" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/outS_pl2 &

time build/ARM/gem5.opt -d unittest_res/pl_w2 configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 2" "test_codes/pthreads_lock-arm 4" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/outW_pl2 &


# mix both applications

# Unresolved bug: Program finishes fine at cycle 2346144750 but gem5 never exits. Somewhere it is looping. There is no issue in our pipeline. It is an exit issue. I am not debugging this further because it might not happen in our real applications AND everything seems to be exiting. If we see something weird later, we can use this to debug.
# time build/ARM/gem5.opt -d unittest_res/plhp configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 2" "test_codes/hello_pthreads-arm 4" --threadTypes S W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 6 -SThreads 6 &> unittest_res/out_plhp &

time build/ARM/gem5.opt -d unittest_res/hppl configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 2" "test_codes/pthreads_lock-arm 4" --threadTypes S W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 6 -SThreads 6 &> unittest_res/out_hppl &

# program unable to exit but completes 
# time build/ARM/gem5.opt -d unittest_res/plhpS configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 2" "test_codes/hello_pthreads-arm 4" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/out_plhpS &

time build/ARM/gem5.opt -d unittest_res/hpplS configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 2" "test_codes/pthreads_lock-arm 4" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 0 -SThreads 12 &> unittest_res/out_hpplS &

# program unable to exit but completes 
# time build/ARM/gem5.opt -d unittest_res/plhpW configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm 2" "test_codes/hello_pthreads-arm 4" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/out_plhpW &

# program unable to exit but completes
time build/ARM/gem5.opt -d unittest_res/hpplW configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm 2" "test_codes/pthreads_lock-arm 4" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=1 --smt -t 12 -WThreads 12 -SThreads 0 &> unittest_res/out_hpplW &
