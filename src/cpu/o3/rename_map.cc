/*
 * Copyright (c) 2016-2018,2019 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder. You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
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

#include "cpu/o3/rename_map.hh"

#include <vector>

#include "cpu/o3/dyn_inst.hh"
#include "cpu/reg_class.hh"
#include "debug/Rename.hh"
#include "debug/IQ.hh"

namespace gem5
{

namespace o3
{

SimpleRenameMap::SimpleRenameMap() : freeList(NULL)
{
}


void
SimpleRenameMap::init(const RegClass &reg_class, SimpleFreeList *_freeList)
{
    assert(freeList == NULL);
    assert(map.empty());

    map.resize(reg_class.numRegs());
    freeList = _freeList;
}

SimpleRenameMap::RenameInfo
SimpleRenameMap::rename(const RegId& arch_reg, bool isWThread, int tid, int sn)
{
    PhysRegIdPtr renamed_reg;
    // Record the current physical register that is renamed to the
    // requested architected register.
    PhysRegIdPtr prev_reg = map[arch_reg.index()];

    DPRINTF(Rename, "[tid:%d] [sn:%llu] Renamed base_newintreg1 %d physical reg %d (%d) old mapping was"
            " %d (%d)\n", tid, sn, arch_reg.index(), arch_reg, prev_reg->flatIndex(), prev_reg->flatIndex());

    if (arch_reg.is(InvalidRegClass)) {
        DPRINTF(Rename, "[tid:%d] [sn:%llu] Renamed base_newintreg2 %d physical reg %d (%d) old mapping was"
            " %d (%d)\n", tid, sn, arch_reg.index(), arch_reg, prev_reg->flatIndex(), prev_reg->flatIndex());
        assert(prev_reg->is(InvalidRegClass));
        renamed_reg = prev_reg;
    } else if (prev_reg->getNumPinnedWrites() > 0) {

        DPRINTF(Rename, "[tid:%d] [sn:%llu] Renamed base_newintreg3 %d physical reg %d (%d) old mapping was"
            " %d (%d)\n", tid, sn, arch_reg.index(), arch_reg, prev_reg->flatIndex(), prev_reg->flatIndex());
        // Do not rename if the register is pinned
        assert(arch_reg.getNumPinnedWrites() == 0);  // Prevent pinning the
                                                     // same register twice
        DPRINTF(Rename, "[tid:%d] Renaming pinned reg, numPinnedWrites %d\n",
                tid,prev_reg->getNumPinnedWrites());
        renamed_reg = prev_reg;
        renamed_reg->decrNumPinnedWrites();
    } else {
        if(isWThread) { 
        // do not rename WThread registers. Keep the old mapping
            renamed_reg = prev_reg;
            DPRINTF(IQ, "[tid:%d] [sn:%llu] arch_reg %d REUSING_REG base_reg %d Renaming reg %d numPinnedWrites %d archwrite %d isPinned %d to physical reg %d (%d) old mapping was  %d (%d)\n",
                    tid, sn, arch_reg, arch_reg.index(), renamed_reg->flatIndex(), renamed_reg->getNumPinnedWritesToComplete(),arch_reg.getNumPinnedWrites(),renamed_reg->isPinned(),renamed_reg->flatIndex(), renamed_reg->flatIndex(),
            prev_reg->flatIndex(), prev_reg->flatIndex());
        } else {
            bool found_reg = freeList->regExists(279);
            renamed_reg = freeList->getReg();
            renamed_reg->sethasrenamed();
            map[arch_reg.index()] = renamed_reg;
            renamed_reg->setNumPinnedWrites(arch_reg.getNumPinnedWrites());
            renamed_reg->setNumPinnedWritesToComplete(
                arch_reg.getNumPinnedWrites() + 1);
            DPRINTF(IQ, "[tid:%d] [sn:%llu] arch_reg %d RENAME_CHECK base_reg %d Renaming reg %d numPinnedWrites %d archwrite %d isPinned %d to physical reg %d (%d) old mapping was  %d (%d) found_reg %d free_list_size %d\n",
                    tid,sn,arch_reg,arch_reg.index(), renamed_reg->flatIndex(), renamed_reg->getNumPinnedWritesToComplete(),arch_reg.getNumPinnedWrites(),renamed_reg->isPinned(),renamed_reg->flatIndex(), renamed_reg->flatIndex(),
            prev_reg->flatIndex(), prev_reg->flatIndex(),found_reg, freeList->get_size());
        }
    }

    DPRINTF(Rename, "[tid:%d] [sn:%llu] Renamed base_initreg %d reg regId %d %d to physical reg %d (%d) old mapping was"
            " %d (%d)\n",
            tid, sn, arch_reg.index(), arch_reg, renamed_reg->flatIndex(), renamed_reg->flatIndex(),
            prev_reg->flatIndex(), prev_reg->flatIndex());

    return RenameInfo(renamed_reg, prev_reg);
}


/**** UnifiedRenameMap methods ****/

void
UnifiedRenameMap::init(const BaseISA::RegClasses &regClasses,
        PhysRegFile *_regFile, UnifiedFreeList *freeList)
{
    regFile = _regFile;

    for (int i = 0; i < renameMaps.size(); i++)
        renameMaps[i].init(*regClasses.at(i), &(freeList->freeLists[i]));
}

bool
UnifiedRenameMap::canRename(DynInstPtr inst) const
{
    for (int i = 0; i < renameMaps.size(); i++) {
        if (inst->numDestRegs((RegClassType)i) >
                renameMaps[i].numFreeEntries()) {
            return false;
        }
    }
    return true;
}

int
UnifiedRenameMap::getRegsAvailable(DynInstPtr inst) const
{
    for (int i = 0; i < renameMaps.size(); i++) {
        if (inst->numDestRegs((RegClassType)i) >
                renameMaps[i].numFreeEntries()) {
            return renameMaps[i].numFreeEntries();
        }
    }
    return true;
}

} // namespace o3
} // namespace gem5
