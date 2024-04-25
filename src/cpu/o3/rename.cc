/*
 * Copyright (c) 2010-2012, 2014-2019 ARM Limited
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

#include "cpu/o3/rename.hh"

#include <list>

#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/limits.hh"
#include "cpu/reg_class.hh"
#include "debug/Activity.hh"
#include "debug/O3PipeView.hh"
#include "debug/Rename.hh"
#include "params/BaseO3CPU.hh"
#include "debug/IQ.hh"
#include "debug/pipelineView.hh"
#include <algorithm>
#include <utility>

namespace gem5
{

namespace o3
{

Rename::Rename(CPU *_cpu, const BaseO3CPUParams &params)
    : cpu(_cpu),
      renamePolicy(params.smtRenamePolicy),
      iewToRenameDelay(params.iewToRenameDelay),
      decodeToRenameDelay(params.decodeToRenameDelay),
      commitToRenameDelay(params.commitToRenameDelay),
      renameWidth(params.renameWidth),
      numThreads(params.numThreads),
      numRenamingThreads(params.smtNumRenamingThreads),
      SingleThreadFetchiew(params.SingleThreadFetchIEW),
      stats(_cpu)
{
    if (renameWidth > MaxWidth)
        fatal("renameWidth (%d) is larger than compiled limit (%d),\n"
             "\tincrease MaxWidth in src/cpu/o3/limits.hh\n",
             renameWidth, static_cast<int>(MaxWidth));

    // @todo: Make into a parameter.
    skidBufferMax = (decodeToRenameDelay + 1) * params.decodeWidth;
    for (uint32_t tid = 0; tid < MaxThreads; tid++) {
        renameStatus[tid] = Idle;
        renameMap[tid] = nullptr;
        instsInProgress[tid] = 0;
        loadsInProgress[tid] = 0;
        storesInProgress[tid] = 0;
        freeEntries[tid] = {0, 0, 0, 0};
        emptyROB[tid] = true;
        stalls[tid] = {false, false};
        serializeInst[tid] = nullptr;
        serializeOnNextInst[tid] = false;
    }

    printf("rename SingleThreadFetchiew %d %d\n",SingleThreadFetchiew,params.SingleThreadFetchiew);
}

std::string
Rename::name() const
{
    return cpu->name() + ".rename";
}

Rename::RenameStats::RenameStats(statistics::Group *parent)
    : statistics::Group(parent, "rename"),
      ADD_STAT(squashCycles, statistics::units::Cycle::get(),
               "Number of cycles rename is squashing"),
      ADD_STAT(squashCyclesSThread, statistics::units::Cycle::get(),
               "Number of cycles rename is squashing S Thread"),
      ADD_STAT(squashCyclesWThread, statistics::units::Cycle::get(),
               "Number of cycles rename is squashing W Thread"),
      ADD_STAT(idleCycles, statistics::units::Cycle::get(),
               "Number of cycles rename is idle"),
      ADD_STAT(idleCyclesSThread, statistics::units::Cycle::get(),
               "Number of cycles rename is idle S Thread"),
      ADD_STAT(idleCyclesWThread, statistics::units::Cycle::get(),
               "Number of cycles rename is idle W Thread"),
      ADD_STAT(blockCycles, statistics::units::Cycle::get(),
               "Number of cycles rename is blocking"),
      ADD_STAT(blockCyclesSThread, statistics::units::Cycle::get(),
               "Number of cycles rename is blocking S Thread"),
      ADD_STAT(blockCyclesWThread, statistics::units::Cycle::get(),
               "Number of cycles rename is blocking W Thread"),
      ADD_STAT(serializeStallCycles, statistics::units::Cycle::get(),
               "count of cycles rename stalled for serializing inst"),
      ADD_STAT(runCycles, statistics::units::Cycle::get(),
               "Number of cycles rename is running"),
      ADD_STAT(runCyclesSThread, statistics::units::Cycle::get(),
               "Number of cycles rename is running S Thread"),
      ADD_STAT(runCyclesWThread, statistics::units::Cycle::get(),
               "Number of cycles rename is running W Thread"),
      ADD_STAT(unblockCycles, statistics::units::Cycle::get(),
               "Number of cycles rename is unblocking"),
      ADD_STAT(renamedInsts, statistics::units::Count::get(),
               "Number of instructions processed by rename"),
      ADD_STAT(squashedInsts, statistics::units::Count::get(),
               "Number of squashed instructions processed by rename"),
      ADD_STAT(ROBFullEvents, statistics::units::Count::get(),
               "Number of times rename has blocked due to ROB full"),
      ADD_STAT(ROBFullEventsS, statistics::units::Count::get(),
               "Number of times rename has blocked due to ROB full for S threads"),
      ADD_STAT(ROBFullEventsW, statistics::units::Count::get(),
               "Number of times rename has blocked due to ROB full for W threads"),

      ADD_STAT(iewStallS, statistics::units::Count::get(),
               "Number of times rename has blocked due to IEW full for S threads"),
      ADD_STAT(iewStallW, statistics::units::Count::get(),
               "Number of times rename has blocked due to IEW full for W threads"),
      ADD_STAT(NoROBFreeS, statistics::units::Count::get(),
               "Number of times rename has blocked due to ROB full for S threads"),
      ADD_STAT(NoROBFreeW, statistics::units::Count::get(),
               "Number of times rename has blocked due to ROB full for W threads"),
      ADD_STAT(NoIQFreeS, statistics::units::Count::get(),
               "Number of times rename has blocked due to IQ full for S threads"),
      ADD_STAT(NoIQFreeW, statistics::units::Count::get(),
               "Number of times rename has blocked due to IQ full for W threads"),
      ADD_STAT(NoLSQFreeS, statistics::units::Count::get(),
               "Number of times rename has blocked due to LSQ full for S threads"),
      ADD_STAT(NoLSQFreeW, statistics::units::Count::get(),
               "Number of times rename has blocked due to LSQ full for W threads"),
      ADD_STAT(NoRenameFreeS, statistics::units::Count::get(),
               "Number of times rename has blocked due to Rename full for S threads"),
      ADD_STAT(NoRenameFreeW, statistics::units::Count::get(),
               "Number of times rename has blocked due to Rename full for W threads"),
      ADD_STAT(SerializeROBFullS, statistics::units::Count::get(),
               "Number of times rename has blocked due to SerializeROBFull full for S threads"),
      ADD_STAT(SerializeROBFullW, statistics::units::Count::get(),
               "Number of times rename has blocked due to SerializeROBFull full for W threads"),
      ADD_STAT(renameDeactivate, statistics::units::Count::get(),
               "Rename Deactivated"),
      ADD_STAT(BlockedBecauseOneThread, statistics::units::Count::get(),
                "Blocked because only 1 thread could go"),
      ADD_STAT(resumeSerializeS, statistics::units::Count::get(),
               "Serialize Resumed"),
      ADD_STAT(resumeSerializeW, statistics::units::Count::get(),
                "Serialize Resumed"),
      ADD_STAT(resumeUnblockingS, statistics::units::Count::get(),
                "Unblocking Resumed"),
      ADD_STAT(resumeUnblockingW, statistics::units::Count::get(),
                "Unblocking Resumed"),
        ADD_STAT(RunningS, statistics::units::Count::get(),
                    "RunningS Resumed"),
        ADD_STAT(RunningW, statistics::units::Count::get(),
                    "RunningS Resumed"),
        ADD_STAT(IdleS, statistics::units::Count::get(),
                    "IdleS Resumed"),
        ADD_STAT(IdleW, statistics::units::Count::get(),
                    "Unblocking Resumed"),
        ADD_STAT(StartSquashS, statistics::units::Count::get(),
                    "StartSquashS Resumed"),
        ADD_STAT(StartSquashW, statistics::units::Count::get(),
                    "StartSquashW Resumed"),
        ADD_STAT(BlockedS, statistics::units::Count::get(),
                    "BlockedS Resumed"),
        ADD_STAT(BlockedW, statistics::units::Count::get(),
                    "BlockedW Resumed"),
        ADD_STAT(UnblockingS, statistics::units::Count::get(),
                    "UnblockingS Resumed"),
        ADD_STAT(UnblockingW, statistics::units::Count::get(),
                    "UnblockingW Resumed"),
        ADD_STAT(SerializeStallS, statistics::units::Count::get(),
                    "SerializeStallS Resumed"),
        ADD_STAT(SerializeStallW, statistics::units::Count::get(),
                    "SerializeStallW Resumed"),
        ADD_STAT(unknownStallS, statistics::units::Count::get(),
                    "Unknown stall"),

      ADD_STAT(IQFullEvents, statistics::units::Count::get(),
               "Number of times rename has blocked due to IQ full"),
      ADD_STAT(IQFullEventsS, statistics::units::Count::get(),
               "Number of times rename has blocked due to IQ full for S threads"),
      ADD_STAT(IQFullEventsW, statistics::units::Count::get(),
               "Number of times rename has blocked due to IQ full for W threads"),
      ADD_STAT(LQFullEvents, statistics::units::Count::get(),
               "Number of times rename has blocked due to LQ full" ),
      ADD_STAT(LQFullEventsS, statistics::units::Count::get(),
               "Number of times rename has blocked due to LQ full for S threads" ),
      ADD_STAT(LQFullEventsW, statistics::units::Count::get(),
               "Number of times rename has blocked due to LQ full for W threads" ),
      ADD_STAT(SQFullEvents, statistics::units::Count::get(),
               "Number of times rename has blocked due to SQ full"),
      ADD_STAT(SQFullEventsS, statistics::units::Count::get(),
               "Number of times rename has blocked due to SQ full for S threads"),
      ADD_STAT(SQFullEventsW, statistics::units::Count::get(),
               "Number of times rename has blocked due to SQ full for W threads"),
      ADD_STAT(fullRegistersEvents, statistics::units::Count::get(),
               "Number of times there has been no free registers"),
      ADD_STAT(fullRegistersEventsS, statistics::units::Count::get(),
               "Number of times there has been no free registers for S threads"),
      ADD_STAT(fullRegistersEventsW, statistics::units::Count::get(),
               "Number of times there has been no free registers for W threads"),
      ADD_STAT(renamedOperands, statistics::units::Count::get(),
               "Number of destination operands rename has renamed"),
      ADD_STAT(lookups, statistics::units::Count::get(),
               "Number of register rename lookups that rename has made"),
      ADD_STAT(intLookups, statistics::units::Count::get(),
               "Number of integer rename lookups"),
      ADD_STAT(fpLookups, statistics::units::Count::get(),
               "Number of floating rename lookups"),
      ADD_STAT(vecLookups, statistics::units::Count::get(),
               "Number of vector rename lookups"),
      ADD_STAT(vecPredLookups, statistics::units::Count::get(),
               "Number of vector predicate rename lookups"),
      ADD_STAT(committedMaps, statistics::units::Count::get(),
               "Number of HB maps that are committed"),
      ADD_STAT(undoneMaps, statistics::units::Count::get(),
               "Number of HB maps that are undone due to squashing"),
      ADD_STAT(serializing, statistics::units::Count::get(),
               "count of serializing insts renamed"),
      ADD_STAT(tempSerializing, statistics::units::Count::get(),
               "count of temporary serializing insts renamed"),
      ADD_STAT(skidInsts, statistics::units::Count::get(),
               "count of insts added to the skid buffer"),

      ADD_STAT(stalledS, statistics::units::Count::get(),
               "Number of cycles all S threads are stalled"),
      ADD_STAT(stalledW, statistics::units::Count::get(),
               "Number of cycles all W threads are stalled"),
      ADD_STAT(stalledSNotW, statistics::units::Count::get(),
               "Number of cycles all S threads are stalled but not W threads"),
      ADD_STAT(stalledSAndW, statistics::units::Count::get(),
               "Number of cycles all S and W threads are stalled"),
      ADD_STAT(notStalled, statistics::units::Count::get(),
               "Number of cycles rename is not stalled"),
    ADD_STAT(blockingIQFull, statistics::units::Count::get(),
               "Blocking from IQ Full"),
    ADD_STAT(blockingIQFullS, statistics::units::Count::get(),
            "Blocking from IQ Full S"),
    ADD_STAT(blockingIQFullW, statistics::units::Count::get(),
            "Blocking from IQ Full W"),
    ADD_STAT(blockingROBFull, statistics::units::Count::get(),
            "Blocking from ROB Full"),
    ADD_STAT(blockingROBFullS, statistics::units::Count::get(),
            "Blocking from ROB Full S"),
    ADD_STAT(blockingROBFullW, statistics::units::Count::get(),
            "Blocking from ROB Full W"),
    ADD_STAT(blockingBandwidthFull, statistics::units::Count::get(),
            "Blocking from bandwidth Full"),
    ADD_STAT(blockingBandwidthFullS, statistics::units::Count::get(),
            "Blocking from bandwidth Full S"),
    ADD_STAT(blockingBandwidthFullW, statistics::units::Count::get(),
            "Blocking from bandwidth Full W"),
    ADD_STAT(blockingRegFull, statistics::units::Count::get(),
               "Blocking from registers Full"),
    ADD_STAT(blockingRegFullS, statistics::units::Count::get(),
            "Blocking from registers Full S"),
    ADD_STAT(blockingRegFullW, statistics::units::Count::get(),
            "Blocking from registers Full W"),
    ADD_STAT(blockingSerialized, statistics::units::Count::get(),
            "Blocking from serialized instruction"),
    ADD_STAT(blockingSerializedS, statistics::units::Count::get(),
            "Blocking from serialized instruction S"),
    ADD_STAT(blockingSerializedW, statistics::units::Count::get(),
            "Blocking from serialized instruction W")
{
    squashCycles.prereq(squashCycles);
    squashCyclesSThread.prereq(squashCyclesSThread);
    squashCyclesWThread.prereq(squashCyclesWThread);
    idleCycles.prereq(idleCycles);
    idleCyclesSThread.prereq(idleCyclesSThread);
    idleCyclesWThread.prereq(idleCyclesWThread);
    blockCycles.prereq(blockCycles);
    blockCyclesSThread.prereq(blockCyclesSThread);
    blockCyclesWThread.prereq(blockCyclesWThread);
    serializeStallCycles.flags(statistics::total);
    runCycles.prereq(runCycles);
    runCyclesSThread.prereq(runCyclesSThread);
    runCyclesWThread.prereq(runCyclesWThread);
    unblockCycles.prereq(unblockCycles);

    renamedInsts.prereq(renamedInsts);
    squashedInsts.prereq(squashedInsts);

    ROBFullEvents.prereq(ROBFullEvents);
    ROBFullEventsS.prereq(ROBFullEventsW);
    ROBFullEventsS.prereq(ROBFullEventsW);
    IQFullEvents.prereq(IQFullEvents);
    IQFullEventsS.prereq(IQFullEventsS);
    IQFullEventsW.prereq(IQFullEventsW);
    LQFullEvents.prereq(LQFullEvents);
    LQFullEventsS.prereq(LQFullEventsS);
    LQFullEventsW.prereq(LQFullEventsW);
    SQFullEvents.prereq(SQFullEvents);
    SQFullEventsS.prereq(SQFullEventsS);
    SQFullEventsW.prereq(SQFullEventsW);
    fullRegistersEvents.prereq(fullRegistersEvents);
    fullRegistersEventsS.prereq(fullRegistersEventsS);
    fullRegistersEventsW.prereq(fullRegistersEventsW);

    renamedOperands.prereq(renamedOperands);
    lookups.prereq(lookups);
    intLookups.prereq(intLookups);
    fpLookups.prereq(fpLookups);
    vecLookups.prereq(vecLookups);
    vecPredLookups.prereq(vecPredLookups);

    committedMaps.prereq(committedMaps);
    undoneMaps.prereq(undoneMaps);
    serializing.flags(statistics::total);
    tempSerializing.flags(statistics::total);
    skidInsts.flags(statistics::total);

    stalledS.prereq(stalledS);
    stalledW.prereq(stalledW);
    stalledSNotW.prereq(stalledSNotW);
    stalledSAndW.prereq(stalledSAndW);
    notStalled.prereq(notStalled);
    blockingIQFull.prereq(blockingIQFull);
    blockingIQFullS.prereq(blockingIQFullS);
    blockingIQFullW.prereq(blockingIQFullW);
    blockingROBFull.prereq(blockingROBFull);
    blockingROBFullS.prereq(blockingROBFullS);
    blockingROBFullW.prereq(blockingROBFullW);
    blockingBandwidthFull.prereq(blockingBandwidthFull);
    blockingBandwidthFullS.prereq(blockingBandwidthFullS);
    blockingBandwidthFullW.prereq(blockingBandwidthFullW);
    blockingRegFull.prereq(blockingRegFull);
    blockingRegFullS.prereq(blockingRegFullS);
    blockingRegFullW.prereq(blockingRegFullW);
    blockingSerialized.prereq(blockingSerialized);
    blockingSerializedS.prereq(blockingSerializedS);
    blockingSerializedW.prereq(blockingSerializedW);
}

void
Rename::regProbePoints()
{
    ppRename = new ProbePointArg<DynInstPtr>(
            cpu->getProbeManager(), "Rename");
    ppSquashInRename = new ProbePointArg<SeqNumRegPair>(cpu->getProbeManager(),
                                                        "SquashInRename");
}

void
Rename::setTimeBuffer(TimeBuffer<TimeStruct> *tb_ptr)
{
    timeBuffer = tb_ptr;

    // Setup wire to read information from time buffer, from IEW stage.
    fromIEW = timeBuffer->getWire(-iewToRenameDelay); 

    // Setup wire to read information from time buffer, from commit stage.
    fromCommit = timeBuffer->getWire(-commitToRenameDelay);

    // Setup wire to write information to previous stages.
    toDecode = timeBuffer->getWire(0);
}

void
Rename::setRenameQueue(TimeBuffer<RenameStruct> *rq_ptr)
{
    renameQueue = rq_ptr;

    // Setup wire to write information to future stages.
    toIEW = renameQueue->getWire(0);
}

void
Rename::setDecodeQueue(TimeBuffer<DecodeStruct> *dq_ptr)
{
    decodeQueue = dq_ptr;

    // Setup wire to get information from decode.
    fromDecode = decodeQueue->getWire(-decodeToRenameDelay);
}

void
Rename::startupStage()
{
    resetStage();
}

void
Rename::clearStates(ThreadID tid)
{
    renameStatus[tid] = Idle;

    freeEntries[tid].iqEntries = iew_ptr->instQueue.numFreeEntries(tid);
    freeEntries[tid].lqEntries = iew_ptr->ldstQueue.numFreeLoadEntries(tid);
    freeEntries[tid].sqEntries = iew_ptr->ldstQueue.numFreeStoreEntries(tid);
    freeEntries[tid].robEntries = commit_ptr->numROBFreeEntries(tid);
    emptyROB[tid] = true;

    stalls[tid].iew = false;
    serializeInst[tid] = NULL;

    instsInProgress[tid] = 0;
    loadsInProgress[tid] = 0;
    storesInProgress[tid] = 0;

    serializeOnNextInst[tid] = false;
}

void
Rename::resetStage()
{
    _status = Inactive;

    resumeSerialize = false;
    resumeUnblocking = false;

    priorityList.clear();

    // Grab the number of free entries directly from the stages.
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        renameStatus[tid] = Idle;

        freeEntries[tid].iqEntries = iew_ptr->instQueue.numFreeEntries(tid);
        freeEntries[tid].lqEntries =
            iew_ptr->ldstQueue.numFreeLoadEntries(tid);
        freeEntries[tid].sqEntries =
            iew_ptr->ldstQueue.numFreeStoreEntries(tid);
        freeEntries[tid].robEntries = commit_ptr->numROBFreeEntries(tid);
        emptyROB[tid] = true;

        stalls[tid].iew = false;
        serializeInst[tid] = NULL;

        instsInProgress[tid] = 0;
        loadsInProgress[tid] = 0;
        storesInProgress[tid] = 0;

        serializeOnNextInst[tid] = false;

        priorityList.push_back(tid);
    }
}

void
Rename::setActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeThreads = at_ptr;
}

void
Rename::setSActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeSThreads = at_ptr;
}

void
Rename::setWActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeWThreads = at_ptr;
}

void
Rename::setAllSThreads(std::list<ThreadID> *at_ptr)
{
    allSThreads = at_ptr;
}

void
Rename::setAllWThreads(std::list<ThreadID> *at_ptr)
{
    allWThreads = at_ptr;
}

void
Rename::activateThread(ThreadID tid)
{
    //renameStatus[tid] = Running;
    renameStatus[tid] = Idle;

    auto thread_it = std::find(priorityList.begin(),
            priorityList.end(), tid);

    if(thread_it == priorityList.end())
    {
        priorityList.push_back(tid);
    }
}

void
Rename::deactivateThread(ThreadID tid)
{
    // Update priority list
    auto thread_it = std::find(priorityList.begin(), priorityList.end(), tid);
    if (thread_it != priorityList.end()) {
        priorityList.erase(thread_it);
    }
}

void
Rename::setRenameMap(UnifiedRenameMap rm_ptr[])
{
    for (ThreadID tid = 0; tid < numThreads; tid++)
        renameMap[tid] = &rm_ptr[tid];
}

void
Rename::setFreeList(UnifiedFreeList *fl_ptr)
{
    freeList = fl_ptr;
}

void
Rename::setScoreboard(Scoreboard *_scoreboard)
{
    scoreboard = _scoreboard;
}

bool
Rename::isDrained() const
{
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        if (instsInProgress[tid] != 0 ||
            !historyBuffer[tid].empty() ||
            !skidBuffer[tid].empty() ||
            !insts[tid].empty() ||
            (renameStatus[tid] != Idle && renameStatus[tid] != Running))
            return false;
    }
    return true;
}

void
Rename::takeOverFrom()
{
    resetStage();
}

void
Rename::drainSanityCheck() const
{
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        assert(historyBuffer[tid].empty());
        assert(insts[tid].empty());
        assert(skidBuffer[tid].empty());
        assert(instsInProgress[tid] == 0);
    }
}

void
Rename::squash(const InstSeqNum &squash_seq_num, ThreadID tid)
{
    DPRINTF(Rename, "[tid:%i] [squash sn:%llu] Squashing instructions.\n",
        tid,squash_seq_num);

    // Clear the stall signal if rename was blocked or unblocking before.
    // If it still needs to block, the blocking should happen the next
    // cycle and there should be space to hold everything due to the squash.
    if (renameStatus[tid] == Blocked ||
        renameStatus[tid] == Unblocking) {
        DPRINTF(Rename, "[tid:%i] Unblocking_at_2\n", tid);
        toDecode->renameUnblock[tid] = 1;

        resumeSerialize = false;
        serializeInst[tid] = NULL;
    } else if (renameStatus[tid] == SerializeStall) {
        if (serializeInst[tid]->seqNum <= squash_seq_num) {
            DPRINTF(Rename, "[tid:%i] [squash sn:%llu] "
                "Rename will resume serializing after squash\n",
                tid,squash_seq_num);
            resumeSerialize = true;
            assert(serializeInst[tid]);
        } else {
            resumeSerialize = false;
            DPRINTF(Rename, "[tid:%i] Unblocking_at_3\n", tid);
            toDecode->renameUnblock[tid] = 1;

            serializeInst[tid] = NULL;
        }
    }

    // Set the status to Squashing.
    renameStatus[tid] = Squashing;

    // Squash any instructions from decode.
    for (int i=0; i<fromDecode->size; i++) {
        if (fromDecode->insts[i]->threadNumber == tid &&
            fromDecode->insts[i]->seqNum > squash_seq_num) {
            fromDecode->insts[i]->setSquashed();
            wroteToTimeBuffer = true;
        }

    }

    // Clear the instruction list and skid buffer in case they have any
    // insts in them.
    insts[tid].clear();

    // Clear the skid buffer in case it has any data in it.
    skidBuffer[tid].clear();

    doSquash(squash_seq_num, tid);
}

void
Rename::tick()
{
    wroteToTimeBuffer = false;

    blockThisCycle = false;

    bool status_change = false;

    SThreadsRenamed = 0;

    //Daniel checking blockage
    unsigned notBlockedS = 0;
    unsigned sThreadCount = 0;
    unsigned notBlockedW = 0;
    unsigned wThreadCount = 0;

    toIEWIndex = 0;

    sortInsts();

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    rename_vals_sent = 0;
    rename_vals_0_sent = 0;
    rename_vals_1_sent = 0;

    RenamePreference.clear();
    // Check stall and squash signals.

    while (threads != end) {
        ThreadID tid = *threads++;

        DPRINTF(Rename, "Processing [tid:%i]\n", tid);

        status_change = checkSignalsAndUpdate(tid) || status_change;

        // Daniel checking rename blocks
        int insts_available = renameStatus[tid] == Unblocking ?
        skidBuffer[tid].size() : insts[tid].size();
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            sThreadCount++;
            if(insts_available != 0 && (renameStatus[tid] == Unblocking || renameStatus[tid] == Running)){
                notBlockedS++;
            }
            if(renameStatus[tid] == Running) {
                ++stats.RunningS;
            } else if(renameStatus[tid] == Idle) {
                ++stats.IdleS;
            } else if(renameStatus[tid] == StartSquash) {
                ++stats.StartSquashS;
            } else if(renameStatus[tid] == Squashing) {
                ++stats.SquashingS;
            } else if(renameStatus[tid] == Blocked) {
                ++stats.BlockedS;
            } else if(renameStatus[tid] == Unblocking) {
                ++stats.UnblockingS;
            } else if(renameStatus[tid] == SerializeStall) {
                ++stats.SerializeStallS;
            } else {
                ++stats.unknownStallS;
            }
        }
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            wThreadCount++;
            if (insts_available != 0 && (renameStatus[tid] == Unblocking || renameStatus[tid] == Running)){
                notBlockedW++;
            }
            if(renameStatus[tid] == Running) {
                ++stats.RunningW;
            } else if(renameStatus[tid] == Idle) {
                ++stats.IdleW;
            } else if(renameStatus[tid] == StartSquash) {
                ++stats.StartSquashW;
            } else if(renameStatus[tid] == Squashing) {
                ++stats.SquashingW;
            } else if(renameStatus[tid] == Blocked) {
                ++stats.BlockedW;
            } else if(renameStatus[tid] == Unblocking) {
                ++stats.UnblockingW;
            } else if(renameStatus[tid] == SerializeStall) {
                ++stats.SerializeStallW;
            }
            // Always rename W threads
            rename(status_change, tid);
        }


    }

    // use a scheduling policy here to only rename 1 thread in 1 cycle
    // only 1 thread renames in a cycle
    bool thread_renamed = false;
    getRenamingThread();

    for(int i = 0; i < RenamePreference.size() ; i++) {
        ThreadID tid = RenamePreference[i];
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            continue;
        } else if(thread_renamed){
            blockThisCycle = true;
            ++stats.BlockedBecauseOneThread;
            block(tid);
        }
        if (renameStatus[tid] == Blocked) {
            ++stats.blockCycles;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                ++stats.blockCyclesSThread;
            } else {
                ++stats.blockCyclesWThread;
            }
        } else if (renameStatus[tid] == Squashing) {
            ++stats.squashCycles;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                ++stats.squashCyclesSThread;
            } else {
                ++stats.squashCyclesWThread;
            }
        } else if (renameStatus[tid] == SerializeStall) {
            ++stats.serializeStallCycles;
        // If we are currently in SerializeStall and resumeSerialize
        // was set, then that means that we are resuming serializing
        // this cycle.  Tell the previous stages to block.
            if (resumeSerialize) {
                if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                    ++stats.resumeSerializeS;
                } else {
                    ++stats.resumeSerializeW;
                }
                resumeSerialize = false;
                block(tid);
                DPRINTF(Rename, "[tid:%i] Unblocking_at_4\n", tid);
                toDecode->renameUnblock[tid] = false;
            }
        } else if (renameStatus[tid] == Unblocking) {
            if (resumeUnblocking) {
                if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                    ++stats.resumeUnblockingS;
                } else {
                    ++stats.resumeUnblockingW;
                }
                block(tid);
                resumeUnblocking = false;
                DPRINTF(Rename, "[tid:%i] Unblocking_at_5\n", tid);
                toDecode->renameUnblock[tid] = false;
            }
        }
        int insts_available = renameStatus[tid] == Unblocking ?
        skidBuffer[tid].size() : insts[tid].size();

        if (insts_available != 0 && (renameStatus[tid] == Running ||
            renameStatus[tid] == Idle)) {
            DPRINTF(Rename,
                    "[tid:%i] "
                    "Not blocked, so attempting to run stage.\n",
                    tid);
            renameInsts(tid);
            SThreadsRenamed++;
            thread_renamed = true;
        } else if (insts_available != 0 && renameStatus[tid] == Unblocking) {
            SThreadsRenamed++;
            renameInsts(tid);

            if (validInsts()) {
                // Add the current inputs to the skid buffer so they can be
                // reprocessed when this stage unblocks.
                skidInsert(tid);
            }

            // If we switched over to blocking, then there's a potential for
            // an overall status change.
            status_change = unblock(tid) || status_change || blockThisCycle;
            thread_renamed = true;
        }
    }

    DPRINTF(pipelineView,"rename_vals_sent %d\n",rename_vals_sent);
    DPRINTF(pipelineView,"rename_vals_0_sent %d\n",rename_vals_0_sent);
    DPRINTF(pipelineView,"rename_vals_1_sent %d\n",rename_vals_1_sent);

    if (sThreadCount > 0 && notBlockedS == 0){
        ++stats.stalledS;
    }
    if (wThreadCount > 0 && notBlockedW == 0){
        ++stats.stalledW;
    }
    if (sThreadCount > 0 && wThreadCount > 0 && notBlockedS == 0 && notBlockedW > 0){
        ++stats.stalledSNotW;
    }
    if (sThreadCount > 0 && notBlockedS == 0 && wThreadCount > 0 && notBlockedW == 0){
        ++stats.stalledSAndW;
    }
    if (notBlockedS > 0 || notBlockedW > 0){
        ++stats.notStalled;
    }

    if (status_change) {
        updateStatus();
    }

    if (wroteToTimeBuffer) {
        DPRINTF(Activity, "Activity this cycle.\n");
        cpu->activityThisCycle();
    }

    threads = activeThreads->begin();

    while (threads != end) {
        ThreadID tid = *threads++;

        // If we committed this cycle then doneSeqNum will be > 0
        if (fromCommit->commitInfo[tid].doneSeqNum != 0 &&
            !fromCommit->commitInfo[tid].squash &&
            renameStatus[tid] != Squashing) {

            removeFromHistory(fromCommit->commitInfo[tid].doneSeqNum,
                                  tid);
        }
    }

    // @todo: make into updateProgress function
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        instsInProgress[tid] -= fromIEW->iewInfo[tid].dispatched;
        loadsInProgress[tid] -= fromIEW->iewInfo[tid].dispatchedToLQ;
        storesInProgress[tid] -= fromIEW->iewInfo[tid].dispatchedToSQ;

        if(!(instsInProgress[tid] >=0))
        {
            DPRINTF(Rename,"[tid:%d] thread_issue progress instsInProgress[tid] %d fromIEW->iewInfo[tid].dispatched %d insts[tid].size() %d\n",tid,instsInProgress[tid],fromIEW->iewInfo[tid].dispatched,insts[tid].size());
        }
        assert(loadsInProgress[tid] >= 0);
        assert(storesInProgress[tid] >= 0);
        assert(instsInProgress[tid] >=0);
    }

    /* Upto 1 S thread can rename in a cycle */
    if(SingleThreadFetchiew) {
        assert(SThreadsRenamed <= 1);
    }
}

void
Rename::rename(bool &status_change, ThreadID tid)
{
    // If status is Running or idle,
    //     call renameInsts()
    // If status is Unblocking,
    //     buffer any instructions coming from decode
    //     continue trying to empty skid buffer
    //     check if stall conditions have passed

    if (renameStatus[tid] == Blocked) {
        ++stats.blockCycles;
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            ++stats.blockCyclesSThread;
        } else {
            ++stats.blockCyclesWThread;
        }
    } else if (renameStatus[tid] == Squashing) {
        ++stats.squashCycles;
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            ++stats.squashCyclesSThread;
        } else {
            ++stats.squashCyclesWThread;
        }
    } else if (renameStatus[tid] == SerializeStall) {
        ++stats.serializeStallCycles;
        // If we are currently in SerializeStall and resumeSerialize
        // was set, then that means that we are resuming serializing
        // this cycle.  Tell the previous stages to block.
        if (resumeSerialize) {
            resumeSerialize = false;
            block(tid);
            DPRINTF(Rename, "[tid:%i] Unblocking_at_4\n", tid);
            toDecode->renameUnblock[tid] = false;
        }
    } else if (renameStatus[tid] == Unblocking) {
        if (resumeUnblocking) {
            block(tid);
            resumeUnblocking = false;
            DPRINTF(Rename, "[tid:%i] Unblocking_at_5\n", tid);
            toDecode->renameUnblock[tid] = false;
        }
    }

    if (renameStatus[tid] == Running ||
        renameStatus[tid] == Idle) {
        DPRINTF(Rename,
                "[tid:%i] "
                "Not blocked, so attempting to run stage.\n",
                tid);

        renameInsts(tid);
    } else if (renameStatus[tid] == Unblocking) {
        renameInsts(tid);

        if (validInsts()) {
            // Add the current inputs to the skid buffer so they can be
            // reprocessed when this stage unblocks.
            skidInsert(tid);
        }

        // If we switched over to blocking, then there's a potential for
        // an overall status change.
        status_change = unblock(tid) || status_change || blockThisCycle;
    }
}

void
Rename::renameInsts(ThreadID tid)
{
    // Instructions can be either in the skid buffer or the queue of
    // instructions coming from decode, depending on the status.
    int insts_available = renameStatus[tid] == Unblocking ?
        skidBuffer[tid].size() : insts[tid].size();

    // Check the decode queue to see if instructions are available.
    // If there are no available instructions to rename, then do nothing.
    if (insts_available == 0) {
        DPRINTF(Rename, "[tid:%i] Nothing to do, breaking out early.\n",
                tid);
        ++stats.idleCycles;
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            ++stats.idleCyclesSThread;
        } else {
            ++stats.idleCyclesWThread;
        }
        return;
    } else if (renameStatus[tid] == Unblocking) {
        ++stats.unblockCycles;
    } else if (renameStatus[tid] == Running) {
        ++stats.runCycles;
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            ++stats.runCyclesSThread;
        } else {
            ++stats.runCyclesWThread;
        }
    }

    // Will have to do a different calculation for the number of free
    // entries.
    int free_rob_entries = calcFreeROBEntries(tid);
    int free_iq_entries  = calcFreeIQEntries(tid);
    int min_free_entries = free_rob_entries;

    FullSource source = ROB;

    if (free_iq_entries < min_free_entries) {
        min_free_entries = free_iq_entries;
        source = IQ;
    }

    // Check if there's any space left.
    if (min_free_entries <= 0) {
        DPRINTF(Rename,
                "[tid:%i] Blocking due to no free ROB/IQ/ entries.\n"
                "ROB has %i free entries.\n"
                "IQ has %i free entries.\n",
                tid, free_rob_entries, free_iq_entries);

        blockThisCycle = true;
        if(source == ROB){
            ++stats.blockingROBFull;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingROBFullS;
            } else {
                ++stats.blockingROBFullW;
            }
        }else{
            ++stats.blockingIQFull;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingIQFullS;
            } else {
                ++stats.blockingIQFullW;
            }
        }  

        block(tid);

        incrFullStat(source,tid);

        return;
    } else if (min_free_entries < insts_available) {
        DPRINTF(Rename,
                "[tid:%i] "
                "Will have to block this cycle. "
                "%i insts available, "
                "but only %i insts can be renamed due to ROB/IQ/LSQ limits.\n",
                tid, insts_available, min_free_entries);

        insts_available = min_free_entries;

        blockThisCycle = true;
        if(source == ROB){
            ++stats.blockingROBFull;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingROBFullS;
            } else {
                ++stats.blockingROBFullW;
            }
        }else{
            ++stats.blockingIQFull;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingIQFullS;
            } else {
                ++stats.blockingIQFullW;
            }
        }  
        incrFullStat(source,tid);
    }

    InstQueue &insts_to_rename = renameStatus[tid] == Unblocking ?
        skidBuffer[tid] : insts[tid];

    DPRINTF(Rename,
            "[tid:%i] "
            "%i available instructions to send iew.\n",
            tid, insts_available);

    DPRINTF(Rename,
            "[tid:%i] "
            "%i insts pipelining from Rename | "
            "%i insts dispatched to IQ last cycle.\n",
            tid, instsInProgress[tid], fromIEW->iewInfo[tid].dispatched);

    // Handle serializing the next instruction if necessary.
    if (serializeOnNextInst[tid]) {
        if (emptyROB[tid] && instsInProgress[tid] == 0) {
            // ROB already empty; no need to serialize.
            serializeOnNextInst[tid] = false;
        } else if (!insts_to_rename.empty()) {
            insts_to_rename.front()->setSerializeBefore();
        }
    }

    int renamed_insts = 0;
    int renamed_insts_s = 0;

    while (insts_available > 0 &&  renamed_insts_s < renameWidth) {
        DPRINTF(Rename, "[tid:%i] Sending instructions to IEW.\n", tid);

        assert(!insts_to_rename.empty());

        DynInstPtr inst = insts_to_rename.front();

        //For all kind of instructions, check ROB and IQ first For load
        //instruction, check LQ size and take into account the inflight loads
        //For store instruction, check SQ size and take into account the
        //inflight stores

        if (inst->isLoad()) {
            if (calcFreeLQEntries(tid) <= 0) {
                DPRINTF(Rename, "[tid:%i] Cannot rename due to no free LQ\n",
                        tid);
                source = LQ;
                incrFullStat(source,tid);
                break;
            }
        }

        if (inst->isStore() || inst->isAtomic()) {
            if (calcFreeSQEntries(tid) <= 0) {
                DPRINTF(Rename, "[tid:%i] Cannot rename due to no free SQ\n",
                        tid);
                source = SQ;
                incrFullStat(source,tid);
                break;
            }
        }

        insts_to_rename.pop_front();

        if (renameStatus[tid] == Unblocking) {
            DPRINTF(Rename,
                    "[tid:%i] "
                    "Removing [sn:%llu] PC:%s from rename skidBuffer\n",
                    tid, inst->seqNum, inst->pcState());
        }

        if (inst->isSquashed()) {
            DPRINTF(Rename,
                    "[tid:%i] "
                    "instruction %i with PC %s is squashed, skipping.\n",
                    tid, inst->seqNum, inst->pcState());

            ++stats.squashedInsts;

            // Decrement how many instructions are available.
            --insts_available;

            continue;
        }

        DPRINTF(Rename,
                "[tid:%i] "
                "Processing instructions here [sn:%llu] with PC %s.\n",
                tid, inst->seqNum, inst->pcState());

        // Check here to make sure there are enough destination registers
        // to rename to.  Otherwise block.
        if (!renameMap[tid]->canRename(inst)) {
            DPRINTF(Rename,
                    "Blocking due to "
                    " lack of free physical registers to rename to.\n");

            blockThisCycle = true;
            insts_to_rename.push_front(inst);
            ++stats.blockingRegFull;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingRegFullS;
            } else {
                ++stats.blockingRegFullW;
            }
            ++stats.fullRegistersEvents;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
                ++stats.fullRegistersEventsS;
            else
                ++stats.fullRegistersEventsW;

            break;
        }

        // Handle serializeAfter/serializeBefore instructions.
        // serializeAfter marks the next instruction as serializeBefore.
        // serializeBefore makes the instruction wait in rename until the ROB
        // is empty.

        // In this model, IPR accesses are serialize before
        // instructions, and store conditionals are serialize after
        // instructions.  This is mainly due to lack of support for
        // out-of-order operations of either of those classes of
        // instructions.
        if (inst->isSerializeBefore() && !inst->isSerializeHandled()) {
            DPRINTF(Rename, "Serialize before instruction encountered.\n");

            if (!inst->isTempSerializeBefore()) {
                stats.serializing++;
                inst->setSerializeHandled();
            } else {
                stats.tempSerializing++;
            }

            // Change status over to SerializeStall so that other stages know
            // what this is blocked on.
            renameStatus[tid] = SerializeStall;

            serializeInst[tid] = inst;

            blockThisCycle = true;
            ++stats.blockingSerialized;
            if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
                ++stats.blockingSerializedS;
            } else {
                ++stats.blockingSerializedW;
            }

            break;
        } else if ((inst->isStoreConditional() || inst->isSerializeAfter()) &&
                   !inst->isSerializeHandled()) {
            DPRINTF(Rename, "Serialize after instruction encountered.\n");

            stats.serializing++;

            inst->setSerializeHandled();

            serializeAfter(insts_to_rename, tid);
        }

        DPRINTF(Rename,
                "[tid:%i] "
                "Processing instructions2 here [sn:%llu] with PC %s.\n",
                tid, inst->seqNum, inst->pcState());

        renameSrcRegs(inst, inst->threadNumber);

        renameDestRegs(inst, inst->threadNumber);

        if (inst->isAtomic() || inst->isStore()) {
            storesInProgress[tid]++;
        } else if (inst->isLoad()) {
            loadsInProgress[tid]++;
        }

        DPRINTF(Rename,
                "[tid:%i] "
                "Processing instructions3 here [sn:%llu] with PC %s.\n",
                tid, inst->seqNum, inst->pcState());

        ++renamed_insts;
        ++rename_vals_sent;
        if(tid == 0) {
            ++rename_vals_0_sent;
        } else {
            ++rename_vals_1_sent;
        }
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            ++renamed_insts_s;
        }
        // Notify potential listeners that source and destination registers for
        // this instruction have been renamed.
        ppRename->notify(inst);

        // Put instruction in rename queue.
        inst->cycleRenamed = cpu->curCycle();
        toIEW->insts[toIEWIndex] = inst;
        ++(toIEW->size);


        // Increment which instruction we're on.
        ++toIEWIndex;

        // Decrement how many instructions are available.
        --insts_available;
    }

    instsInProgress[tid] += renamed_insts;
    stats.renamedInsts += renamed_insts;

    // If we wrote to the time buffer, record this.
    if (toIEWIndex) {
        wroteToTimeBuffer = true;
    }

    // Check if there's any instructions left that haven't yet been renamed.
    // If so then block.
    if (insts_available) {
        blockThisCycle = true;
        ++stats.blockingBandwidthFull;
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            ++stats.blockingBandwidthFullS;
        } else {
            ++stats.blockingBandwidthFullW;
        }
    }

    if (blockThisCycle) {
        block(tid);
        toDecode->renameUnblock[tid] = false;
    }
}

void
Rename::skidInsert(ThreadID tid)
{
    DynInstPtr inst = NULL;
    while (!insts[tid].empty()) {
        inst = insts[tid].front();

        insts[tid].pop_front();

        assert(tid == inst->threadNumber);

        DPRINTF(Rename, "[tid:%i] Inserting [sn:%llu] PC: %s into Rename "
                "skidBuffer\n", tid, inst->seqNum, inst->pcState());

        ++stats.skidInsts;

        skidBuffer[tid].push_back(inst);
    }
    DPRINTF(Rename, "[tid:%i] skidBuffer size: %i, skidBufferMax: %i\n", tid, skidBuffer[tid].size(), skidBufferMax);
    if (skidBuffer[tid].size() > skidBufferMax) {
        InstQueue::iterator it;
        warn("Skidbuffer contents:\n");
        for (it = skidBuffer[tid].begin(); it != skidBuffer[tid].end(); it++) {
            warn("[tid:%i] %s [sn:%llu].\n", tid,
                    (*it)->staticInst->disassemble(
                        inst->pcState().instAddr()),
                    (*it)->seqNum);
        }
        panic("Skidbuffer Exceeded Max Size");
    }
}

void
Rename::sortInsts()
{
    int insts_from_decode = fromDecode->size;
    for (int i = 0; i < insts_from_decode; ++i) {
        const DynInstPtr &inst = fromDecode->insts[i];
        insts[inst->threadNumber].push_back(inst);
        //Daniel debug
        DPRINTF(Rename, "[tid:%i] insts[] size: %i\n", inst->threadNumber, insts[inst->threadNumber].size());
        if(inst->threadNumber == 4)
        {
            DPRINTF(Rename, "FROM rename putting in queue\n");
        }
#if TRACING_ON
        if (debug::O3PipeView) {
            inst->renameTick = curTick() - inst->fetchTick;
        }
#endif
    }
}

bool
Rename::skidsEmpty()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (!skidBuffer[tid].empty())
            return false;
    }

    return true;
}

void
Rename::updateStatus()
{
    bool any_unblocking = false;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (renameStatus[tid] == Unblocking) {
            any_unblocking = true;
            break;
        }
    }

    // Rename will have activity if it's unblocking.
    if (any_unblocking) {
        if (_status == Inactive) {
            _status = Active;

            DPRINTF(Activity, "Activating stage.\n");

            cpu->activateStage(CPU::RenameIdx);
        }
    } else {
        // If it's not unblocking, then rename will not have any internal
        // activity.  Switch it to inactive.
        if (_status == Active) {
            _status = Inactive;
            DPRINTF(Activity, "Deactivating stage.\n");
            ++stats.renameDeactivate;
            cpu->deactivateStage(CPU::RenameIdx);
        }
    }
}

bool
Rename::block(ThreadID tid)
{
    DPRINTF(Rename, "[tid:%i] Blocking. status %d resumeUnblocking %d Blocked %d\n", tid,renameStatus[tid],resumeUnblocking,Blocked);

    // Add the current inputs onto the skid buffer, so they can be
    // reprocessed when this stage unblocks.
    skidInsert(tid);

    // Only signal backwards to block if the previous stages do not think
    // rename is already blocked.

    if (renameStatus[tid] != Blocked) {
        // If resumeUnblocking is set, we unblocked during the squash,
        // but now we're have unblocking status. We need to tell earlier
        // stages to block.
        if (resumeUnblocking || renameStatus[tid] != Unblocking) {
            toDecode->renameBlock[tid] = true;
            toDecode->renameUnblock[tid] = false;
            wroteToTimeBuffer = true;
        }

        // Rename can not go from SerializeStall to Blocked, otherwise
        // it would not know to complete the serialize stall.
        if (renameStatus[tid] != SerializeStall) {
            // Set status to Blocked.
            renameStatus[tid] = Blocked;
            return true;
        }
    }

    return false;
}

bool
Rename::unblock(ThreadID tid)
{
    DPRINTF(Rename, "[tid:%i] Trying to unblock.\n", tid);
    // Rename is done unblocking if the skid buffer is empty.
    if (skidBuffer[tid].empty() && renameStatus[tid] != SerializeStall) {

        DPRINTF(Rename, "[tid:%i] Done unblocking.\n", tid);
        toDecode->renameUnblock[tid] = true;
        wroteToTimeBuffer = true;

        renameStatus[tid] = Running;
        return true;
    }

    return false;
}

void
Rename::doSquash(const InstSeqNum &squashed_seq_num, ThreadID tid)
{
    auto hb_it = historyBuffer[tid].begin();

    // After a syscall squashes everything, the history buffer may be empty
    // but the ROB may still be squashing instructions.
    // Go through the most recent instructions, undoing the mappings
    // they did and freeing up the registers.
    while (!historyBuffer[tid].empty() &&
           hb_it->instSeqNum > squashed_seq_num) {
        assert(hb_it != historyBuffer[tid].end());

        DPRINTF(Rename, "[tid:%i] Removing history entry with sequence "
                "number %i (archReg: %d , newPhysReg: %d , prevPhysReg: %d ).\n",
                tid, hb_it->instSeqNum, hb_it->archReg.index(),
                hb_it->newPhysReg->index(), hb_it->prevPhysReg->index());

        // Undo the rename mapping only if it was really a change.
        // Special regs that are not really renamed (like misc regs
        // and the zero reg) can be recognized because the new mapping
        // is the same as the old one.  While it would be merely a
        // waste of time to update the rename table, we definitely
        // don't want to put these on the free list.
        // only add a register back to the free list if it is a S thread. For W thread we never rename and once a mapping exists, it is never changed, so we never free these registers
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            if (hb_it->newPhysReg != hb_it->prevPhysReg) {
                // Tell the rename map to set the architected register to the
                // previous physical register that it was renamed to.
                renameMap[tid]->setEntry(hb_it->archReg, hb_it->prevPhysReg);

                // Put the renamed physical register back on the free list.
                freeList->addReg(hb_it->newPhysReg);
            }
        }

        // Notify potential listeners that the register mapping needs to be
        // removed because the instruction it was mapped to got squashed. Note
        // that this is done before hb_it is incremented.
        ppSquashInRename->notify(std::make_pair(hb_it->instSeqNum,
                                                hb_it->newPhysReg));

        historyBuffer[tid].erase(hb_it++);

        ++stats.undoneMaps;
    }
}

void
Rename::removeFromHistory(InstSeqNum inst_seq_num, ThreadID tid)
{
    DPRINTF(Rename, "[tid:%i] Removing a committed instruction from the "
            "history buffer %u (size=%i), until [sn:%llu].\n",
            tid, tid, historyBuffer[tid].size(), inst_seq_num);

    auto hb_it = historyBuffer[tid].end();

    --hb_it;

    if (historyBuffer[tid].empty()) {
        DPRINTF(Rename, "[tid:%i] History buffer is empty.\n", tid);
        return;
    } else if (hb_it->instSeqNum > inst_seq_num) {
        DPRINTF(Rename, "[tid:%i] [sn:%llu] "
                "Old sequence number encountered. "
                "Ensure that a syscall happened recently.\n",
                tid,inst_seq_num);
        return;
    }

    // Commit all the renames up until (and including) the committed sequence
    // number. Some or even all of the committed instructions may not have
    // rename histories if they did not have destination registers that were
    // renamed.
    while (!historyBuffer[tid].empty() &&
           hb_it != historyBuffer[tid].end() &&
           hb_it->instSeqNum <= inst_seq_num) {

        // Don't free special phys regs like misc and zero regs, which
        // can be recognized because the new mapping is the same as
        // the old one.
        // only add a register back to the free list if it is a S thread. For W thread we never rename and once a mapping exists, it is never changed, so we never free these registers
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            DPRINTF(Rename, "[tid:%i] Freeing up older rename of reg %i (%s), "
                "[sn:%llu].\n",
                tid, hb_it->prevPhysReg->index(),
                hb_it->prevPhysReg->className(),
                hb_it->instSeqNum);

            if (hb_it->newPhysReg != hb_it->prevPhysReg) {
                freeList->addReg(hb_it->prevPhysReg);
            }
        }

        ++stats.committedMaps;

        historyBuffer[tid].erase(hb_it--);
    }
}

void
Rename::renameSrcRegs(const DynInstPtr &inst, ThreadID tid)
{
    gem5::ThreadContext *tc = inst->tcBase();
    UnifiedRenameMap *map = renameMap[tid];
    unsigned num_src_regs = inst->numSrcRegs();
    auto *isa = tc->getIsaPtr();

     DPRINTF(Rename,
                "[tid:%i] "
                "Processing instructions here srcs [sn:%llu] with PC %s numsrc %d.\n",
                tid, inst->seqNum, inst->pcState(),num_src_regs);

    // Get the architectual register numbers from the source and
    // operands, and redirect them to the right physical register.
    for (int src_idx = 0; src_idx < num_src_regs; src_idx++) {
        const RegId& src_reg = inst->srcRegIdx(src_idx);
        const RegId flat_reg = src_reg.flatten(*isa);
        PhysRegIdPtr renamed_reg;

        renamed_reg = map->lookup(flat_reg);
        switch (flat_reg.classValue()) {
          case InvalidRegClass:
            break;
          case IntRegClass:
            stats.intLookups++;
            break;
          case FloatRegClass:
            stats.fpLookups++;
            break;
          case VecRegClass:
          case VecElemClass:
            stats.vecLookups++;
            break;
          case VecPredRegClass:
            stats.vecPredLookups++;
            break;
          case CCRegClass:
          case MiscRegClass:
            break;

          default:
            panic("Invalid register class: %d.", flat_reg.classValue());
        }

        DPRINTF(Rename,
                "[tid:%i] "
                "Looking up %s arch reg %i, got phys reg %i (%s)\n",
                tid, flat_reg.className(),
                src_reg.index(), renamed_reg->index(),
                renamed_reg->className());

        inst->renameSrcReg(src_idx, renamed_reg);

        // See if the register is ready or not. INORDER
        if (scoreboard->getReg(renamed_reg) && (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || renamed_reg->isFixedMapping())) {
            DPRINTF(Rename,
                    "[tid:%i] [sn:%d]"
                    "MARKING_HERE Register %d (flat: %d) (%s) is ready tid_type %d fixed_mapping %d.\n",
                    tid, inst->seqNum, renamed_reg->index(), renamed_reg->flatIndex(),
                    renamed_reg->className(),cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType(),renamed_reg->isFixedMapping());

            if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
                inst->markSrcRegReady(src_idx);
            } else {
                inst->markSrcRegReadyW(src_idx);
            } 
        } else {
            DPRINTF(Rename,
                    "[tid:%i] "
                    "Register %d (flat: %d) (%s) is not ready.\n",
                    tid, renamed_reg->index(), renamed_reg->flatIndex(),
                    renamed_reg->className());
        }

        ++stats.lookups;
    }
}

void
Rename::renameDestRegs(const DynInstPtr &inst, ThreadID tid)
{
    gem5::ThreadContext *tc = inst->tcBase();
    UnifiedRenameMap *map = renameMap[tid];
    unsigned num_dest_regs = inst->numDestRegs();
    auto *isa = tc->getIsaPtr();

    DPRINTF(Rename,
                "[tid:%i] "
                "Processing instructions here dests [sn:%llu] with PC %s numdest %d.\n",
                tid, inst->seqNum, inst->pcState(),num_dest_regs);

    // Rename the destination registers.
    for (int dest_idx = 0; dest_idx < num_dest_regs; dest_idx++) {
        const RegId& dest_reg = inst->destRegIdx(dest_idx);
        UnifiedRenameMap::RenameInfo rename_result;

        RegId flat_dest_regid = dest_reg.flatten(*isa);
        flat_dest_regid.setNumPinnedWrites(dest_reg.getNumPinnedWrites());

        // stop renaming for W threads -> the dest reg rename function maintains the original mapping. We pass if the thread is wimpy or not to this function.
        bool isWThread = (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak);

        rename_result = map->rename(flat_dest_regid, isWThread, tid, inst->seqNum);

        // For weak threads, we store the pinned values here. Since we can have WAW hazards, we dont want 
        // to overwrite pinned values.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) { 

            inst->setNumPinnedWrites(flat_dest_regid.getNumPinnedWrites(),dest_idx);
            inst->setNumPinnedWritesToComplete(flat_dest_regid.getNumPinnedWrites() + 1,dest_idx);
            DPRINTF(IQ, "[tid:%d] RENAME_PLACE_INST_CHECK [sn:%llu] Renaming reg %d numPinnedWrites %d archwrite %d isPinned %d\n",
                    tid,inst->seqNum,rename_result.first->flatIndex(), inst->getNumPinnedWritesToComplete(dest_idx),flat_dest_regid.getNumPinnedWrites(),inst->isPinned(dest_idx));

        }

        inst->flattenedDestIdx(dest_idx, flat_dest_regid);

        scoreboard->unsetReg(rename_result.first);

        DPRINTF(Rename,
                "[tid:%i] "
                "Renaming arch reg %i (%s) to physical reg %i (%i).\n",
                tid, dest_reg.index(), dest_reg.className(),
                rename_result.first->index(),
                rename_result.first->flatIndex());

        // Record the rename information so that a history can be kept.
        RenameHistory hb_entry(inst->seqNum, flat_dest_regid,
                               rename_result.first,
                               rename_result.second);

        historyBuffer[tid].push_front(hb_entry);

        DPRINTF(Rename, "[tid:%i] [sn:%llu] "
                "Adding instruction to history buffer (size=%i).\n",
                tid,(*historyBuffer[tid].begin()).instSeqNum,
                historyBuffer[tid].size());

        // Tell the instruction to rename the appropriate destination
        // register (dest_idx) to the new physical register
        // (rename_result.first), and record the previous physical
        // register that the same logical register was renamed to
        // (rename_result.second).
        inst->renameDestReg(dest_idx,
                            rename_result.first,
                            rename_result.second);

        ++stats.renamedOperands;
    }
}

int
Rename::calcFreeROBEntries(ThreadID tid)
{
    int num_free = freeEntries[tid].robEntries -
                  (instsInProgress[tid] - fromIEW->iewInfo[tid].dispatched);

    DPRINTF(Rename,"[tid:%i] %i rob free\n",tid,num_free);

    return num_free;
}

int
Rename::calcFreeIQEntries(ThreadID tid)
{
    int num_free = freeEntries[tid].iqEntries -
                  (instsInProgress[tid] - fromIEW->iewInfo[tid].dispatched);

    DPRINTF(Rename,"[tid:%i] %i iq free\n",tid,num_free);

    return num_free;
}

int
Rename::calcFreeLQEntries(ThreadID tid)
{
        int num_free = freeEntries[tid].lqEntries -
            (loadsInProgress[tid] - fromIEW->iewInfo[tid].dispatchedToLQ);
        DPRINTF(Rename,
                "calcFreeLQEntries: free lqEntries: %d, loadsInProgress: %d, "
                "loads dispatchedToLQ: %d\n",
                freeEntries[tid].lqEntries, loadsInProgress[tid],
                fromIEW->iewInfo[tid].dispatchedToLQ);
        return num_free;
}

int
Rename::calcFreeSQEntries(ThreadID tid)
{
        int num_free = freeEntries[tid].sqEntries -
            (storesInProgress[tid] - fromIEW->iewInfo[tid].dispatchedToSQ);
        DPRINTF(Rename, "calcFreeSQEntries: free sqEntries: %d, "
                "storesInProgress: %d, stores dispatchedToSQ: %d\n",
                freeEntries[tid].sqEntries, storesInProgress[tid],
                fromIEW->iewInfo[tid].dispatchedToSQ);
        return num_free;
}

unsigned
Rename::validInsts()
{
    unsigned inst_count = 0;

    for (int i=0; i<fromDecode->size; i++) {
        if (!fromDecode->insts[i]->isSquashed())
            inst_count++;
    }

    return inst_count;
}

void
Rename::readStallSignals(ThreadID tid)
{
    if (fromIEW->iewBlock[tid]) {
        stalls[tid].iew = true;
    }

    if (fromIEW->iewUnblock[tid]) {
        assert(stalls[tid].iew);
        stalls[tid].iew = false;
    }
}

bool
Rename::checkStall(ThreadID tid)
{
    bool ret_val = false;
    bool isSType = (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong);

    if (stalls[tid].iew) {
        DPRINTF(Rename,"[tid:%i] Stall from IEW stage detected.\n", tid);
        ret_val = true;
        if(isSType)
            ++stats.iewStallS;
        else    
            ++stats.iewStallW;
    } else if (calcFreeROBEntries(tid) <= 0) {
        DPRINTF(Rename,"[tid:%i] Stall: ROB has 0 free entries.\n", tid);
        ret_val = true;
        if(isSType)
            ++stats.NoROBFreeS;
        else    
            ++stats.NoROBFreeW;
    } else if (calcFreeIQEntries(tid) <= 0) {
        DPRINTF(Rename,"[tid:%i] Stall: IQ has 0 free entries.\n", tid);
        ret_val = true;
        if(isSType)
            ++stats.NoIQFreeS;
        else    
            ++stats.NoIQFreeW;
    } else if (calcFreeLQEntries(tid) <= 0 && calcFreeSQEntries(tid) <= 0) {
        DPRINTF(Rename,"[tid:%i] Stall: LSQ has 0 free entries.\n", tid);
        ret_val = true;
        if(isSType)
            ++stats.NoLSQFreeS;
        else    
            ++stats.NoLSQFreeW;
    } else if (renameMap[tid]->numFreeEntries() <= 0) {
        DPRINTF(Rename,"[tid:%i] Stall: RenameMap has 0 free entries renameMap[tid].numFreeEntries() %d\n", tid,renameMap[tid]->numFreeEntries());
        ret_val = true;
        if(isSType)
            ++stats.NoRenameFreeS;
        else    
            ++stats.NoRenameFreeW;
    } else if (renameStatus[tid] == SerializeStall &&
               (!emptyROB[tid] || instsInProgress[tid])) {
        DPRINTF(Rename,"[tid:%i] Stall: Serialize stall and ROB is not "
                "empty.\n",
                tid);
        ret_val = true;
        if(isSType)
            ++stats.SerializeROBFullS;
        else    
            ++stats.SerializeROBFullW;
    }

    return ret_val;
}

void
Rename::readFreeEntries(ThreadID tid)
{
    if (fromIEW->iewInfo[tid].usedIQ)
        freeEntries[tid].iqEntries = fromIEW->iewInfo[tid].freeIQEntries;

    if (fromIEW->iewInfo[tid].usedLSQ) {
        freeEntries[tid].lqEntries = fromIEW->iewInfo[tid].freeLQEntries;
        freeEntries[tid].sqEntries = fromIEW->iewInfo[tid].freeSQEntries;
    }

    if (fromCommit->commitInfo[tid].usedROB) {
        freeEntries[tid].robEntries =
            fromCommit->commitInfo[tid].freeROBEntries;
        emptyROB[tid] = fromCommit->commitInfo[tid].emptyROB;
    }

    DPRINTF(Rename, "[tid:%i] Free IQ: %i, Free ROB: %i, "
                    "Free LQ: %i, Free SQ: %i, FreeRM %i(%i %i %i %i %i %i)\n",
            tid,
            freeEntries[tid].iqEntries,
            freeEntries[tid].robEntries,
            freeEntries[tid].lqEntries,
            freeEntries[tid].sqEntries,
            renameMap[tid]->numFreeEntries(),
            renameMap[tid]->numFreeEntries(IntRegClass),
            renameMap[tid]->numFreeEntries(FloatRegClass),
            renameMap[tid]->numFreeEntries(VecRegClass),
            renameMap[tid]->numFreeEntries(VecElemClass),
            renameMap[tid]->numFreeEntries(VecPredRegClass),
            renameMap[tid]->numFreeEntries(CCRegClass));

    DPRINTF(Rename, "[tid:%i] %i instructions not yet in ROB\n",
            tid, instsInProgress[tid]);
}

bool
Rename::checkSignalsAndUpdate(ThreadID tid)
{
    // Check if there's a squash signal, squash if there is
    // Check stall signals, block if necessary.
    // If status was blocked
    //     check if stall conditions have passed
    //         if so then go to unblocking
    // If status was Squashing
    //     check if squashing is not high.  Switch to running this cycle.
    // If status was serialize stall
    //     check if ROB is empty and no insts are in flight to the ROB

    bool isSType = (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong);

    readFreeEntries(tid);
    readStallSignals(tid);

    DPRINTF(Rename,"[tid:%d] CHECKING STATUS OF THREAD renameStatus[tid]: %d squash %d checkStall(tid) %d\n",tid,renameStatus[tid],fromCommit->commitInfo[tid].squash,checkStall(tid));

    if (fromCommit->commitInfo[tid].squash) {
        DPRINTF(Rename, "[tid:%i] Squashing instructions due to squash from "
                "commit.\n", tid);

        squash(fromCommit->commitInfo[tid].doneSeqNum, tid);

        return true;
    }

    if (checkStall(tid)) {
        return block(tid);
    }

    if (renameStatus[tid] == Blocked) {
        DPRINTF(Rename, "[tid:%i] Done blocking, switching to unblocking.\n",
                tid);

        renameStatus[tid] = Unblocking;

        unblock(tid);

        return true;
    }

    if (renameStatus[tid] == Squashing) {

        // Switch status to running if rename isn't being told to block or
        // squash this cycle.
        if (resumeSerialize) {
            DPRINTF(Rename,
                    "[tid:%i] Done squashing, switching to serialize.\n", tid);

            renameStatus[tid] = SerializeStall;
            return true;
        } else if (resumeUnblocking) {
            DPRINTF(Rename,
                    "[tid:%i] Done squashing, switching to unblocking.\n",
                    tid);
            renameStatus[tid] = Unblocking;
            return true;
        } else {
            DPRINTF(Rename, "[tid:%i] Done squashing, switching to running.\n",
                    tid);
            renameStatus[tid] = Running;
            return false;
        }
    }

    if (renameStatus[tid] == SerializeStall) {
        // Stall ends once the ROB is free.
        DPRINTF(Rename, "[tid:%i] Done with serialize stall, switching to "
                "unblocking.\n", tid);

        DynInstPtr serial_inst = serializeInst[tid];

        renameStatus[tid] = Unblocking;

        unblock(tid);

        DPRINTF(Rename, "[tid:%i] Processing instruction [%lli] with "
                "PC %s.\n", tid, serial_inst->seqNum, serial_inst->pcState());

        // Put instruction into queue here.
        serial_inst->clearSerializeBefore();

        if (!skidBuffer[tid].empty()) {
            skidBuffer[tid].push_front(serial_inst);
        } else {
            insts[tid].push_front(serial_inst);
        }

        DPRINTF(Rename, "[tid:%i] Instruction must be processed by rename."
                " Adding to front of list.\n", tid);

        serializeInst[tid] = NULL;

        return true;
    }

    // If we've reached this point, we have not gotten any signals that
    // cause rename to change its status.  Rename remains the same as before.
    return false;
}

void
Rename::serializeAfter(InstQueue &inst_list, ThreadID tid)
{
    if (inst_list.empty()) {
        // Mark a bit to say that I must serialize on the next instruction.
        serializeOnNextInst[tid] = true;
        return;
    }

    // Set the next instruction as serializing.
    inst_list.front()->setSerializeBefore();
}

void
Rename::incrFullStat(const FullSource &source, ThreadID tid)
{

    bool isSType = (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong);
    switch (source) {
      case ROB:
        ++stats.ROBFullEvents;
        if(isSType)
            ++stats.ROBFullEventsS;
        else
            ++stats.ROBFullEventsW;
        break;
      case IQ:
        ++stats.IQFullEvents;
        if(isSType)
            ++stats.IQFullEventsS;
        else
            ++stats.IQFullEventsW;
        break;
      case LQ:
        ++stats.LQFullEvents;
        if(isSType)
            ++stats.LQFullEventsS;
        else
            ++stats.LQFullEventsW;
        break;
      case SQ:
        ++stats.SQFullEvents;
        if(isSType)
            ++stats.SQFullEventsS;
        else
            ++stats.SQFullEventsW;
        break;
      default:
        panic("Rename full stall stat should be incremented for a reason!");
        break;
    }
}

void
Rename::dumpHistory()
{
    std::list<RenameHistory>::iterator buf_it;

    for (ThreadID tid = 0; tid < numThreads; tid++) {

        buf_it = historyBuffer[tid].begin();

        while (buf_it != historyBuffer[tid].end()) {
            cprintf("Seq num: %i\nArch reg[%s]: %i New phys reg:"
                    " %i[%s] Old phys reg: %i[%s]\n",
                    (*buf_it).instSeqNum,
                    (*buf_it).archReg.className(),
                    (*buf_it).archReg.index(),
                    (*buf_it).newPhysReg->index(),
                    (*buf_it).newPhysReg->className(),
                    (*buf_it).prevPhysReg->index(),
                    (*buf_it).prevPhysReg->className());

            buf_it++;
        }
    }
}


///////////////////////////////////////
//                                   //
//  SMT FETCH POLICY MAINTAINED HERE //
//                                   //
///////////////////////////////////////

bool comparePairsRename(const std::pair<int, int>& pair1, const std::pair<int, int>& pair2) {
    return pair1.first < pair2.first; // Sort based on the first element of each pair
}

void
Rename::getRenamingThread()
{
    switch (renamePolicy) {
        case SMTFetchPolicy::SWIQCount:
        SWiqCountPriority();
    }
}


void 
Rename::SWiqCountPriority() { 
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::less<unsigned> > SQ;
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::less<unsigned> > WQ;
    std::map<unsigned, ThreadID> SthreadMap;
    std::map<unsigned, ThreadID> WthreadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    std::vector<std::pair<int, int>> ThreadAvailICountS;
    std::vector<std::pair<int, int>> ThreadAvailICountW;

    // create 2 lists for S threads and W threads 
    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = renameStatus[tid] == Unblocking ?
        skidBuffer[tid].size() : insts[tid].size();

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        {
            SQ.push(iqCount);
            SthreadMap[iqCount] = tid;
            ThreadAvailICountS.push_back(std::make_pair(iqCount,tid));
        }
        else
        {
            WQ.push(iqCount);
            WthreadMap[iqCount] = tid;
            ThreadAvailICountW.push_back(std::make_pair(iqCount,tid));
        }
    }

    std::sort(ThreadAvailICountS.begin(), ThreadAvailICountS.end(), comparePairsRename);
    std::sort(ThreadAvailICountW.begin(), ThreadAvailICountW.end(), comparePairsRename);

    for (const auto& pair : ThreadAvailICountS) {
        RenamePreference.push_back(pair.second);
    }
    for (const auto& pair : ThreadAvailICountW) {
        RenamePreference.push_back(pair.second);
    }
}


ThreadID
Rename::roundRobin()
{
    // std::list<ThreadID>::iterator pri_iter = priorityList.begin();
    // std::list<ThreadID>::iterator end      = priorityList.end();

    std::list<ThreadID>::iterator pri_iter = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    ThreadID high_pri;
    while (pri_iter != end) {
        high_pri = *pri_iter;
        assert(high_pri <= numThreads);

        if (renameStatus[high_pri] == Running ||
            renameStatus[high_pri] == Unblocking ||
            renameStatus[high_pri] == Idle) {

            priorityList.erase(pri_iter);
            priorityList.push_back(high_pri);

            return high_pri;
        }

        pri_iter++;
    }

    return InvalidThreadID;
}

ThreadID
Rename::SWiqCount()
{
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > SQ;
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > WQ;
    std::map<unsigned, ThreadID> SthreadMap;
    std::map<unsigned, ThreadID> WthreadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    // create 2 lists for S threads and W threads 
    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = renameStatus[tid] == Unblocking ?
        skidBuffer[tid].size() : insts[tid].size();

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        {
            SQ.push(iqCount);
            SthreadMap[iqCount] = tid;
        }
        else
        {
            WQ.push(iqCount);
            WthreadMap[iqCount] = tid;
        }
    }

    while (!SQ.empty()) {
        ThreadID high_pri = SthreadMap[SQ.top()];

        if (renameStatus[high_pri] == Running ||
            renameStatus[high_pri] == Unblocking ||
            renameStatus[high_pri] == Idle)
            return high_pri;
        else
            SQ.pop();
    }
    while (!WQ.empty()) {
        ThreadID high_pri = WthreadMap[WQ.top()];

        if (renameStatus[high_pri] == Running ||
            renameStatus[high_pri] == Unblocking ||
            renameStatus[high_pri] == Idle)
            return high_pri;
        else
            WQ.pop();
    }

    return InvalidThreadID;
}

ThreadID
Rename::iqCount()
{
    //sorted from lowest->highest
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > PQ;
    std::map<unsigned, ThreadID> threadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = fromIEW->iewInfo[tid].iqCount;

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        PQ.push(iqCount);
        threadMap[iqCount] = tid;
    }

    while (!PQ.empty()) {
        ThreadID high_pri = threadMap[PQ.top()];

        if (renameStatus[high_pri] == Running ||
            renameStatus[high_pri] == Unblocking ||
            renameStatus[high_pri] == Idle)
            return high_pri;
        else
            PQ.pop();

    }

    return InvalidThreadID;
}

ThreadID
Rename::lsqCount()
{
    //sorted from lowest->highest
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > PQ;
    std::map<unsigned, ThreadID> threadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned ldstqCount = fromIEW->iewInfo[tid].ldstqCount;

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        PQ.push(ldstqCount);
        threadMap[ldstqCount] = tid;
    }

    while (!PQ.empty()) {
        ThreadID high_pri = threadMap[PQ.top()];

        if (renameStatus[high_pri] == Running ||
            renameStatus[high_pri] == Unblocking ||
            renameStatus[high_pri] == Idle)
            return high_pri;
        else
            PQ.pop();
    }

    return InvalidThreadID;
}

} // namespace o3
} // namespace gem5
