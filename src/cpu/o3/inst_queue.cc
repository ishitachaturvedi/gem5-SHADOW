/*
 * Copyright (c) 2011-2014, 2017-2020 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/o3/inst_queue.hh"

#include <limits>
#include <vector>
#include <unordered_set>

#include "base/logging.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/fu_pool.hh"
#include "cpu/o3/limits.hh"
#include "debug/IQ.hh"
#include "enums/OpClass.hh"
#include "params/BaseO3CPU.hh"
#include "sim/core.hh"

// clang complains about std::set being overloaded with Packet::set if
// we open up the entire namespace std
using std::list;

namespace gem5
{

namespace o3
{

InstructionQueue::FUCompletion::FUCompletion(const DynInstPtr &_inst,
    int fu_idx, InstructionQueue *iq_ptr)
    : Event(Stat_Event_Pri, AutoDelete),
      inst(_inst), fuIdx(fu_idx), iqPtr(iq_ptr), freeFU(false)
{
}

void
InstructionQueue::FUCompletion::process()
{
    iqPtr->processFUCompletion(inst, freeFU ? fuIdx : -1);
    inst = NULL;
}


const char *
InstructionQueue::FUCompletion::description() const
{
    return "Functional unit completion";
}

InstructionQueue::InstructionQueue(CPU *cpu_ptr, IEW *iew_ptr,
        const BaseO3CPUParams &params)
    : cpu(cpu_ptr),
      iewStage(iew_ptr),
      fuPool(params.fuPool),
      iqPolicy(params.smtIQPolicy),
      numThreads(params.numThreads),
      numEntries(params.numIQEntries),
      totalWidth(params.issueWidth),
      commitToIEWDelay(params.commitToIEWDelay),
      iqStats(cpu, totalWidth),
      iqIOStats(cpu)
{
    assert(fuPool);

    const auto &reg_classes = params.isa[0]->regClasses();
    // Set the number of total physical registers
    // As the vector registers have two addressing modes, they are added twice
    numPhysRegs = params.numPhysIntRegs + params.numPhysFloatRegs +
                    params.numPhysVecRegs +
                    params.numPhysVecRegs * (
                            reg_classes.at(VecElemClass)->numRegs() /
                            reg_classes.at(VecRegClass)->numRegs()) +
                    params.numPhysVecPredRegs +
                    params.numPhysCCRegs;

    //Create an entry for each physical register within the
    //dependency graph.
    dependGraph.resize(numPhysRegs);

    // Resize the register scoreboard.
    regScoreboard.resize(numPhysRegs);

    //Initialize Mem Dependence Units
    for (ThreadID tid = 0; tid < MaxThreads; tid++) {
        memDepUnit[tid].init(params, tid, cpu_ptr);
        memDepUnit[tid].setIQ(this);
    }

    resetState();

    //Figure out resource sharing policy
    if (iqPolicy == SMTQueuePolicy::Dynamic) {
        //Set Max Entries to Total ROB Capacity
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = numEntries;
        }

    } else if (iqPolicy == SMTQueuePolicy::Partitioned) {
        //@todo:make work if part_amt doesnt divide evenly.
        int part_amt = numEntries / numThreads;

        //Divide ROB up evenly
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = part_amt;
        }

        DPRINTF(IQ, "IQ sharing policy set to Partitioned:"
                "%i entries per thread.\n",part_amt);
    } else if (iqPolicy == SMTQueuePolicy::Threshold) {
        double threshold =  (double)params.smtIQThreshold / 100;

        int thresholdIQ = (int)((double)threshold * numEntries);

        //Divide up by threshold amount
        for (ThreadID tid = 0; tid < numThreads; tid++) {
            maxEntries[tid] = thresholdIQ;
        }

        DPRINTF(IQ, "IQ sharing policy set to Threshold:"
                "%i entries per thread.\n",thresholdIQ);
   }
    for (ThreadID tid = numThreads; tid < MaxThreads; tid++) {
        maxEntries[tid] = 0;
    }
}

InstructionQueue::~InstructionQueue()
{
    dependGraph.reset();
    dependGraph.resetWAWGraph();
    dependGraph.resetRAW();
    dependGraph.resetWAR();
    
#ifdef DEBUG
    cprintf("Nodes traversed: %i, removed: %i\n",
            dependGraph.nodesTraversed, dependGraph.nodesRemoved);
#endif
}

std::string
InstructionQueue::name() const
{
    return cpu->name() + ".iq";
}

InstructionQueue::IQStats::IQStats(CPU *cpu, const unsigned &total_width)
    : statistics::Group(cpu),
    ADD_STAT(instsAdded, statistics::units::Count::get(),
             "Number of instructions added to the IQ (excludes non-spec)"),
    ADD_STAT(nonSpecInstsAdded, statistics::units::Count::get(),
             "Number of non-speculative instructions added to the IQ"),
    ADD_STAT(instsIssued, statistics::units::Count::get(),
             "Number of instructions issued"),
    ADD_STAT(intInstsIssued, statistics::units::Count::get(),
             "Number of integer instructions issued"),
    ADD_STAT(floatInstsIssued, statistics::units::Count::get(),
             "Number of float instructions issued"),
    ADD_STAT(branchInstsIssued, statistics::units::Count::get(),
             "Number of branch instructions issued"),
    ADD_STAT(memInstsIssued, statistics::units::Count::get(),
             "Number of memory instructions issued"),
    ADD_STAT(fuBusyS, statistics::units::Count::get(),
             "FU busy when requested"),
    ADD_STAT(fuBusyW, statistics::units::Count::get(),
             "FU busy when requested"),
    ADD_STAT(miscInstsIssued, statistics::units::Count::get(),
             "Number of miscellaneous instructions issued"),
    ADD_STAT(squashedInstsIssued, statistics::units::Count::get(),
             "Number of squashed instructions issued"),
    ADD_STAT(squashedInstsExamined, statistics::units::Count::get(),
             "Number of squashed instructions iterated over during squash; "
             "mainly for profiling"),
    ADD_STAT(squashedOperandsExamined, statistics::units::Count::get(),
             "Number of squashed operands that are examined and possibly "
             "removed from graph"),
    ADD_STAT(squashedNonSpecRemoved, statistics::units::Count::get(),
             "Number of squashed non-spec instructions that were removed"),
    ADD_STAT(numIssuedDist, statistics::units::Count::get(),
             "Number of insts issued each cycle"),
    ADD_STAT(statFuBusy, statistics::units::Count::get(),
             "attempts to use FU when none available"),
    ADD_STAT(statFuNoFree, statistics::units::Count::get(),
             "attempts to use FU when none available"),
    ADD_STAT(statIssuedInstType, statistics::units::Count::get(),
             "Number of instructions issued per FU type, per thread"),
    ADD_STAT(statFuBusyPerThread, statistics::units::Count::get(),
             "Number of instructions structural stalls per FU type, per thread"),
    ADD_STAT(statOlderROBNotIssuedPerThread, statistics::units::Count::get(),
             "Number of times instruction not issued because older ROB instruction not issued, per thread"),
    ADD_STAT(statStalledOnControlInstructionPerThread, statistics::units::Count::get(),
             "Number of times instruction not issued because older control instruction not completed, per thread"),
    ADD_STAT(statFuBusyPerThreadCollective, statistics::units::Count::get(),
             "Number of times instruction not issued because FU not available, per thread"),
    ADD_STAT(statStalledOnMemoryReorderPerThread, statistics::units::Count::get(),
             "Number of times instruction not issued because older memory instruction not completed, per thread"),
    ADD_STAT(statStalledNotOldestInIQPerThread, statistics::units::Count::get(),
             "Number of times instruction not issued because older IQ instruction not issued, per thread"),
    ADD_STAT(statNumCheckIssuePerThread, statistics::units::Count::get(),
             "Number of times we tried to issue instructions"),
    ADD_STAT(statNumIssueNotPossiblePerThread, statistics::units::Count::get(),
             "Number of times issue not possible"),
    ADD_STAT(issueRate, statistics::units::Rate<
                statistics::units::Count, statistics::units::Cycle>::get(),
             "Inst issue rate", instsIssued / cpu->baseStats.numCycles),
    ADD_STAT(fuBusy, statistics::units::Count::get(), "FU busy when requested"),
    ADD_STAT(fuBusyRate, statistics::units::Rate<
                statistics::units::Count, statistics::units::Count>::get(),
             "FU busy rate (busy events/executed inst)"),
    ADD_STAT(statOlderROBNotIssuedPerThreadRate, statistics::units::Count::get(),
             "Number of times instruction not issued because older ROB instruction not issued, per thread rate (busy events/executed inst)"),
    ADD_STAT(statStalledOnControlInstructionPerThreadRate, statistics::units::Count::get(),
             "Number of times instruction not issued because older control instruction not completed, per thread rate (busy events/executed inst)"),
    ADD_STAT(statStalledOnMemoryReorderPerThreadRate, statistics::units::Count::get(),
             "Number of times instruction not issued because older memory instruction not completed, per thread rate (busy events/executed inst)"),
    ADD_STAT(statStalledNotOldestInIQPerThreadRate, statistics::units::Count::get(),
             "Number of times instruction not issued because older IQ instruction not issued, per thread rate (busy events/executed inst)")
{
    instsAdded
        .prereq(instsAdded);

    nonSpecInstsAdded
        .prereq(nonSpecInstsAdded);

    instsIssued
        .prereq(instsIssued);

    intInstsIssued
        .prereq(intInstsIssued);

    floatInstsIssued
        .prereq(floatInstsIssued);

    branchInstsIssued
        .prereq(branchInstsIssued);

    memInstsIssued
        .prereq(memInstsIssued);

    miscInstsIssued
        .prereq(miscInstsIssued);

    squashedInstsIssued
        .prereq(squashedInstsIssued);

    squashedInstsExamined
        .prereq(squashedInstsExamined);

    squashedOperandsExamined
        .prereq(squashedOperandsExamined);

    squashedNonSpecRemoved
        .prereq(squashedNonSpecRemoved);
    
    fuBusyS.prereq(fuBusyS);
    fuBusyW.prereq(fuBusyW);
/*
    queueResDist
        .init(Num_OpClasses, 0, 99, 2)
        .name(name() + ".IQ:residence:")
        .desc("cycles from dispatch to issue")
        .flags(total | pdf | cdf )
        ;
    for (int i = 0; i < Num_OpClasses; ++i) {
        queueResDist.subname(i, opClassStrings[i]);
    }
*/
    numIssuedDist
        .init(0,total_width,1)
        .flags(statistics::pdf)
        ;
/*
    dist_unissued
        .init(Num_OpClasses+2)
        .name(name() + ".unissued_cause")
        .desc("Reason ready instruction not issued")
        .flags(pdf | dist)
        ;
    for (int i=0; i < (Num_OpClasses + 2); ++i) {
        dist_unissued.subname(i, unissued_names[i]);
    }
*/
    statIssuedInstType
        .init(cpu->numThreads,enums::Num_OpClass)
        .flags(statistics::total | statistics::pdf | statistics::dist)
        ;
    statIssuedInstType.ysubnames(enums::OpClassStrings);

    statFuBusyPerThread.init(cpu->numThreads,enums::Num_OpClass)
        .flags(statistics::total | statistics::pdf | statistics::dist)
        ;
    statFuBusyPerThread.ysubnames(enums::OpClassStrings);

    //
    //  How long did instructions for a particular FU type wait prior to issue
    //
/*
    issueDelayDist
        .init(Num_OpClasses,0,99,2)
        .name(name() + ".")
        .desc("cycles from operands ready to issue")
        .flags(pdf | cdf)
        ;
    for (int i=0; i<Num_OpClasses; ++i) {
        std::stringstream subname;
        subname << opClassStrings[i] << "_delay";
        issueDelayDist.subname(i, subname.str());
    }
*/
    issueRate
        .flags(statistics::total)
        ;

    statFuBusy
        .init(Num_OpClasses)
        .flags(statistics::pdf | statistics::dist)
        ;
    for (int i=0; i < Num_OpClasses; ++i) {
        statFuBusy.subname(i, enums::OpClassStrings[i]);
    }
    statFuNoFree
        .init(Num_OpClasses)
        .flags(statistics::pdf | statistics::dist)
        ;
    for (int i=0; i < Num_OpClasses; ++i) {
        statFuNoFree.subname(i, enums::OpClassStrings[i]);
    }

    statFuBusyPerThreadCollective
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    fuBusy
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statOlderROBNotIssuedPerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statNumCheckIssuePerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statNumIssueNotPossiblePerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statStalledOnControlInstructionPerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statStalledOnMemoryReorderPerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    statStalledNotOldestInIQPerThread
        .init(cpu->numThreads)
        .flags(statistics::total)
        ;

    fuBusyRate
        .flags(statistics::total)
        ;
    fuBusyRate = fuBusy / instsIssued;
    statOlderROBNotIssuedPerThreadRate = statOlderROBNotIssuedPerThread / instsIssued;
    statStalledOnControlInstructionPerThreadRate = statStalledOnControlInstructionPerThread / instsIssued;
    statStalledOnMemoryReorderPerThreadRate = statStalledOnMemoryReorderPerThread / instsIssued;
    statStalledNotOldestInIQPerThreadRate = statStalledNotOldestInIQPerThread / instsIssued;
}

InstructionQueue::IQIOStats::IQIOStats(statistics::Group *parent)
    : statistics::Group(parent),
    ADD_STAT(intInstQueueReads, statistics::units::Count::get(),
             "Number of integer instruction queue reads"),
    ADD_STAT(intInstQueueWrites, statistics::units::Count::get(),
             "Number of integer instruction queue writes"),
    ADD_STAT(intInstQueueWakeupAccesses, statistics::units::Count::get(),
             "Number of integer instruction queue wakeup accesses"),
    ADD_STAT(fpInstQueueReads, statistics::units::Count::get(),
             "Number of floating instruction queue reads"),
    ADD_STAT(fpInstQueueWrites, statistics::units::Count::get(),
             "Number of floating instruction queue writes"),
    ADD_STAT(fpInstQueueWakeupAccesses, statistics::units::Count::get(),
             "Number of floating instruction queue wakeup accesses"),
    ADD_STAT(vecInstQueueReads, statistics::units::Count::get(),
             "Number of vector instruction queue reads"),
    ADD_STAT(vecInstQueueWrites, statistics::units::Count::get(),
             "Number of vector instruction queue writes"),
    ADD_STAT(vecInstQueueWakeupAccesses, statistics::units::Count::get(),
             "Number of vector instruction queue wakeup accesses"),
    ADD_STAT(intAluAccesses, statistics::units::Count::get(),
             "Number of integer alu accesses"),
    ADD_STAT(fpAluAccesses, statistics::units::Count::get(),
             "Number of floating point alu accesses"),
    ADD_STAT(vecAluAccesses, statistics::units::Count::get(),
             "Number of vector alu accesses")
{
    using namespace statistics;
    intInstQueueReads
        .flags(total);

    intInstQueueWrites
        .flags(total);

    intInstQueueWakeupAccesses
        .flags(total);

    fpInstQueueReads
        .flags(total);

    fpInstQueueWrites
        .flags(total);

    fpInstQueueWakeupAccesses
        .flags(total);

    vecInstQueueReads
        .flags(total);

    vecInstQueueWrites
        .flags(total);

    vecInstQueueWakeupAccesses
        .flags(total);

    intAluAccesses
        .flags(total);

    fpAluAccesses
        .flags(total);

    vecAluAccesses
        .flags(total);
}

void
InstructionQueue::resetState()
{
    //Initialize thread IQ counts
    for (ThreadID tid = 0; tid < MaxThreads; tid++) {
        count[tid] = 0;
        instList[tid].clear();
    }

    // Initialize the number of free IQ entries.
    freeEntries = numEntries;

    // Note that in actuality, the registers corresponding to the logical
    // registers start off as ready.  However this doesn't matter for the
    // IQ as the instruction should have been correctly told if those
    // registers are ready in rename.  Thus it can all be initialized as
    // unready.
    for (int i = 0; i < numPhysRegs; ++i) {
        regScoreboard[i] = false;
    }

    for (ThreadID tid = 0; tid < MaxThreads; ++tid) {
        squashedSeqNum[tid] = 0;
    }

    for (int i = 0; i < Num_OpClasses; ++i) {
        while (!readyInsts[i].empty())
            readyInsts[i].pop();
        queueOnList[i] = false;
        readyIt[i] = listOrder.end();
    }
    nonSpecInsts.clear();
    listOrder.clear();
    deferredMemInsts.clear();
    blockedMemInsts.clear();
    retryMemInsts.clear();
    wbOutstanding = 0;
}

void
InstructionQueue::setActiveThreads(list<ThreadID> *at_ptr)
{
    activeThreads = at_ptr;
}

void
InstructionQueue::setIssueToExecuteQueue(TimeBuffer<IssueStruct> *i2e_ptr)
{
      issueToExecuteQueue = i2e_ptr;
}

void
InstructionQueue::setTimeBuffer(TimeBuffer<TimeStruct> *tb_ptr)
{
    timeBuffer = tb_ptr;

    fromCommit = timeBuffer->getWire(-commitToIEWDelay);
}

bool
InstructionQueue::isDrained() const
{
    bool drained = dependGraph.empty() &&
                   instsToExecute.empty() &&
                   wbOutstanding == 0;
    for (ThreadID tid = 0; tid < numThreads; ++tid)
        drained = drained && memDepUnit[tid].isDrained();

    return drained;
}

void
InstructionQueue::drainSanityCheck() const
{
    assert(dependGraph.empty());
    assert(instsToExecute.empty());
    for (ThreadID tid = 0; tid < numThreads; ++tid)
        memDepUnit[tid].drainSanityCheck();
}

void
InstructionQueue::takeOverFrom()
{
    resetState();
}

int
InstructionQueue::entryAmount(ThreadID num_threads)
{
    if (iqPolicy == SMTQueuePolicy::Partitioned) {
        return numEntries / num_threads;
    } else {
        return 0;
    }
}


void
InstructionQueue::resetEntries()
{
    if (iqPolicy != SMTQueuePolicy::Dynamic || numThreads > 1) {
        int active_threads = activeThreads->size();

        list<ThreadID>::iterator threads = activeThreads->begin();
        list<ThreadID>::iterator end = activeThreads->end();

        while (threads != end) {
            ThreadID tid = *threads++;

            if (iqPolicy == SMTQueuePolicy::Partitioned) {
                maxEntries[tid] = numEntries / active_threads;
            } else if (iqPolicy == SMTQueuePolicy::Threshold &&
                       active_threads == 1) {
                maxEntries[tid] = numEntries;
            }
        }
    }
}

unsigned
InstructionQueue::numFreeEntries()
{
    return freeEntries;
}

unsigned
InstructionQueue::numFreeEntries(ThreadID tid)
{
    return maxEntries[tid] - count[tid];
}

// Might want to do something more complex if it knows how many instructions
// will be issued this cycle.
bool
InstructionQueue::isFull()
{
    if (freeEntries == 0) {
        return(true);
    } else {
        return(false);
    }
}

bool
InstructionQueue::isFull(ThreadID tid)
{
    if (numFreeEntries(tid) == 0) {
        return(true);
    } else {
        return(false);
    }
}

bool
InstructionQueue::hasReadyInsts()
{
    if (!listOrder.empty()) {
        return true;
    }

    for (int i = 0; i < Num_OpClasses; ++i) {
        if (!readyInsts[i].empty()) {
            return true;
        }
    }

    return false;
}

void
InstructionQueue::insert(const DynInstPtr &new_inst)
{
    if (new_inst->isFloating()) {
        iqIOStats.fpInstQueueWrites++;
    } else if (new_inst->isVector()) {
        iqIOStats.vecInstQueueWrites++;
    } else {
        iqIOStats.intInstQueueWrites++;
    }

    // Make sure the instruction is valid
    assert(new_inst);

    DPRINTF(IQ, "[tid:%d] Adding instruction [sn:%llu] PC %s to the IQ QueueSize %d.\n",
            new_inst->threadNumber, new_inst->seqNum, new_inst->pcState(),count[new_inst->threadNumber]);

    assert(freeEntries != 0);

    instList[new_inst->threadNumber].push_back(new_inst);

    --freeEntries;

    new_inst->setInIQ();

    // print the data for this
    if (new_inst->traceData) {
        DPRINTF(IQ,"[tid:%d] dumping inst data [sn:%llu] PC %s "
            "to the IQ.\n",
            new_inst->threadNumber,new_inst->seqNum, new_inst->pcState());
        new_inst->traceData->dump();
    }

    // Look through its source registers (physical regs), and mark any
    // dependencies.
    addToDependents(new_inst);

    // Have this instruction set itself as the producer of its destination
    // register(s).
    addToProducers(new_inst);

    if (new_inst->isMemRef()) {
        memDepUnit[new_inst->threadNumber].insert(new_inst);
    } else {
        DPRINTF(IQ, "[tid:%d] addIfReady1 Adding instruction [sn:%llu] PC %s to the IQ.\n",
            new_inst->threadNumber, new_inst->seqNum, new_inst->pcState());
        addIfReady(new_inst);
    }

    ++iqStats.instsAdded;

    count[new_inst->threadNumber]++;

    assert(freeEntries == (numEntries - countInsts()));
}

void
InstructionQueue::insertNonSpec(const DynInstPtr &new_inst)
{
    // if((cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Weak && (!new_inst->isReadBarrier() && !new_inst->isWriteBarrier()))) {
    //     panic("Issuing W thread inst as non-speculative sn:%d tid:%d\n",new_inst->seqNum,new_inst->threadNumber);
    // }
    // @todo: Clean up this code; can do it by setting inst as unable
    // to issue, then calling normal insert on the inst.
    if (new_inst->isFloating()) {
        iqIOStats.fpInstQueueWrites++;
    } else if (new_inst->isVector()) {
        iqIOStats.vecInstQueueWrites++;
    } else {
        iqIOStats.intInstQueueWrites++;
    }

    assert(new_inst);

    nonSpecInsts[new_inst->seqNum] = new_inst;

    new_inst->staticInst->setNonSpeculative();

    DPRINTF(IQ, "[tid:%d] Adding non-speculative instruction [sn:%llu] PC %s "
            "to the IQ QueueSize %d.\n",
            new_inst->threadNumber,new_inst->seqNum, new_inst->pcState(),count[new_inst->threadNumber]);

    assert(freeEntries != 0);

    instList[new_inst->threadNumber].push_back(new_inst);

    --freeEntries;

    new_inst->setInIQ();

    // Have this instruction set itself as the producer of its destination
    // register(s).
    addToProducers(new_inst);

    // If it's a memory instruction, add it to the memory dependency
    // unit.
    if (new_inst->isMemRef()) {
        memDepUnit[new_inst->threadNumber].insertNonSpec(new_inst);
    }

    ++iqStats.nonSpecInstsAdded;

    count[new_inst->threadNumber]++;

    assert(freeEntries == (numEntries - countInsts()));
}

void
InstructionQueue::insertBarrier(const DynInstPtr &barr_inst)
{
    memDepUnit[barr_inst->threadNumber].insertBarrier(barr_inst);

    insertNonSpec(barr_inst);
}

DynInstPtr
InstructionQueue::getInstToExecute()
{
    assert(!instsToExecute.empty());

    // sort list based on seq numbers
    // instsToExecute.sort([this](const DynInstPtr& inst1, const DynInstPtr& inst2) {
    // return compareBySeqNum(inst1, inst2);});
    //.sort(compareBySeqNum);

    instsToExecute.sort([this](const DynInstPtr& inst1, const DynInstPtr& inst2) {
    return compareBySeqNum(inst1, inst2);
    });

    DynInstPtr inst = std::move(instsToExecute.front());
    instsToExecute.pop_front();
    if (inst->isFloating()) {
        iqIOStats.fpInstQueueReads++;
    } else if (inst->isVector()) {
        iqIOStats.vecInstQueueReads++;
    } else {
        iqIOStats.intInstQueueReads++;
    }
    return inst;
}

void
InstructionQueue::addToOrderList(OpClass op_class)
{
    assert(!readyInsts[op_class].empty());

    ListOrderEntry queue_entry;

    queue_entry.queueType = op_class;

    queue_entry.oldestInst = readyInsts[op_class].top()->seqNum;

    ListOrderIt list_it = listOrder.begin();
    ListOrderIt list_end_it = listOrder.end();

    while (list_it != list_end_it) {
        if ((*list_it).oldestInst > queue_entry.oldestInst) {
            break;
        }

        list_it++;
    }

    readyIt[op_class] = listOrder.insert(list_it, queue_entry);
    queueOnList[op_class] = true;
}

void
InstructionQueue::moveToYoungerInst(ListOrderIt list_order_it)
{
    // Get iterator of next item on the list
    // Delete the original iterator
    // Determine if the next item is either the end of the list or younger
    // than the new instruction.  If so, then add in a new iterator right here.
    // If not, then move along.
    ListOrderEntry queue_entry;
    OpClass op_class = (*list_order_it).queueType;
    ListOrderIt next_it = list_order_it;

    ++next_it;

    queue_entry.queueType = op_class;
    queue_entry.oldestInst = readyInsts[op_class].top()->seqNum;

    while (next_it != listOrder.end() &&
           (*next_it).oldestInst < queue_entry.oldestInst) {
        ++next_it;
    }

    readyIt[op_class] = listOrder.insert(next_it, queue_entry);
}

void
InstructionQueue::processFUCompletion(const DynInstPtr &inst, int fu_idx)
{
    DPRINTF(IQ, "[tid:%d] Processing FU completion [sn:%llu]\n", inst->threadNumber, inst->seqNum);
    assert(!cpu->switchedOut());
    // The CPU could have been sleeping until this op completed (*extremely*
    // long latency op).  Wake it if it was.  This may be overkill.
   --wbOutstanding;
    iewStage->wakeCPU();

    if (fu_idx > -1)
        fuPool->freeUnitNextCycle(fu_idx);

    // @todo: Ensure that these FU Completions happen at the beginning
    // of a cycle, otherwise they could add too many instructions to
    // the queue.
    issueToExecuteQueue->access(-1)->size++;
    instsToExecute.push_back(inst);
}

// @todo: Figure out a better way to remove the squashed items from the
// lists.  Checking the top item of each list to see if it's squashed
// wastes time and forces jumps.
void
InstructionQueue::scheduleReadyInsts()
{
    DPRINTF(IQ, "Attempting to schedule ready instructions from "
            "the IQ, IQ size %d.\n",listOrder.size());

    IssueStruct *i2e_info = issueToExecuteQueue->access(0);

    DynInstPtr mem_inst;
    while ((mem_inst = getDeferredMemInstToExecute())) {
        addReadyMemInst(mem_inst);
    }

    // See if any cache blocked instructions are able to be executed
    while ((mem_inst = getBlockedMemInstToExecute())) {
        addReadyMemInst(mem_inst);
    }

    // Have iterator to head of the list
    // While I haven't exceeded bandwidth or reached the end of the list,
    // Try to get a FU that can do what this op needs.
    // If successful, change the oldestInst to the new top of the list, put
    // the queue in the proper place in the list.
    // Increment the iterator.
    // This will avoid trying to schedule a certain op class if there are no
    // FUs that handle it.
    int total_issued = 0;
    ListOrderIt order_it = listOrder.begin();
    ListOrderIt order_end_it = listOrder.end();
    int listSize = listOrder.size();
    int counter = 0;

    // Ishita: No instruction from a thread should issue after the memory instrution is issued
    // because memory instructions are re-issued some times.
    // if a memory instruction is re-issued we need to set a flag to stop issue for the thread. Flag is set after memory instruction
    // is marked to execute.
    // Need to really think about this for in-order.
    // Merge issue and execute?
    // Need to think hard. Do tomorrow.

    while (total_issued < totalWidth && order_it != order_end_it) {
        OpClass op_class = (*order_it).queueType;

        DPRINTF(IQ, "Looking at class type %d\n",op_class);

        counter++;

        assert(!readyInsts[op_class].empty());

        DynInstPtr issuing_inst = readyInsts[op_class].top();

        if (issuing_inst->isFloating()) {
            iqIOStats.fpInstQueueReads++;
        } else if (issuing_inst->isVector()) {
            iqIOStats.vecInstQueueReads++;
        } else {
            iqIOStats.intInstQueueReads++;
        }

        
        if(!issuing_inst->isInROB())
        {
            DPRINTF(IQ, "[tid:%d] : instruction is not in ROB PC %s prior issued %d "
                    "[sn:%llu]\n",
                    issuing_inst->threadNumber, issuing_inst->pcState(),cpu->rob.AreOlderInstIssued(issuing_inst->threadNumber,issuing_inst->seqNum),
                    issuing_inst->seqNum);
        } else {
            DPRINTF(IQ, "[tid:%d] :  instruction is in ROB PC %s prior issued %d "
                    "[sn:%llu]\n",
                    issuing_inst->threadNumber, issuing_inst->pcState(),cpu->rob.AreOlderInstIssued(issuing_inst->threadNumber,issuing_inst->seqNum),
                    issuing_inst->seqNum);
        }

        assert(issuing_inst->seqNum == (*order_it).oldestInst);

        if (issuing_inst->isSquashed()) {
            readyInsts[op_class].pop();

            if (!readyInsts[op_class].empty()) {
                moveToYoungerInst(order_it);
            } else {
                readyIt[op_class] = listOrder.end();
                queueOnList[op_class] = false;
            }

            listOrder.erase(order_it++);

            ++iqStats.squashedInstsIssued;

            continue;
        }

        int idx = FUPool::NoCapableFU;
        Cycles op_latency = Cycles(1);
        ThreadID tid = issuing_inst->threadNumber;
        
        if (op_class != No_OpClass) {
            idx = fuPool->getUnit(op_class);
            if (issuing_inst->isFloating()) {
                iqIOStats.fpAluAccesses++;
            } else if (issuing_inst->isVector()) {
                iqIOStats.vecAluAccesses++;
            } else {
                iqIOStats.intAluAccesses++;
            }
            if (idx > FUPool::NoFreeFU) {
                op_latency = fuPool->getOpLatency(op_class);
            }
        }

        iqStats.statNumCheckIssuePerThread[tid]++;
        // If we have an instruction that doesn't require a FU, or a
        // valid FU, then schedule for execution.
        if (idx != FUPool::NoFreeFU
        // condition for in-order execution and dont issue instructions if you are waiting on a control instruction to finish
        // W threads: for memory instructions we need to stop the issue of instructions for a thread for which a memory instruction has not been marked as not faulting. If a memory instruction is marked as "needs to be reissued" in execute, we cannot move the pipeline forward. We use a flag like we do in control instructions for this. At a given time only 1 memory instruction can be serviced. We dont want any 
        //&& ((cpu->rob.AreOlderInstIssued(issuing_inst->threadNumber,issuing_inst->seqNum) && !cpu->thread[tid]->ControlInstIssued && !(cpu->thread[tid]->MemInstIssued && cpu->thread[tid]->MemInstIssued!= issuing_inst->seqNum) &&  !olderIssuePending(issuing_inst->seqNum, issuing_inst->threadNumber)) || cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        && ((!cpu->thread[tid]->ControlInstIssued && !(cpu->thread[tid]->MemInstIssued && cpu->thread[tid]->MemInstIssued!= issuing_inst->seqNum) &&  !olderIssuePending(issuing_inst->seqNum, issuing_inst->threadNumber)) || cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        )
        {
            // ensure that there is no OoO issue going on. No older seq number should have been marked as issued for this tid.
            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && youngerInstIssued(issuing_inst->seqNum, issuing_inst->threadNumber)) {
                panic("[tid:%d] Instruction [sn:%llu] is being issued OoO for W thread!\n",tid,issuing_inst->seqNum);
            }

            int8_t total_src_regs1 = issuing_inst->numSrcRegs();
            for (int src_reg_idx = 0;
                            src_reg_idx < total_src_regs1;
                            src_reg_idx++) {
                const PhysRegIdPtr reg = issuing_inst->renamedSrcIdx(src_reg_idx);
                RegVal regval = 0;
                DPRINTF(IQ,"[tid:%d] REG_ISSUE1_OUTVALS for SRC PC %s [sn:%llu] total_src_regs %d reg %d : ",issuing_inst->threadNumber, issuing_inst->pcState(), issuing_inst->seqNum,total_src_regs1,reg->flatIndex());
                if (!reg->is(InvalidRegClass) && !reg->is(MiscRegClass) && !reg->is(VecRegClass) && !reg->is(VecPredRegClass))
                    regval = cpu->getReg(reg);
                DPRINTF(IQ,"\n");
            }
            int8_t total_dest_regs1 = issuing_inst->numDestRegs();
            for (int dest_reg_idx = 0;
                dest_reg_idx < total_dest_regs1;
                dest_reg_idx++)
            {
                const PhysRegIdPtr reg = issuing_inst->renamedDestIdx(dest_reg_idx);
                RegVal regval = 0;
                DPRINTF(IQ,"[tid:%d] REG_ISSUE1_OUTVALS for DEST PC %s [sn:%llu] total_dest_regs %d reg %d : ",issuing_inst->threadNumber,issuing_inst->pcState(),issuing_inst->seqNum,total_dest_regs1,reg->flatIndex());
                if (!reg->is(InvalidRegClass) && !reg->is(MiscRegClass) && !reg->is(VecRegClass) && !reg->is(VecPredRegClass))
                    regval = cpu->getReg(reg);
                DPRINTF(IQ,"\n");
            }

            // if instruction is a control instruction -> mark thread as issuing a control instruction 
            if(issuing_inst->isControl() && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
                assert(!cpu->thread[tid]->ControlInstIssued);
                assert(cpu->thread[tid]->ControlInstSeq == -1);
                cpu->thread[tid]->ControlInstIssued = true;
                cpu->thread[tid]->ControlInstSeq = issuing_inst->seqNum;
                DPRINTF(IQ, "[tid:%i]: SETTING_ControlInstIssued %s "
                    "[sn:%llu] ControlInstIssued %d\n",
                    tid, issuing_inst->pcState(),
                    issuing_inst->seqNum,cpu->thread[tid]->ControlInstIssued);
            }

            if((issuing_inst->isLoad() || issuing_inst->isStore() || issuing_inst->isAtomic()) && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && (cpu->thread[tid]->MemInstSeq != issuing_inst->seqNum)) {
                assert(!cpu->thread[tid]->MemInstIssued);
                assert(cpu->thread[tid]->MemInstSeq == -1);
                cpu->thread[tid]->MemInstIssued = true;
                cpu->thread[tid]->MemInstSeq = issuing_inst->seqNum;
                DPRINTF(IQ, "[tid:%i]: SETTING_MemInstIssued %s "
                    "[sn:%llu] MemInstIssued %d\n",
                    tid, issuing_inst->pcState(),
                    issuing_inst->seqNum,cpu->thread[tid]->MemInstIssued);
            }

            fuPool->markUnitBusy(idx,op_class);


            if (op_latency == Cycles(1)) {
                i2e_info->size++;
                instsToExecute.push_back(issuing_inst);

                // Add the FU onto the list of FU's to be freed next
                // cycle if we used one.
                if (idx >= 0)
                    fuPool->freeUnitNextCycle(idx);
            } else {
                bool pipelined = fuPool->isPipelined(op_class);
                // Generate completion event for the FU
                ++wbOutstanding;
                FUCompletion *execution = new FUCompletion(issuing_inst,
                                                           idx, this);

                cpu->schedule(execution,
                              cpu->clockEdge(Cycles(op_latency - 1)));

                if (!pipelined) {
                    // If FU isn't pipelined, then it must be freed
                    // upon the execution completing.
                    execution->setFreeFU();
                } else {
                    // Add the FU onto the list of FU's to be freed next cycle.

                    DPRINTF(IQ, "[tid:%i] : Free in pipelined 1 cycle %s "
                    "[sn:%llu] opclass %d\n",
                    tid, issuing_inst->pcState(),
                    issuing_inst->seqNum,op_class);
                    fuPool->freeUnitNextCycle(idx);
                }
            }

            DPRINTF(IQ, "[tid:%i] : Issuing instruction PC %s "
                    "[sn:%llu] isControl() %d isDirectCtrl() %d isIndirectCtrl() %d isCondCtrl() %d isUncondCtrl() %d\n",
                    tid, issuing_inst->pcState(),
                    issuing_inst->seqNum,issuing_inst->isControl(),issuing_inst->isDirectCtrl(),issuing_inst->isIndirectCtrl(),issuing_inst->isCondCtrl(),issuing_inst->isUncondCtrl());

            readyInsts[op_class].pop();

            if (!readyInsts[op_class].empty()) {
                moveToYoungerInst(order_it);
            } else {
                readyIt[op_class] = listOrder.end();
                queueOnList[op_class] = false;
            }

            issuing_inst->setIssued();
            ++total_issued;

#if TRACING_ON
            issuing_inst->issueTick = curTick() - issuing_inst->fetchTick;
#endif

            if (issuing_inst->firstIssue == -1)
                issuing_inst->firstIssue = curTick();

            if (!issuing_inst->isMemRef()) {
                // Memory instructions can not be freed from the IQ until they
                // complete.
                ++freeEntries;
                count[tid]--;

                DPRINTF(IQ, "[tid:%d] Removing non Mem instruction instruction [sn:%llu] PC %s "
            "to the IQ QueueSize %d isNop %d.\n",
            issuing_inst->threadNumber,issuing_inst->seqNum, issuing_inst->pcState(),count[issuing_inst->threadNumber],issuing_inst->isNop());

                issuing_inst->clearInIQ();
            } else {
                memDepUnit[tid].issue(issuing_inst);
            }

            listOrder.erase(order_it++);
            iqStats.statIssuedInstType[tid][op_class]++;
        } else {
            iqStats.statFuBusy[op_class]++;
            iqStats.fuBusy[tid]++;
            iqStats.statNumIssueNotPossiblePerThread[tid]++;

            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() != Strong) {
                iqStats.fuBusyS++;
            } else {
                iqStats.fuBusyW++;
            }

            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() != Strong) {
                if(cpu->thread[tid]->ControlInstIssued)
                    iqStats.statStalledOnControlInstructionPerThread[tid]++;
                else if(cpu->thread[tid]->MemInstIssued && cpu->thread[tid]->MemInstIssued!= issuing_inst->seqNum)
                    iqStats.statStalledOnMemoryReorderPerThread[tid]++;
                else if(olderIssuePending(issuing_inst->seqNum, issuing_inst->threadNumber))
                    iqStats.statStalledNotOldestInIQPerThread[tid]++;
                else if(idx != FUPool::NoFreeFU) {
                    iqStats.statFuBusyPerThread[tid][op_class]++;
                    iqStats.statFuBusyPerThreadCollective[tid]++;
                }
            }
            else {
                iqStats.statFuBusyPerThread[tid][op_class]++;
                iqStats.statFuBusyPerThreadCollective[tid]++;
            }
            ++order_it;

            DPRINTF(IQ, "[tid:%d] : could not instruction PC %s "
                    "[sn:%llu] isControl() %d isDirectCtrl() %d isIndirectCtrl() %d isCondCtrl() %d isUncondCtrl() %d AreOlderInstIssued %d ControlInstIssued %d MemInstIssued %d olderIssuePending %d pendingSn %d\n",
                    tid, issuing_inst->pcState(),
                    issuing_inst->seqNum,issuing_inst->isControl(),issuing_inst->isDirectCtrl(),issuing_inst->isIndirectCtrl(),issuing_inst->isCondCtrl(),issuing_inst->isUncondCtrl(),cpu->rob.AreOlderInstIssued(issuing_inst->threadNumber,issuing_inst->seqNum),!cpu->thread[tid]->ControlInstIssued,!cpu->thread[tid]->MemInstIssued,!olderIssuePending(issuing_inst->seqNum, issuing_inst->threadNumber),SeqNumolderIssuePending(issuing_inst->seqNum, issuing_inst->threadNumber));
        }
    }

    iqStats.numIssuedDist.sample(total_issued);
    iqStats.instsIssued+= total_issued;


    // If we issued any instructions, tell the CPU we had activity.
    // @todo If the way deferred memory instructions are handeled due to
    // translation changes then the deferredMemInsts condition should be
    // removed from the code below.
    if (total_issued || !retryMemInsts.empty() || !deferredMemInsts.empty()) {
        cpu->activityThisCycle();
    } else {
        DPRINTF(IQ, "Not able to schedule any instructions.\n");
    }

    // check if op is free -> Backend utilization check
    for(int i = 0; i < OpClass::Num_OpClass; i++) {
        int idx = FUPool::NoCapableFU;
        OpClass op_class1 = OpClass(i);
        idx = fuPool->getUnit(op_class1);
        if(idx == FUPool::NoFreeFU) {
            iqStats.statFuNoFree[op_class1]++;
        }
    }
}

void
InstructionQueue::scheduleNonSpec(const InstSeqNum &inst)
{
    DPRINTF(IQ, "Marking nonspeculative instruction [sn:%llu] as ready "
            "to execute.\n", inst);

    NonSpecMapIt inst_it = nonSpecInsts.find(inst);

    assert(inst_it != nonSpecInsts.end());

    ThreadID tid = (*inst_it).second->threadNumber;

    (*inst_it).second->setAtCommit();

    (*inst_it).second->setCanIssue();

    if (!(*inst_it).second->isMemRef()) {
        DPRINTF(IQ, "[tid:%d] addIfReady2 Adding instruction [sn:%llu] PC %s to the IQ.\n",
            tid,(*inst_it).second->seqNum, (*inst_it).second->pcState());
        addIfReady((*inst_it).second);
    } else {
        memDepUnit[tid].nonSpecInstReady((*inst_it).second);
    }

    (*inst_it).second = NULL;

    nonSpecInsts.erase(inst_it);
}

void
InstructionQueue::commit(const InstSeqNum &inst, ThreadID tid)
{
    DPRINTF(IQ, "[tid:%i] Committing instructions older than [sn:%llu]\n",
            tid,inst);

    ListIt iq_it = instList[tid].begin();

    while (iq_it != instList[tid].end() &&
           (*iq_it)->seqNum <= inst) {
        ++iq_it;
        instList[tid].pop_front();
    }

    assert(freeEntries == (numEntries - countInsts()));
}

bool InstructionQueue::olderIssuePending(const InstSeqNum &inst, ThreadID tid) {
    ListIt iq_it = instList[tid].begin();

    bool olderIssuePending = false;

    while (iq_it != instList[tid].end() && 
           (*iq_it)->seqNum < inst) {
        if(!(*iq_it)->isIssued()) {
            olderIssuePending = true;
            break;
        }
        ++iq_it;
    }

    return olderIssuePending;
}

int InstructionQueue::SeqNumolderIssuePending(const InstSeqNum &inst, ThreadID tid) {
    ListIt iq_it = instList[tid].begin();

    int olderIssuePending = -1;

    while (iq_it != instList[tid].end() && 
           (*iq_it)->seqNum < inst) {
        if(!(*iq_it)->isIssued()) {
            olderIssuePending = (*iq_it)->seqNum;
            return olderIssuePending;
        }
        ++iq_it;
    }

    return olderIssuePending;
}

bool InstructionQueue::youngerInstIssued(const InstSeqNum &inst, ThreadID tid) {
    ListIt iq_it = instList[tid].begin();

    bool youngerInstIssued = false;

    while (iq_it != instList[tid].end() && 
           (*iq_it)->seqNum > inst) {
        if((*iq_it)->isIssued()) {
            youngerInstIssued = true;
            break;
        }
        ++iq_it;
    }

    return youngerInstIssued;
}


int
InstructionQueue::wakeDependents(const DynInstPtr &completed_inst)
{

    int dependents = 0;
    ThreadID tid = completed_inst->threadNumber;

    // wake dependents if they have not been woken already during squash
    if(!(completed_inst->HasWokenDependents() && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak)) {
        int8_t total_src_regs1 = completed_inst->numSrcRegs();
        for (int src_reg_idx = 0;
                        src_reg_idx < total_src_regs1;
                        src_reg_idx++) {
            const PhysRegIdPtr reg = completed_inst->renamedSrcIdx(src_reg_idx);
            RegVal regval = 0;
            DPRINTF(IQ,"[tid:%d] REGOUTVALS for SRC PC %s [sn:%llu] total_src_regs %d reg %d : ",tid,completed_inst->pcState(),completed_inst->seqNum,total_src_regs1,reg->flatIndex());
            if (!reg->is(InvalidRegClass) && !reg->is(MiscRegClass) && !reg->is(VecRegClass) && !reg->is(VecPredRegClass))
                regval = cpu->getReg(reg);
            DPRINTF(IQ,"\n");
        }
        int8_t total_dest_regs1 = completed_inst->numDestRegs();
        for (int dest_reg_idx = 0;
            dest_reg_idx < total_dest_regs1;
            dest_reg_idx++)
        {
            const PhysRegIdPtr reg = completed_inst->renamedDestIdx(dest_reg_idx);
            RegVal regval = 0;
            DPRINTF(IQ,"[tid:%d] REGOUTVALS for DEST PC %s [sn:%llu] total_dest_regs %d reg %d : ",tid,completed_inst->pcState(),completed_inst->seqNum,total_dest_regs1,reg->flatIndex());
            if (!reg->is(InvalidRegClass) && !reg->is(MiscRegClass) && !reg->is(VecRegClass) && !reg->is(VecPredRegClass))
                regval = cpu->getReg(reg);
            DPRINTF(IQ,"\n");
        }

        // The instruction queue here takes care of both floating and int ops
        if (completed_inst->isFloating()) {
            iqIOStats.fpInstQueueWakeupAccesses++;
        } else if (completed_inst->isVector()) {
            iqIOStats.vecInstQueueWakeupAccesses++;
        } else {
            iqIOStats.intInstQueueWakeupAccesses++;
        }

        completed_inst->lastWakeDependents = curTick();

        DPRINTF(IQ, "[tid:%d] Waking dependents of completed instruction numDests %d PC: %s [sn:%llu].\n",tid,completed_inst->numDestRegs(),completed_inst->pcState(),completed_inst->seqNum);

        assert(!completed_inst->isSquashed());

        // Tell the memory dependence unit to wake any dependents on this
        // instruction if it is a memory instruction.  Also complete the memory
        // instruction at this point since we know it executed without issues.
        if (completed_inst->isMemRef()) {
            memDepUnit[tid].completeInst(completed_inst);

            DPRINTF(IQ, "[tid:%d] Completing mem instruction PC: %s [sn:%llu]\n",
                tid,completed_inst->pcState(), completed_inst->seqNum);

            ++freeEntries;
            completed_inst->memOpDone(true);

            DPRINTF(IQ, "[tid:%d] Removing MemComplete instruction instruction [sn:%llu] PC %s "
                "to the IQ QueueSize %d.\n",
                completed_inst->threadNumber,completed_inst->seqNum, completed_inst->pcState(),count[completed_inst->threadNumber]);

            count[tid]--;
        } else if (completed_inst->isReadBarrier() ||
                completed_inst->isWriteBarrier()) {
            // Completes a non mem ref barrier
            memDepUnit[tid].completeInst(completed_inst);
        }

        std::unordered_set<int>dest_regs_set;

        int8_t total_dest_regs = completed_inst->numDestRegs();

        DPRINTF(IQ, "[tid:%d] INSIDE111 PLACE Waking dependents of completed instruction numDests %d.\n",tid,total_dest_regs);

        for (int dest_reg_idx = 0;
            dest_reg_idx < total_dest_regs;
            dest_reg_idx++)
        {
            PhysRegIdPtr dest_reg =
                completed_inst->renamedDestIdx(dest_reg_idx);

            DPRINTF(IQ, "[tid:%d] PLACEA11 Waking any dependents on register %i (%s) tidType %d RAW_DEP_WAKE [sn:%llu] pinned %d numPinnedComplete %d.\n",
                    tid,dest_reg->index(),
                    dest_reg->className(),cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType(),completed_inst->seqNum,dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete());

            // Special case of uniq or control registers.  They are not
            // handled by the IQ and thus have no dependency graph entry.
            if (dest_reg->isFixedMapping()) {
                DPRINTF(IQ, "[tid:%d] Reg %d [%s] is part of a fix mapping, skipping\n",
                        tid,dest_reg->index(), dest_reg->className());
                continue;
            }

            // Avoid waking up dependents if the register is pinned
            dest_reg->decrNumPinnedWritesToComplete();
            DPRINTF(IQ, "[tid:%d] PLACEA22 Waking any dependents on register %i (%s) tidType %d RAW_POST_DEP_WAKE [sn:%llu] pinned %d numPinnedComplete %d.\n",
                    tid,dest_reg->index(),
                    dest_reg->className(),cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType(),completed_inst->seqNum,dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete());

            if (dest_reg->isPinned())
                completed_inst->setPinnedRegsWritten();

            DPRINTF(IQ, "[tid:%d] PLACEA22 Waking any dependents on register %i (%s) tidType %d RAW_POST1_DEP_WAKE [sn:%llu] pinned %d numPinnedComplete %d.\n",
                    tid,dest_reg->index(),
                    dest_reg->className(),cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType(),completed_inst->seqNum,dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete());


            if(dest_reg->getNumPinnedWritesToComplete() < 0) {
                panic("[tid:%d] Number of pinned writes %d less than 0!!\n",tid,dest_reg->getNumPinnedWritesToComplete());
            }

            if (dest_reg->getNumPinnedWritesToComplete() != 0) {
                DPRINTF(IQ, "[tid:%d] Reg %d [%s] is pinned, skipping pinned_val %d\n",
                        tid,dest_reg->index(), dest_reg->className(),dest_reg->getNumPinnedWritesToComplete());
                continue;
            }

            DPRINTF(IQ, "[tid:%d] Waking any dependents on register %i (%s).\n",
                    tid,dest_reg->index(),
                    dest_reg->className());

            // For S threads we keep the same flow as before. 
            // For W threads -> We need to check that this inst is the earliest entry in the
            // vector (sanity check). 
            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                //Go through the dependency chain, marking the registers as
                //ready within the waiting instructions.
                DynInstPtr dep_inst = dependGraph.pop(dest_reg->flatIndex()); 

                while (dep_inst) { 
                    DPRINTF(IQ, "[tid:%d] ***Waking up a dependent instruction, [sn:%llu] "
                            "PC %s.\n", tid,dep_inst->seqNum, dep_inst->pcState());

                    // Might want to give more information to the instruction
                    // so that it knows which of its source registers is
                    // ready.  However that would mean that the dependency
                    // graph entries would need to hold the src_reg_idx.

                    dep_inst->markSrcRegReady();

                    DPRINTF(IQ, "[tid:%d] addIfReady3 Adding instruction [sn:%llu] PC %s to the IQ.\n",tid, dep_inst->seqNum, dep_inst->pcState());
                    addIfReady(dep_inst);

                    dep_inst = dependGraph.pop(dest_reg->flatIndex());

                    ++dependents;
                }
                // Reset the head node now that all of its dependents have
                // been woken up.
                assert(dependGraph.empty(dest_reg->flatIndex())); 
                dependGraph.clearInst(dest_reg->flatIndex()); 
            } else {
                // first check that this inst is the head inst for the dependence chain
                // if it is not, we have done something wrong
                if(!dependGraph.isInstOldestRAW(dest_reg->flatIndex(),completed_inst)) {
                    DynInstPtr inst_check =  dependGraph.getOldestInst(dest_reg->flatIndex());
                    DPRINTF(IQ,"[tid:%d] Got inst %d\n",tid,inst_check);
                    panic("We issued an instruction which was not the oldest instruction to be issued for this dependence chain! Fix it!!\n Wanted sn:%llu got sn:%llu PC wanted %s PC got %s\n",completed_inst->seqNum,inst_check->seqNum,completed_inst->pcState(),inst_check->pcState());
                }  

                //Go through the dependency chain, marking the registers as
                //ready within the waiting instructions.
                DynInstPtr dep_inst = dependGraph.popRAW(dest_reg->flatIndex()); 

                while (dep_inst) { 
                    DPRINTF(IQ, "[tid:%d] PLACE1 RAW Waking up a dependent instruction, [sn:%llu] "
                            "PC %s RAW_WAKE %d.\n", tid, dep_inst->seqNum, dep_inst->pcState(),dest_reg->flatIndex());

                    // Might want to give more information to the instruction
                    // so that it knows which of its source registers is
                    // ready.  However that would mean that the dependency
                    // graph entries would need to hold the src_reg_idx.

                    // mark the dependence as resolved
                    //dep_inst->markSrcDepRegReady(dest_reg->flatIndex());

                    dep_inst->markSrcRegReadyWDone(dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] addIfReady4 Adding instruction [sn:%llu] PC %s to the IQ.\n", tid, dep_inst->seqNum, dep_inst->pcState());
                    addIfReady(dep_inst);

                    dep_inst = dependGraph.popRAW(dest_reg->flatIndex());

                    ++dependents;
                }
                // ensure that all dependent instructions have been woken up
                assert(dependGraph.emptyRAW(dest_reg->flatIndex()));
                dependGraph.clearInstRAW(dest_reg->flatIndex()); 
            }

            // Mark the scoreboard as having that register ready.
            regScoreboard[dest_reg->flatIndex()] = true;

            // WAW dependencies: Remove the top instruction from the various dest_regs 
            // and mark the next instruction as ready
            // WAW dependencies: wake up the next instruction with a WAW dependence on this instruction. and remove this instruction
            // from the linked list Ishita dependenceWAWGraph

            // only for W threads: 
            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak)
            {
                if (dest_regs_set.find(dest_reg_idx) == dest_regs_set.end())
                {

                    DPRINTF(IQ, "[tid:%d] WAW_INSTS_HEREAA %d, [sn:%llu] "
                                "PC %s.\n", tid,dependGraph.countNodesWAW(dest_reg->flatIndex()), completed_inst->seqNum, completed_inst->pcState());

                    // remove the instruction from the top
                    DynInstPtr popped_inst = dependGraph.popFront(dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] WAW_INSTS_HERE1 %d, [sn:%llu] "
                                "PC %s.\n", tid, dependGraph.countNodesWAW(dest_reg->flatIndex()), popped_inst->seqNum, popped_inst->pcState());

                    // mark the dest regs as ready for the next instruction present in the list
                    DynInstPtr dep_inst = dependGraph.getNextInst(dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] WAW_INSTS_HERE2 %d, [sn:%llu] "
                                "PC %s.\n", tid, dependGraph.countNodesWAW(dest_reg->flatIndex()), popped_inst->seqNum, popped_inst->pcState());

                    if(dep_inst) {

                        DPRINTF(IQ, "[tid:%d] WAW_INSTS_HERE3 %d, [sn:%llu] "
                                "PC %s.\n", tid, dependGraph.countNodesWAW(dest_reg->flatIndex()), popped_inst->seqNum, popped_inst->pcState());

                        if(popped_inst == dep_inst) {
                            DPRINTF(IQ, "[tid:%d] PLACE3 SAME INST TWICE!!! %d %d, [sn:%llu] "
                                "PC %s.\n", tid, popped_inst,dep_inst,dep_inst->seqNum, dep_inst->pcState());
                        }

                        dep_inst->markDestDepRegReady(dest_reg->flatIndex(),dep_inst->numDestRegs());

                        // mark dest regs as ready for this instruction
                        dep_inst->markDestRegReady(dest_reg->flatIndex(),dep_inst->numDestRegs());

                        DPRINTF(IQ, "[tid:%d] addIfReady5 Adding instruction [sn:%llu] PC %s to the IQ numWARPending[0] %d.\n", tid, dep_inst->seqNum, dep_inst->pcState(), dep_inst->numWARPending[0]);
                        addIfReady(dep_inst);
                    }

                    DPRINTF(IQ, "[tid:%d] WAW_INSTS_HERE4 %d, [sn:%llu] "
                                "PC %s.\n", tid, dependGraph.countNodesWAW(dest_reg->flatIndex()), popped_inst->seqNum, popped_inst->pcState());
                }  
            }
        }

        DPRINTF(IQ, "[tid:%d] INSIDE222 PLACE Waking dependents of completed instruction numDests %d.\n",tid,total_dest_regs);

        /** for W threads, we need to wake up WAR dependents on the src registers. To
         * Do this we need to go through the vector entries and free up all dependencies.
         */
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {

            int8_t total_src_regs = completed_inst->numSrcRegs();
            for (int src_reg_idx = 0;
                            src_reg_idx < total_src_regs;
                            src_reg_idx++) {

                PhysRegIdPtr src_reg = completed_inst->renamedSrcIdx(src_reg_idx);

                int locInVec = dependGraph.getInstLocInWAR(src_reg->flatIndex(), completed_inst);

                if(!src_reg->isFixedMapping()) {
                    DPRINTF(IQ, "[tid:%d] PLACE4 STARTING TO WAKE INSTS, [sn:%llu] "
                            "PC %s locInVec: %d reg_num %d insts_left %d.\n", tid, completed_inst->seqNum, completed_inst->pcState(),locInVec,src_reg->flatIndex(),dependGraph.countNodes(src_reg->flatIndex(), locInVec));
                }

                /** There should be an entry for this */
                bool locInVecFound = (locInVec != -1) ? 1 : 0;

                if(locInVecFound) {
                    DynInstPtr dep_inst = dependGraph.popWAR(src_reg->flatIndex(), locInVec); 

                    while (dep_inst) { // Ishita -> Wakes up dependents here

                        int reg_index = dependGraph.decreaseNumWARPending(src_reg->flatIndex(), dep_inst, locInVec);

                        // remove the instruction from the top

                        // Might want to give more information to the instruction
                        // so that it knows which of its source registers is
                        // ready.  However that would mean that the dependency
                        // graph entries would need to hold the src_reg_idx.

                        DPRINTF(IQ, "[tid:%d] PLACE3 WAR STARTING TO WAKE INSTS WRITE_INST, [sn:%llu] "
                                "PC %s index %d numWARPending %d.\n", tid, dep_inst->seqNum, dep_inst->pcState(),reg_index,dep_inst->numWARPending[reg_index]);

                        if(dep_inst->numWARPending[reg_index] == 0) 
                        {
                            dep_inst->markDestDepRegReady(src_reg->flatIndex(),dep_inst->numDestRegs());

                            dep_inst->markDestRegReady(src_reg->flatIndex(),dep_inst->numDestRegs());

                            DPRINTF(IQ, "[tid:%d] addIfReady6 Adding instruction [sn:%llu] PC %s to the IQ.\n", tid, dep_inst->seqNum, dep_inst->pcState());
                            addIfReady(dep_inst);

                            DPRINTF(IQ, "[tid:%d] PLACE6 WAR STARTING TO WAKE INSTS, [sn:%llu] "
                                    "PC %s insts_left %d.\n", tid, dep_inst->seqNum, dep_inst->pcState(),dependGraph.countNodes(src_reg->flatIndex(), locInVec));
                        } else {
                            DPRINTF(IQ, "[tid:%d] PLACE6 could not wake WAR, [sn:%llu] "
                                    "PC %s insts_left %d numWARPending %d idx %d index %d.\n", tid, dep_inst->seqNum, dep_inst->pcState(),dependGraph.countNodes(src_reg->flatIndex(), locInVec),dep_inst->numWARPending[reg_index],src_reg->flatIndex(),reg_index);

                            if(dep_inst->numWARPending[reg_index] < 1) {
                                panic("[tid:%d] numWARPending less than 0! sn:%d val %d reg %d",tid,dep_inst->seqNum, dep_inst->numWARPending[reg_index], src_reg->flatIndex());
                            }
                        }

                        dep_inst = dependGraph.popWAR(src_reg->flatIndex(), locInVec);

                        ++dependents;
                    }
                    // ensure that all dependent instructions have been woken up
                    assert(dependGraph.emptyWAR(src_reg->flatIndex(),locInVec));
                    dependGraph.clearInstWAR(src_reg->flatIndex(),locInVec); 
                }
            }
        }
    } else {
        DPRINTF(IQ, "[tid:%d] Already woke dependents, [sn:%llu] "
                                    "PC %s.\n", tid, completed_inst->seqNum, completed_inst->pcState());
    }
    return dependents;
}

void
InstructionQueue::addReadyMemInst(const DynInstPtr &ready_inst)
{
    OpClass op_class = ready_inst->opClass();

    readyInsts[op_class].push(ready_inst);

    // Will need to reorder the list if either a queue is not on the list,
    // or it has an older instruction than last time.
    if (!queueOnList[op_class]) {
        addToOrderList(op_class);
    } else if (readyInsts[op_class].top()->seqNum  <
               (*readyIt[op_class]).oldestInst) {
        listOrder.erase(readyIt[op_class]);
        addToOrderList(op_class);
    }

    int tid = ready_inst->threadNumber;

    // if this is a weak thread, set up the pinned threads value here.
    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
        
        int8_t total_dest_regs = ready_inst->numDestRegs();

        DPRINTF(IQ, "[tid:%d] STEP1, MEMORY putting it onto "
            "the ready list, PC %s  [sn:%llu] total_dest_regs %d DataPrefetch %d InstPrefetch %d.\n",
            tid, ready_inst->pcState(), ready_inst->seqNum,total_dest_regs, ready_inst->isDataPrefetch(), ready_inst->isInstPrefetch());

        for (int dest_reg_idx = 0;
            dest_reg_idx < total_dest_regs;
            dest_reg_idx++)
        {
            PhysRegIdPtr dest_reg = ready_inst->renamedDestIdx(dest_reg_idx);
            
            dest_reg->setNumPinnedWrites(ready_inst->getnumWrites(dest_reg_idx));
            dest_reg->setNumPinnedWritesToComplete(
                ready_inst->getNumPinnedWritesToComplete(dest_reg_idx));
            DPRINTF(IQ, "[tid:%d] LOOK_REGS_FINAL_WRITE [sn:%llu] RegNum %d %i (%s) pinned %d numPinnedComplete %d.\n",
            tid, ready_inst->seqNum,dest_reg->flatIndex(), dest_reg->index(),
            dest_reg->className(),dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete());
        }
    }

    DPRINTF(IQ, "[tid:%d] In Memory Instruction is ready to issue, putting it onto "
            "the ready list, PC %s opclass:%i [sn:%llu].\n",
            tid, ready_inst->pcState(), op_class, ready_inst->seqNum);
}

void
InstructionQueue::rescheduleMemInst(const DynInstPtr &resched_inst)
{
    DPRINTF(IQ, "[tid:%d] Rescheduling mem inst [sn:%llu]\n", resched_inst->threadNumber, resched_inst->seqNum);

    // Reset DTB translation state
    resched_inst->translationStarted(false);
    resched_inst->translationCompleted(false);

    resched_inst->clearCanIssue();
    memDepUnit[resched_inst->threadNumber].reschedule(resched_inst);
}

void
InstructionQueue::replayMemInst(const DynInstPtr &replay_inst)
{
    memDepUnit[replay_inst->threadNumber].replay();
}

void
InstructionQueue::deferMemInst(const DynInstPtr &deferred_inst)
{
    deferredMemInsts.push_back(deferred_inst);
}

void
InstructionQueue::blockMemInst(const DynInstPtr &blocked_inst)
{
    blocked_inst->clearIssued();
    blocked_inst->clearCanIssue();
    blockedMemInsts.push_back(blocked_inst);
    DPRINTF(IQ, "[tid:%d] Memory inst [sn:%llu] PC %s is blocked, will be "
            "reissued later\n", blocked_inst->threadNumber,blocked_inst->seqNum,
            blocked_inst->pcState());
}

void
InstructionQueue::cacheUnblocked()
{
    DPRINTF(IQ, "Cache is unblocked, rescheduling blocked memory "
            "instructions\n");
    retryMemInsts.splice(retryMemInsts.end(), blockedMemInsts);
    // Get the CPU ticking again
    cpu->wakeCPU();
}

DynInstPtr
InstructionQueue::getDeferredMemInstToExecute()
{
    for (ListIt it = deferredMemInsts.begin(); it != deferredMemInsts.end();
         ++it) {
        if ((*it)->translationCompleted() || (*it)->isSquashed()) {
            DynInstPtr mem_inst = std::move(*it);
            deferredMemInsts.erase(it);
            return mem_inst;
        }
    }
    return nullptr;
}

DynInstPtr
InstructionQueue::getBlockedMemInstToExecute()
{
    if (retryMemInsts.empty()) {
        return nullptr;
    } else {
        DynInstPtr mem_inst = std::move(retryMemInsts.front());
        retryMemInsts.pop_front();
        return mem_inst;
    }
}

void
InstructionQueue::violation(const DynInstPtr &store,
        const DynInstPtr &faulting_load)
{
    iqIOStats.intInstQueueWrites++;
    memDepUnit[store->threadNumber].violation(store, faulting_load);
}

bool 
InstructionQueue::compareBySeqNum(const DynInstPtr& inst1, const DynInstPtr& inst2) {
    //return inst1->seqNum < inst2->seqNum;

    int tid1 = inst1->threadNumber;
    int tid2 = inst2->threadNumber;

    if ((tid1 == tid2) && (cpu->thread[tid1]->tc->getProcessPtr()->getprocessThreadType() == Weak)) {
        return inst1->seqNum < inst2->seqNum;
    }
    
    return false;
}
void
InstructionQueue::squash(ThreadID tid)
{
    DPRINTF(IQ, "[tid:%i] Starting to squash instructions in "
            "the IQ.\n", tid);

    // Read instruction sequence number of last instruction out of the
    // time buffer.
    squashedSeqNum[tid] = fromCommit->commitInfo[tid].doneSeqNum;

    doSquash(tid);

    // Also tell the memory dependence unit to squash.
    memDepUnit[tid].squash(squashedSeqNum[tid], tid);
}

void
InstructionQueue::doSquash(ThreadID tid)
{
    // Start at the tail.
    ListIt squash_it = instList[tid].end();
    --squash_it;

    DPRINTF(IQ, "[tid:%i] Squashing until sequence number %i!\n",
            tid, squashedSeqNum[tid]);

    // Squash any instructions younger than the squashed sequence number
    // given.
    while (squash_it != instList[tid].end() &&
           (*squash_it)->seqNum > squashedSeqNum[tid]) {

        DynInstPtr squashed_inst = (*squash_it);
        if (squashed_inst->isFloating()) {
            iqIOStats.fpInstQueueWrites++;
        } else if (squashed_inst->isVector()) {
            iqIOStats.vecInstQueueWrites++;
        } else {
            iqIOStats.intInstQueueWrites++;
        }

        // Only handle the instruction if it actually is in the IQ and
        // hasn't already been squashed in the IQ.
        if (squashed_inst->threadNumber != tid ||
            squashed_inst->isSquashedInIQ()) {
            --squash_it;
            continue;
        }

        squashed_inst->squashedInQueue = true;
        // if the squashed instruction is a control instruction then mark the control stall as reset
        if(squashed_inst->isControl() && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && cpu->thread[tid]->ControlInstSeq == squashed_inst->seqNum) {
            assert(cpu->thread[tid]->ControlInstIssued);
            cpu->thread[tid]->ControlInstIssued = false;
            cpu->thread[tid]->ControlInstSeq = -1;
            DPRINTF(IQ, "[tid:%i]: RELIEVING4_ControlInstIssued %s "
                "[sn:%llu] ControlInstIssued %d\n",
                tid, squashed_inst->pcState(),
                squashed_inst->seqNum,cpu->thread[tid]->ControlInstIssued);
        }

        if((squashed_inst->isLoad() || squashed_inst->isStore() || squashed_inst->isAtomic()) && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && cpu->thread[tid]->MemInstSeq == squashed_inst->seqNum) {
            assert(cpu->thread[tid]->MemInstIssued);
            cpu->thread[tid]->MemInstIssued = false;
            cpu->thread[tid]->MemInstSeq = -1;
            DPRINTF(IQ, "[tid:%i]: SETTING4_MemInstIssued %s "
                "[sn:%llu] MemInstIssued %d\n",
                tid, squashed_inst->pcState(),
                squashed_inst->seqNum,cpu->thread[tid]->MemInstIssued);
        }

        DPRINTF(IQ, "[tid:%i] STEP_INSIDE_Instruction2 [sn:%llu] PC %s squashed isIssued() %d isMemRef() %d memOpDone() %d HasWokenDependents() %d isNonSpeculative() %d isSquashed %d.\n",
                    tid, squashed_inst->seqNum, squashed_inst->pcState(),squashed_inst->isIssued(),squashed_inst->isMemRef(),squashed_inst->memOpDone(),squashed_inst->HasWokenDependents(),squashed_inst->isNonSpeculative(),squashed_inst->isSquashed());

        // need to keep this condition because squash is not always because of control. It is also because of memory violation. 
        // to keep the system flexible for OoO memory using LSQ we will keep the check that weak threads can remove instructions
        // if they have not written back yet.
        if (
            ((!squashed_inst->isIssued() && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) 
            || (!squashed_inst->HasWokenDependents() && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak))
            ||
            (squashed_inst->isMemRef() &&
             !squashed_inst->memOpDone())) {
        // if (!squashed_inst->isIssued()
        //     ||
        //     (squashed_inst->isMemRef() &&
        //      !squashed_inst->memOpDone())) {

            DPRINTF(IQ, "[tid:%i] Instruction [sn:%llu] PC %s squashed.\n",
                    tid, squashed_inst->seqNum, squashed_inst->pcState());

            bool is_acq_rel = squashed_inst->isFullMemBarrier() &&
                         (squashed_inst->isLoad() ||
                          (squashed_inst->isStore() &&
                             !squashed_inst->isStoreConditional()));

            // Remove the instruction from the dependency list.
            if (is_acq_rel ||
                (!squashed_inst->isNonSpeculative() &&
                 !squashed_inst->isStoreConditional() &&
                 !squashed_inst->isAtomic() &&
                 !squashed_inst->isReadBarrier() &&
                 !squashed_inst->isWriteBarrier())
                 || cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak
                 ) {
                
                int8_t total_src_regs = squashed_inst->numSrcRegs();
                for (int src_reg_idx = 0;
                     src_reg_idx < total_src_regs;
                     src_reg_idx++)
                {
                    PhysRegIdPtr src_reg =
                        squashed_inst->renamedSrcIdx(src_reg_idx);

                    // Only remove it from the dependency graph if it
                    // was placed there in the first place.

                    // Instead of doing a linked list traversal, we
                    // can just remove these squashed instructions
                    // either at issue time, or when the register is
                    // overwritten.  The only downside to this is it
                    // leaves more room for error.

                    // we need to ensure that any entries in the dependence graph
                    // for this instruction are removed in the W thread also.
                    // we need to check if any of the vector entries has this instruction, if yes, we delete the
                    // entry.
                    // If this instruction is the head node of a vector in the dependence graph, 
                    // we delete the entire vector entry from the graph.
                    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                        if (!squashed_inst->readySrcIdx(src_reg_idx) &&
                            !src_reg->isFixedMapping()) {
                            dependGraph.remove(src_reg->flatIndex(),
                                            squashed_inst); // Ishita: clear the dependence graph on squash
                        }
                    } else {
                        DPRINTF(IQ, "[tid:%d] PLACE7 STARTING TO SQUASH, [sn:%llu] "
                        "PC %s index %d HasWokenDependents %d isFixedMapping %d.\n", tid, squashed_inst->seqNum, squashed_inst->pcState(),src_reg->flatIndex(),squashed_inst->HasWokenDependents(),src_reg->isFixedMapping());

                        // if this instruction has not already removed all the dependencies
                        if (
                            //!squashed_inst->readySrcIdx(src_reg_idx) && // look and remove reg if not already removed because we always add it to dep graph
                            !src_reg->isFixedMapping() && !squashed_inst->HasWokenDependents()) {

                            dependGraph.removeRAW(src_reg->flatIndex(),
                                            squashed_inst); // Ishita: clear the dependence graph on squash
                        }
                        if (!src_reg->isFixedMapping() && !squashed_inst->HasWokenDependents()) {
                            DynInstPtr last_instWAR = dependGraph.getWARLastInst(src_reg->flatIndex());

                            DPRINTF(IQ, "[tid:%d] CHECK_SQUASH_INST111, [sn:%llu] "
                                    "PC %s current %d orig %d last_seq %d instList %d reg %d.\n", tid, squashed_inst->seqNum, squashed_inst->pcState(),last_instWAR,squashed_inst, dependGraph.getWARSeqNum(src_reg->flatIndex()),dependGraph.instInListWAR(src_reg->flatIndex()),src_reg->flatIndex());
                            
                            // a. This inst should be the last entry in the WAR dependence chain -> assert it 
                            // since squash is in reverse instruction order
                            assert(last_instWAR->seqNum == squashed_inst->seqNum);
                            // b. Then check that the WAR chain has no entries for this
                            assert(dependGraph.instInListWAR(src_reg->flatIndex()) == 0); 
                            // remove this instruction from the queue
                            dependGraph.clearlastInstWAR(src_reg->flatIndex());
                        }
                    }
                    ++iqStats.squashedOperandsExamined;
                }

            } else if (!squashed_inst->isStoreConditional() ||
                       !squashed_inst->isCompleted()) {
                NonSpecMapIt ns_inst_it =
                    nonSpecInsts.find(squashed_inst->seqNum);

                // we remove non-speculative instructions from
                // nonSpecInsts already when they are ready, and so we
                // cannot always expect to find them
                if (ns_inst_it == nonSpecInsts.end()) {
                    // loads that became ready but stalled on a
                    // blocked cache are alreayd removed from
                    // nonSpecInsts, and have not faulted
                    assert(squashed_inst->getFault() != NoFault ||
                           squashed_inst->isMemRef());
                } else {

                    (*ns_inst_it).second = NULL;

                    nonSpecInsts.erase(ns_inst_it);

                    ++iqStats.squashedNonSpecRemoved;
                }
            }
        }


        // IQ clears out the heads of the dependency graph only when
        // instructions reach writeback stage. If an instruction is squashed
        // before writeback stage, its head of dependency graph would not be
        // cleared out; it holds the instruction's DynInstPtr. This
        // prevents freeing the squashed instruction's DynInst.
        // Thus, we need to manually clear out the squashed instructions'
        // heads of dependency graph.

        bool is_acq_rel = squashed_inst->isFullMemBarrier() &&
                         (squashed_inst->isLoad() ||
                          (squashed_inst->isStore() &&
                             !squashed_inst->isStoreConditional()));

        // if ((is_acq_rel ||
        //         (!squashed_inst->isNonSpeculative() &&
        //          !squashed_inst->isStoreConditional() &&
        //          !squashed_inst->isAtomic() &&
        //          !squashed_inst->isReadBarrier() &&
        //          !squashed_inst->isWriteBarrier()))
        //     && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && !squashed_inst->HasWokenDependents()) {
        // if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && !squashed_inst->HasWokenDependents() && !squashed_inst->isIssued()) 
        
        if(((!squashed_inst->HasWokenDependents())
            ||
            (squashed_inst->isMemRef() &&
             !squashed_inst->memOpDone())) && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak ) 
        {
            int8_t total_dest_regs = squashed_inst->numDestRegs();
            for (int dest_reg_idx = 0;
                dest_reg_idx < total_dest_regs;
                dest_reg_idx++)
            {
                PhysRegIdPtr dest_reg =
                    squashed_inst->renamedDestIdx(dest_reg_idx);
                if (dest_reg->isFixedMapping()){
                    continue;
                }
                assert(dependGraph.empty(dest_reg->flatIndex())); 
                dependGraph.clearInst(dest_reg->flatIndex());

                DPRINTF(IQ, "[tid:%d] PLACE1 STARTING TO SQUASH, [sn:%llu] "
                            "PC %s reg %d readyToCommit %d isExecuted %d isCompleted %d HasWokenDependents %d.\n", tid, squashed_inst->seqNum, squashed_inst->pcState(),dest_reg->flatIndex(),squashed_inst->readyToCommit(),squashed_inst->isCompleted(),squashed_inst->isExecuted(),squashed_inst->HasWokenDependents());

                // only for W threads: 
                // remove this instruction from the dependence graphs due to squash
                //if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && !squashed_inst->HasWokenDependents())
                {
                    // for WAW dependence: this should be the last inst in the 
                    // WAW dependence graph
                    // assert that it is, then remove the instruction from the graph
                    DynInstPtr last_instWAW = dependGraph.getLastWAWInst(dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] PLACE8 INSIDE SQUASH, [sn:%llu] "
                            "PC %s last_instWAW %d reg %d readyToCommit %d isExecuted %d isCompleted %d HasWokenDependents %d.\n", tid, squashed_inst->seqNum, squashed_inst->pcState(),last_instWAW,dest_reg->flatIndex(),squashed_inst->readyToCommit(),squashed_inst->isCompleted(),squashed_inst->isExecuted(),squashed_inst->HasWokenDependents());

                    assert(last_instWAW->seqNum == squashed_inst->seqNum);
                    dependGraph.removeInst(dest_reg->flatIndex(),
                                            squashed_inst);

                    // remove the instruction from the RAW dependence graph
                    DynInstPtr last_instRAW = dependGraph.getRAWLastInst(dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] CHECK_SQUASH_INST, [sn:%llu] "
                            "PC %s current %d orig %d last_seq %d instList %d.\n", tid, squashed_inst->seqNum, squashed_inst->pcState(),last_instRAW,squashed_inst, dependGraph.getRAWSeqNum(dest_reg->flatIndex()),dependGraph.instInListRAW(dest_reg->flatIndex()));

                    // a. This inst should be the last entry in the RAW dependence chain -> assert it 
                    // since squash is in reverse instruction order
                    assert(last_instRAW->seqNum == squashed_inst->seqNum);
                    // b. Then check that the RAW chain has no entries for this
                    assert(dependGraph.instInListRAW(dest_reg->flatIndex()) == 0); 
                    // remove this instruction from the queue
                    dependGraph.clearlastInstRAW(dest_reg->flatIndex());

                    // remove inst from WAR
                    dependGraph.removeWAR(dest_reg->flatIndex(),
                                                squashed_inst);
                }
            }
            squashed_inst->setWokeDependents();
        }
        instList[tid].erase(squash_it--);
        ++iqStats.squashedInstsExamined;

        // free squashed inst here
        if (!squashed_inst->isIssued()
            ||
            (squashed_inst->isMemRef() &&
             !squashed_inst->memOpDone())) {

            // Might want to also clear out the head of the dependency graph.

            // Mark it as squashed within the IQ.
            squashed_inst->setSquashedInIQ();

            // @todo: Remove this hack where several statuses are set so the
            // inst will flow through the rest of the pipeline.
            squashed_inst->setIssued();
            squashed_inst->setCanCommit();
            squashed_inst->clearInIQ();

            //Update Thread IQ Count
            count[squashed_inst->threadNumber]--;

            DPRINTF(IQ, "[tid:%d] Removing Squashed instruction instruction [sn:%llu] PC %s "
            "to the IQ QueueSize %d.\n",
            squashed_inst->threadNumber,squashed_inst->seqNum, squashed_inst->pcState(),count[squashed_inst->threadNumber]);

            ++freeEntries;
        }
    }
}

bool
InstructionQueue::PqCompare::operator()(
        const DynInstPtr &lhs, const DynInstPtr &rhs) const
{
    return lhs->seqNum > rhs->seqNum;
}

bool
InstructionQueue::addToDependents(const DynInstPtr &new_inst)
{
    // Loop through the instruction's source registers, adding
    // them to the dependency list if they are not ready.
    int8_t total_src_regs = new_inst->numSrcRegs();
    bool return_val = false;

    int tid = new_inst->threadNumber;
    // print src and dest regs
    DPRINTF(IQ,"[tid:%d] CONSIDER inst [sn:%llu] PC %s RTI %d numSrc %d numDest %d.\n ", new_inst->threadNumber, new_inst->seqNum, new_inst->pcState(), new_inst->readyToIssue(), new_inst->numSrcRegs(), new_inst->numDestRegs());
    for (int src_reg_idx = 0;
         src_reg_idx < total_src_regs;
         src_reg_idx++)
    {
        PhysRegIdPtr src_reg = new_inst->renamedSrcIdx(src_reg_idx);

         DPRINTF(IQ, "[tid:%d] LOOK_REGS_READ RegNum %d %i (%s) [sn:%llu] pinned %d numPinnedComplete %d isFixedMapping %d.\n",
                new_inst->threadNumber,src_reg->flatIndex(), src_reg->index(),
                src_reg->className(),new_inst->seqNum,src_reg->isPinned(),src_reg->getNumPinnedWritesToComplete(),src_reg->isFixedMapping());

        // Only add it to the dependency graph if it's not ready. If the thread is a weak thread we dont use SB. We use our dependence graph to check deopendencies.
        if (!new_inst->readySrcIdx(src_reg_idx) || cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Weak) { 

            // Check the IQ's scoreboard to make sure the register
            // hasn't become ready while the instruction was in flight
            // between stages.  Only if it really isn't ready should
            // it be added to the dependency graph.

            // for S threads we keep this flow unchanged. For W threads, if the reg is not FixedMapping, 
            // we just check if the size of the dependece graph for the src reg is 0. If that is true,
            // Then there are no instructions holding the src register and we can mark the reg as
            // ready. Else we need to add this instruction as a dependent in the dependence graph.

            DPRINTF(IQ, "[tid:%d] INSIDE1 Instruction PC %s has src reg %i (%s) that "
                            "is being added to the dependency chain.\n",
                            new_inst->threadNumber, new_inst->pcState(), src_reg->index(),
                            src_reg->className());

            if(cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                if (src_reg->isFixedMapping()) {
                    continue;
                } else if (!regScoreboard[src_reg->flatIndex()]) {
                    DPRINTF(IQ, "[tid:%d] Instruction PC [sn:%llu] %s has src reg %i (%s) that "
                            "is being added to the dependency chain.\n",
                            tid, new_inst->seqNum,new_inst->pcState(), src_reg->index(),
                            src_reg->className());

                    dependGraph.insert(src_reg->flatIndex(), new_inst);

                    // Change the return value to indicate that something
                    // was added to the dependency graph.
                    return_val = true;
                } else {
                    DPRINTF(IQ, "[tid:%d] Instruction PC [sn:%llu] %s has src reg %i (%s) that "
                            "became ready before it reached the IQ.\n",
                            tid,new_inst->seqNum,new_inst->pcState(), src_reg->index(),
                            src_reg->className());
                    // Mark a register ready within the instruction.
                    new_inst->markSrcRegReady(src_reg_idx); 
                }
            } else {
                if (src_reg->isFixedMapping()) {
                    continue;
                } else if(!dependGraph.isRAWSrcReady(src_reg->flatIndex())) {

                    DPRINTF(IQ, "[tid:%d] Instruction PC %s has src reg %i (%s) that "
                            "is being added to the dependency chain.\n",
                            tid,new_inst->pcState(), src_reg->index(),
                            src_reg->className());

                    // add the inst as a dependence on the last entry of the dependence graph
                    int index = dependGraph.insertBehindRAW(src_reg->flatIndex(), new_inst);
                    
                    DPRINTF(IQ,"[tid:%d] SIZE_OF_ENTRIES1 %d ENTRY %d\n",tid,dependGraph.sizeofRAWFull(src_reg->flatIndex()),dependGraph.sizeofRAWEntry(src_reg->flatIndex()));

                    DPRINTF(IQ, "[tid:%d] PLACE8 RAW_NOT_READY, [sn:%llu] "
                        "PC %s index_placed %d depends on seqNum:%llu reg %d.\n", tid,new_inst->seqNum, new_inst->pcState(),index,dependGraph.getRAWSeqNum(src_reg->flatIndex()),src_reg->flatIndex()); 
                    // Change the return value to indicate that something
                    // was added to the dependency graph.
                    return_val = true;
                } else {  
                    DPRINTF(IQ, "[tid:%d] PLACE9 RAW_READY, [sn:%llu] "
                        "PC %s reg %d.\n", tid, new_inst->seqNum, new_inst->pcState(),src_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] Instruction PC %s has src reg %i (%s) that "
                            "became ready before it reached the IQ.\n",
                            tid, new_inst->pcState(), src_reg->index(),
                            src_reg->className());
                    // Mark a register ready within the instruction.
                    new_inst->markSrcRegReadyW(src_reg_idx); 

                    DPRINTF(IQ,"[tid:%d] SIZE_OF_ENTRIES2 %d ENTRY %d\n",tid,dependGraph.sizeofRAWFull(src_reg->flatIndex()),dependGraph.sizeofRAWEntry(src_reg->flatIndex()));
                }
            }
        } else {
            DPRINTF(IQ, "[tid:%d] RAW_ALREADY_READY Instruction PC %s has src reg %i (%s) that "
                            "became ready already!!.\n",
                            tid,new_inst->pcState(), src_reg->index(),
                            src_reg->className());
            
        }
    }

    // For W threads: we check if the size of the WAR dependence vector is zero, if yes, then the
    // register is ready. If not we add it to as a dependence
    if(cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Weak) {

        DPRINTF(IQ, "[tid:%d] PLACE10 DEST_REGS, [sn:%llu] "
                        "PC %s.\n", tid,new_inst->seqNum, new_inst->pcState());
        int8_t total_dest_regs = new_inst->numDestRegs();
         for (int dest_reg_idx = 0;
         dest_reg_idx < total_dest_regs;
         dest_reg_idx++)
        {
            PhysRegIdPtr dest_reg = new_inst->renamedDestIdx(dest_reg_idx);

            DPRINTF(IQ, "[tid:%d] LOOK_REGS_WRITE RegNum %d %i (%s) [sn:%llu] pinned %d numPinnedComplete %d isFixedMapping %d.\n",
                tid,dest_reg->flatIndex(), dest_reg->index(),
                dest_reg->className(),new_inst->seqNum,dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete(),dest_reg->isFixedMapping());

            DPRINTF(IQ, "[tid:%d] PLACE11 DEST_REGS, [sn:%llu] "
                        "PC %s.\n", tid,new_inst->seqNum, new_inst->pcState());

            DPRINTF(IQ, "[tid:%d] INSIDE3 Instruction PC %s has src reg %i (%s) that "
                            "is being added to the dependency chain.\n",
                            tid,new_inst->pcState(), dest_reg->index(),
                            dest_reg->className());

            // dont put fixed mapping registers here, they are dealt with separately
            if(!dependGraph.isWARSrcReady(dest_reg->flatIndex()) && !dest_reg->isFixedMapping() ) {

                DPRINTF(IQ, "[tid:%d] PLACE12 WAR_SRC_NOT_READY reg %d, [sn:%llu] "
                        "PC %s depends on seqNum:%llu reg %d place idx %d insts_present %d insts_in_queue %d.\n", tid,dest_reg->flatIndex(), new_inst->seqNum, new_inst->pcState(),dependGraph.getWARSeqNum(dest_reg->flatIndex()),dest_reg->flatIndex(),dependGraph.WARSrcLastIdx(dest_reg->flatIndex()),dependGraph.getWARlistSize(dest_reg->flatIndex()),dependGraph.instInListWAR(dest_reg->flatIndex()));


                DPRINTF(IQ, "[tid:%d] WAR Instruction PC %s has src reg %i (%s) that "
                            "is being added to the dependency chain.\n",
                            tid,new_inst->pcState(), dest_reg->index(),
                            dest_reg->className());
                // add the inst as a dependence on the last entry of the dependence graph
                dependGraph.insertBehindWAR(dest_reg->flatIndex(), new_inst, dest_reg_idx);

                DPRINTF(IQ,"[tid:%d] SIZE_OF_ENTRIES4 %d ENTRY %d numWARPending %d\n",tid,dependGraph.sizeofWAWFull(dest_reg->flatIndex()),dependGraph.sizeofWAWEntry(dest_reg->flatIndex()),new_inst->numWARPending[dest_reg_idx]);

                DPRINTF(IQ, "[tid:%d] PLACE12 WAR_SRC_NOT_READY_POST, [sn:%llu] "
                        "PC %s depends on seqNum:%llu .\n", tid,new_inst->seqNum, new_inst->pcState(),dependGraph.getWARSeqNum(dest_reg->flatIndex()));


                DPRINTF(IQ, "[tid:%d] PLACE112 WAR_SRC_NOT_READY_POST, [sn:%llu] "
                        "PC %s depends on seqNum:%llu reg %d place idx %d insts_present %d insts_in_queue %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dependGraph.getWARSeqNum(dest_reg->flatIndex()),dest_reg->flatIndex(),dependGraph.WARSrcLastIdx(dest_reg->flatIndex()),dependGraph.getWARlistSize(dest_reg->flatIndex()),dependGraph.instInListWAR(dest_reg->flatIndex()));

                // Change the return value to indicate that something
                // was added to the dependency graph.
                return_val = true;
            } else {

                DPRINTF(IQ, "[tid:%d] PLACE13 WAR_SRC_READY, [sn:%llu] "
                        "PC %s reg %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->flatIndex());

                DPRINTF(IQ, "[tid:%d] Instruction PC %s has src reg %i (%s) that "
                            "became ready before it reached the IQ.\n",
                            tid,new_inst->pcState(), dest_reg->index(),
                            dest_reg->className());
                    // Mark a register ready within the instruction.

                DPRINTF(IQ, "[tid:%d] PLACE3 WAR1 STARTING TO WAKE INSTS WRITE_INST, [sn:%llu] "
                            "PC %s idx %d total_dest_regs %d dest_reg->isFixedMapping() %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->flatIndex(),total_dest_regs,dest_reg->isFixedMapping());

                new_inst->markDestDepRegReady(dest_reg_idx,total_dest_regs);

                new_inst->markDestRegReady(dest_reg_idx,total_dest_regs); 
            }
            // WAW used to be here
            
        }
    }

    return return_val;
}

void
InstructionQueue::addToProducers(const DynInstPtr &new_inst)
{
    // Nothing really needs to be marked when an instruction becomes
    // the producer of a register's value, but for convenience a ptr
    // to the producing instruction will be placed in the head node of
    // the dependency links.
    int8_t total_dest_regs = new_inst->numDestRegs();

    int tid = new_inst->threadNumber;

    DPRINTF(IQ,"[tid:%d] Adding inst to dependence graph total: %d\n",new_inst->threadNumber,total_dest_regs);

    DPRINTF(IQ, "[tid:%d] PLACE14 ADDING_TO_PRODUCERS, [sn:%llu] "
                        "PC %s.\n", tid,new_inst->seqNum, new_inst->pcState());

    std::unordered_set<int>dest_regs_set;
    for (int dest_reg_idx = 0;
         dest_reg_idx < total_dest_regs;
         dest_reg_idx++)
    {
        PhysRegIdPtr dest_reg = new_inst->renamedDestIdx(dest_reg_idx);

        // Some registers have fixed mapping, and there is no need to track
        // dependencies as these instructions must be executed at commit.
        if (dest_reg->isFixedMapping()) {
            continue;
        }

        if(cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            if (!dependGraph.empty(dest_reg->flatIndex())) {
                panic("Dependency graph %i (%s) (flat: %i) not empty!",
                    dest_reg->index(), dest_reg->className(),
                    dest_reg->flatIndex()); 
            }
            DPRINTF(IQ, "[tid:%d] adding to producer, [sn:%llu] "
                        "PC %s REG %i (%s) (flat: %i) REG %d pinnedWrites %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->index(), dest_reg->className(),
                    dest_reg->flatIndex(),dest_reg->index(),dest_reg->getNumPinnedWritesToComplete());

            dependGraph.setInst(dest_reg->flatIndex(), new_inst); // create new entry for the destination register 
        } else {

            DPRINTF(IQ, "[tid:%d] PLACE14 ADDING_TO_RAW_PRODUCERS, [sn:%llu] "
                        "PC %s REG %i (%s) (flat: %i) idx_push %d index %d numWARPending %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->index(), dest_reg->className(),
                    dest_reg->flatIndex(),dependGraph.countNodesRAW(dest_reg->flatIndex()),dest_reg_idx,new_inst->numWARPending[dest_reg_idx]);

            // add this instruction at the end of the dependence graph
            // dependence graph need not be empty as we dont rename registers.
            dependGraph.setInstPushBack(dest_reg->flatIndex(), new_inst);

            DPRINTF(IQ,"[tid:%d] SIZE_OF_ENTRIES3 %d ENTRY %d\n",tid,dependGraph.sizeofRAWFull(dest_reg->flatIndex()),dependGraph.sizeofRAWEntry(dest_reg->flatIndex()));
        }

        // Mark the scoreboard to say it's not yet ready.
        regScoreboard[dest_reg->flatIndex()] = false;           
    }

    if(cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
        for (int dest_reg_idx = 0;
            dest_reg_idx < total_dest_regs;
            dest_reg_idx++)
        {
            PhysRegIdPtr dest_reg = new_inst->renamedDestIdx(dest_reg_idx);

            // for WAW dependencies, we add this instruction to the dependenceWAWGraph
            // when this instruction is over, it will be removed from the graph and will wake up 
            // the instruction after it and place it at the head of the graph
            // only for W threads: 
            if (dest_regs_set.find(dest_reg_idx) == dest_regs_set.end())
            {
                dest_regs_set.insert(dest_reg_idx);

                DPRINTF(IQ,"[tid:%d] Putting inst in WAW queue [sn:%llu] "
                        "PC %s reg %d fixed mapping %d index %d numWARPending %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->flatIndex(),dest_reg->isFixedMapping(),dest_reg_idx,new_inst->numWARPending[dest_reg_idx]);

                bool isFirstEntry = true;

                int entry_num = -1;
                
                if(!dest_reg->isFixedMapping()) {
                    entry_num = dependGraph.insertBehindWAW(dest_reg->flatIndex(), new_inst);

                    DPRINTF(IQ,"[tid:%d] [sn:%d] WAW_DEST_NUM_CHECK REG %d SIZE_OF_ENTRIES6 %d ENTRY %d entry_num %d\n",tid,new_inst->seqNum,dest_reg->flatIndex(),dependGraph.sizeofWAWFull(dest_reg->flatIndex()),dependGraph.sizeofWAWEntry(dest_reg->flatIndex()),entry_num);

                    if(entry_num != 0) {
                        isFirstEntry = false;
                    }

                }
                // mark dest regs as ready if the entry is the first entry of the linked list
                if(isFirstEntry)
                {
                    DPRINTF(IQ, "[tid:%d] PLACE15 DEP_INST_READY, [sn:%llu] "
                        "PC %s reg %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->flatIndex());

                    DPRINTF(IQ, "[tid:%d] PLACE3 WAR2 STARTING TO WAKE INSTS WRITE_INST, [sn:%llu] "
                            "PC %s idx %d total_dest_regs %d dest_reg->isFixedMapping() %d.\n", tid,new_inst->seqNum, new_inst->pcState(),dest_reg->flatIndex(),total_dest_regs,dest_reg->isFixedMapping());

                    new_inst->markDestDepRegReady(dest_reg_idx,total_dest_regs);
                    new_inst->markDestRegReady(dest_reg_idx,total_dest_regs); 
                } else {
                    DPRINTF(IQ, "[tid:%d] PLACE25 WAW_HAZARD, [sn:%llu] "
                        "PC %s entry_num %d.\n", tid,new_inst->seqNum, new_inst->pcState(),entry_num);
                }
            }    
            else {
                panic("same reg being written to twice!!\n");
            }  
        }
    }

    DPRINTF(IQ, "[tid:%d] IMPORTANT_PLACE_INST_IN_WAR, [sn:%llu] "
                        "PC %s total_regs %d.\n", tid,new_inst->seqNum, new_inst->pcState(),new_inst->numSrcRegs());

    // add srcs to vector for WAR dependence list
    if(cpu->thread[new_inst->threadNumber]->tc->getProcessPtr()->getprocessThreadType() == Weak) {

        DPRINTF(IQ, "[tid:%d] IMPORTANT_PLACE_INST_IN_WAR11, [sn:%llu] "
                        "PC %s total_regs %d isExecuted %d.\n", tid,new_inst->seqNum, new_inst->pcState(),new_inst->numSrcRegs(),new_inst->isExecuted());

        int8_t total_src_regs = new_inst->numSrcRegs();

        for (int src_reg_idx = 0;
         src_reg_idx < total_src_regs;
         src_reg_idx++) {

            PhysRegIdPtr src_reg = new_inst->renamedSrcIdx(src_reg_idx);


            // push if not fixedMapping ISHITA:TODO
            if(!src_reg->isFixedMapping()) 
            {
                DPRINTF(IQ, "[tid:%d] PLACE16 DEP_PLACE_INST_IN_WAR, [sn:%llu] "
                            "PC %s reg %d.\n", tid,new_inst->seqNum, new_inst->pcState(),src_reg->flatIndex());

                // add each src to a new vector entry for the WAR dependence graph
                dependGraph.setInstPushBackWAR(src_reg->flatIndex(), new_inst);

                DPRINTF(IQ,"[tid:%d] SIZE_OF_ENTRIES5 %d ENTRY %d\n",tid,dependGraph.sizeofWARFull(src_reg->flatIndex()),dependGraph.sizeofWAREntry(src_reg->flatIndex()));
            }
        }
    }
}

void
InstructionQueue::addIfReady(const DynInstPtr &inst)
{
    // If the instruction now has all of its source registers
    // available, then add it to the list of ready instructions.
    if (inst->readyToIssue()) {

        int tid = inst->threadNumber;

        // if this is a weak thread, set up the pinned threads value here.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            
            int8_t total_dest_regs = inst->numDestRegs();

            DPRINTF(IQ, "[tid:%d] STEP1, putting it onto "
                "the ready list, PC %s  [sn:%llu] total_dest_regs %d.\n",
                tid,inst->pcState(), inst->seqNum,total_dest_regs);

            for (int dest_reg_idx = 0;
                dest_reg_idx < total_dest_regs;
                dest_reg_idx++)
            {
                PhysRegIdPtr dest_reg = inst->renamedDestIdx(dest_reg_idx);
                
                dest_reg->setNumPinnedWrites(inst->getnumWrites(dest_reg_idx));
                dest_reg->setNumPinnedWritesToComplete(
                    inst->getNumPinnedWritesToComplete(dest_reg_idx));
                DPRINTF(IQ, "[tid:%d] LOOK_REGS_FINAL_WRITE [sn:%llu] RegNum %d %i (%s) pinned %d numPinnedComplete %d.\n",
                tid,inst->seqNum,dest_reg->flatIndex(), dest_reg->index(),
                dest_reg->className(),dest_reg->isPinned(),dest_reg->getNumPinnedWritesToComplete());
            }
        }

        //Add the instruction to the proper ready list.
        if (inst->isMemRef()) {

            DPRINTF(IQ, "[tid:%d] Checking if memory instruction can issue.\n",tid);

            // Message to the mem dependence unit that this instruction has
            // its registers ready.
            memDepUnit[inst->threadNumber].regsReady(inst);

            return;
        }

        OpClass op_class = inst->opClass();

        DPRINTF(IQ, "[tid:%d] Instruction is ready to issue, putting it onto "
                "the ready list, PC %s opclass:%i [sn:%llu] type %d queueOnList[op_class] %d.\n",
                tid,inst->pcState(), op_class, inst->seqNum,cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType(),queueOnList[op_class]);

        readyInsts[op_class].push(inst);

        // Will need to reorder the list if either a queue is not on the list,
        // or it has an older instruction than last time.
        if (!queueOnList[op_class]) {
            addToOrderList(op_class);
            DPRINTF(IQ, "[tid:%d] ADD_TO_QUEU1 to issue, putting it onto "
                "the ready list, PC %s opclass:%i [sn:%llu] type %d.\n",
                tid,inst->pcState(), op_class, inst->seqNum,cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType());
        } else if (readyInsts[op_class].top()->seqNum  <
                   (*readyIt[op_class]).oldestInst) {
            DPRINTF(IQ, "[tid:%d] ADD_TO_QUEU2 to issue, putting it onto "
                "the ready list, PC %s opclass:%i [sn:%llu] type %d.\n",
                tid,inst->pcState(), op_class, inst->seqNum,cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType());
            listOrder.erase(readyIt[op_class]);
            addToOrderList(op_class);
        }
    }
}

int
InstructionQueue::countInsts()
{
    return numEntries - freeEntries;
}

void
InstructionQueue::dumpLists()
{
    for (int i = 0; i < Num_OpClasses; ++i) {
        cprintf("Ready list %i size: %i\n", i, readyInsts[i].size());

        cprintf("\n");
    }

    cprintf("Non speculative list size: %i\n", nonSpecInsts.size());

    NonSpecMapIt non_spec_it = nonSpecInsts.begin();
    NonSpecMapIt non_spec_end_it = nonSpecInsts.end();

    cprintf("Non speculative list: ");

    while (non_spec_it != non_spec_end_it) {
        cprintf("%s [sn:%llu]", (*non_spec_it).second->pcState(),
                (*non_spec_it).second->seqNum);
        ++non_spec_it;
    }

    cprintf("\n");

    ListOrderIt list_order_it = listOrder.begin();
    ListOrderIt list_order_end_it = listOrder.end();
    int i = 1;

    cprintf("List order: ");

    while (list_order_it != list_order_end_it) {
        cprintf("%i OpClass:%i [sn:%llu] ", i, (*list_order_it).queueType,
                (*list_order_it).oldestInst);

        ++list_order_it;
        ++i;
    }

    cprintf("\n");
}


void
InstructionQueue::dumpInsts()
{
    for (ThreadID tid = 0; tid < numThreads; ++tid) {
        int num = 0;
        int valid_num = 0;
        ListIt inst_list_it = instList[tid].begin();

        while (inst_list_it != instList[tid].end()) {
            cprintf("Instruction:%i\n", num);
            if (!(*inst_list_it)->isSquashed()) {
                if (!(*inst_list_it)->isIssued()) {
                    ++valid_num;
                    cprintf("Count:%i\n", valid_num);
                } else if ((*inst_list_it)->isMemRef() &&
                           !(*inst_list_it)->memOpDone()) {
                    // Loads that have not been marked as executed
                    // still count towards the total instructions.
                    ++valid_num;
                    cprintf("Count:%i\n", valid_num);
                }
            }

            cprintf("PC: %s\n[sn:%llu]\n[tid:%i]\n"
                    "Issued:%i\nSquashed:%i\n",
                    (*inst_list_it)->pcState(),
                    (*inst_list_it)->seqNum,
                    (*inst_list_it)->threadNumber,
                    (*inst_list_it)->isIssued(),
                    (*inst_list_it)->isSquashed());

            if ((*inst_list_it)->isMemRef()) {
                cprintf("MemOpDone:%i\n", (*inst_list_it)->memOpDone());
            }

            cprintf("\n");

            inst_list_it++;
            ++num;
        }
    }

    cprintf("Insts to Execute list:\n");

    int num = 0;
    int valid_num = 0;
    ListIt inst_list_it = instsToExecute.begin();

    while (inst_list_it != instsToExecute.end())
    {
        cprintf("Instruction:%i\n",
                num);
        if (!(*inst_list_it)->isSquashed()) {
            if (!(*inst_list_it)->isIssued()) {
                ++valid_num;
                cprintf("Count:%i\n", valid_num);
            } else if ((*inst_list_it)->isMemRef() &&
                       !(*inst_list_it)->memOpDone()) {
                // Loads that have not been marked as executed
                // still count towards the total instructions.
                ++valid_num;
                cprintf("Count:%i\n", valid_num);
            }
        }

        cprintf("PC: %s\n[sn:%llu]\n[tid:%i]\n"
                "Issued:%i\nSquashed:%i\n",
                (*inst_list_it)->pcState(),
                (*inst_list_it)->seqNum,
                (*inst_list_it)->threadNumber,
                (*inst_list_it)->isIssued(),
                (*inst_list_it)->isSquashed());

        if ((*inst_list_it)->isMemRef()) {
            cprintf("MemOpDone:%i\n", (*inst_list_it)->memOpDone());
        }

        cprintf("\n");

        inst_list_it++;
        ++num;
    }
}

} // namespace o3
} // namespace gem5