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

def read_val(fin, stat_names):

    pattern = r'\b\d+\b'

    stat_val = []
    for i in range(len(stat_names)):
        stat_val.append(0)

    for line in fin:
        if any(keyword in line for keyword in stat_names):
            line = line.strip()
            line1 =  line.split(' ')
            filtered_list = [word for word in line1 if word != '']
            index = -10000
            splitted_words = filtered_list[0].split('.')
            check = splitted_words[-1]
            for i in range(len(stat_names)):
                #if stat_names[i] == check:
                if stat_names[i] in filtered_list[0]:
                    index = i
            if(filtered_list[1] != "nan"):
                number_value = float(filtered_list[1])
                stat_val[index] = number_value + stat_val[index]
    return stat_val

def get_stats(parent_folder, files_check, stat_names, output_file, directory):

    csv = open(output_file, "w")

    # start first line
    line = "input,benchmark,stat,1C8T S,2C4T S,4C2T S,8C1T S,1C8T W,2C4T W,4C2T W,8C1T W"
    line = line + "\n"
    csv.write(line)

    # go over each file:
    for bmrk in parent_folder:
        # a subset has data for all S and W configs
        for file_subset in files_check:
            all_stats = []
            for i in range(len(stat_names)):
                all_stats.append([])
            for file in file_subset:
                file_to_read = directory +"/"+ bmrk +"/"+ file + "/stats.txt"
                dir_ro_search = directory +"/"+ bmrk +"/"+ file
                dir_to_search_in = directory +"/"+ bmrk
                file_name = os.path.basename(dir_ro_search)
                files_in_directory = os.listdir(dir_to_search_in)
                if file_name in files_in_directory:
                    fin=open(file_to_read,"r")
                    packed_stats =  read_val(fin, stat_names)
                    for i in range(len(packed_stats)):
                        all_stats[i].append(packed_stats[i])
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

def main():

    parent_folder = [
    #  "bc",
    #   "apsp",
    #   "bfs",
    #   "community_detection",
    #   "pagerank",
    #   "triangle_counting",
    #   "connected_components",
    #    "unittest_res"
        "matmul"
    ]  

    files_check = [
        [
            # "C1T1_7_5000_20",
            # "C1T1_1_5000_20_half",
            # "C1T2_bfsSpagerankW",
            # "C1T2_bfsSpagerankW_double"
            #"C1T1_7_5000_20",
            #"C1T1_7_5000_20W",
            #"C1T2_bfsSbfsW",
            #"C1T2_bfsSbfsW_double"
            "test_matmulS",
            "test_matmulW",
            "test_matmulSW"
        ]
    ]

    stat_names = ["simTicks","issueRate","numIssuedDist::mean","numCommittedDist::mean","fetch.nisnDist::mean","icache.ReadReq.accesses::total","wbRate::total","rename.ROBFullEventsS","rename.ROBFullEventsW","rename.idleCycles","rename.IQFullEventsS","rename.IQFullEventsW","rename.LQFullEventsS","rename.LQFullEventsW","rename.SQFullEventsS","rename.SQFullEventsW","rename.fullRegistersEventsS","rename.fullRegistersEventsW","rename.blockCycles","iew.blockCycles","decode.blockedCycles","decode.idleCycles","iew.idleCycles","statFuBusyPerThreadCollective","statStalledOnControlInstructionPerThread","statStalledOnMemoryReorderPerThread","statStalledNotOldestInIQPerThread","statNumCheckIssuePerThread","statNumIssueNotPossiblePerThread","fuBusy","totalIpc"]

    # # stat_names = ["numCycles","numCommittedDist::0","numCommittedDist::1","numCommittedDist::2","numCommittedDist::3","numCommittedDist::4","numCommittedDist::5","numCommittedDist::6","numCommittedDist::7","numCommittedDist::8"]

    # # stat_names = ["numCommittedDist::mean"]

    #stat_names = ["simTicks"]

    output_file = "stats.csv"
    directory = os.getcwd()

    get_stats(parent_folder, files_check, stat_names, output_file, directory)


if __name__ == "__main__":
    main()

# %%
