# test hello-pthreads
time build/ARM/gem5.opt -d unittest_res/hp_s configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12 > unittest_res/outS_hp &

time build/ARM/gem5.opt -d unittest_res/hp_w configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0 > unittest_res/outW_hp &

# test pthreads-lock
time build/ARM/gem5.opt -d unittest_res/pl_s configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12 > unittest_res/outS_pl &

time build/ARM/gem5.opt -d unittest_res/pl_w configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0 > unittest_res/outW_pl &

#test 2 applications simultaneously

# test hello-pthreads
time build/ARM/gem5.opt -d unittest_res/hp_s2 configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm"  "test_codes/hello_pthreads-arm" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12 > unittest_res/outS_hp2 &

time build/ARM/gem5.opt -d unittest_res/hp_w2 configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" "test_codes/hello_pthreads-arm" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0 > unittest_res/outW_hp2 &


# test pthreads-lock
time build/ARM/gem5.opt -d unittest_res/pl_s2 configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12 > unittest_res/outS_hp2 &

time build/ARM/gem5.opt -d unittest_res/pl_w2 configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0 > unittest_res/outW_hp2 &

# mix both applications
time build/ARM/gem5.opt -d unittest_res/plhp configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/hello_pthreads-arm" --threadTypes S W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12 > unittest_res/out_plhp &

time build/ARM/gem5.opt -d unittest_res/hppl configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" "test_codes/pthreads_lock-arm" --threadTypes S W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0 > unittest_res/out_hppl &
