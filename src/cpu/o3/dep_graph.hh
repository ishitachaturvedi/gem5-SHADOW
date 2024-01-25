/*
 * Copyright (c) 2012 ARM Limited
 * All rights reserved
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
 * Copyright (c) 2006 The Regents of The University of Michigan
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

#ifndef __CPU_O3_DEP_GRAPH_HH__
#define __CPU_O3_DEP_GRAPH_HH__

#include "cpu/o3/comm.hh"
namespace gem5
{

namespace o3
{

/** Node in a linked list. */
template <class DynInstPtr>
class DependencyEntry
{
  public:
    DependencyEntry()
        : inst(NULL), next(NULL)
    { }

    DynInstPtr inst;
    //Might want to include data about what arch. register the
    //dependence is waiting on.
    DependencyEntry<DynInstPtr> *next;
};

/** Array of linked list that maintains the dependencies between
 * producing instructions and consuming instructions.  Each linked
 * list represents a single physical register, having the future
 * producer of the register's value, and all consumers waiting on that
 * value on the list.  The head node of each linked list represents
 * the producing instruction of that register.  Instructions are put
 * on the list upon reaching the IQ, and are removed from the list
 * either when the producer completes, or the instruction is squashed.
*/
template <class DynInstPtr>
class DependencyGraph
{
  public:
    typedef DependencyEntry<DynInstPtr> DepEntry;

    /** Default construction.  Must call resize() prior to use. */
    DependencyGraph()
        : numEntries(0), memAllocCounter(0), nodesTraversed(0), nodesRemoved(0)
    { }

    ~DependencyGraph();

    /** Resize the dependency graph to have num_entries registers. */
    void resize(int num_entries);

    /** Clears all of the linked lists. */
    void reset();
    void resetWAWGraph();
    void resetRAW();
    void resetRAW(RegIndex idx, int num);
    void resetWAR();
    void resetWAR(RegIndex idx, int num);

    /** Inserts an instruction to be dependent on the given index. */
    void insert(RegIndex idx, const DynInstPtr &new_inst);

    /** Inserts an instruction to be output (WAW) dependent on the given index. */
    bool insertBehindWAW(RegIndex idx, const DynInstPtr &new_inst);

    /** Inserts an instruction to be output (RAW) dependent on thelast entry for the given index. */
    int insertBehindRAW(RegIndex idx, const DynInstPtr &new_inst);

    /** get the seq number of the inst this inst depends on */
    int getRAWSeqNum(RegIndex idx);
    int getWARSeqNum(RegIndex idx);

    DynInstPtr getRAWLastInst(RegIndex idx);
    DynInstPtr getWARLastInst(RegIndex idx);

    /** Inserts an instruction to be input (WAR) dependent on thelast entry for the given index. */
    void insertBehindWAR(RegIndex idx, const DynInstPtr &new_inst);

    int instInListWAR(RegIndex idx);

    int instInListRAW(RegIndex idx);

    /** Sets the producing instruction of a given register. */
    void setInst(RegIndex idx, const DynInstPtr &new_inst)
    { dependGraph[idx].inst = new_inst; }

    /** Adds the proding inst at the end of the vector for W threads */
    void setInstPushBack(RegIndex idx, const DynInstPtr &new_inst)
    { 
        DepEntry *new_entry = new DepEntry;
        new_entry->next = NULL;
        new_entry->inst = new_inst;
        dependenceRAWGraph[idx].push_back(*new_entry);
    }

    void setInstPushBackWAR(RegIndex idx, const DynInstPtr &new_inst)
    { 
        DepEntry *new_entry = new DepEntry;
        new_entry->next = NULL;
        new_entry->inst = new_inst;
        dependenceWARGraph[idx].push_back(*new_entry);
    }

    /** Check if there are no entries for the dependence graph -> The src reg is ready */
    bool isRAWSrcReady(RegIndex idx) { return (dependenceRAWGraph[idx].size() == 0); }

    /** Check if there are no entries for the dependence graph -> The dest reg is ready */
    bool isWARSrcReady(RegIndex idx) { return (dependenceWARGraph[idx].size() == 0); }

    int WARSrcLastIdx(RegIndex idx) { 
        int index = dependenceWARGraph[idx].size() - 1;
        return index; 
    }

    int getWARlistSize(RegIndex idx);

    /** Clears the producing instruction. */
    void clearInst(RegIndex idx)
    { dependGraph[idx].inst = NULL; }

    /** Clears the producing instruction -> Removes the first entry from the vector for this register idx. */
    void clearInstRAW(RegIndex idx)
    { 
        dependenceRAWGraph[idx][0].inst = NULL; 
        dependenceRAWGraph[idx].erase(dependenceRAWGraph[idx].begin());
    }

    /** Clears the producing instruction -> Removes the last entry from the vector for this register idx -> for squash. */
    void clearlastInstRAW(RegIndex idx)
    { 
        int size = dependenceRAWGraph[idx].size() - 1;
        dependenceRAWGraph[idx][size].inst = NULL; 
        dependenceRAWGraph[idx].pop_back();
    }

    void clearlastInstWAR(RegIndex idx)
    { 
        int size = dependenceWARGraph[idx].size() - 1;
        dependenceWARGraph[idx][size].inst = NULL; 
        dependenceWARGraph[idx].pop_back();
        int size_later = dependenceWARGraph[idx].size() - 1;
        assert(size == size_later + 1);
    }

    /** Clears the producing instruction -> Removes the first entry from the vector for this register idx. */
     void clearInstWAR(RegIndex idx, int num)
    { 
        dependenceWARGraph[idx][num].inst = NULL; 
        dependenceWARGraph[idx].erase(dependenceWARGraph[idx].begin() + num);
    }

    /** checks that the inst in question is the oldest inst in the dependence vector for this instruction */
    bool isInstOldestRAW(RegIndex idx, const DynInstPtr &new_inst) { return (dependenceRAWGraph[idx][0].inst == new_inst); }

    DynInstPtr getOldestInst(RegIndex idx) { return dependenceRAWGraph[idx][0].inst; }

    /** Removes an instruction from a single linked list. */
    void remove(RegIndex idx, const DynInstPtr &inst_to_remove);

    /** Removes an instruction from a single linked list. */
    void removeInst(RegIndex idx, const DynInstPtr &inst_to_remove);

    void removeLastInst(RegIndex idx, const DynInstPtr &inst_to_remove);

    DynInstPtr getLastWAWInst(RegIndex idx);

    /** Removes an instruction from a single linked list. If the instruction is the head of the linked list, we 
     * delete all the nodes and remove the entire vector entry.
     */
    void removeRAW(RegIndex idx, const DynInstPtr &inst_to_remove);

    void removeWAR(RegIndex idx, const DynInstPtr &inst_to_remove);

    /** Removes and returns the newest dependent of a specific register. */
    DynInstPtr pop(RegIndex idx);

    /** get the head inst in WAW Dependence graph. */
    DynInstPtr getNextInst(RegIndex idx);

    /** Removes the next oldest output reg dependent of a specific register. */
    DynInstPtr popFront(RegIndex idx);

    /** Removes the next oldest output reg dependent of a specific register for vector entry 0 (oldest instruction which has issued, which holds all
     * other instructions as dependence). */
    DynInstPtr popRAW(RegIndex idx);

    DynInstPtr popWAR(RegIndex idx, int num);

    /** Get the location of the vector entry for the WAR inst */
    int getInstLocInWAR(RegIndex idx, const DynInstPtr &new_inst);

    /** Checks if the entire dependency graph is empty. */
    bool empty() const;

    /** Checks if there are any dependents on a specific register. */
    bool empty(RegIndex idx) const { return !dependGraph[idx].next; }

    /** Checks if there are any dependents on a specific register for the first entry of the vector. */
    bool emptyRAW(RegIndex idx) const { return !dependenceRAWGraph[idx][0].next; }

    bool emptyWAR(RegIndex idx, int num) const { return !dependenceWARGraph[idx][num].next; }

    int countNodes(RegIndex idx, int num);

    int countNodesWAW(RegIndex idx);

    int countNodesRAW(RegIndex idx);

    bool emptyWAWGraph(RegIndex idx) const { return dependenceWAWGraph[idx].size(); }

    /** Debugging function to dump out the dependency graph.
     */
    void dump();

  private:
    /** Array of linked lists.  Each linked list is a list of all the
     *  instructions that depend upon a given register.  The actual
     *  register's index is used to index into the graph; ie all
     *  instructions in flight that are dependent upon r34 will be
     *  in the linked list of dependGraph[34].
     */
    std::vector<DepEntry> dependGraph;

    /** This is for WAW entries: Array of linked lists. For each entry, 
     * the prior entry is the instruction it depends on, and the next entry
     * is the instruction which has a WAW dependence on it for the given
     * register. When an instruction completes, it removes itself. Only when an instruction
     * is at the head of the dependence list for a given write register
     * it is allowed to issue. 
     * This takes care of any reordering of WAW hazards. 
     * Basically when an instruction completes, it wakes the next instruction in the linked list
     * and removes itself from the list.
     */
    std::vector<std::vector<DynInstPtr>> dependenceWAWGraph;

    /** This is for RAW dependencies for W threads. Since we dont have register renaming, we need
     * to keep track of which instructions depend on what. There can be 2 cases:
     * A. 1. R1 <- R2 + R3
     * 2. R1 <- R1 + R4
     * 3. R5 <- R1 + R7
     * B. 1. R1 <- R2 + R3
     * 2. R5 <- R1 + R4
     * 3. R5 <- R1 + R7
     * In case (A) b depends on a and c depends on b. In case (B) both b and c depend on A.
     * To wake up the correct instructions, for each reg, we have a vector of linked lists.
     * For (A) inst 1 is added to the vector. inst 2 is added behind it. As inst 2 is also a 
     * producer of R1, inst 2 is added as a new entry to the vector.
     * Inst 3 is added behind inst 2. When inst 1 completes, it frees up inst 2 and 
     * removes its entry. When inst 2 completes it frees up inst 3 and removes its entry.
     * For case (B) inst 1 is added as a producer. Inst 2 is added to its LL. Then inst 3 is
     * Added to its LL. After inst 1 completes, it frees up inst 2 and inst 3 for issue.
     */
    std::vector<std::vector<DepEntry>> dependenceRAWGraph;

    /** Do the same thing as RAW for WAR dependencies */
    std::vector<std::vector<DepEntry>> dependenceWARGraph;

    /** Number of linked lists; identical to the number of registers. */
    int numEntries;

    // Debug variable, remove when done testing.
    unsigned memAllocCounter;

  public:
    // Debug variable, remove when done testing.
    uint64_t nodesTraversed;
    // Debug variable, remove when done testing.
    uint64_t nodesRemoved;
};

template <class DynInstPtr>
DependencyGraph<DynInstPtr>::~DependencyGraph()
{
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resize(int num_entries)
{
    numEntries = num_entries;
    dependGraph.resize(numEntries);
    dependenceWAWGraph.resize(numEntries);
    dependenceRAWGraph.resize(numEntries);
    dependenceWARGraph.resize(numEntries);
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::reset()
{
    // Clear the dependency graph
    DepEntry *curr;
    DepEntry *prev;

    for (int i = 0; i < numEntries; ++i) {
        curr = dependGraph[i].next;

        while (curr) {
            memAllocCounter--;

            prev = curr;
            curr = prev->next;
            prev->inst = NULL;

            delete prev;
        }

        if (dependGraph[i].inst) {
            dependGraph[i].inst = NULL;
        }

        dependGraph[i].next = NULL;
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resetRAW(RegIndex idx, int num)
{
    // Clear the dependency graph for a vector entry
    DepEntry *curr;
    DepEntry *prev;

    curr = dependenceRAWGraph[idx][num].next;

    while (curr) {
        memAllocCounter--;

        prev = curr;
        curr = prev->next;
        prev->inst = NULL;

        delete prev;
    }

    if (dependenceRAWGraph[idx][num].inst) {
        dependenceRAWGraph[idx][num].inst = NULL;
    }

    dependenceRAWGraph[idx][num].next = NULL;
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resetRAW()
{
    // clear the entire dependence graph
    // iterate over regs
    for (int i = 0; i < numEntries; ++i) {
        // iterate over the entries in the reg
        int size = dependenceRAWGraph[i].size();
        for(int j = 0; j < size; j++) {
            resetRAW(i,j);
            // remove this entry from the vector
            dependenceRAWGraph[i][j].inst = NULL; 
        }
        // clear the entire vector for this register
        dependenceRAWGraph[i].clear();
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resetWAR(RegIndex idx, int num)
{
    // Clear the dependency graph for a vector entry
    DepEntry *curr;
    DepEntry *prev;

    curr = dependenceWARGraph[idx][num].next;

    while (curr) {
        memAllocCounter--;

        prev = curr;
        curr = prev->next;
        prev->inst = NULL;

        delete prev;
    }

    if (dependenceWARGraph[idx][num].inst) {
        dependenceWARGraph[idx][num].inst = NULL;
    }

    dependenceWARGraph[idx][num].next = NULL;
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resetWAR()
{
    // clear the entire dependence graph
    // iterate over regs
    for (int i = 0; i < numEntries; ++i) {
        // iterate over the entries in the reg
        int size = dependenceWARGraph[i].size();
        for(int j = 0; j < size; j++) {
            resetWAR(i,j);
            // remove this entry from the vector
            dependenceWARGraph[i][j].inst = NULL; 
        }
        // clear the entire vector for this register
        dependenceWARGraph[i].clear();
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::resetWAWGraph()
{

    // keep popping the entries 
    for (int i = 0; i < numEntries; ++i) {
        dependenceWAWGraph[i].clear();
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::insert(RegIndex idx, const DynInstPtr &new_inst)
{
    //Add this new, dependent instruction at the head of the dependency
    //chain.

    // First create the entry that will be added to the head of the
    // dependency chain.
    DepEntry *new_entry = new DepEntry;
    new_entry->next = dependGraph[idx].next;
    new_entry->inst = new_inst;

    // Then actually add it to the chain.
    dependGraph[idx].next = new_entry;

    ++memAllocCounter;
}

/** Add a new instruction at the end of the linked
 * list. 
 */
template <class DynInstPtr>
bool
DependencyGraph<DynInstPtr>::insertBehindWAW(RegIndex idx, const DynInstPtr &new_inst)
{
    bool isFirstEntry = false;
    if(dependenceWAWGraph[idx].size() == 0) {
        isFirstEntry = true;
    }

    dependenceWAWGraph[idx].push_back(new_inst);

    return isFirstEntry;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::insertBehindRAW(RegIndex idx, const DynInstPtr &new_inst)
{
    // create the entry
    DepEntry *new_entry = new DepEntry;
    new_entry->inst = new_inst;
    new_entry->next = NULL;

    // Then actually add it to the chain. Add it to the last inst on this chain.
    int index = dependenceRAWGraph[idx].size() - 1;
    assert(dependenceRAWGraph[idx][index].inst != NULL);

    // go to the last entry, the add to chain. Dont replace current entries
    DepEntry *curr = &dependenceRAWGraph[idx][index];

    // Find the instruction to remove within the dependency linked list.
    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = new_entry;

    ++memAllocCounter;

    return index;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::getRAWSeqNum(RegIndex idx)
{
    int size = dependenceRAWGraph[idx].size() - 1;
    DepEntry *node = &dependenceRAWGraph[idx][size];
    int val = node->inst->seqNum;
    return val;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::getWARSeqNum(RegIndex idx)
{
    int size = dependenceWARGraph[idx].size() - 1;
    DepEntry *node = &dependenceWARGraph[idx][size];
    int val = node->inst->seqNum;
    return val;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::getRAWLastInst(RegIndex idx)
{
    int size = dependenceRAWGraph[idx].size() - 1;
    DepEntry* node = &dependenceRAWGraph[idx][size];
    DynInstPtr inst = node->inst;
    return inst;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::getWARLastInst(RegIndex idx)
{
    int size = dependenceWARGraph[idx].size() - 1;
    DepEntry* node = &dependenceWARGraph[idx][size];
    DynInstPtr inst = node->inst;
    return inst;
}


template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::insertBehindWAR(RegIndex idx, const DynInstPtr &new_inst)
{
    // create the entry
    DepEntry *new_entry = new DepEntry;
    new_entry->inst = new_inst;
    new_entry->next = NULL;

    // Then actually add it to the chain.

    // this should be added to the last entry, not 0th entry..
    //dependenceWARGraph[idx][index].next = new_entry;

    int index = dependenceWARGraph[idx].size() - 1;
    assert(dependenceWARGraph[idx][index].inst != NULL);

    // go to the last entry, the add to chain. Dont replace current entries
    DepEntry *curr = &dependenceWARGraph[idx][index];

    // Find the instruction to remove within the dependency linked list.
    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = new_entry;

    ++memAllocCounter;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::instInListWAR(RegIndex idx)
{
    int index = dependenceWARGraph[idx].size() - 1;

    int count = 0;
    DepEntry* current = dependenceWARGraph[idx][index].next;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}


template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::instInListRAW(RegIndex idx)
{
    int index = dependenceRAWGraph[idx].size() - 1;

    int count = 0;
    DepEntry* current = dependenceRAWGraph[idx][index].next;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::remove(RegIndex idx,
                                    const DynInstPtr &inst_to_remove)
{
    DepEntry *prev = &dependGraph[idx];
    DepEntry *curr = dependGraph[idx].next;

    // Make sure curr isn't NULL.  Because this instruction is being
    // removed from a dependency list, it must have been placed there at
    // an earlier time.  The dependency chain should not be empty,
    // unless the instruction dependent upon it is already ready.
    if (curr == NULL) {
        return;
    }

    nodesRemoved++;

    // Find the instruction to remove within the dependency linked list.
    while (curr->inst != inst_to_remove) {
        prev = curr;
        curr = curr->next;
        nodesTraversed++;

        assert(curr != NULL);
    }

    // Now remove this instruction from the list.
    prev->next = curr->next;

    --memAllocCounter;

    // Could push this off to the destructor of DependencyEntry
    curr->inst = NULL;

    delete curr;
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::removeRAW(RegIndex idx,
                                    const DynInstPtr &inst_to_remove)
{
    std::size_t size = dependenceRAWGraph[idx].size();
    // iterate over all the vector entries till we find our inst
    for(std::size_t i = 0; i < size; i++) {
        
        /** look for this inst in the dependence graph
         * if we find it, we remove it
         */
        DepEntry *prev = &dependenceRAWGraph[idx][i];
        DepEntry *curr = dependenceRAWGraph[idx][i].next;

        // Make sure curr isn't NULL.  Because this instruction is being
        // removed from a dependency list, it must have been placed there at
        // an earlier time.  The dependency chain should not be empty,
        // unless the instruction dependent upon it is already ready.
        if (curr == NULL) {
            continue;
        }

        // Find the instruction to remove within the dependency linked list.
        while (curr != NULL && curr->inst != inst_to_remove) { // cur should be NULL
            prev = curr;
            curr = curr->next;
            nodesTraversed++;
        }

        // not all vectors will have this inst
        if(curr!=NULL && curr->inst == inst_to_remove)
        {
            nodesRemoved++;

            // Now remove this instruction from the list.
            prev->next = curr->next;

            --memAllocCounter;

            // Could push this off to the destructor of DependencyEntry
            curr->inst = NULL;

            delete curr;
        }
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::removeWAR(RegIndex idx,
                                    const DynInstPtr &inst_to_remove)
{
    std::size_t size = dependenceWARGraph[idx].size();
    // iterate over all the vector entries till we find our inst
    for(std::size_t i = 0; i < size; i++) {
        
        /** look for this inst in the dependence graph
         * if we find it, we remove it
         */
        DepEntry *prev = &dependenceWARGraph[idx][i];
        DepEntry *curr = dependenceWARGraph[idx][i].next;

        // Make sure curr isn't NULL.  Because this instruction is being
        // removed from a dependency list, it must have been placed there at
        // an earlier time.  The dependency chain should not be empty,
        // unless the instruction dependent upon it is already ready.
        if (curr == NULL) {
            continue;
        }

        // Find the instruction to remove within the dependency linked list.
        while (curr!=NULL && curr->inst != inst_to_remove) {
            prev = curr;
            curr = curr->next;
            nodesTraversed++;
        }

        // not all vectors will have this inst
        if(curr!=NULL && curr->inst == inst_to_remove)
        {
            nodesRemoved++;

            // Now remove this instruction from the list.
            prev->next = curr->next;

            --memAllocCounter;

            // Could push this off to the destructor of DependencyEntry
            curr->inst = NULL;

            delete curr;
        }
    }
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::removeInst(RegIndex idx,
                                    const DynInstPtr &inst_to_remove)
{
    int start_size = dependenceWAWGraph[idx].size();

    int index = -1;
    for(int i = 0; i < start_size; i++) {
        DynInstPtr current = dependenceWAWGraph[idx][i];
        if(current == inst_to_remove) {
            index = i;
            break;
        }
    }

    assert(index!=-1);
    // should be the last index
    assert(index == (start_size -1));
    dependenceWAWGraph[idx][index] = NULL;
    dependenceWAWGraph[idx].erase(dependenceWAWGraph[idx].begin() + index);

    int end_size = dependenceWAWGraph[idx].size();;
    // an instruction should be removed
    assert(start_size == (end_size + 1));
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::getLastWAWInst(RegIndex idx)
{

    int size = dependenceWAWGraph[idx].size() - 1;

    return dependenceWAWGraph[idx][size];
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::pop(RegIndex idx)
{
    DepEntry *node;
    node = dependGraph[idx].next;
    DynInstPtr inst = NULL;
    if (node) {
        inst = node->inst;
        dependGraph[idx].next = node->next;
        node->inst = NULL;
        memAllocCounter--;
        delete node;
    }
    return inst;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::popFront(RegIndex idx)
{
   DynInstPtr inst = dependenceWAWGraph[idx][0];
   dependenceWAWGraph[idx][0] = NULL;
   dependenceWAWGraph[idx].erase(dependenceWAWGraph[idx].begin());
   return inst;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::popRAW(RegIndex idx)
{
    DepEntry *node;
    node = dependenceRAWGraph[idx][0].next;
    DynInstPtr inst = NULL;
    if (node) {
        inst = node->inst;
        dependenceRAWGraph[idx][0].next = node->next;
        node->inst = NULL;
        memAllocCounter--;
        delete node;
    }
    return inst;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::popWAR(RegIndex idx, int num)
{
    DepEntry *node;
    node = dependenceWARGraph[idx][num].next;
    DynInstPtr inst = NULL;
    if (node) {
        inst = node->inst;
        dependenceWARGraph[idx][num].next = node->next;
        node->inst = NULL;
        memAllocCounter--;
        delete node;
    }
    return inst;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::countNodes(RegIndex idx, int num) {
    int count = 0;
    DepEntry* current = dependenceWARGraph[idx][num].next;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::countNodesWAW(RegIndex idx) {
    int count = dependenceWAWGraph[idx].size();
    return count;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::countNodesRAW(RegIndex idx) {
    std::size_t size = dependenceRAWGraph[idx].size();

    return size;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::getWARlistSize(RegIndex idx) {
    int index = dependenceWARGraph[idx].size() - 1;
    int count = 0;
    DepEntry* current = dependenceWARGraph[idx][index].next;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}

template <class DynInstPtr>
int
DependencyGraph<DynInstPtr>::getInstLocInWAR(RegIndex idx, const DynInstPtr &new_inst)
{
    std::size_t size = dependenceWARGraph[idx].size();
    // iterate over all the vector entries till we find our inst
    for(std::size_t i = 0; i < size; i++) {
        if(dependenceWARGraph[idx][i].inst == new_inst)
            return i;
    }
    return -1;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraph<DynInstPtr>::getNextInst(RegIndex idx)
{
    return dependenceWAWGraph[idx][0];
}

template <class DynInstPtr>
bool
DependencyGraph<DynInstPtr>::empty() const
{
    for (int i = 0; i < numEntries; ++i) {
        if (!empty(i))
            return false;
        if(!emptyWAWGraph(i))
            return false;
    }
    return true;
}

template <class DynInstPtr>
void
DependencyGraph<DynInstPtr>::dump()
{
    DepEntry *curr;

    for (int i = 0; i < numEntries; ++i)
    {
        curr = &dependGraph[i];

        if (curr->inst) {
            cprintf("dependGraph[%i]: producer: %s [sn:%lli] consumer: ",
                    i, curr->inst->pcState(), curr->inst->seqNum);
        } else {
            cprintf("dependGraph[%i]: No producer. consumer: ", i);
        }

        while (curr->next != NULL) {
            curr = curr->next;

            cprintf("%s [sn:%lli] ",
                    curr->inst->pcState(), curr->inst->seqNum);
        }

        cprintf("\n");
    }
    cprintf("memAllocCounter: %i\n", memAllocCounter);
}

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_DEP_GRAPH_HH__
