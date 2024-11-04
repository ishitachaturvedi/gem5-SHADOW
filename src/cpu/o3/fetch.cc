/*
 * Copyright (c) 2010-2014 ARM Limited
 * Copyright (c) 2012-2013 AMD
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

#include "cpu/o3/fetch.hh"

#include <algorithm>
#include <cstring>
#include <list>
#include <map>
#include <queue>

#include "arch/generic/tlb.hh"
#include "base/random.hh"
#include "base/types.hh"
#include "cpu/base.hh"
#include "cpu/exetrace.hh"
#include "cpu/nop_static_inst.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/limits.hh"
#include "debug/Activity.hh"
#include "debug/Drain.hh"
#include "debug/Fetch.hh"
#include "debug/O3CPU.hh"
#include "debug/O3PipeView.hh"
#include "mem/packet.hh"
#include "params/BaseO3CPU.hh"
#include "sim/byteswap.hh"
#include "sim/core.hh"
#include "sim/eventq.hh"
#include "sim/full_system.hh"
#include "sim/system.hh"
#include "debug/pipelineView.hh"

namespace gem5
{

namespace o3
{

Fetch::IcachePort::IcachePort(Fetch *_fetch, CPU *_cpu, std::string icacheType, bool isSplitCache) :
        RequestPort(_cpu->name() + ".icache_" + icacheType + "_port", _cpu), fetch(_fetch)
{
    if(isSplitCache)
        isStrong = (icacheType == "strong");
    else   
        isStrong = true;
}


Fetch::Fetch(CPU *_cpu, const BaseO3CPUParams &params)
    : fetchPolicy(params.smtFetchPolicy),
      policyWeighting(params.smtPolicyWeight),
      cpu(_cpu),
      branchPred(nullptr),
      decodeToFetchDelay(params.decodeToFetchDelay),
      renameToFetchDelay(params.renameToFetchDelay),
      iewToFetchDelay(params.iewToFetchDelay),
      commitToFetchDelay(params.commitToFetchDelay),
      fetchWidth(params.fetchWidth),
      decodeWidth(params.decodeWidth),
      numDecodingThreads(params.smtNumDecodingThreads),
      retryPkt(NULL),
      retryTid(InvalidThreadID),
      cacheBlkSize(cpu->cacheLineSize()),
      fetchBufferSize(params.fetchBufferSize),
      fetchBufferMask(fetchBufferSize - 1),
      fetchQueueSize(params.fetchQueueSize),
      numThreads(params.numThreads),
      numFetchingThreads(params.smtNumFetchingThreads),
      predictOnWThreads(params.predictOnWThreads),
    //   icachePort(this, _cpu, "strong", params.UseSplitCache),
    //   icachePortS(this, _cpu, "strong", params.UseSplitCache),
    //   icachePortW(this, _cpu, "strong", params.UseSplitCache),
      icachePort(this, _cpu, "strong",params.UseSplitCache),
      icachePortS(this, _cpu, "strong",params.UseSplitCache),
      icachePortW(this, _cpu, "weak", params.UseSplitCache),
      UseSplitCache(params.UseSplitCache),
      SingleThreadFetchiew(params.SingleThreadFetchIEW),
      finishTranslationEvent(this), fetchStats(_cpu, this)
{

    if (numThreads > MaxThreads)
        fatal("numThreads (%d) is larger than compiled limit (%d),\n"
              "\tincrease MaxThreads in src/cpu/o3/limits.hh\n",
              numThreads, static_cast<int>(MaxThreads));
    if (fetchWidth > MaxWidth)
        fatal("fetchWidth (%d) is larger than compiled limit (%d),\n"
             "\tincrease MaxWidth in src/cpu/o3/limits.hh\n",
             fetchWidth, static_cast<int>(MaxWidth));
    if (fetchBufferSize > cacheBlkSize)
        fatal("fetch buffer size (%u bytes) is greater than the cache "
              "block size (%u bytes)\n", fetchBufferSize, cacheBlkSize);
    if (cacheBlkSize % fetchBufferSize)
        fatal("cache block (%u bytes) is not a multiple of the "
              "fetch buffer (%u bytes)\n", cacheBlkSize, fetchBufferSize);

    for (int i = 0; i < MaxThreads; i++) {
        fetchStatus[i] = Idle;
        DPRINTF(Fetch, "(Idle) FetchStatus Update: fetchStatus[%i] = %d\n", i, fetchStatus[i]);
        decoder[i] = nullptr;
        pc[i].reset(params.isa[0]->newPCState());
        fetchOffset[i] = 0;
        macroop[i] = nullptr;
        delayedCommit[i] = false;
        memReq[i] = nullptr;
        stalls[i] = {false, false};
        fetchBuffer[i] = NULL;
        fetchBufferPC[i] = 0;
        fetchBufferValid[i] = false;
        lastIcacheStall[i] = 0;
        issuePipelinedIfetch[i] = false;
        StalledOnConditionalBranch[i] = false;
    }

    branchPred = params.branchPred;

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        decoder[tid] = params.decoder[tid];
        // Create space to buffer the cache line data,
        // which may not hold the entire cache line.
        fetchBuffer[tid] = new uint8_t[fetchBufferSize];
    }

    // Get the size of an instruction.
    instSize = decoder[0]->moreBytesSize();
}

std::string Fetch::name() const { return cpu->name() + ".fetch"; }

void
Fetch::regProbePoints()
{
    ppFetch = new ProbePointArg<DynInstPtr>(cpu->getProbeManager(), "Fetch");
    ppFetchRequestSent = new ProbePointArg<RequestPtr>(cpu->getProbeManager(),
                                                       "FetchRequest");

}

Fetch::FetchStatGroup::FetchStatGroup(CPU *cpu, Fetch *fetch)
    : statistics::Group(cpu, "fetch"),
    ADD_STAT(icacheStallCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch is stalled on an Icache miss"),
    ADD_STAT(icacheStallCyclesSThread, statistics::units::Cycle::get(),
             "Number of cycles S threads fetch is stalled on an Icache miss"),
    ADD_STAT(icacheStallCyclesWThread, statistics::units::Cycle::get(),
             "Number of cycles W threads fetch is stalled on an Icache miss"),
    ADD_STAT(insts, statistics::units::Count::get(),
             "Number of instructions fetch has processed"),
    ADD_STAT(instsSThread, statistics::units::Count::get(),
             "Number of instructions fetch has processed"),
    ADD_STAT(instsWThread, statistics::units::Count::get(),
             "Number of instructions fetch has processed"),
    ADD_STAT(branches, statistics::units::Count::get(),
             "Number of branches that fetch encountered"),
    ADD_STAT(predictedBranches, statistics::units::Count::get(),
             "Number of branches that fetch has predicted taken"),
    ADD_STAT(cycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has run and was not squashing or "
             "blocked"),
    ADD_STAT(FetchNotValid, statistics::units::Count::get(),
             "How many times the fetch block is invalid"),
    ADD_STAT(FetchBufferExceeded, statistics::units::Count::get(),
             "Fetch Buffer Size Exceeded"),
    ADD_STAT(FetchQueueFull, statistics::units::Count::get(),
             "Fetch Queue full for a thread"),
    ADD_STAT(NeedToFetchMoreMemory, statistics::units::Count::get(),
             "Fetch needs more memory"),
    ADD_STAT(QuiescePendingForThread, statistics::units::Count::get(),
             "Quisence made fetch stop"),
    ADD_STAT(FetchQueueTryingToDecode, statistics::units::Count::get(),
             "Trying to send Fetch to Decode"),
    ADD_STAT(FetchQueueSendingToDecode, statistics::units::Count::get(),
             "Sending Fetch to Decode"),
    ADD_STAT(squashCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has spent squashing"),
    ADD_STAT(tlbCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has spent waiting for tlb"),
    ADD_STAT(idleCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch was idle"),
    ADD_STAT(noInstFetched, statistics::units::Cycle::get(),
             "NUmber of cycles with no instructions fetched"),
    ADD_STAT(blockedCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has spent blocked"),
    ADD_STAT(miscStallCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has spent waiting on interrupts, or bad "
             "addresses, or out of MSHRs"),
    ADD_STAT(pendingDrainCycles, statistics::units::Cycle::get(),
             "Number of cycles fetch has spent waiting on pipes to drain"),
    ADD_STAT(noActiveThreadStallCycles, statistics::units::Cycle::get(),
             "Number of stall cycles due to no active thread to fetch from"),
    ADD_STAT(pendingTrapStallCycles, statistics::units::Cycle::get(),
             "Number of stall cycles due to pending traps"),
    ADD_STAT(pendingQuiesceStallCycles, statistics::units::Cycle::get(),
             "Number of stall cycles due to pending quiesce instructions"),
    ADD_STAT(icacheWaitRetryStallCycles, statistics::units::Cycle::get(),
             "Number of stall cycles due to full MSHR"),
    ADD_STAT(icacheWaitRetryStallCyclesSThread, statistics::units::Cycle::get(),
             "Number of stall cycles due to full MSHR"),
    ADD_STAT(icacheWaitRetryStallCyclesWThread, statistics::units::Cycle::get(),
             "Number of stall cycles due to full MSHR"),
    ADD_STAT(cacheLines, statistics::units::Count::get(),
             "Number of cache lines fetched"),
    ADD_STAT(icacheSquashes, statistics::units::Count::get(),
             "Number of outstanding Icache misses that were squashed"),
    ADD_STAT(tlbSquashes, statistics::units::Count::get(),
             "Number of outstanding ITLB misses that were squashed"),
    ADD_STAT(nisnDist, statistics::units::Count::get(),
             "Number of instructions fetched each cycle (Total)"),
    ADD_STAT(idleRate, statistics::units::Ratio::get(),
             "Ratio of cycles fetch was idle",
             idleCycles / cpu->baseStats.numCycles),
    ADD_STAT(branchRate, statistics::units::Ratio::get(),
             "Number of branch fetches per cycle",
             branches / cpu->baseStats.numCycles),
    ADD_STAT(rate, statistics::units::Rate<
                    statistics::units::Count, statistics::units::Cycle>::get(),
             "Number of inst fetches per cycle",
             insts / cpu->baseStats.numCycles),
    ADD_STAT(stalledS, statistics::units::Count::get(),
            "Number of cycles all S threads are stalled"),
    ADD_STAT(stalledW, statistics::units::Count::get(),
            "Number of cycles all W threads are stalled"),
    ADD_STAT(stalledSNotW, statistics::units::Count::get(),
            "Number of cycles all S threads are stalled but not W threads"),
    ADD_STAT(stalledSAndW, statistics::units::Count::get(),
            "Number of cycles all S and W threads are stalled"),
    ADD_STAT(notStalled, statistics::units::Count::get(),
            "Number of cycles decode is not stalled"),
    ADD_STAT(multipleRunning, statistics::units::Count::get(),
            "Multiple threads running together"),
    ADD_STAT(NoThreadToFetch, statistics::units::Count::get(),
            "Cycle with no thread to fetch from"),
    ADD_STAT(NumFetchCycles, statistics::units::Count::get(),
            "Number of fetch cycles for each thread") ,
    ADD_STAT(FetchQueueEmpty, statistics::units::Count::get(),
            "Number of fetch cycles where fetch queue is empty"),
    ADD_STAT(DecodeWidthFull, statistics::units::Count::get(),
            "Number of fetch cycles thread could not move to decode because decode width full"),
    ADD_STAT(RunningCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state Running "),
    ADD_STAT(IdleCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state Idle "),  
    ADD_STAT(SquashingCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state Squashing "),  
    ADD_STAT(BlockedCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state Blocked "),  
    ADD_STAT(FetchingCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state Fetching "),  
    ADD_STAT(TrapPendingCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state TrapPending "),  
    ADD_STAT(QuiescePendingCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state QuiescePending "),  
    ADD_STAT(ItlbWaitCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state ItlbWait "),  
    ADD_STAT(IcacheWaitResponseCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state IcacheWaitResponse "),  
    ADD_STAT(IcacheWaitRetryCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state IcacheWaitRetry "),  
    ADD_STAT(IcacheAccessCompleteCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state IcacheAccessComplete "),  
    ADD_STAT(NoGoodAddrCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state NoGoodAddr "),  
    ADD_STAT(BlockedOnBranchCount, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state BlockedOnBranch "),
    ADD_STAT(BlockedOnBranchCountNoSThread, statistics::units::Count::get(),
            "Number of fetch cycles with fetch state BlockedOnBranch and no S threads are active")
{
        NumFetchCycles
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchQueueEmpty
            .init(cpu->numThreads)
            .flags(statistics::total);
        DecodeWidthFull
            .init(cpu->numThreads)
            .flags(statistics::total);
        RunningCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchNotValid
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchBufferExceeded
            .init(cpu->numThreads)
            .flags(statistics::total);
        IdleCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchQueueFull
            .init(cpu->numThreads)
            .flags(statistics::total);
        NeedToFetchMoreMemory
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchQueueTryingToDecode
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchQueueSendingToDecode
            .init(cpu->numThreads)
            .flags(statistics::total);
        QuiescePendingForThread
            .init(cpu->numThreads)
            .flags(statistics::total);
        SquashingCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        BlockedCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        FetchingCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        TrapPendingCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        QuiescePendingCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        ItlbWaitCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        IcacheWaitResponseCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        IcacheWaitRetryCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        IcacheAccessCompleteCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        NoGoodAddrCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        BlockedOnBranchCount
            .init(cpu->numThreads)
            .flags(statistics::total);
        BlockedOnBranchCountNoSThread
            .init(cpu->numThreads)
            .flags(statistics::total);
        icacheStallCycles
            .prereq(icacheStallCycles);
        icacheStallCyclesSThread
            .prereq(icacheStallCyclesSThread);
        icacheStallCyclesWThread
            .prereq(icacheStallCyclesWThread);
        insts
            .prereq(insts);
        instsSThread
            .prereq(instsSThread);
        instsWThread
            .prereq(instsWThread);
        branches
            .prereq(branches);
        predictedBranches
            .prereq(predictedBranches);
        cycles
            .prereq(cycles);
        squashCycles
            .prereq(squashCycles);
        tlbCycles
            .prereq(tlbCycles);
        // idleCycles
        //     .prereq(idleCycles);
        blockedCycles
            .prereq(blockedCycles);
        cacheLines
            .prereq(cacheLines);
        miscStallCycles
            .prereq(miscStallCycles);
        pendingDrainCycles
            .prereq(pendingDrainCycles);
        noActiveThreadStallCycles
            .prereq(noActiveThreadStallCycles);
        pendingTrapStallCycles
            .prereq(pendingTrapStallCycles);
        pendingQuiesceStallCycles
            .prereq(pendingQuiesceStallCycles);
        icacheWaitRetryStallCycles
            .prereq(icacheWaitRetryStallCycles);
        icacheWaitRetryStallCyclesSThread
            .prereq(icacheWaitRetryStallCyclesSThread);
        icacheWaitRetryStallCyclesWThread
            .prereq(icacheWaitRetryStallCyclesWThread);
        icacheSquashes
            .prereq(icacheSquashes);
        tlbSquashes
            .prereq(tlbSquashes);
        nisnDist
            .init(/* base value */ 0,
              /* last value */ fetch->fetchWidth,
              /* bucket size */ 1)
            .flags(statistics::pdf);
        idleRate
            .prereq(idleRate);
        branchRate
            .flags(statistics::total);
        rate
            .flags(statistics::total);
        stalledS.prereq(stalledS);
        stalledW.prereq(stalledW);
        stalledSNotW.prereq(stalledSNotW);
        stalledSAndW.prereq(stalledSAndW);
        notStalled.prereq(notStalled);
        multipleRunning.prereq(multipleRunning);
}
void
Fetch::setTimeBuffer(TimeBuffer<TimeStruct> *time_buffer)
{
    timeBuffer = time_buffer;

    // Create wires to get information from proper places in time buffer.
    fromDecode = timeBuffer->getWire(-decodeToFetchDelay);
    fromRename = timeBuffer->getWire(-renameToFetchDelay);
    fromIEW = timeBuffer->getWire(-iewToFetchDelay);
    fromCommit = timeBuffer->getWire(-commitToFetchDelay);
}

void
Fetch::setActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeThreads = at_ptr;
}

void
Fetch::setSActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeSThreads = at_ptr;
}

void
Fetch::setWActiveThreads(std::list<ThreadID> *at_ptr)
{
    activeWThreads = at_ptr;
}

void
Fetch::setAllSThreads(std::list<ThreadID> *at_ptr)
{
    allSThreads = at_ptr;
}

void
Fetch::setAllWThreads(std::list<ThreadID> *at_ptr)
{
    allWThreads = at_ptr;
}

void
Fetch::setFetchQueue(TimeBuffer<FetchStruct> *ftb_ptr)
{
    // Create wire to write information to proper place in fetch time buf.
    toDecode = ftb_ptr->getWire(0);
}

void
Fetch::startupStage()
{
    //assert(priorityList.empty());
    resetStage();

    // Fetch needs to start fetching instructions at the very beginning,
    // so it must start up in active state.
    switchToActive();
}

void
Fetch::clearStates(ThreadID tid)
{
    fetchStatus[tid] = Running;
    DPRINTF(Fetch, "(Running 2) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    set(pc[tid], cpu->pcState(tid));
    fetchOffset[tid] = 0;
    macroop[tid] = NULL;
    delayedCommit[tid] = false;
    memReq[tid] = NULL;
    stalls[tid].decode = false;
    stalls[tid].drain = false;
    fetchBufferPC[tid] = 0;
    fetchBufferValid[tid] = false;
    fetchQueue[tid].clear();
    StalledOnConditionalBranch[tid] = false;

    // TODO not sure what to do with priorityList for now
    // priorityList.push_back(tid);
}

void
Fetch::resetStage()
{
    numInst = 0;
    interruptPending = false;
    cacheBlocked = false;

    priorityList.clear();

    // Setup PC and nextPC with initial state.
    for (ThreadID tid = 0; tid < numThreads; ++tid) {
        fetchStatus[tid] = Running;
        DPRINTF(Fetch, "(Running) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
        set(pc[tid], cpu->pcState(tid));
        fetchOffset[tid] = 0;
        macroop[tid] = NULL;

        delayedCommit[tid] = false;
        memReq[tid] = NULL;

        stalls[tid].decode = false;
        stalls[tid].drain = false;

        fetchBufferPC[tid] = 0;
        fetchBufferValid[tid] = false;

        fetchQueue[tid].clear();

        priorityList.push_back(tid);
        StalledOnConditionalBranch[tid] = false;
    }

    wroteToTimeBuffer = false;
    _status = Inactive;
}

void
Fetch::processCacheCompletion(PacketPtr pkt)
{
    ThreadID tid = cpu->contextToThread(pkt->req->contextId());

    DPRINTF(Fetch, "[tid:%i] Waking up from cache miss.\n", tid);
    assert(!cpu->switchedOut());

    // Only change the status if it's still waiting on the icache access
    // to return.
    if (fetchStatus[tid] != IcacheWaitResponse ||
        pkt->req != memReq[tid]) {
        ++fetchStats.icacheSquashes;
        delete pkt;
        return;
    }

    memcpy(fetchBuffer[tid], pkt->getConstPtr<uint8_t>(), fetchBufferSize);
    fetchBufferValid[tid] = true;

    // Wake up the CPU (if it went to sleep and was waiting on
    // this completion event).
    cpu->wakeCPU();

    DPRINTF(Activity, "[tid:%i] Activating fetch due to cache completion\n",
            tid);

    switchToActive();

    // Only switch to IcacheAccessComplete if we're not stalled as well.
    if (checkStall(tid)) {
        fetchStatus[tid] = Blocked;
        DPRINTF(Fetch, "(Blocked) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    } else {
        fetchStatus[tid] = IcacheAccessComplete;
        DPRINTF(Fetch, "(IcacheAccessComplete) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    }

    pkt->req->setAccessLatency();
    cpu->ppInstAccessComplete->notify(pkt);
    // Reset the mem req to NULL.
    delete pkt;
    memReq[tid] = NULL;
}

void
Fetch::drainResume()
{
    for (ThreadID i = 0; i < numThreads; ++i) {
        stalls[i].decode = false;
        stalls[i].drain = false;
    }
}

void
Fetch::drainSanityCheck() const
{
    assert(isDrained());
    assert(retryPkt == NULL);
    assert(retryTid == InvalidThreadID);
    assert(!cacheBlocked);
    assert(!interruptPending);

    for (ThreadID i = 0; i < numThreads; ++i) {
        assert(!memReq[i]);
        assert(fetchStatus[i] == Idle || stalls[i].drain);
    }

    branchPred->drainSanityCheck();
}

bool
Fetch::isDrained() const
{
    /* Make sure that threads are either idle of that the commit stage
     * has signaled that draining has completed by setting the drain
     * stall flag. This effectively forces the pipeline to be disabled
     * until the whole system is drained (simulation may continue to
     * drain other components).
     */
    for (ThreadID i = 0; i < numThreads; ++i) {
        // Verify fetch queues are drained
        if (!fetchQueue[i].empty())
            return false;

        // Return false if not idle or drain stalled
        if (fetchStatus[i] != Idle) {
            if (fetchStatus[i] == Blocked && stalls[i].drain)
                continue;
            else
                return false;
        }
    }

    /* The pipeline might start up again in the middle of the drain
     * cycle if the finish translation event is scheduled, so make
     * sure that's not the case.
     */
    return !finishTranslationEvent.scheduled();
}

void
Fetch::takeOverFrom()
{
    assert(cpu->getInstPort().isConnected());
    resetStage();

}

void
Fetch::drainStall(ThreadID tid)
{
    assert(cpu->isDraining());
    assert(!stalls[tid].drain);
    DPRINTF(Drain, "%i: Thread drained.\n", tid);
    stalls[tid].drain = true;
}

void
// Daniel modified:

Fetch::wakeFromQuiesce(ThreadID tid)
{
    DPRINTF(Fetch, "Waking up from quiesce\n");
    // Hopefully this is safe
    // @todo: Allow other threads to wake from quiesce.
    DPRINTF(Fetch, "(Running 3.1) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    DPRINTF(Fetch, "(Running 3.1) FetchStatus Update: fetchStatus[%i] = %d\n", 0, fetchStatus[0]);
    fetchStatus[tid] = Running;
    DPRINTF(Fetch, "(Running 3.2) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
}
/*
Fetch::wakeFromQuiesce()
{
    DPRINTF(Fetch, "Waking up from quiesce\n");
    // Hopefully this is safe
    // @todo: Allow other threads to wake from quiesce.
    fetchStatus[0] = Running;
    DPRINTF(Fetch, "(Running 3) FetchStatus Update: fetchStatus[%i] = %d\n", 0, fetchStatus[0]);
}*/

void
Fetch::switchToActive()
{
    if (_status == Inactive) {
        DPRINTF(Activity, "Activating stage.\n");

        cpu->activateStage(CPU::FetchIdx);

        _status = Active;
    }
}

void
Fetch::switchToInactive()
{
    if (_status == Active) {
        DPRINTF(Activity, "Deactivating stage.\n");

        cpu->deactivateStage(CPU::FetchIdx);

        _status = Inactive;
    }
}

void
Fetch::deactivateThread(ThreadID tid)
{
    // Update priority list
    auto thread_it = std::find(priorityList.begin(), priorityList.end(), tid);
    if (thread_it != priorityList.end()) {
        priorityList.erase(thread_it);
    }
}

void
Fetch::activateThread(ThreadID tid)
{
    auto thread_it = std::find(priorityList.begin(),
            priorityList.end(), tid);

    DPRINTF(Fetch,"[tid:%d] Trying to ACTIVATING_THREAD FETCH\n",tid);
    if(thread_it == priorityList.end())
    {
        DPRINTF(Fetch,"[tid:%d] FINALLY_ACTIVATING_THREAD\n",tid);
        priorityList.push_back(tid);
    }

    fetchStatus[tid] = Running;
    DPRINTF(Fetch, "(Running 4) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    set(pc[tid], cpu->pcState(tid));
    fetchOffset[tid] = 0;
    macroop[tid] = NULL;
    delayedCommit[tid] = false;
    memReq[tid] = NULL;
    stalls[tid].decode = false;
    stalls[tid].drain = false;
    fetchBufferPC[tid] = 0;
    fetchBufferValid[tid] = false;
    fetchQueue[tid].clear();
    StalledOnConditionalBranch[tid] = false;
}

bool
Fetch::lookupAndUpdateNextPC(const DynInstPtr &inst, PCStateBase &next_pc)
{
    // Do branch prediction check here.
    // A bit of a misnomer...next_PC is actually the current PC until
    // this function updates it.
    bool predict_taken;

    if (!inst->isControl()) {
        inst->staticInst->advancePC(next_pc);
        inst->setPredTarg(next_pc);
        inst->setPredTaken(false);
        return false;
    }

    ThreadID tid = inst->threadNumber;

    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || predictOnWThreads){
        predict_taken = branchPred->predict(inst->staticInst, inst->seqNum,
                                        next_pc, tid);
    } else {
        // stop branch prediction for W threads. Just predict as not taken right now.
        //predict_taken = branchPred->predict(inst->staticInst, inst->seqNum, //INORDER turn off branch prediction
        //                                next_pc, tid);
        predict_taken = true;
    }

    if (predict_taken) {
        DPRINTF(Fetch, "[tid:%i] [sn:%llu] Branch at PC %#x "
                "predicted to be taken to %s\n",
                tid, inst->seqNum, inst->pcState().instAddr(), next_pc);
    } else {
        DPRINTF(Fetch, "[tid:%i] [sn:%llu] Branch at PC %#x "
                "predicted to be not taken\n",
                tid, inst->seqNum, inst->pcState().instAddr());
    }

    DPRINTF(Fetch, "[tid:%i] [sn:%llu] Branch at PC %#x "
            "predicted to go to %s\n",
            tid, inst->seqNum, inst->pcState().instAddr(), next_pc);
    inst->setPredTarg(next_pc);
    inst->setPredTaken(predict_taken);

    ++fetchStats.branches;

    if (predict_taken) {
        ++fetchStats.predictedBranches;
    }

    return predict_taken;
}

bool
Fetch::fetchCacheLine(Addr vaddr, ThreadID tid, Addr pc)
{
    Fault fault = NoFault;

    assert(!cpu->switchedOut());

    // @todo: not sure if these should block translation.
    //AlphaDep
    if (cacheBlocked) {
        DPRINTF(Fetch, "[tid:%i] Can't fetch cache line, cache blocked\n",
                tid);
        return false;
    } else if (checkInterrupt(pc) && !delayedCommit[tid]) {
        // Hold off fetch from getting new instructions when:
        // Cache is blocked, or
        // while an interrupt is pending and we're not in PAL mode, or
        // fetch is switched out.
        DPRINTF(Fetch, "[tid:%i] Can't fetch cache line, interrupt pending\n",
                tid);
        return false;
    }

    // Align the fetch address to the start of a fetch buffer segment.
    Addr fetchBufferBlockPC = fetchBufferAlignPC(vaddr);

    DPRINTF(Fetch, "[tid:%i] Fetching cache line %#x for addr %#x\n",
            tid, fetchBufferBlockPC, vaddr);
    // Setup the memReq to do a read of the first instruction's address.
    // Set the appropriate read size and flags as well.
    // Build request here.
    RequestPtr mem_req = std::make_shared<Request>(
        fetchBufferBlockPC, fetchBufferSize,
        Request::INST_FETCH, cpu->instRequestorId(), pc,
        cpu->thread[tid]->contextId());

    mem_req->taskId(cpu->taskId());

    memReq[tid] = mem_req;

    // Initiate translation of the icache block
    fetchStatus[tid] = ItlbWait;
    DPRINTF(Fetch, "(ItlbWait) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
    FetchTranslation *trans = new FetchTranslation(this);
    cpu->mmu->translateTiming(mem_req, cpu->thread[tid]->getTC(),
                              trans, BaseMMU::Execute);
    return true;
}

void
Fetch::finishTranslation(const Fault &fault, const RequestPtr &mem_req)
{
    ThreadID tid = cpu->contextToThread(mem_req->contextId());
    Addr fetchBufferBlockPC = mem_req->getVaddr();

    assert(!cpu->switchedOut());

    bool isStrong = (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong);

    IcachePort icachePort = ((cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || !UseSplitCache)) ? icachePortS : icachePortW;

    assert(!(isStrong && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak));

    assert(!(!UseSplitCache && (&icachePort == &icachePortW)));

    // Wake up CPU if it was idle
    cpu->wakeCPU();

    if (fetchStatus[tid] != ItlbWait || mem_req != memReq[tid] ||
        mem_req->getVaddr() != memReq[tid]->getVaddr()) {
        DPRINTF(Fetch, "[tid:%i] Ignoring itlb completed after squash\n",
                tid);
        ++fetchStats.tlbSquashes;
        return;
    }


    // If translation was successful, attempt to read the icache block.
    if (fault == NoFault) {
        // Check that we're not going off into random memory
        // If we have, just wait around for commit to squash something and put
        // us on the right track
        if (!cpu->system->isMemAddr(mem_req->getPaddr())) {
            warn("Address %#x is outside of physical memory, stopping fetch\n",
                    mem_req->getPaddr());
            fetchStatus[tid] = NoGoodAddr;
            DPRINTF(Fetch, "(NoGoodAddr) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
            memReq[tid] = NULL;
            return;
        }

        // Build packet here.
        PacketPtr data_pkt = new Packet(mem_req, MemCmd::ReadReq);
        data_pkt->dataDynamic(new uint8_t[fetchBufferSize]);

        fetchBufferPC[tid] = fetchBufferBlockPC;
        fetchBufferValid[tid] = false;
        DPRINTF(Fetch, "Fetch: Doing instruction read.\n");

        fetchStats.cacheLines++;

        // Access the cache.
        if (!icachePort.sendTimingReq(data_pkt)) {
            assert(retryPkt == NULL);
            assert(retryTid == InvalidThreadID);
            DPRINTF(Fetch, "[tid:%i] Out of MSHRs!\n", tid);

            fetchStatus[tid] = IcacheWaitRetry;
            DPRINTF(Fetch, "(IcacheWaitRetry) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
            retryPkt = data_pkt;
            retryTid = tid;
            cacheBlocked = true;
        } else {
            DPRINTF(Fetch, "[tid:%i] Doing Icache access.\n", tid);
            DPRINTF(Activity, "[tid:%i] Activity: Waiting on I-cache "
                    "response.\n", tid);

            lastIcacheStall[tid] = curTick();
            fetchStatus[tid] = IcacheWaitResponse;
            DPRINTF(Fetch, "(IcacheWaitResponse) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
            // Notify Fetch Request probe when a packet containing a fetch
            // request is successfully sent
            ppFetchRequestSent->notify(mem_req);
        }
    } else {
        // Don't send an instruction to decode if we can't handle it.
        if (!(numInst < fetchWidth) ||
                !(fetchQueue[tid].size() < fetchQueueSize)) {
            assert(!finishTranslationEvent.scheduled());
            finishTranslationEvent.setFault(fault);
            finishTranslationEvent.setReq(mem_req);
            cpu->schedule(finishTranslationEvent,
                          cpu->clockEdge(Cycles(1)));
            return;
        }
        DPRINTF(Fetch,
                "[tid:%i] Got back req with addr %#x but expected %#x\n",
                tid, mem_req->getVaddr(), memReq[tid]->getVaddr());
        // Translation faulted, icache request won't be sent.
        memReq[tid] = NULL;

        // Send the fault to commit.  This thread will not do anything
        // until commit handles the fault.  The only other way it can
        // wake up is if a squash comes along and changes the PC.
        const PCStateBase &fetch_pc = *pc[tid];

        DPRINTF(Fetch, "[tid:%i] Translation faulted, building noop.\n", tid);
        // We will use a nop in ordier to carry the fault.
        DynInstPtr instruction = buildInst(tid, nopStaticInstPtr, nullptr,
                fetch_pc, fetch_pc, false);
        instruction->setNotAnInst();

        instruction->setPredTarg(fetch_pc);
        instruction->fault = fault;
        wroteToTimeBuffer = true;

        DPRINTF(Activity, "Activity this cycle.\n");
        cpu->activityThisCycle();

        fetchStatus[tid] = TrapPending;
        DPRINTF(Fetch, "(TrapPending) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

        DPRINTF(Fetch, "[tid:%i] Blocked, need to handle the trap.\n", tid);
        DPRINTF(Fetch, "[tid:%i] fault (%s) detected @ PC %s.\n",tid, fault->name(), *pc[tid]);
    }
    _status = updateFetchStatus();
}

void
Fetch::doSquash(const PCStateBase &new_pc, const DynInstPtr squashInst,
        ThreadID tid)
{
    DPRINTF(Fetch, "[tid:%i] Squashing, setting PC to: %s.\n",
            tid, new_pc);

    set(pc[tid], new_pc);
    fetchOffset[tid] = 0;
    if (squashInst && squashInst->pcState().instAddr() == new_pc.instAddr())
        macroop[tid] = squashInst->macroop;
    else
        macroop[tid] = NULL;
    decoder[tid]->reset();

    // Clear the icache miss if it's outstanding.
    if (fetchStatus[tid] == IcacheWaitResponse) {
        DPRINTF(Fetch, "[tid:%i] Squashing outstanding Icache miss.\n",
                tid);
        memReq[tid] = NULL;
    } else if (fetchStatus[tid] == ItlbWait) {
        DPRINTF(Fetch, "[tid:%i] Squashing outstanding ITLB miss.\n",
                tid);
        memReq[tid] = NULL;
    }

    // Get rid of the retrying packet if it was from this thread.
    if (retryTid == tid) {
        assert(cacheBlocked);
        if (retryPkt) {
            delete retryPkt;
        }
        retryPkt = NULL;
        retryTid = InvalidThreadID;
    }

    fetchStatus[tid] = Squashing;
    DPRINTF(Fetch, "(Squashing) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

    // Empty fetch queue
    fetchQueue[tid].clear();

    // microops are being squashed, it is not known wheather the
    // youngest non-squashed microop was  marked delayed commit
    // or not. Setting the flag to true ensures that the
    // interrupts are not handled when they cannot be, though
    // some opportunities to handle interrupts may be missed.
    delayedCommit[tid] = true;

    ++fetchStats.squashCycles;
}

// OFF_BP
void
Fetch::doBranchRedirect(const PCStateBase &new_pc, const DynInstPtr squashInst,
        ThreadID tid)
{
    DPRINTF(Fetch, "[tid:%i] Rediecting brnahc, setting PC to: %s.\n",
            tid, new_pc);

    set(pc[tid], new_pc);
    fetchOffset[tid] = 0;
    if (squashInst && squashInst->pcState().instAddr() == new_pc.instAddr())
        macroop[tid] = squashInst->macroop;
    else
        macroop[tid] = NULL;
    decoder[tid]->reset();

    // Clear the icache miss if it's outstanding.
    if (fetchStatus[tid] == IcacheWaitResponse) {
        DPRINTF(Fetch, "[tid:%i] Squashing outstanding Icache miss.\n",
                tid);
        memReq[tid] = NULL;
    } else if (fetchStatus[tid] == ItlbWait) {
        DPRINTF(Fetch, "[tid:%i] Squashing outstanding ITLB miss.\n",
                tid);
        memReq[tid] = NULL;
    }

    // Get rid of the retrying packet if it was from this thread.
    if (retryTid == tid) {
        assert(cacheBlocked);
        if (retryPkt) {
            delete retryPkt;
        }
        retryPkt = NULL;
        retryTid = InvalidThreadID;
    }

    DPRINTF(Fetch, "Redirecting FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

    // Empty fetch queue
    fetchQueue[tid].clear();

    // // microops are being squashed, it is not known wheather the
    // // youngest non-squashed microop was  marked delayed commit
    // // or not. Setting the flag to true ensures that the
    // // interrupts are not handled when they cannot be, though
    // // some opportunities to handle interrupts may be missed.
    // delayedCommit[tid] = true;
}

void
Fetch::squashFromDecode(const PCStateBase &new_pc, const DynInstPtr squashInst,
        const InstSeqNum seq_num, ThreadID tid)
{
    DPRINTF(Fetch, "[tid:%i] Squashing from decode.\n", tid);

    // if the instruction which caused squash was a mispredict, current stage is BlockedOnBranch and the instrution is an unconditional branch mark
    // fetch to be in unconditional branching state.
    // We will keep track of this after squashing is done, to decide if we want to go into Running state or to keep squash in BlockedOnBranch state
    // till the branch is resolved in the iew state

    // OFF_BP
    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && !predictOnWThreads) {
        assert(fetchStatus[tid] == BlockedOnBranch);

        if(fromDecode->decodeInfo[tid].isConditionalBranch) {
            StalledOnConditionalBranch[tid] = true;
            StalledOnConditionalBranchSeq[tid] = seq_num;
        }
    }

    // BP_OFF
    // DPRINTFN("[tid:%i] [sn:%llu] FETCH MAKING SURE %d.\n", tid, seq_num, StalledOnConditionalBranch[tid]);

    doSquash(new_pc, squashInst, tid);

    // Tell the CPU to remove any instructions that are in flight between
    // fetch and decode.
    cpu->removeInstsUntil(seq_num, tid);
}

bool
Fetch::checkStall(ThreadID tid) const
{
    bool ret_val = false;

    if (stalls[tid].drain) {
        assert(cpu->isDraining());
        DPRINTF(Fetch,"[tid:%i] Drain stall detected.\n",tid);
        ret_val = true;
    }

    return ret_val;
}

Fetch::FetchStatus
Fetch::updateFetchStatus()
{
    //Check Running
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (fetchStatus[tid] == Running ||
            fetchStatus[tid] == Squashing ||
            fetchStatus[tid] == IcacheAccessComplete) {

            if (_status == Inactive) {
                DPRINTF(Activity, "[tid:%i] Activating stage.\n",tid);

                if (fetchStatus[tid] == IcacheAccessComplete) {
                    DPRINTF(Activity, "[tid:%i] Activating fetch due to cache"
                            "completion\n",tid);
                }

                cpu->activateStage(CPU::FetchIdx);
            }

            return Active;
        }
    }

    // Stage is switching from active to inactive, notify CPU of it.
    if (_status == Active) {
        DPRINTF(Activity, "Deactivating stage.\n");

        cpu->deactivateStage(CPU::FetchIdx);
    }

    return Inactive;
}

void
Fetch::squash(const PCStateBase &new_pc, const InstSeqNum seq_num,
        DynInstPtr squashInst, ThreadID tid)
{
    DPRINTF(Fetch, "[tid:%i] Squash from commit.\n", tid);

    doSquash(new_pc, squashInst, tid);

    // Tell the CPU to remove any instructions that are not in the ROB.
    cpu->removeInstsNotInROB(tid);
}

void
Fetch::tick()
{
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();
    ToDecodePreference.clear();
    bool status_change = false;

    wroteToTimeBuffer = false;

    //Daniel checking blockage
    unsigned notBlockedS = 0;
    unsigned sThreadCount = 0;
    unsigned notBlockedW = 0;
    unsigned wThreadCount = 0;

    for (ThreadID i = 0; i < numThreads; ++i) {
        issuePipelinedIfetch[i] = false;
    }

    while (threads != end) {
        ThreadID tid = *threads++;

        DPRINTF(Fetch, "[tid:%d] Iterating Stage.\n",tid);
        // Check the signals for each thread to determine the proper status
        // for each thread.
        bool updated_status = checkSignalsAndUpdate(tid);
        status_change =  status_change || updated_status;
        // Daniel checking fetch blocks
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong){
            sThreadCount++;
            if(fetchStatus[tid] == Running){
                notBlockedS++;
            }
        }
        if (cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            wThreadCount++;
            if (fetchStatus[tid] == Running){
                notBlockedW++;
            }
        }
    }

    // NoGoodAddrCount

    if (sThreadCount > 0 && notBlockedS == 0){
        ++fetchStats.stalledS;
    }
    if (wThreadCount > 0 && notBlockedW == 0){
        ++fetchStats.stalledW;
    }
    if (sThreadCount > 0 && wThreadCount > 0 && notBlockedS == 0 && notBlockedW > 0){
        ++fetchStats.stalledSNotW;
    }
    if (sThreadCount > 0 && notBlockedS == 0 && wThreadCount > 0 && notBlockedW == 0){
        ++fetchStats.stalledSAndW;
    }
    if (notBlockedS > 0 || notBlockedW > 0){
        ++fetchStats.notStalled;
    }
    if (notBlockedS + notBlockedW > 1){
        ++fetchStats.multipleRunning;
    }

    DPRINTF(Fetch, "Running stage.\n");

    if (FullSystem) {
        if (fromCommit->commitInfo[0].interruptPending) {
            interruptPending = true;
        }

        if (fromCommit->commitInfo[0].clearInterrupt) {
            interruptPending = false;
        }
    }

    for (threadFetched = 0; threadFetched < numFetchingThreads;
        threadFetched++) {
        // Fetch each of the actively fetching threads.
        fetch(status_change);
    }

    // Record number of instructions fetched this cycle for distribution.
    fetchStats.nisnDist.sample(numInst);

    if (status_change) {
        // Change the fetch stage status if there was a status change.
        _status = updateFetchStatus();
    }

    // Issue the next I-cache request if possible.
    for (ThreadID i = 0; i < numThreads; ++i) {
        if (issuePipelinedIfetch[i]) {
            pipelineIcacheAccesses(i);
        }
    }

    // Send instructions enqueued into the fetch queue to decode.
    // Limit rate by fetchWidth.  Stall if decode is stalled.
    unsigned insts_to_decode = 0;
    unsigned available_insts = 0;

    for (auto tid : *activeThreads) {
        if (!stalls[tid].decode) {
            available_insts += fetchQueue[tid].size();
        }
    }

    int fetch_vals_sent = 0;
    int fetch_vals_0_sent = 0;
    int fetch_vals_1_sent = 0;

    ToDecodeThreadPriority();

    if(SingleThreadFetchiew) {
        // filling the queue with only 1 thread in a cycle
        // Prioritizing S thread and then W thread.
        for(int i = 0; i < numThreads; i++) {
            ThreadID tid = ToDecodePreference[i];
            if (!stalls[tid].decode) {
                int insts_to_decode = 0;
                while(!fetchQueue[tid].empty() && insts_to_decode < decodeWidth) {
                    const auto& inst = fetchQueue[tid].front();
                    toDecode->insts[toDecode->size++] = inst;
                    DPRINTF(Fetch, "[tid:%i] [sn:%llu] Sending instruction to decode "
                        "from fetch queue. Fetch queue size: %i.\n",
                        tid, inst->seqNum, fetchQueue[tid].size());
                    wroteToTimeBuffer = true;
                    fetchQueue[tid].pop_front();
                    insts_to_decode++;
                    fetch_vals_sent++;
                    if(tid == 0) {
                        fetch_vals_0_sent++;
                    } else {
                        fetch_vals_1_sent++;
                    }
                }
                break;  // have only 1 thread issue in a cycle
            }
        }
    } else {
            for(int i = 0; i < numThreads; i++) {
            ThreadID tid = ToDecodePreference[i];
            if(tid >= 0) 
                ++fetchStats.FetchQueueTryingToDecode[tid];
            if (!stalls[tid].decode) {
                if(tid >= 0) 
                    ++fetchStats.FetchQueueSendingToDecode[tid];
                // get stats if fetchQueue Empty or decode width exhasted
                if(fetchQueue[tid].empty() && tid > 0) {
                    ++fetchStats.FetchQueueEmpty[tid];
                }
                if(insts_to_decode >= decodeWidth && tid > 0) {
                    ++fetchStats.DecodeWidthFull[tid];
                }
                while(!fetchQueue[tid].empty() && insts_to_decode < decodeWidth) {
                    if(tid > 0) {
                        ++fetchStats.NumFetchCycles[tid];
                    }
                    const auto& inst = fetchQueue[tid].front();
                    toDecode->insts[toDecode->size++] = inst;
                    DPRINTF(Fetch, "[tid:%i] [sn:%llu] Sending instruction to decode "
                        "from fetch queue. Fetch queue size: %i.\n",
                        tid, inst->seqNum, fetchQueue[tid].size());
                    wroteToTimeBuffer = true;
                    fetchQueue[tid].pop_front();
                    insts_to_decode++;
                    fetch_vals_sent++;
                    if(tid == 0) {
                        fetch_vals_0_sent++;
                    } else {
                        fetch_vals_1_sent++;
                    }
                }
            }
        }

        // // Pick a random thread to start trying to grab instructions from
        // auto tid_itr = activeThreads->begin();
        // std::advance(tid_itr,
        //         random_mt.random<uint8_t>(0, activeThreads->size() - 1));

        // ToDecodeThreadPriority();

        // while (available_insts != 0 && insts_to_decode < decodeWidth) {
        //     ThreadID tid = *tid_itr;
        //     if (!stalls[tid].decode && !fetchQueue[tid].empty()) {
        //         const auto& inst = fetchQueue[tid].front();
        //         toDecode->insts[toDecode->size++] = inst;
        //         DPRINTF(Fetch, "[tid:%i] [sn:%llu] Sending instruction to decode "
        //                 "from fetch queue. Fetch queue size: %i.\n",
        //                 tid, inst->seqNum, fetchQueue[tid].size());

        //         wroteToTimeBuffer = true;
        //         fetchQueue[tid].pop_front();
        //         insts_to_decode++;
        //         available_insts--;
        //         fetch_vals_sent++;
        //         if(tid == 0) {
        //             fetch_vals_0_sent++;
        //         } else {
        //             fetch_vals_1_sent++;
        //         }
        //     }

        //     tid_itr++;
        //     // Wrap around if at end of active threads list
        //     if (tid_itr == activeThreads->end())
        //         tid_itr = activeThreads->begin();
        // }
    }

    DPRINTF(pipelineView,"fetch_vals_sent %d\n",fetch_vals_sent);
    DPRINTF(pipelineView,"fetch_vals_0_sent %d\n",fetch_vals_0_sent);
    DPRINTF(pipelineView,"fetch_vals_1_sent %d\n",fetch_vals_1_sent);

    // If there was activity this cycle, inform the CPU of it.
    if (wroteToTimeBuffer) {
        DPRINTF(Activity, "Activity this cycle.\n");
        cpu->activityThisCycle();
    }

    // Reset the number of the instruction we've fetched.
    numInst = 0;
}

bool
Fetch::checkSignalsAndUpdate(ThreadID tid)
{
    // OFF_BP
    DPRINTF(Fetch, "[tid:%i] Checking Update Signals state %d BranchResolved From decode %d decodeToFetchDelay %d check_all %d.\n",tid, fetchStatus[tid], fromDecode->decodeInfo[tid].BranchResolved, decodeToFetchDelay, (fetchStatus[tid] != BlockedOnBranch
        || (fetchStatus[tid] == BlockedOnBranch && fromIEW->iewInfo[tid].BranchResolved == true) // if it is blocked on branch, it can only be unblocked by IEW bypassing the next pointer. 
       ));
    // Update the per thread stall statuses.
    if (fromDecode->decodeBlock[tid]) {
        stalls[tid].decode = true;
    }

    if (fromDecode->decodeUnblock[tid]) {
        assert(stalls[tid].decode);
        assert(!fromDecode->decodeBlock[tid]);
        stalls[tid].decode = false;
    }

    // Check squash signals from commit.
    if (fromCommit->commitInfo[tid].squash) {

        DPRINTF(Fetch, "[tid:%i] Squashing instructions due to squash "
                "from commit.\n",tid);

        if(fromIEW->iewInfo[tid].nextPC)
            DPRINTF(Fetch, "[tid:%i] Squashing instructions due to squash "
               "from commit PCFromIEW %s.\n",tid, *fromIEW->iewInfo[tid].nextPC);

        // if we have a squash from commit, then there has to be an interrupt
        // assert that the squash from commit is from a newer instruction than the one that 
        // put the system in stallforbranch, if the system is stalling on branch
        // then reset the stall on branch to false because the instruction which put us in this state
        // will be killed anyway
        if(StalledOnConditionalBranch[tid]) {
            assert(fromCommit->commitInfo[tid].doneSeqNum < StalledOnConditionalBranchSeq[tid]);
            StalledOnConditionalBranch[tid] = false;
        }


        // In any case, squash.
        squash(*fromCommit->commitInfo[tid].pc,
               fromCommit->commitInfo[tid].doneSeqNum,
               fromCommit->commitInfo[tid].squashInst, tid);

        // If it was a branch mispredict on a control instruction, update the
        // branch predictor with that instruction, otherwise just kill the
        // invalid state we generated in after sequence number
        // update BP only for S threads
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || predictOnWThreads) {
            if (fromCommit->commitInfo[tid].mispredictInst &&
                fromCommit->commitInfo[tid].mispredictInst->isControl()) {
                branchPred->squash(fromCommit->commitInfo[tid].doneSeqNum,
                        *fromCommit->commitInfo[tid].pc,
                        fromCommit->commitInfo[tid].branchTaken, tid);
            } 
            // else {
            //     branchPred->squash(fromCommit->commitInfo[tid].doneSeqNum,
            //                     tid);
            // }
        }

        return true;
    } else if (fromCommit->commitInfo[tid].doneSeqNum && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || predictOnWThreads) {
        // Update the branch predictor if it wasn't a squashed instruction
        // that was broadcasted.
        branchPred->update(fromCommit->commitInfo[tid].doneSeqNum, tid);
    }

    // Check squash signals from decode.
    if (fromDecode->decodeInfo[tid].squash) {
        DPRINTF(Fetch, "[tid:%i] Squashing instructions due to squash "
                "from decode.\n",tid);

        // Update the branch predictor.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong || predictOnWThreads) {
            if (fromDecode->decodeInfo[tid].branchMispredict) {
                branchPred->squash(fromDecode->decodeInfo[tid].doneSeqNum,
                        *fromDecode->decodeInfo[tid].nextPC,
                        fromDecode->decodeInfo[tid].branchTaken, tid);
            }
            // else {
            //     branchPred->squash(fromDecode->decodeInfo[tid].doneSeqNum,
            //                     tid);
            // }
        }

        if (fetchStatus[tid] != Squashing) {

            DPRINTF(Fetch, "Squashing from decode with PC = %s\n",
                *fromDecode->decodeInfo[tid].nextPC);
            // Squash unless we're already squashing
            squashFromDecode(*fromDecode->decodeInfo[tid].nextPC,
                             fromDecode->decodeInfo[tid].squashInst,
                             fromDecode->decodeInfo[tid].doneSeqNum,
                             tid);

            return true;
        }
    }

    if (checkStall(tid) &&
        fetchStatus[tid] != IcacheWaitResponse &&
        fetchStatus[tid] != IcacheWaitRetry &&
        fetchStatus[tid] != ItlbWait &&
        fetchStatus[tid] != QuiescePending) {
        DPRINTF(Fetch, "[tid:%i] Setting to blocked\n",tid);

        fetchStatus[tid] = Blocked;
        DPRINTF(Fetch, "(Blocked 2) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

        return true;
    }

    // // DPRINTFN("[tid:%i] Checking Update Signals state %d BranchResolved From decode %d decodeToFetchDelay %d check_all %d.\n",tid, fetchStatus[tid], fromDecode->decodeInfo[tid].BranchResolved, decodeToFetchDelay, (fetchStatus[tid] != BlockedOnBranch
    //     || (fetchStatus[tid] == BlockedOnBranch && fromIEW->iewInfo[tid].BranchResolved == true) // if it is blocked on branch, it can only be unblocked by IEW bypassing the next pointer. 
    //    ));

    // OFF_BP
    if (fetchStatus[tid] == Blocked ||
        fetchStatus[tid] == Squashing 
        || (fetchStatus[tid] == BlockedOnBranch && fromIEW->iewInfo[tid].BranchResolved == true) // if it is blocked on branch, it can only be unblocked by IEW bypassing the next pointer. 
        // the # cycles to resolve branch should be deterministic, so we can just issue it after a given number of cycles // Ishita
        // the implementation logic should not be hard in real HW, here I just wait for IEW to signal that the fetch stage can now go and fix the next pc to the correct instruction
        ) {
        // Switch status to running if fetch isn't being told to block or
        // squash this cycle.
        // if(fetchStatus[tid] == BlockedOnBranch) {
        //     DPRINTFN("BACK TO RUNNING BlockedOnBranch\n");
        // }

        //DPRINTFN("UNBLOCKING THE SYSTE OLD STATE WAS %d\n",fetchStatus[tid]);

        DPRINTF(Fetch, "[tid:%i] Done squashing, switching to running.\n",
                tid);       

        // if fetch squashed due to a conditional branch in decode we need to continue to stall fetch. 
        if(fetchStatus[tid] != Squashing || (fetchStatus[tid] == Squashing && !StalledOnConditionalBranch[tid])) {
            
            // if we are stalled because of a branch and the branch is now resolved, we can reset the fetch stage to start execution
            if( fetchStatus[tid] == BlockedOnBranch) {
                doBranchRedirect(*fromIEW->iewInfo[tid].nextPC, fromIEW->iewInfo[tid].squashBranchInst, tid);
            }
            fetchStatus[tid] = Running;
            StalledOnConditionalBranch[tid] = false;
            //DPRINTFN("[tid:%i] FETCH BACK TO RUNNING %d.\n", tid, StalledOnConditionalBranch[tid]);
        } else {
            fetchStatus[tid] = BlockedOnBranch;
            //DPRINTFN("[tid:%i] WE STILL STALL %d.\n", tid, StalledOnConditionalBranch[tid]);
        }
        DPRINTF(Fetch, "(Running 5) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

        return true;
    }

    // If we've reached this point, we have not gotten any signals that
    // cause fetch to change its status.  Fetch remains the same as before.
    return false;
}

DynInstPtr
Fetch::buildInst(ThreadID tid, StaticInstPtr staticInst,
        StaticInstPtr curMacroop, const PCStateBase &this_pc,
        const PCStateBase &next_pc, bool trace)
{
    // Get a sequence number.
    InstSeqNum seq = cpu->getAndIncrementInstSeq();

    DynInst::Arrays arrays;
    arrays.numSrcs = staticInst->numSrcRegs();
    arrays.numDests = staticInst->numDestRegs();

    // Create a new DynInst from the instruction fetched.
    DynInstPtr instruction = new (arrays) DynInst(
            arrays, staticInst, curMacroop, this_pc, next_pc, seq, cpu);
    instruction->setTid(tid);

    instruction->setThreadState(cpu->thread[tid]);

    DPRINTF(Fetch, "[tid:%i] Instruction PC %s created [sn:%lli].\n",
            tid, this_pc, seq);

    DPRINTF(Fetch, "[tid:%i] Instruction is: %s\n", tid,
            instruction->staticInst->disassemble(this_pc.instAddr()));

    // DPRINTFN("[tid:%i] Instruction PC %s created [sn:%lli] Instruction is: %s pc addr %#x\n ", tid, this_pc, seq,
    //         instruction->staticInst->disassemble(this_pc.instAddr()),this_pc.instAddr());

    // if(this_pc.instAddr() == 0x400cc4) {
    //     DPRINTFN("FOUND THE NEEDED PC\n");
    // }

#if TRACING_ON
    if (trace) {
        instruction->traceData =
            cpu->getTracer()->getInstRecord(curTick(), cpu->tcBase(tid),
                    instruction->staticInst, this_pc, curMacroop);
    }
#else
    instruction->traceData = NULL;
#endif

    // Add instruction to the CPU's list of instructions.
    instruction->setInstListIt(cpu->addInst(instruction));

    // Write the instruction to the first slot in the queue
    // that heads to decode.
    assert(numInst < fetchWidth);
    instruction->cycleInFetch = cpu->curCycle();
    fetchQueue[tid].push_back(instruction);
    assert(fetchQueue[tid].size() <= fetchQueueSize);
    DPRINTF(Fetch, "[tid:%i] Fetch queue entry created (%i/%i).\n",
            tid, fetchQueue[tid].size(), fetchQueueSize);
    //toDecode->insts[toDecode->size++] = instruction;

    // Keep track of if we can take an interrupt at this boundary
    delayedCommit[tid] = instruction->isDelayedCommit();

    return instruction;
}

void
Fetch::fetch(bool &status_change)
{
    //////////////////////////////////////////
    // Start actual fetch
    //////////////////////////////////////////
    ThreadID tid = getFetchingThread();

    assert(!cpu->switchedOut());

    if (tid == InvalidThreadID) {
        // Breaks looping condition in tick()
        threadFetched = numFetchingThreads;

        if (numThreads == 1) {  // @todo Per-thread stats
            profileStall(0);
        }
        ++fetchStats.NoThreadToFetch;
        return;
    }

    DPRINTF(Fetch, "Attempting to fetch from [tid:%i]\n", tid);

    // The current PC.
    PCStateBase &this_pc = *pc[tid];

    Addr pcOffset = fetchOffset[tid];
    Addr fetchAddr = (this_pc.instAddr() + pcOffset) & decoder[tid]->pcMask();

    bool inRom = isRomMicroPC(this_pc.microPC());

    // If returning from the delay of a cache miss, then update the status
    // to running, otherwise do the cache access.  Possibly move this up
    // to tick() function.
    if (fetchStatus[tid] == IcacheAccessComplete) {
        DPRINTF(Fetch, "[tid:%i] Icache miss is complete.\n", tid);

        fetchStatus[tid] = Running;
        DPRINTF(Fetch, "(Running 6) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);

        status_change = true;
    } else if (fetchStatus[tid] == Running) {
        // Align the fetch PC so its at the start of a fetch buffer segment.
        Addr fetchBufferBlockPC = fetchBufferAlignPC(fetchAddr);

        // If buffer is no longer valid or fetchAddr has moved to point
        // to the next cache block, AND we have no remaining ucode
        // from a macro-op, then start fetch from icache.
        if (!(fetchBufferValid[tid] &&
                    fetchBufferBlockPC == fetchBufferPC[tid]) && !inRom &&
                !macroop[tid]) {
            DPRINTF(Fetch, "[tid:%i] Attempting to translate and read "
                    "instruction, starting at PC %s.\n", tid, this_pc);

            fetchCacheLine(fetchAddr, tid, this_pc.instAddr());

            if (fetchStatus[tid] == IcacheWaitResponse)
                ++fetchStats.icacheStallCycles;
            if (fetchStatus[tid] == IcacheWaitResponse && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
                ++fetchStats.icacheStallCyclesSThread;
            if (fetchStatus[tid] == IcacheWaitResponse && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak)
                ++fetchStats.icacheStallCyclesWThread;
            else if (fetchStatus[tid] == ItlbWait)
                ++fetchStats.tlbCycles;
            else
                ++fetchStats.miscStallCycles;
            return;
        } else if (checkInterrupt(this_pc.instAddr()) &&
                !delayedCommit[tid]) {
            // Stall CPU if an interrupt is posted and we're not issuing
            // an delayed commit micro-op currently (delayed commit
            // instructions are not interruptable by interrupts, only faults)
            ++fetchStats.miscStallCycles;
            DPRINTF(Fetch, "[tid:%i] Fetch is stalled!\n", tid);
            return;
        }
    } else {
        if (fetchStatus[tid] == Idle) {
            ++fetchStats.idleCycles;
            DPRINTF(Fetch, "[tid:%i] Fetch is idle!\n", tid);
        }

        // Status is Idle, so fetch should do nothing.
        return;
    }

    ++fetchStats.cycles;

    std::unique_ptr<PCStateBase> next_pc(this_pc.clone());

    StaticInstPtr staticInst = NULL;
    StaticInstPtr curMacroop = macroop[tid];

    // If the read of the first instruction was successful, then grab the
    // instructions from the rest of the cache line and put them into the
    // queue heading to decode.

    DPRINTF(Fetch, "[tid:%i] Adding instructions to queue to "
            "decode.\n", tid);

    // Need to keep track of whether or not a predicted branch
    // ended this fetch block.
    bool predictedBranch = false;

    // Need to halt fetch if quiesce instruction detected
    bool quiesce = false;

    const unsigned numInsts = fetchBufferSize / instSize;
    unsigned blkOffset = (fetchAddr - fetchBufferPC[tid]) / instSize;

    auto *dec_ptr = decoder[tid];
    const Addr pc_mask = dec_ptr->pcMask();

    // Loop through instruction memory from the cache.
    // Keep issuing while fetchWidth is available and branch is not
    // predicted taken
    {
         for (int byte = 0; byte < 64; byte++)
         {
             if(byte && byte%16 == 0){
                 DPRINTFR(Fetch, "\n");
             }
             DPRINTFR(Fetch, "%02x ", fetchBuffer[tid][byte]);
         }
         DPRINTFR(Fetch, "\n");
     }

    if(fetchQueue[tid].size() >= fetchQueueSize) {
        ++fetchStats.FetchQueueFull[tid];
    }

    while (numInst < fetchWidth && fetchQueue[tid].size() < fetchQueueSize
           && !predictedBranch && !quiesce) {
        // We need to process more memory if we aren't going to get a
        // StaticInst from the rom, the current macroop, or what's already
        // in the decoder.
        bool needMem = !inRom && !curMacroop && !dec_ptr->instReady();
        fetchAddr = (this_pc.instAddr() + pcOffset) & pc_mask;
        Addr fetchBufferBlockPC = fetchBufferAlignPC(fetchAddr);

        if (needMem) { 
            // If buffer is no longer valid or fetchAddr has moved to point
            // to the next cache block then start fetch from icache.
            if (!fetchBufferValid[tid] ||
                fetchBufferBlockPC != fetchBufferPC[tid])
            {
                ++fetchStats.FetchNotValid[tid];
                break;
            }
            if (blkOffset >= numInsts) {
                ++fetchStats.FetchBufferExceeded[tid];
                // We need to process more memory, but we've run out of the
                // current block.
                break;
            }

            memcpy(dec_ptr->moreBytesPtr(),
                    fetchBuffer[tid] + blkOffset * instSize, instSize);
            decoder[tid]->moreBytes(this_pc, fetchAddr);

            if (dec_ptr->needMoreBytes()) {
                blkOffset++;
                fetchAddr += instSize;
                pcOffset += instSize;
            }
        }

        // Extract as many instructions and/or microops as we can from
        // the memory we've processed so far.
        do {

            if (!(curMacroop || inRom)) {
                if (dec_ptr->instReady()) {
                    staticInst = dec_ptr->decode(this_pc);
                    // Increment stat of fetched instructions.
                    ++fetchStats.insts;
                    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
                        ++fetchStats.instsSThread;
                    if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak)
                        ++fetchStats.instsWThread;
                    if (staticInst->isMacroop()) {
                        curMacroop = staticInst;
                    } else {
                        pcOffset = 0;
                    }
                } else {
                    // We need more bytes for this instruction so blkOffset and
                    // pcOffset will be updated
                    ++fetchStats.NeedToFetchMoreMemory[tid];
                    break;
                }
            }

            // Whether we're moving to a new macroop because we're at the
            // end of the current one, or the branch predictor incorrectly
            // thinks we are...
            bool newMacro = false;
            if (curMacroop || inRom) {
                if (inRom) {
                    staticInst = dec_ptr->fetchRomMicroop(
                            this_pc.microPC(), curMacroop);
                } else {
                    staticInst = curMacroop->fetchMicroop(this_pc.microPC());
                }
                newMacro |= staticInst->isLastMicroop();
            }

            DynInstPtr instruction = buildInst(
                    tid, staticInst, curMacroop, this_pc, *next_pc, true);

            ppFetch->notify(instruction);
            numInst++;

#if TRACING_ON
            if (debug::O3PipeView) {
                instruction->fetchTick = curTick();
            }
#endif

            set(next_pc, this_pc);

            // If we're branching after this instruction, quit fetching
            // from the same block. // Ishita

            bool condititional_branch_inst_issued = false;
            //assert(!instruction->isCondCtrl() && this_pc.branching());

            predictedBranch |= this_pc.branching();
            predictedBranch |= lookupAndUpdateNextPC(instruction, *next_pc);
            if (predictedBranch) {
                DPRINTF(Fetch, "Branch detected with PC = %s\n", this_pc);
            }

            //DPRINTFN("Inst_fetch [sn:%llu] PC %s isCondCtrl %d isDirectCtrl %d isIndirectCtrl %d isUncondCtrl %d\n",instruction->seqNum,instruction->pcState(),instruction->isCondCtrl(),instruction->isDirectCtrl(),instruction->isIndirectCtrl(),instruction->isUncondCtrl());

            condititional_branch_inst_issued  = (instruction->isCondCtrl() || instruction->isDirectCtrl() ||  instruction->isIndirectCtrl() || instruction->isUncondCtrl()) && cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak && !predictOnWThreads;

            newMacro |= this_pc.instAddr() != next_pc->instAddr();

            // Move to the next instruction, unless we have a branch.
            set(this_pc, *next_pc);

            inRom = isRomMicroPC(this_pc.microPC());

            if (newMacro) {
                fetchAddr = this_pc.instAddr() & pc_mask;
                blkOffset = (fetchAddr - fetchBufferPC[tid]) / instSize;
                pcOffset = 0;
                curMacroop = NULL;
            }

            // if the code is having a conditional brnahc and is a weak thread, then stop fetch

            //Ishita -> set next PC to stall
            // OFF_BP
            if(condititional_branch_inst_issued) {
                fetchStatus[tid] = BlockedOnBranch;
                // DPRINTFN("WILL_BLOCK_HERE [sn:%llu] PC %s isCondCtrl %d isDirectCtrl %d isIndirectCtrl %d isUncondCtrl %d STATE %d\n",instruction->seqNum,instruction->pcState(),instruction->isCondCtrl(),instruction->isDirectCtrl(),instruction->isIndirectCtrl(),instruction->isUncondCtrl(),fetchStatus[tid]);
                break;
            }

            if (instruction->isQuiesce()) {
                DPRINTF(Fetch,
                        "Quiesce instruction encountered, halting fetch!\n");
                fetchStatus[tid] = QuiescePending;
                DPRINTF(Fetch, "(QuiescePending) FetchStatus Update: fetchStatus[%i] = %d\n", tid, fetchStatus[tid]);
                status_change = true;
                quiesce = true;
                ++fetchStats.QuiescePendingForThread[tid];
                break;
            }

        } while ((curMacroop || dec_ptr->instReady()) &&
                 numInst < fetchWidth &&
                 fetchQueue[tid].size() < fetchQueueSize);

        // Re-evaluate whether the next instruction to fetch is in micro-op ROM
        // or not.
        inRom = isRomMicroPC(this_pc.microPC());
    }

    if(numInst == 0) {
        ++fetchStats.noInstFetched;
    }

    if (predictedBranch) {
        DPRINTF(Fetch, "[tid:%i] Done fetching, predicted branch "
                "instruction encountered.\n", tid);
    } else if (numInst >= fetchWidth) {
        DPRINTF(Fetch, "[tid:%i] Done fetching, reached fetch bandwidth "
                "for this cycle.\n", tid);
    } else if (blkOffset >= fetchBufferSize) {
        DPRINTF(Fetch, "[tid:%i] Done fetching, reached the end of the"
                "fetch buffer.\n", tid);
    }

    macroop[tid] = curMacroop;
    fetchOffset[tid] = pcOffset;

    if (numInst > 0) {
        wroteToTimeBuffer = true;
    }

    // pipeline a fetch if we're crossing a fetch buffer boundary and not in
    // a state that would preclude fetching
    fetchAddr = (this_pc.instAddr() + pcOffset) & pc_mask;
    Addr fetchBufferBlockPC = fetchBufferAlignPC(fetchAddr);
    issuePipelinedIfetch[tid] = fetchBufferBlockPC != fetchBufferPC[tid] &&
        fetchStatus[tid] != IcacheWaitResponse &&
        fetchStatus[tid] != ItlbWait &&
        fetchStatus[tid] != IcacheWaitRetry &&
        fetchStatus[tid] != QuiescePending &&
        !curMacroop;
}

void
Fetch::recvReqRetry(bool isStrong)
{
    if (retryPkt != NULL) {


        assert(cacheBlocked);
        assert(retryTid != InvalidThreadID);

        //Daniel Debugging
        DPRINTF(Fetch, "[retryTid:%i] fetchStatus[retryTid]: %d, IcacheWaitRetry: %d\n", retryTid, fetchStatus[retryTid], IcacheWaitRetry);

        assert(fetchStatus[retryTid] == IcacheWaitRetry);

        IcachePort icachePort = (isStrong || !UseSplitCache) ? icachePortS : icachePortW;

        assert(!(!UseSplitCache && (&icachePort == &icachePortW)));

        if (icachePort.sendTimingReq(retryPkt)) {
            fetchStatus[retryTid] = IcacheWaitResponse;
            DPRINTF(Fetch, "(IcacheWaitResponse 2) FetchStatus Update: fetchStatus[%i] = %d\n", retryTid, fetchStatus[retryTid]);
            // Notify Fetch Request probe when a retryPkt is successfully sent.
            // Note that notify must be called before retryPkt is set to NULL.
            ppFetchRequestSent->notify(retryPkt->req);
            retryPkt = NULL;
            retryTid = InvalidThreadID;
            cacheBlocked = false;
        }
    } else {
        assert(retryTid == InvalidThreadID);
        // Access has been squashed since it was sent out.  Just clear
        // the cache being blocked.
        cacheBlocked = false;
    }
}

///////////////////////////////////////
//                                   //
//  SMT FETCH POLICY MAINTAINED HERE //
//                                   //
///////////////////////////////////////
ThreadID
Fetch::getFetchingThread()
{
    if (numThreads > 1) {
        switch (fetchPolicy) {
          case SMTFetchPolicy::RoundRobin:
            return roundRobin();
          case SMTFetchPolicy::IQCount:
            return iqCount();
          case SMTFetchPolicy::LSQCount:
            return lsqCount();
          case SMTFetchPolicy::Branch:
            return branchCount();
          case SMTFetchPolicy::SWIQCount:
            return SWiqCount();
          case SMTFetchPolicy::SWFetchCount:
            return SWFetchCount();
          default:
            return InvalidThreadID;
        }
    } else {
        std::list<ThreadID>::iterator thread = activeThreads->begin();
        if (thread == activeThreads->end()) {
            return InvalidThreadID;
        }

        ThreadID tid = *thread;

        if (fetchStatus[tid] == Running ||
            fetchStatus[tid] == IcacheAccessComplete ||
            fetchStatus[tid] == Idle) {
            return tid;
        } else {
            return InvalidThreadID;
        }
    }
}

ThreadID
Fetch::roundRobin()
{
    // std::list<ThreadID>::iterator pri_iter = priorityList.begin();
    // std::list<ThreadID>::iterator end      = priorityList.end();

    std::list<ThreadID>::iterator pri_iter = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    ThreadID high_pri;
    while (pri_iter != end) {
        high_pri = *pri_iter;
        assert(high_pri <= numThreads);

        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle) {

            priorityList.erase(pri_iter);
            priorityList.push_back(high_pri);

            return high_pri;
        }

        pri_iter++;
    }

    return InvalidThreadID;
}

ThreadID
Fetch::SWiqCount()
{
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > SQ;
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > WQ;
    std::priority_queue<float, std::vector<float>,
                        std::greater<float> > SWQ;
    std::map<unsigned, ThreadID> SthreadMap;
    std::map<unsigned, ThreadID> WthreadMap;
    std::map<float, ThreadID> SWthreadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    float weight = policyWeighting;

    // create 2 lists for S threads and W threads 
    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = fromIEW->iewInfo[tid].iqCount;
        unsigned FreeEntries = fromIEW->iewInfo[tid].iqCount;

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        {
            SQ.push(iqCount);
            SthreadMap[iqCount] = tid;
            SWQ.push((iqCount+1)*(1-weight));
            SWthreadMap[(iqCount+1)*(1-weight)] = tid;
        }
        else
        {
            WQ.push(iqCount);
            WthreadMap[iqCount] = tid;
            SWQ.push((iqCount+1)*weight);
            SWthreadMap[(iqCount+1)*weight] = tid;
        }
    }
    if(weight >= 1 || weight <= 0){
        while (!SQ.empty()) {
            ThreadID high_pri = SthreadMap[SQ.top()];

            if (fetchStatus[high_pri] == Running ||
                fetchStatus[high_pri] == IcacheAccessComplete ||
                fetchStatus[high_pri] == Idle)
                return high_pri;
            else
                SQ.pop();
        }
        while (!WQ.empty()) {
            ThreadID high_pri = WthreadMap[WQ.top()];

            if (fetchStatus[high_pri] == Running ||
                fetchStatus[high_pri] == IcacheAccessComplete ||
                fetchStatus[high_pri] == Idle)
                return high_pri;
            else
                WQ.pop();
        }
    }
    if(weight < 1 && weight > 0){
        while (!SWQ.empty()) {
            ThreadID high_pri = SWthreadMap[SWQ.top()];

            if (fetchStatus[high_pri] == Running ||
                fetchStatus[high_pri] == IcacheAccessComplete ||
                fetchStatus[high_pri] == Idle)
                return high_pri;
            else
                SWQ.pop();
        }
    }
    return InvalidThreadID;
}




ThreadID
Fetch::SWFetchCount()
{
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > SQ;
    std::priority_queue<unsigned, std::vector<unsigned>,
                        std::greater<unsigned> > WQ;
    std::priority_queue<float, std::vector<float>,
                        std::greater<float> > SWQ;
    std::map<unsigned, ThreadID> SthreadMap;
    std::map<unsigned, ThreadID> WthreadMap;
    std::map<float, ThreadID> SWthreadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    // create 2 lists for S threads and W threads 
    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = fetchQueue[tid].size();

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong)
        {
            SQ.push(iqCount);
            SthreadMap[iqCount] = tid;
            SWQ.push(iqCount);
            SWthreadMap[iqCount] = tid;
        }
        else
        {
            WQ.push(iqCount);
            WthreadMap[iqCount] = tid;
            SWQ.push(iqCount);
            SWthreadMap[iqCount] = tid;
        }
    }

    while (!SQ.empty()) {
        ThreadID high_pri = SthreadMap[SQ.top()];

        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle
            && (fetchQueue[high_pri].size() < fetchQueueSize))
            return high_pri;
        else
            SQ.pop();
    }
    while (!WQ.empty()) {
        ThreadID high_pri = WthreadMap[WQ.top()];

        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle)
            return high_pri;
        else
            WQ.pop();
    }
    while (!SWQ.empty()) {
        ThreadID high_pri = SWthreadMap[SWQ.top()];

        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle)
            return high_pri;
        else
            SWQ.pop();
    }
    return InvalidThreadID;

}

ThreadID
Fetch::iqCount()
{
    //sorted from lowest->highest
    std::priority_queue<float, std::vector<float>,
                        std::greater<float> > PQ;
    std::map<float, ThreadID> threadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    int total_threads = 0;
    int tid1present = 0;
    int tidgreat1present = 0;
    int tid1absent = 0;

    while (threads != end) {
        ThreadID tid = *threads++;
        float iqCount = fromIEW->iewInfo[tid].iqCount;

        float freeIQEntries = fromIEW->iewInfo[tid].freeIQEntries;
        float ratio_free_entries = iqCount / (freeIQEntries + iqCount);

        // bias towards S threads
        // if(tid == 0 || tid == 1) {
        //     ratio_free_entries = ratio_free_entries * 10;
        // }

        // printf("checkRatio tid:%d iqCount %f freeIQEntries %f ratio_free_entries %f\n",tid,iqCount,freeIQEntries,ratio_free_entries);

        iqCount = ratio_free_entries;

        //unsigned iqCount = tid;
        total_threads++;
        if (tid == 1) {
            tid1present = 1;
        } else if (tid > 1) {
            tidgreat1present = 1;
        }

        // Collect stats
        switch (fetchStatus[tid]) {
            case Running:
                ++fetchStats.RunningCount[tid];
                break;
            case Idle:
                ++fetchStats.IdleCount[tid];
                break;
            case Squashing:
                ++fetchStats.SquashingCount[tid];
                break;
            case Blocked:
                ++fetchStats.BlockedCount[tid];
                break;
            case Fetching:
                ++fetchStats.FetchingCount[tid];
                break;
            case TrapPending:
                ++fetchStats.TrapPendingCount[tid];
                break;
            case QuiescePending:
                ++fetchStats.QuiescePendingCount[tid];
                break;
            case ItlbWait:
                ++fetchStats.ItlbWaitCount[tid];
                break;
            case IcacheWaitResponse:
                ++fetchStats.IcacheWaitResponseCount[tid];
                break;
            case IcacheWaitRetry:
                ++fetchStats.IcacheWaitRetryCount[tid];
                break;
            case IcacheAccessComplete:
                ++fetchStats.IcacheAccessCompleteCount[tid];
                break;
            case NoGoodAddr:
                ++fetchStats.NoGoodAddrCount[tid];
                break;
            case BlockedOnBranch:
                ++fetchStats.BlockedOnBranchCount[tid];
                if(!tid1present)
                    ++fetchStats.BlockedOnBranchCountNoSThread[tid];
                break;
        }

        //we can potentially get tid collisions if two threads
        //have the same iqCount, but this should be rare.
        PQ.push(iqCount);
        // Check if this iqCount is already in the map
        if (threadMap.find(iqCount) == threadMap.end()) {
            // If not in the map, add it
            threadMap[iqCount] = tid;
        } else {
            // If it's already in the map, keep the lower ThreadID
            if (tid < threadMap[iqCount] && 
                    (fetchStatus[tid] == Running ||
                fetchStatus[tid] == IcacheAccessComplete ||
                fetchStatus[tid] == Idle)) {
                threadMap[iqCount] = tid;
            }
        }
    }

    if (!tid1present && tidgreat1present) {
        tid1absent = 1;
    }

    // // Experimental print
    // if (total_threads > 1) {
    //     threads = activeThreads->begin();
    //     end = activeThreads->end();
    //     printf("Total threads %d tid1absent %d ", total_threads, tid1absent);
    //     while (threads != end) {
    //         ThreadID tid = *threads++;
    //         unsigned iqCount = fromIEW->iewInfo[tid].iqCount;
    //         printf("tid:%d status:%d iqCount:%d ", tid, fetchStatus[tid], iqCount);
    //     }
    // }

    while (!PQ.empty()) {
        ThreadID high_pri = threadMap[PQ.top()];
        float top_iqCount = PQ.top();
        //printf("Thread %d, Status: %d, IQCount: %f\n",high_pri, fetchStatus[high_pri], top_iqCount);
        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle) {
            // if (total_threads > 1) {
            //     printf("issuing thread %d\n", high_pri);
            // }
            return high_pri;
        }
        else
            PQ.pop();

    }

    // if (total_threads > 1)
    //     printf(" no thread issued\n");

    return InvalidThreadID;
}



ThreadID
Fetch::lsqCount()
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

        if (fetchStatus[high_pri] == Running ||
            fetchStatus[high_pri] == IcacheAccessComplete ||
            fetchStatus[high_pri] == Idle)
            return high_pri;
        else
            PQ.pop();
    }

    return InvalidThreadID;
}

void
Fetch::ToDecodeThreadPriority()
{
    assert(ToDecodePreference.size() == 0);
    ToDecodePreference.resize(numThreads,-1);
    //SWiqCountPriority();
    IQCountPriority();
}

void 
Fetch::SWiqCountPriority() { //Ishita
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
        unsigned iqCount = fetchQueue[tid].size();

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

    int iter = 0;
    while (!SQ.empty()) {
        ThreadID high_pri = SthreadMap[SQ.top()];
        ToDecodePreference[iter] = high_pri;
        SQ.pop();
        iter++;
    }
    while (!WQ.empty()) {
        ThreadID high_pri = WthreadMap[WQ.top()];
        ToDecodePreference[iter] = high_pri;
        WQ.pop();
        iter++;
    }
}

void 
Fetch::IQCountPriority() { //Ishita
    //std::priority_queue<unsigned, std::vector<unsigned>,
    //                    std::greater<unsigned> > Queue;
    std::priority_queue<unsigned> Queue;
    std::map<unsigned, ThreadID> threadMap;

    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    // create 2 lists for S threads and W threads 
    while (threads != end) {
        ThreadID tid = *threads++;
        unsigned iqCount = fetchQueue[tid].size();

        Queue.push(iqCount);
        threadMap[iqCount] = tid;
    }

    int iter = 0;
    while (!Queue.empty()) {
        ThreadID high_pri = threadMap[Queue.top()];
        ToDecodePreference[iter] = high_pri;
        Queue.pop();
        iter++;
    }
}

ThreadID
Fetch::branchCount()
{
    panic("Branch Count Fetch policy unimplemented\n");
    return InvalidThreadID;
}

void
Fetch::pipelineIcacheAccesses(ThreadID tid)
{
    if (!issuePipelinedIfetch[tid]) {
        return;
    }

    // The next PC to access.
    const PCStateBase &this_pc = *pc[tid];

    if (isRomMicroPC(this_pc.microPC())) {
        return;
    }

    Addr pcOffset = fetchOffset[tid];
    Addr fetchAddr = (this_pc.instAddr() + pcOffset) & decoder[tid]->pcMask();

    // Align the fetch PC so its at the start of a fetch buffer segment.
    Addr fetchBufferBlockPC = fetchBufferAlignPC(fetchAddr);

    // Unless buffer already got the block, fetch it from icache.
    if (!(fetchBufferValid[tid] && fetchBufferBlockPC == fetchBufferPC[tid])) {
        DPRINTF(Fetch, "[tid:%i] Issuing a pipelined I-cache access, "
                "starting at PC %s.\n", tid, this_pc);

        fetchCacheLine(fetchAddr, tid, this_pc.instAddr());
    }
}

void
Fetch::profileStall(ThreadID tid)
{
    DPRINTF(Fetch,"There are no more threads available to fetch from.\n");

    // @todo Per-thread stats

    if (stalls[tid].drain) {
        ++fetchStats.pendingDrainCycles;
        DPRINTF(Fetch, "Fetch is waiting for a drain!\n");
    } else if (activeThreads->empty()) {
        ++fetchStats.noActiveThreadStallCycles;
        DPRINTF(Fetch, "Fetch has no active thread!\n");
    } else if (fetchStatus[tid] == Blocked) {
        ++fetchStats.blockedCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is blocked!\n", tid);
    } else if (fetchStatus[tid] == Squashing) {
        ++fetchStats.squashCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is squashing!\n", tid);
    } else if (fetchStatus[tid] == IcacheWaitResponse) {
        ++fetchStats.icacheStallCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is waiting cache response!\n",
                tid);
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            ++fetchStats.icacheStallCyclesSThread;
        }
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            ++fetchStats.icacheStallCyclesWThread;
        }
    } else if (fetchStatus[tid] == ItlbWait) {
        ++fetchStats.tlbCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is waiting ITLB walk to "
                "finish!\n", tid);
    } else if (fetchStatus[tid] == TrapPending) {
        ++fetchStats.pendingTrapStallCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is waiting for a pending trap!\n",
                tid);
    } else if (fetchStatus[tid] == QuiescePending) {
        ++fetchStats.pendingQuiesceStallCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is waiting for a pending quiesce "
                "instruction!\n", tid);
    } else if (fetchStatus[tid] == IcacheWaitRetry) {
        ++fetchStats.icacheWaitRetryStallCycles;
        DPRINTF(Fetch, "[tid:%i] Fetch is waiting for an I-cache retry!\n",
                tid);
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Strong) {
            ++fetchStats.icacheWaitRetryStallCyclesSThread;
        }
        if(cpu->thread[tid]->tc->getProcessPtr()->getprocessThreadType() == Weak) {
            ++fetchStats.icacheWaitRetryStallCyclesWThread;
        }
    } else if (fetchStatus[tid] == NoGoodAddr) {
            DPRINTF(Fetch, "[tid:%i] Fetch predicted non-executable address\n",
                    tid);        
    } else {
        DPRINTF(Fetch, "[tid:%i] Unexpected fetch stall reason "
            "(Status: %i)\n",
            tid, fetchStatus[tid]);
    }
}

bool
Fetch::IcachePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(O3CPU, "Fetch unit received timing\n");
    // We shouldn't ever get a cacheable block in Modified state
    assert(pkt->req->isUncacheable() ||
           !(pkt->cacheResponding() && !pkt->hasSharers()));
    fetch->processCacheCompletion(pkt);

    return true;
}

void
Fetch::IcachePort::recvReqRetry()
{
    fetch->recvReqRetry(isStrong);
}

} // namespace o3
} // namespace gem5
