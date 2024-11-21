# Copyright (c) 2016-2017 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# build hello_pthreads for arm: aarch64-linux-gnu-gcc hello_pthreads.c -o hello_pthreads-arm -static -lpthread

# to run various SMT config CPUs: use o3_grace_1thread, o3_grace_2thread, o3_grace_4thread or o3_grace_8thread for 1,2,4,8 SMT thread CPU configs. Each config has a different ROB,IQ,Reg size. All other values are unmodified.

# Run gem5 command: build/ARM/gem5.opt configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=5 --smt > out

# build/ARM/gem5.opt configs/example/se_SMT_ARM.py "/scratch/ishitac/starbench/rgbyuv/pthread/rgbyuv -i /scratch/ishitac/starbench/rgbyuv/pthread/sample_1280_853.ppm -c 1 -t 8 -p 1" --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=3 --smt

# multiple workloads: Many workloads run on 1 cpu. We dont have a config with many workloads and many CPUs. It brings in the question of load balancing between CPUs. That is beyond
# the scope of this work. 
#time build/ARM/gem5.opt -d out_md5_1_dir configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm " "test_codes/pthreads_lock-arm" --caches --l1d_size=3kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=11 --smt -t 3

#time build/ARM/gem5.opt -d SThreads configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes S W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 0 -SThreads 6 > outS

# Adding S and W threads and workload types:
# time build/ARM/gem5.opt --debug-flags=Rename -d out_md5_1_dir configs/example/se_SMT_ARM.py "/scratch/ishitac/starbench/md5/pthread/md5 -i 3 -c 1 -t 2 " "/scratch/ishitac/starbench/md5/pthread/md5 -i 3 -c 1 -t 2 " --threadTypes S W --caches --l1d_size=3kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=2 --smt -t 6 -WThreads 3 -SThreads 3

# time build/ARM/gem5.opt configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes S W --caches --l1d_size=3kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=2 --smt -t 10 -WThreads 5 -SThreads 5

# time build/ARM/gem5.opt configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" --threadTypes W --caches --l1d_size=3kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=2 --smt -t 6 -WThreads 6 -SThreads 0

# time build/ARM/gem5.opt -d WThreads configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3_grace" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 6 -SThreads 0

# time build/ARM/gem5.opt configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes S S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 0 -SThreads 12

# time build/ARM/gem5.opt configs/example/se_SMT_ARM.py "test_codes/pthreads_lock-arm" "test_codes/pthreads_lock-arm" --threadTypes W W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 12 -WThreads 12 -SThreads 0

# TEST
# build/ARM/gem5.opt --debug-flags=O3CPU configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 0 -SThreads 6

# time build/ARM/gem5.opt -d WThreads configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 6 -SThreads 0

# build/ARM/gem5.opt --debug-end=151250 --debug-flags=LSQUnit,O3CPUAll,MemDepUnit configs/example/se_SMT_ARM.py "test_codes/hello_pthreads-arm" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 0 -SThreads 6 > out


# build/ARM/gem5.opt configs/example/se_SMT_ARM.py "bmrk_binaries/CRONO/tsp 2 16" --threadTypes S --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 0 -SThreads 6 > outS

# time build/ARM/gem5.opt -d WThreads_tsp configs/example/se_SMT_ARM.py "bmrk_binaries/CRONO/tsp 2 16" --threadTypes W --caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --l2cache --cpu="o3" --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 --num-cpus=10 --smt -t 6 -WThreads 6 -SThreads 0 > out

"""This script is the syscall emulation example script from the ARM
Research Starter Kit on System Modeling. More information can be found
at: http://www.arm.com/ResearchEnablement/SystemModeling
"""

import os
import m5
from m5.util import addToPath
from m5.objects import *
import argparse
import shlex
from pdb import set_trace as bp

m5.util.addToPath("..")

from common import Options
from common import ObjectList
from common import MemConfig
from common.cores.arm import O3_ARM_v7a, HPI, O3_Novocore, O3_ARM_grace, ex5_big
from common import Simulation

import sys
pathadd = os.getcwd()+'/configs/example/arm/'
sys.path.append(pathadd)
import devices

# Pre-defined CPU configurations. Each tuple must be ordered as : (cpu_class,
# l1_icache_class, l1_dcache_class, walk_cache_class, l2_Cache_class). Any of
# the cache class may be 'None' if the particular cache is not present.
cpu_type = {
    "atomic": (AtomicSimpleCPU, None, None, None),
    "minor": (MinorCPU, devices.L1I, devices.L1D, devices.L2),
    "hpi": (HPI.HPI, HPI.HPI_ICache, HPI.HPI_DCache, HPI.HPI_L2),
    "O3CPU":(DerivO3CPU,devices.L1I, devices.L1D, devices.L2),
    "big_core":(ex5_big.ex5_big,ex5_big.L1I,ex5_big.L1I, ex5_big.L1D, ex5_big.L2),
    "o3": (
        O3_ARM_v7a.O3_ARM_v7a_3,
        O3_ARM_v7a.O3_ARM_v7a_ICache_Strong,
        O3_ARM_v7a.O3_ARM_v7a_ICache_Weak,
        O3_ARM_v7a.O3_ARM_v7a_DCache,
        O3_ARM_v7a.O3_ARM_v7aL2,
    ),
    "o3_novo": (
        O3_Novocore.NovoO3CPU,
        O3_Novocore.O3_ARM_Novocore_ICache,
        O3_Novocore.O3_ARM_Novocore_ICache,
        O3_Novocore.O3_ARM_Novocore_DCache,
        O3_Novocore.O3_ARM_Novocore_L2,
    ),
    "o3_grace": (
        O3_ARM_grace.Grace12Wide,
        O3_ARM_grace.O3_ARM_grace_ICache,
        O3_ARM_grace.O3_ARM_grace_ICache,
        O3_ARM_grace.O3_ARM_grace_DCache,
        O3_ARM_grace.O3_ARM_grace_L2,
    ),"o3_grace_test": (
        O3_ARM_grace.Grace12Wide_testConfig,
        O3_ARM_grace.O3_ARM_grace_ICache,
        O3_ARM_grace.O3_ARM_grace_ICache,
        O3_ARM_grace.O3_ARM_grace_DCache,
        O3_ARM_grace.O3_ARM_grace_L2
        # O3_ARM_grace.O3_ARM_grace_ICache_Perfect,
        # O3_ARM_grace.O3_ARM_grace_ICache_Perfect,
        # O3_ARM_grace.O3_ARM_grace_DCache_Perfect,
        # O3_ARM_grace.O3_ARM_grace_L2_Perfect,
    ),
}


class SimpleSeSystem(System):
    """
    Example system class for syscall emulation mode
    """

    # Use a fixed cache line size of 64 bytes
    cache_line_size = 64

    def __init__(self, args, **kwargs):
        super(SimpleSeSystem, self).__init__(**kwargs)

        # Setup book keeping to be able to use CpuClusters from the
        # devices module.
        self._clusters = []
        self._num_cpus = 0

        # Create a voltage and clock domain for system components
        self.voltage_domain = VoltageDomain(voltage="3.3V")
        self.clk_domain = SrcClockDomain(
            clock="1GHz", voltage_domain=self.voltage_domain
        )

        # Create the off-chip memory bus.
        self.membus = SystemXBar()

        # Wire up the system port that gem5 uses to load the kernel
        # and to perform debug accesses.
        self.system_port = self.membus.cpu_side_ports

        # Add CPUs to the system. A cluster of CPUs typically have
        # private L1 caches and a shared L2 cache.

        self.cpu_cluster = devices.CpuCluster(
            self, args.num_cpus, args.cpu_freq, "1.2V", *cpu_type[args.cpu], args.t
        )

        if(args.FirstThreadSOtherW == True):
            args.smtIQPolicy = "SDynamicWStatic"

        for cpu in self.cpu_cluster.cpus:
            cpu.numThreads = args.t
            cpu.SThreads = args.SThreads
            cpu.WThreads = args.WThreads
            cpu.numROBEntries = args.ROBSize
            cpu.LQEntries = args.LQEntries
            cpu.numWIQEntries = args.numWIQEntries
            cpu.numSIQEntries =  args.numSIQEntries
            cpu.runTillSThreads = args.runTillSThreads
            cpu.SingleThreadFetchIEW = args.SingleThreadFetchIEW
            cpu.UseSplitCache = args.UseSplitCache
            cpu.FirstThreadSOtherW = args.FirstThreadSOtherW
            cpu.smtIQPolicy = args.smtIQPolicy
            cpu.MainSAllPW = args.MainSAllPW
            cpu.predictOnWThreads = args.predictOnWThreads
            cpu.numPhysFloatRegs = args.numPhysFloatRegs
            cpu.numPhysVecRegs = args.numPhysVecRegs
            cpu.numPhysIntRegs = args.numPhysIntRegs
            cpu.smtROBPolicy = args.smtROBPolicy
            cpu.smtLSQPolicy = args.smtLSQPolicy
            cpu.SQEntries = args.SQEntries
            cpu.numIQEntries = args.numIQEntries

        # for cpu in self.cpu_cluster.cpus:
        #     cpu.numThreads = numThreads

        # Create a cache hierarchy (unless we are simulating a
        # functional CPU in atomic memory mode) for the CPU cluster
        # and connect it to the shared memory bus.
        if self.cpu_cluster.memoryMode() == "timing":
            self.cpu_cluster.addL1()
            self.cpu_cluster.addL2(self.cpu_cluster.clk_domain)
        self.cpu_cluster.connectMemSide(self.membus)

        # Tell gem5 about the memory mode used by the CPUs we are
        # simulating.
        self.mem_mode = self.cpu_cluster.memoryMode()

    def numCpuClusters(self):
        return len(self._clusters)

    def addCpuCluster(self, cpu_cluster, num_cpus):
        assert cpu_cluster not in self._clusters
        assert num_cpus > 0
        self._clusters.append(cpu_cluster)
        self._num_cpus += num_cpus

    def numCpus(self):
        return self._num_cpus


def get_processes(cmd,threadTypes,env):
    """Interprets provided args and returns a list of processes"""

    cwd = os.getcwd()
    multiprocesses = []

    index = 0

    for idx, c in enumerate(cmd):
        argv = shlex.split(c)

        #bp()
        process = Process(pid=100 + idx, cwd=cwd, cmd=argv, executable=argv[0])
        process.gid = os.getgid()
        process.processThreadType = threadTypes[index]

        if env:
            with open(env, "r") as f:
                process.env = [line.rstrip() for line in f]

        print("info: %d. command and arguments: %s" % (idx + 1, process.cmd))
        multiprocesses.append(process)
        index = index + 1

    return multiprocesses

def create(args):
    """Create and configure the system object."""

    system = SimpleSeSystem(args)

    system.multi_thread = True

    # Tell components about the expected physical memory ranges. This
    # is, for example, used by the MemConfig helper to determine where
    # to map DRAMs in the physical address space.
    system.mem_ranges = [AddrRange(start=0, size=args.mem_size)]

    # Configure the off-chip memory system.
    MemConfig.config_mem(args, system)

    # Parse the command line and get a list of Processes instances
    # that we can pass to gem5.
    processes = get_processes(args.commands_to_run,args.threadTypes,args.env)

    #print("ARGS :",args.commands_to_run)
    # if len(processes) != args.num_cores:
    #     print("Error: Cannot map %d command(s) onto %d CPU(s)"% (len(processes), args.num_cores)
    #     )
    #     sys.exit(1)

    system.workload = SEWorkload.init_compatible(processes[0].executable)

    # Assign one workload to each CPU
    print("system.cpu_cluster.cpus ",len(system.cpu_cluster.cpus)," processes ",len(processes))

    # For 1 process per thread
    # for cpu, workload in zip(system.cpu_cluster.cpus, processes):
    #     cpu.workload = workload
    #     cpu.numThreads = 1

    # For 1 process per many threads
    # workload = processes[0]

    # for cpu in system.cpu_cluster.cpus:
    #     cpu.workload = workload

    # For many workloads with many threads
    # Constraint: All processes run on 1 CPU. We dont have support for multiple CPUs currently if we want to run multiple 

    for cpu in system.cpu_cluster.cpus:
        cpu.workload = processes
        cpu.threadTypes = args.threadTypes

    return system


def main():
    parser = argparse.ArgumentParser()
    Options.addCommonOptions(parser)
    Options.addSEOptions(parser)


    parser.add_argument(
        "commands_to_run",
        metavar="command(s)",
        nargs="*",
        help="Command(s) to run",
    )

    parser.add_argument(
        "--threadTypes",
        metavar="type",
        nargs="+",
        help="Thread types Strong or Weak for this application. If 2 Strong applications: --threadTypes S S",
    )

    parser.add_argument(
        "--cpu",
        type=str,
        choices=list(cpu_type.keys()),
        default="atomic",
        help="CPU model to use",
    )
    parser.add_argument("--cpu-freq", type=str, default="4GHz")
    parser.add_argument(
        "--num-cores", type=int, default=1, help="Number of CPU cores"
    )

    parser.add_argument(
        "--checkpoint-path",
        type=str,
        required=False,
        default="riscv-hello-checkpoint/",
        help="The directory to store the checkpoint.",
    )

    parser.add_argument('-t', type=int, default = 1)

    parser.add_argument('-SThreads', type=int, default = 1)

    parser.add_argument('-WThreads', type=int, default = 0)

    parser.add_argument('-ROBSize', type=int, default = 128)

    parser.add_argument('-numSIQEntries', type=int, default = 120)

    parser.add_argument('-numIQEntries', type=int, default = 120)

    parser.add_argument('-numWIQEntries', type=int, default = 10)
    
    parser.add_argument('-LQEntries', type=int, default = 68)
    
    parser.add_argument('-SQEntries', type=int, default = 72)

    parser.add_argument('-runTillSThreads', type=bool, default=False)

    parser.add_argument('-numPhysFloatRegs', type=int, default = 264)

    parser.add_argument('-numPhysVecRegs', type=int, default = 264)

    parser.add_argument('-numPhysIntRegs', type=int, default = 264)

    parser.add_argument('-env', type=str)

    parser.add_argument('-predictOnWThreads', type=bool, default=False)

    parser.add_argument('-SingleThreadFetchIEW', type=bool, default=False)

    parser.add_argument('-UseSplitCache', type=bool, default=False)

    #run first thread as S for ILP and others as W as TLP
    parser.add_argument('-FirstThreadSOtherW', type=bool, default=False)

    parser.add_argument('-smtIQPolicy', type=str, default="SDynamicWStatic")

    parser.add_argument('-smtROBPolicy', type=str, default="Partitioned")

    parser.add_argument('-MainSAllPW', type=bool, default=False)

    parser.add_argument('-smtLSQPolicy', type=str, default="SDynamicWStatic")

    args = parser.parse_args()

    if(args.WThreads == 0):
        args.SThreads =  args.t

    # Create a mapping dictionary for ThreadTypes
    mapping = {'S': 1, 'W': 2}

    # Use a list comprehension to create a new list with the mapped values
    args.threadTypes = [mapping[item] for item in args.threadTypes]

    # Create a single root node for gem5's object hierarchy. There can
    # only exist one root node in the simulator at any given
    # time. Tell gem5 that we want to use syscall emulation mode
    # instead of full system mode.
    root = Root(full_system=False)

    # Populate the root node with a system. A system corresponds to a
    # single node with shared memory.
    root.system = create(args)

    #exit(0)

    # Instantiate the C++ object hierarchy. After this point,
    # SimObjects can't be instantiated anymore.
    m5.instantiate()


    # Start the simulator. This gives control to the C++ world and
    # starts the simulator. The returned event tells the simulation
    # script why the simulator exited.
    event = m5.simulate()

    # Print the reason for the simulation exit. Some exit codes are
    # requests for service (e.g., checkpoints) from the simulation
    # script. We'll just ignore them here and exit.
    print(event.getCause(), " @ ", m5.curTick())
    sys.exit(event.getCode())


if __name__ == "__m5_main__":
    main()