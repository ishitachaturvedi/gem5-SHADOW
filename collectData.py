# %%
from cmath import nan
from enum import Enum
import glob, os
from os import listdir
from os.path import isfile, join
from os import walk
import statistics
import matplotlib.pyplot as plt
import numpy as np
import statistics
from statistics import mean
import re

def read_val(fin, stat_names, stat_names_multithread, stage_specfic_stats):

    pattern = r'\b\d+\b'

    stat_val = []
    all_stage_stats = []
    stat_val_stage = []
    # add entries for general stats
    for i in range(len(stat_names)):
        stat_val.append(0)
    # add entries for stage specific stats + place all stage specific stats in 1 list for easier initial check
    for i in range(len(stage_specfic_stats)):
        stat_val_stage.append([])
        for j in range(len(stage_specfic_stats[i][1])):
            stat_val_stage[i].append(0)
            all_stage_stats.append(stage_specfic_stats[i][1][j])

    for line in fin:
        if any(keyword in line for keyword in (stat_names + stat_names_multithread + all_stage_stats)):
            line = line.strip()
            line1 =  line.split(' ')
            filtered_list = [word for word in line1 if word != '']
            index = -10000
            index1 = -10000
            index2 = -10000
            splitted_words = filtered_list[0].split('.')
            # take the first word
            # consider the last word of splitted_words:
            # Split based on ":"
            # filtered_list1 = splitted_words[-1].split('::')
            filtered_list1 = splitted_words[-1].split('.')
            # now split if "_" 
            filtered_list2 = filtered_list1[0].split('_')
            final_word = filtered_list2[0]
            for i in range(len(stat_names)):
                if(stat_names[i].strip() == final_word.strip()):
                    index = i
                    break
            if(index!=-10000):
                if(filtered_list[1] != "nan"):
                    number_value = float(filtered_list[1])
                    stat_val[index] = number_value + stat_val[index]
            if any(keyword1 in line for keyword1 in (all_stage_stats)):    
                for i in range(len(stage_specfic_stats)):
                    if(splitted_words[-2].strip() == stage_specfic_stats[i][0].strip()):
                        for j in range(len(stage_specfic_stats[i][1])):
                            if(stage_specfic_stats[i][1][j].strip() == final_word.strip()):
                                index1 = i
                                index2 = j
                                break
                if(index1!=-10000 and index2!=-10000):
                    if(filtered_list[1] != "nan"):
                        number_value = float(filtered_list[1])
                        stat_val_stage[index1][index2] = number_value + stat_val_stage[index1][index2]
    return stat_val, stat_val_stage

def get_stats(parent_folder, files_check, stat_names, stat_names_multithread, output_file, directory, stage_specfic_stats):

    csv = open(output_file, "w")

    # start first line
    line = "input,benchmark,stat,Strong,Strong+Weak"
    line = line + "\n"
    csv.write(line)

    # go over each file:
    for bmrk in parent_folder:
        # a subset has data for all S and W configs
        for file_subset in files_check:
            all_stats = []
            all_stats_stage = []
            for i in range(len(stat_names)):
                all_stats.append([])
            for i in range(len(stage_specfic_stats)):
                all_stats_stage.append([])
                for j in range(len(stage_specfic_stats[i][1])):
                    all_stats_stage[i].append([])
            for file in file_subset:
                file_to_read = directory +"/"+ bmrk +"/"+ file + "/stats.txt"
                dir_ro_search = directory +"/"+ bmrk +"/"+ file
                dir_to_search_in = directory +"/"+ bmrk
                file_name = os.path.basename(dir_ro_search)
                files_in_directory = os.listdir(dir_to_search_in)
                if file_name in files_in_directory:
                    fin=open(file_to_read,"r")
                    packed_stats, packed_stats_stage =  read_val(fin, stat_names, stat_names_multithread, stage_specfic_stats)
                    for i in range(len(packed_stats)):
                        all_stats[i].append(packed_stats[i])
                    for i in range(len(packed_stats_stage)):
                        for j in range(len(packed_stats_stage[i])):
                            all_stats_stage[i][j].append(packed_stats_stage[i][j])
                else:
                    for i in range(len(stat_names)):
                        all_stats[i].append(-1)
        
            os.chdir(directory)
            for i in range(len(stat_names)):
                # place value in the csv 
                substring = file_subset[0][2:]
                line = substring + ","
                line = line + bmrk + ","
                line = line + stat_names[i] + ","
                for j in range(len(file_subset)):
                    line = line + str(all_stats[i][j]) + ","
                line = line + "\n"
                csv.write(line)
            # make entries fir stage specific stats
            for i in range(len(stage_specfic_stats)):
                for j in range(len(stage_specfic_stats[i][1])):
                    substring = file_subset[0][2:]
                    line = substring + ","
                    line = line + bmrk + ","
                    line = line + stage_specfic_stats[i][0]+"."+stage_specfic_stats[i][1][j] + ","
                    for k in range(len(file_subset)):
                        line = line + str(all_stats_stage[i][j][k]) + ","
                    line = line + "\n"
                    csv.write(line)
def main():

    parent_folder = [
        "matmul"
    ]  

    files_check = [
        [
            "test_matmulS",
            "test_matmulSW"
        ]
    ]

    # global stats
    stat_names = ["simTicks","issueRate","numIssuedDist::mean","statFuBusyPerThreadCollective","statStalledOnControlInstructionPerThread","statNumIssueNotPossiblePerThread","fuBusyS","fuBusyW","totalIpc", "cycleCountS", "cycleCountW","statFuNoFree::IntAlu","statFuNoFree::IntMult","statFuNoFree::IntDiv","statFuNoFree::MemRead","statFuNoFree::MemWrite"]

    fetch_stats = ["cacheStallCycles","icacheStallCyclesSThread","icacheStallCyclesWThread","noActiveThreadStallCycles","blockedCycles","insts","instsSThread","instsWThread","branches","cycles","blockedCycles","noActiveThreadStallCycles","icacheWaitRetryStallCycles","cacheLines","icacheSquashes","tlbSquashes","idleRate","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","multipleRunning"]

    decode_stats = ["idleCycles","blockedCycles","runCycles","unblockCycles","branchMispred","branchMispredSThread","branchMispredWThread","controlMispred","stalledS","stalledW","notStalled","blocking","blockingS"]

    rename_stats = ["squashCycles","squashCyclesSThread","squashCyclesWThread","idleCycles","idleCyclesSThread","idleCyclesWThread","blockCycles","blockCyclesSThread","blockCyclesWThread","serializeStallCycles","runCycles","runCyclesSThread","runCyclesWThread","ROBFullEvents","ROBFullEventsS","ROBFullEventsW","IQFullEvents","IQFullEventsS","IQFullEventsW","LQFullEvents","LQFullEventsS","LQFullEventsW","SQFullEvents","SQFullEventsS","SQFullEventW","fullRegistersEvents","fullRegistersEventsS","fullRegistersEventsW","renamedOperands","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","blockingIQFull","blockingIQFullS","blockingIQFullW","blockingROBFull","blockingROBFullS","blockingROBFullW","blockingBandwidthFull","blockingBandwidthFullS","blockingBandwidthFullW","blockingRegFull","blockingRegFullS","blockingRegFullW"]

    iew_stats = ["idleCycles","idleCyclesS","idleCyclesW","squashCycles","squashCyclesS","squashCyclesW","blockCycles","blockCyclesS","blockCyclesW","unblockCycles","dispatchedInsts","dispSquashedInsts","dispSquashedInstsS","dispSquashedInstsW","iqFullEvents","iqFullEventsS","iqFullEventsW","lsqFullEvents","lsqFullEventsS","lsqFullEventsW","memOrderViolationEvents","branchMispredicts","instsToCommit","wbRateTotal","wbRateSTotal","wbRateWTotal","stalledS","stalledW","notStalled","blockingS","blockingW","blockingIQFull","blockingIQFullS","blockingLSQFull","blockingLSQFullS","blockingBandwidthFull","blockingBandwidthFullS"]

    commit_stats = ["commitSquashedInsts","branchMispredicts","branchMispredictsS","branchMispredictsW","totalTransitTimeTotal","totalTransitTimeSTotal","totalTransitTimeWTotal","totalInstructionTimeTotal","totalInstructionTimeSTotal","totalInstructionTimeWTotal","IEWTimeTotal","IQTimeTotal","ROBTimeTotal","totalReadyTimeTotal","renameTimeTotal","decodeTimeTotal","fetchTimeTotal"]

    stage_specfic_stats = [["fetch",fetch_stats],["decode",decode_stats],["rename",rename_stats],["iew",iew_stats],["commit",commit_stats]]
    
    stat_names_multithread = []
    

    output_file = "stats.csv"
    directory = os.getcwd()

    get_stats(parent_folder, files_check, stat_names, stat_names_multithread, output_file, directory, stage_specfic_stats)


if __name__ == "__main__":
    main()

# %%
