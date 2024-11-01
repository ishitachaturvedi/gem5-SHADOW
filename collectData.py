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

# %%
# generate a csv file which compares the stats of 2 files

def read_val(fin, stat_names, stage_specfic_stats, cpu_stage_specfic_stats, cpulist, SimStats, m5_ops_stats, loop_type):

    stat_name_main = []
    stat_val = []
    stat_val_temp = []
    for i in range(len(SimStats)):
        stat_name_main.append(SimStats[i])
        stat_val.append(0)
        stat_val_temp.append(0)

    # add entries for general stats
    for i in range(len(stat_names)):
        stat_val.append(0)
        stat_val_temp.append(0)
        if(len(cpulist) == 0):
            stat_name_main.append("system.cpu_cluster.cpus."+stat_names[i])
        else:
            for i in len(cpulist):
                stat_name_main.append("system.cpu_cluster."+cpulist[i]+"."+stat_names[i])
    # add entries for stage specific stats + place all stage specific stats in 1 list for easier initial check
    for i in range(len(stage_specfic_stats)):
        for j in range(len(stage_specfic_stats[i][1])):
            stat_val.append(0)
            stat_val_temp.append(0)
            stat_name_main.append("system.cpu_cluster."+stage_specfic_stats[i][0]+"."+stage_specfic_stats[i][1][j])
    for i in range(len(cpu_stage_specfic_stats)):
        for j in range(len(cpu_stage_specfic_stats[i][1])):
            stat_val.append(0)
            stat_val_temp.append(0)
            if(len(cpulist) == 0):
                stat_name_main.append("system.cpu_cluster.cpus."+cpu_stage_specfic_stats[i][0]+"."+cpu_stage_specfic_stats[i][1][j])
            else:
                for i in len(cpulist):
                    stat_name_main.append("system.cpu_cluster."+cpulist[i]+"."+cpu_stage_specfic_stats[i][0]+"."+cpu_stage_specfic_stats[i][1][j])
    ticks_per_cycle = 0

    avg_counter = 0
    line_look_for = 0
    line_found = 0
    if(m5_ops_stats):
        if(loop_type == "main_loop"):
            line_look_for = "thread_1.numIters"
        else:
            print("loop_type ",loop_type," not implemented")
            exit(0)

    for line in fin:
        line = line.strip()
        line1 =  line.split(' ')
        # if we are starting new data collection with m5_ops, then we need to add new data to the total count
        if(loop_type == "main_loop" and m5_ops_stats and ("Begin Simulation Statistics" in line) and line_found):
            line_found = 0
            for i in range(len(stat_val)):
                stat_val[i] += stat_val_temp[i]
            avg_counter += 1
        # get number of ticks per cycle
        if("system.cpu_cluster.clk_domain.clock" in line1[0]):
            line1 = [x for x in line1 if x != '']
            ticks_per_cycle = float(line1[1])
        if(line1[0] in (stat_name_main)):
            line1 = [x for x in line1 if x != '']
            index = stat_name_main.index(line1[0])
            value =  float(line1[1])
            # if the value is a tick divide it by ticks_per_cycle to get # of cycles
            if("Tick" in line1[-1] and ticks_per_cycle!=0):
               value =  value / ticks_per_cycle
            if not m5_ops_stats:
                stat_val[index] = value
            else:
                stat_val_temp[index] = value
        if(m5_ops_stats and line_look_for in line):
            line1 = [x for x in line1 if x != '']
            value =  float(line1[1])
            if(value > 0):
                line_found = 1

    if(m5_ops_stats):
        for i in range(len(stat_val)):
            stat_val[i] = stat_val[i]/avg_counter

    print("avg_counter ",avg_counter)

    return stat_val

def get_stats(parent_folder, files_check, stat_names, output_file, directory, stage_specfic_stats, cpu_stage_specfic_stats, top_line, cpulist, SimStats, m5_ops_stats, loop_type):

    csv = open(output_file, "w")

    # start first line
    line = top_line
    line = line + "\n"
    csv.write(line)

    stat_name_main = []

    for i in range(len(SimStats)):
        stat_name_main.append(SimStats[i])
    for i in range(len(stat_names)):
        if(len(cpulist) == 0):
            stat_name_main.append("system.cpu_cluster.cpus."+stat_names[i])
        else:
            for i in len(cpulist):
                stat_name_main.append("system.cpu_cluster."+cpulist[i]+"."+stat_names[i])
    # add entries for stage specific stats + place all stage specific stats in 1 list for easier initial check
    for i in range(len(stage_specfic_stats)):
        for j in range(len(stage_specfic_stats[i][1])):
            stat_name_main.append("system.cpu_cluster."+stage_specfic_stats[i][0]+"."+stage_specfic_stats[i][1][j])
    for i in range(len(cpu_stage_specfic_stats)):
        for j in range(len(cpu_stage_specfic_stats[i][1])):
            if(len(cpulist) == 0):
                stat_name_main.append("system.cpu_cluster.cpus."+cpu_stage_specfic_stats[i][0]+"."+cpu_stage_specfic_stats[i][1][j])
            else:
                for i in len(cpulist):
                    stat_name_main.append("system.cpu_cluster."+cpulist[i]+"."+cpu_stage_specfic_stats[i][0]+"."+cpu_stage_specfic_stats[i][1][j])

    # go over each file:
    for bmrk in parent_folder:
        # a subset has data for all S and W configs
        for file_subset in files_check:
            all_stats = []
            for i in range(len(stat_name_main)):
                all_stats.append([])
            for file in file_subset:
                file_to_read = directory +"/"+ bmrk +"/"+ file + "/stats.txt"
                dir_ro_search = directory +"/"+ bmrk +"/"+ file
                dir_to_search_in = directory +"/"+ bmrk
                print("Working on bmrk ",file)
                file_name = os.path.basename(dir_ro_search)
                files_in_directory = os.listdir(dir_to_search_in)
                if file_name in files_in_directory:
                    fin=open(file_to_read,"r")
                    stat_val=  read_val(fin, stat_names, stage_specfic_stats, cpu_stage_specfic_stats, cpulist, SimStats, m5_ops_stats, loop_type)
                    for i in range(len(stat_val)):
                        all_stats[i].append(stat_val[i])
                else:
                    for i in range(len(stat_val)):
                        all_stats[i].append(-1)


            os.chdir(directory)
            for i in range(len(stat_name_main)):
                # place value in the csv 
                substring = file_subset[0]
                line = substring + ","
                line = line + bmrk + ","
                line = line + stat_name_main[i] + ","
                for j in range(len(file_subset)):
                    line = line + str(all_stats[i][j]) + ","
                line = line + "\n"
                csv.write(line)


def plot_per_cycle_status(file):
    fetch = []
    decode = []
    rename = []
    issue = []
    execute = []
    writeback = []
    commit = []
    cycles = []

    fetch0 = []
    decode0 = []
    rename0 = []
    issue0 = []
    execute0 = []
    writeback0 = []
    commit0 = []
    cycles0 = []

    fetch1 = []
    decode1 = []
    rename1 = []
    issue1 = []
    execute1 = []
    writeback1 = []
    commit1 = []
    cycles1 = []

    counter = 20
    iter = 0
    main_start_index = 100000
    main_end_index = 150000
    size_of_chunk = 50000
    indexs_passed = 0

    marker_styles = ['+', 'x', '^']

    fin=open(file,"r")

    for line in fin:
        line = line.strip()
        line1 =  line.split(' ')
        if("fetch_vals_sent" in line):
            fetch.append(int(line1[-1]))
        if("fetch_vals_0_sent" in line):
            fetch0.append(int(line1[-1]))
        if("fetch_vals_1_sent" in line):
            fetch1.append(int(line1[-1]))
        if("decode_vals_sent" in line):
            decode.append(int(line1[-1]))
        if("decode_vals_0_sent" in line):
            decode0.append(int(line1[-1]))
        if("decode_vals_1_sent" in line):
            decode1.append(int(line1[-1]))
        if("rename_vals_sent" in line):
            rename.append(int(line1[-1]))
        if("rename_vals_0_sent" in line):
            rename0.append(int(line1[-1]))
        if("rename_vals_1_sent" in line):
            rename1.append(int(line1[-1]))
        if("issue_vals_sent" in line):
            issue.append(int(line1[-1]))
        if("issue_vals_0_sent" in line):
            issue0.append(int(line1[-1]))
        if("issue_vals_1_sent" in line):
            issue1.append(int(line1[-1]))
        if("execute_vals_sent" in line):
            execute.append(int(line1[-1]))
        if("execute_vals_0_sent" in line):
            execute0.append(int(line1[-1]))
        if("execute_vals_1_sent" in line):
            execute1.append(int(line1[-1]))
        if("writeback_vals_sent" in line):
            writeback.append(int(line1[-1]))
        if("writeback_vals_0_sent" in line):
            writeback0.append(int(line1[-1]))
        if("writeback_vals_1_sent" in line):
            writeback1.append(int(line1[-1]))
        if("commit_vals_sent" in line):
            commit.append(int(line1[-1]))
        if("commit_vals_0_sent" in line):
            commit0.append(int(line1[-1]))
        if("commit_vals_1_sent" in line):
            commit1.append(int(line1[-1]))
            indexs_passed += 1
            
            if(len(commit1) ==  size_of_chunk):
                if (indexs_passed == (main_start_index + (iter+1)* size_of_chunk)):

                    for i in range(len(writeback)):
                        cycles.append(i)

                    start_index = main_start_index + (iter)* size_of_chunk
                    end_index = start_index + size_of_chunk

                    print("testing indexs_passed ",indexs_passed," start_index ",start_index," end_index ",end_index," cycles ",len(cycles)," fetch ",len(fetch))

                    # plot this data
                    # Create new figure and axis for Fetch
                    fig_fetch, ax_fetch = plt.subplots()
                    ax_fetch.scatter(cycles, fetch, label='Fetch', marker=marker_styles[0])
                    ax_fetch.scatter(cycles, fetch0, label='Fetch0', marker=marker_styles[1])
                    ax_fetch.scatter(cycles, fetch1, label='Fetch1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_fetch.set_xlabel('Cycle')
                    ax_fetch.set_ylabel('Time')
                    ax_fetch.set_title('Fetch')
                    # Adding legend
                    ax_fetch.legend()
                    fig_fetch.set_size_inches(20, 12)
                    # Save the plot
                    val = str(iter)
                    plt.savefig('plots/Fetch_' + file   + val + '.png')
                    plt.close(fig_fetch)

                    # Create new figure and axis for Decode
                    fig_decode, ax_decode = plt.subplots()
                    ax_decode.scatter(cycles, decode, label='Decode', marker=marker_styles[0])
                    ax_decode.scatter(cycles, decode0, label='Decode0', marker=marker_styles[1])
                    ax_decode.scatter(cycles, decode1, label='Decode1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_decode.set_xlabel('Cycle')
                    ax_decode.set_ylabel('Time')
                    ax_decode.set_title('Decode')
                    # Adding legend
                    ax_decode.legend()
                    fig_decode.set_size_inches(20, 12)
                    # Save the plot
                    plt.savefig('plots/Decode_' + file   + val + '.png')
                    plt.close(fig_decode)        

                    # Create new figure and axis for Rename
                    fig_rename, ax_rename = plt.subplots()
                    ax_rename.scatter(cycles, rename, label='Rename', marker=marker_styles[0])
                    ax_rename.scatter(cycles, rename0, label='Rename0', marker=marker_styles[1])
                    ax_rename.scatter(cycles, rename1, label='Rename1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_rename.set_xlabel('Cycle')
                    ax_rename.set_ylabel('Time')
                    ax_rename.set_title('Rename')
                    # Adding legend
                    ax_rename.legend()
                    fig_rename.set_size_inches(20, 12)
                    # Save the plot
                    val = str(iter)
                    plt.savefig('plots/Rename_' + file  + val + '.png')
                    plt.close(fig_rename)

                    # Create new figure and axis for Issue
                    fig_issue, ax_issue = plt.subplots()
                    ax_issue.scatter(cycles, issue, label='Issue', marker=marker_styles[0])
                    ax_issue.scatter(cycles, issue0, label='Issue0', marker=marker_styles[1])
                    ax_issue.scatter(cycles, issue1, label='Issue1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_issue.set_xlabel('Cycle')
                    ax_issue.set_ylabel('Time')
                    ax_issue.set_title('Issue')
                    # Adding legend
                    ax_issue.legend()
                    fig_issue.set_size_inches(20, 12)
                    # Save the plot
                    plt.savefig('plots/Issue_' + file  + val + '.png')
                    plt.close(fig_issue)

                    # Create new figure and axis for Execute
                    fig_execute, ax_execute = plt.subplots()
                    ax_execute.scatter(cycles, execute, label='Execute', marker=marker_styles[0])
                    ax_execute.scatter(cycles, execute0, label='Execute0', marker=marker_styles[1])
                    ax_execute.scatter(cycles, execute1, label='Execute1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_execute.set_xlabel('Cycle')
                    ax_execute.set_ylabel('Time')
                    ax_execute.set_title('Execute')
                    # Adding legend
                    ax_execute.legend()
                    fig_execute.set_size_inches(20, 12)
                    # Save the plot
                    plt.savefig('plots/Execute_' + file  + val + '.png')
                    plt.close(fig_execute)

                    # Create new figure and axis for Writeback
                    fig_writeback, ax_writeback = plt.subplots()
                    ax_writeback.scatter(cycles, writeback, label='Writeback', marker=marker_styles[0])
                    ax_writeback.scatter(cycles, writeback0, label='Writeback0', marker=marker_styles[1])
                    ax_writeback.scatter(cycles, writeback1, label='Writeback1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_writeback.set_xlabel('Cycle')
                    ax_writeback.set_ylabel('Time')
                    ax_writeback.set_title('Writeback')
                    # Adding legend
                    ax_writeback.legend()
                    fig_writeback.set_size_inches(20, 12)
                    # Save the plot
                    plt.savefig('plots/Writeback_' + file + val + '.png')
                    plt.close(fig_writeback)

                    # Create new figure and axis for Commit
                    fig_commit, ax_commit = plt.subplots()
                    ax_commit.scatter(cycles, commit, label='Commit', marker=marker_styles[0])
                    ax_commit.scatter(cycles, commit0, label='Commit0', marker=marker_styles[1])
                    ax_commit.scatter(cycles, commit1, label='Commit1', marker=marker_styles[2])
                    # Adding labels and title
                    ax_commit.set_xlabel('Cycle')
                    ax_commit.set_ylabel('Time')
                    ax_commit.set_title('Commit')
                    # Adding legend
                    ax_commit.legend()
                    fig_commit.set_size_inches(20, 12)
                    # Save the plot
                    plt.savefig('plots/Commit_' + file + val + '.png')
                    plt.close(fig_commit)

                    iter += 1

                    if(iter == counter):
                        break

                # clear all the lists
                fetch = []
                decode = []
                rename = []
                issue = []
                execute = []
                writeback = []
                commit = []
                cycles = []

                fetch0 = []
                decode0 = []
                rename0 = []
                issue0 = []
                execute0 = []
                writeback0 = []
                commit0 = []
                cycles0 = []

                fetch1 = []
                decode1 = []
                rename1 = []
                issue1 = []
                execute1 = []
                writeback1 = []
                commit1 = []
                cycles1 = []

def main():

    # parent_folder = [
    #     "HIP_pthreads_bmrk/prefix_sum/results"
    # ]  

    # files_check = [
    #     [
    #         "ThreadS_32",
    #         "ThreadSW1_32",
    #         "ThreadSW2_32",
    #         "ThreadSW3_32",
    #         "ThreadSW4_32",
    #         "Thread6S_32",
    #         "ThreadS1W4_32",
    #         "APU_32"
    #     ],
    #     [
    #         "ThreadS_512",
    #         "ThreadSW1_512",
    #         "ThreadSW2_512",
    #         "ThreadSW3_512",
    #         "ThreadSW4_512",
    #         "Thread6S_512",
    #         "ThreadS1W4_512",
    #         "APU_512"
    #     ],
    #     [
    #         "ThreadS_1024",
    #         "ThreadSW1_1024",
    #         "ThreadSW2_1024",
    #         "ThreadSW3_1024",
    #         "ThreadSW4_1024",
    #         "Thread6S_1024",
    #         "ThreadS1W4_1024",
    #         "APU_1024"
    #     ],
    #     [
    #         "ThreadS_8192",
    #         "ThreadSW1_8192",
    #         "ThreadSW2_8192",
    #         "ThreadSW3_8192",
    #         "ThreadSW4_8192",
    #         "Thread6S_8192",
    #         "ThreadS1W4_8192",
    #         "APU_8192"
    #     ],
    #     [
    #         "ThreadS_16384",
    #         "ThreadSW1_16384",
    #         "ThreadSW2_16384",
    #         "ThreadSW3_16384",
    #         "ThreadSW4_16384",
    #         "Thread6S_16384",
    #         "ThreadS1W4_16384",
    #         "APU_16384"
    #     ],
    #     [
    #         "ThreadS_65536",
    #         "ThreadSW1_65536",
    #         "ThreadSW2_65536",
    #         "ThreadSW3_65536",
    #         "ThreadSW4_65536",
    #         "Thread6S_65536",
    #         "ThreadS1W4_65536",
    #         "APU_65536"
    #     ]
    #]

    parent_folder = [
        #"HIP_pthreads_bmrk/convolution/results"
        #"sparse_matrix/novocore_changing_matrix_size"
        #'sparse_matrix/Scale_everything'
        #"results/CRONO"
        "dense_matrix/Scale_everything"
        #"CRONO_results/bc/test"
    ]  

    m5_ops_stats = True

    files_check = [
        [
            # DENSE MATRIX
            "Dynamic_1pThread_200_50_5_baseline",
            "Dynamic_1pThread_200_50_5_scaled",
            "pthreadDynamic_2S2W_200_50_5_baseline",
            "pthreadDynamic_2S2W_200_50_5_scaled",
            "pthreadDynamic_2S3W_200_50_5_baseline",
            "pthreadDynamic_2S3W_200_50_5_scaled",
            "pthreadDynamic_2S4W_200_50_5_baseline",
            "pthreadDynamic_2S4W_200_50_5_scaled",
            "pthreadDynamic_2S5W_200_50_5_baseline",
            "pthreadDynamic_2S5W_200_50_5_scaled",

            # "Dynamic_1pThread_64_2",
            # "pthreadDynamic_2S2W_64_2",
            # "pthreadDynamic_2S3W_64_2",
            # "pthreadDynamic_2S4W_64_2",
            # "pthreadDynamic_2S5W_64_2",
            # "pthreadDynamic_2S6W_64_2",
            # "pthreadDynamic_2S7W_64_2",


            # SPARSE
            # "Dynamic_1pThread_1000_90Per_sparse_baseline",
            # "Dynamic_1pThread_1000_90Per_sparse_scaled",
            # "pthreadDynamic_2S1W_1000_90Per_sparse_baseline",
            # "pthreadDynamic_2S1W_1000_90Per_sparse_scaled",
            # # "pthreadDynamic_2S2W_1000_90Per_sparse_baseline",
            # # "pthreadDynamic_2S2W_1000_90Per_sparse_scaled",
            # "pthreadDynamic_2S3W_1000_90Per_sparse_baseline",
            # "pthreadDynamic_2S3W_1000_90Per_sparse_scaled",
            # "pthreadDynamic_2S4W_1000_90Per_sparse_baseline",
            # "pthreadDynamic_2S4W_1000_90Per_sparse_scaled",
            # "pthreadDynamic_2S5W_1000_90Per_sparse_baseline",
            # "pthreadDynamic_2S5W_1000_90Per_sparse_scaled",
            # "pthreadDynamic_2S6W_1000_90Per_sparse_baseline",
            # "pthreadDynamic_2S6W_1000_90Per_sparse_scaled",

        ]
    ]

    top_line = "input,benchmark,stat"
    for i in range(len(files_check[0])):
        top_line = top_line + "," + str(files_check[0][i])

    SimStats = ["simTicks"]

    # global stats

    # rename_stats = []
    # fetch_stats = []
    # iew_stats = []

    stat_names = ["numCycles","issueRate","totalIpc","ipc::1","ipc::0","TotalInstIssued","TotalOoOInstIssued","TotalInstIssued::0","TotalOoOInstIssued::0","TotalInstIssued::1","TotalOoOInstIssued::1"]

    stat_names = ["numCycles","issueRate","numIssuedDist::mean","statFuBusyPerThreadCollective","statStalledOnControlInstructionPerThread","statNumIssueNotPossiblePerThread","fuBusyS","fuBusyW","totalIpc", "cycleCountS", "cycleCountW","statFuNoFree::IntAlu","statFuNoFree::IntMult","statFuNoFree::IntDiv","statFuNoFree::MemRead","statFuNoFree::MemWrite","ipc::0","ipc::1","ipc::2","ipc::3","ipc::4","ipc::5","cpi::0","cpi::1","cpi::2","cpi::3","cpi::4","cpi::5","totalCpi","committedInsts::0","committedInsts::1","committedInsts::2","committedInsts::3","committedInsts::4","committedInsts::5","fuBusyRate::0","fuBusyRate::1","fuBusyRate::2","fuBusyRate::3","fuBusyRate::4","fuBusyRate::5","NoReadyInst","ReadyInstMoreThanBW","TotalOoOInstIssued::1","thread_1.numIters","thread_2.numIters","thread_3.numIters","thread_4.numIters","thread_5.numIters","thread_6.numIters","thread_7.numIters","thread_8.numIters","thread_1.MutexOverhead","thread_2.MutexOverhead","thread_3.MutexOverhead","thread_4.MutexOverhead","thread_1.BarrierOverhead","thread_2.BarrierOverhead","thread_3.BarrierOverhead","thread_4.BarrierOverhead","TimeSpentWaitingOnMem::0","TimeSpentWaitingOnMem::1","TimeSpentWaitingOnMem::2","TimeSpentWaitingOnMem::3","TimeSpentWaitingOnMem::4","AverageInstinIQ::0","AverageInstinIQ::1","AverageInstinIQ::2","AverageInstinIQ::total","statFuBusy::IntMult","statFuBusy::MemRead","statFuBusy::MemWrite","statAvgFUUtilization::IntMult","statAvgFUUtilization::MemRead","statAvgFUUtilization::MemWrite","AverageNonIssuedInstPerThread::0","AverageNonIssuedInstPerThread::1","AverageNonIssuedInstPerThread::2","AverageNonIssuedInstPerThread::3","AverageNonIssuedInstPerThread::4","AverageNonIssuedInstPerThread::total","AverageIssuedInstPerThread::0","AverageIssuedInstPerThread::1","AverageIssuedInstPerThread::2","AverageIssuedInstPerThread::3","AverageIssuedInstPerThread::total","AverageInstsThread::0","AverageInstsThread::1","AverageInstsThread::2","AverageInstsThread::3","AverageInstsThread::4","AverageInstsThread::total","AverageNonIssuedInstPerThreadDependsOnMemory::0","AverageNonIssuedInstPerThreadDependsOnMemory::1","AverageNonIssuedInstPerThreadDependsOnMemory::2","AverageNonIssuedInstPerThreadDependsOnMemory::3","AverageNonIssuedInstPerThreadDependsOnMemory::4","AverageNonIssuedInstPerThreadDependsOnMemory::total","AverageNonIssuedInstPerThreadDependsOnCompute::0","AverageNonIssuedInstPerThreadDependsOnCompute::1","AverageNonIssuedInstPerThreadDependsOnCompute::2","AverageNonIssuedInstPerThreadDependsOnCompute::3","AverageNonIssuedInstPerThreadDependsOnCompute::4","AverageNonIssuedInstPerThreadDependsOnCompute::total","AverageNonIssuedInstPerThreadDependsOnTotal::0","AverageNonIssuedInstPerThreadDependsOnTotal::1","AverageNonIssuedInstPerThreadDependsOnTotal::2","AverageNonIssuedInstPerThreadDependsOnTotal::3","AverageNonIssuedInstPerThreadDependsOnTotal::4","AverageNonIssuedInstPerThreadDependsOnTotal::total","AvgInstInFlight","AvgMemInFlight","AvgCompInFlight","AvgInstStalled","AvgMemStalled","AvgCompStalled","AvgOnlyMemInFLight","AvgOnlyCompInFLight","AvgMemAndCompInFLight","AvgNothingInFLight","cannot_issue_Flight","ready_for_issue_Flight","issued_Flight","checked_for_issue_Flight","in_ready_queue_Flight","executed_Flight","ready_for_commit_Flight","committed_Flight"]

    l2_stats = ["overallMissRate::total","overallMissLatency::total","demandHits::total","demandMisses::total","demandMissLatency::total","demandAccesses::total"]

    fetch_stats = ["cacheStallCycles","icacheStallCycles","icacheStallCyclesSThread","icacheStallCyclesWThread","noActiveThreadStallCycles","blockedCycles","insts","instsSThread","instsWThread","branches","cycles","blockedCycles","noActiveThreadStallCycles","icacheWaitRetryStallCycles","cacheLines","icacheSquashes","tlbSquashes","idleRate","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","multipleRunning","nisnDist::0","nisnDist::1","nisnDist::2","nisnDist::3","nisnDist::4","nisnDist::5","nisnDist::6","nisnDist::7","nisnDist::8","nisnDist::9","nisnDist::10","nisnDist::11","nisnDist::12","nisnDist::mean","squashCycles","rate","branchRate","noInstFetched","NoThreadToFetch","RunningCount::0","RunningCount::1","RunningCount::2","RunningCount::total","IdleCount::0","IdleCount::1","IdleCount::2","IdleCount::total","SquashingCount::0","SquashingCount::1","SquashingCount::2","SquashingCount::total","BlockedCount::0","BlockedCount::1","BlockedCount::2","BlockedCount::total","FetchingCount::0","FetchingCount::1","FetchingCount::2","FetchingCount::total","TrapPendingCount::0","TrapPendingCount::1","TrapPendingCount::2","TrapPendingCount::total","QuiescePendingCount::0","QuiescePendingCount::1","QuiescePendingCount::2","QuiescePendingCount::total","ItlbWaitCount::0","ItlbWaitCount::1","ItlbWaitCount::2","ItlbWaitCount::total","IcacheWaitResponseCount::0","IcacheWaitResponseCount::1","IcacheWaitResponseCount::2","IcacheWaitResponseCount::total","IcacheWaitRetryCount::0","IcacheWaitRetryCount::1","IcacheWaitRetryCount::2","IcacheWaitRetryCount::total","IcacheAccessCompleteCount::0","IcacheAccessCompleteCount::1","IcacheAccessCompleteCount::2","IcacheAccessCompleteCount::total","NoGoodAddrCount::0","NoGoodAddrCount::1","NoGoodAddrCount::2","NoGoodAddrCount::total","BlockedOnBranchCount::0","BlockedOnBranchCount::1","BlockedOnBranchCount::2","BlockedOnBranchCount::total","FetchNotValid::0","FetchNotValid::1","FetchNotValid::2","FetchNotValid::total","FetchBufferExceeded::0","FetchBufferExceeded::1","FetchBufferExceeded::2","FetchBufferExceeded::total","FetchQueueFull::0","FetchQueueFull::1","FetchQueueFull::2","FetchQueueFull::total","NeedToFetchMoreMemory::0","NeedToFetchMoreMemory::1","NeedToFetchMoreMemory::2","NeedToFetchMoreMemory::total","QuiescePendingForThread::0","QuiescePendingForThread::1","QuiescePendingForThread::2","QuiescePendingForThread::total","BlockedOnBranchCountNoSThread::0","BlockedOnBranchCountNoSThread::1","BlockedOnBranchCountNoSThread::2","BlockedOnBranchCountNoSThread::total","FetchQueueEmpty::0","FetchQueueEmpty::1","FetchQueueEmpty::2","FetchQueueEmpty::total","DecodeWidthFull::0","DecodeWidthFull::1","DecodeWidthFull::2","DecodeWidthFull::total","FetchQueueTryingToDecode::0","FetchQueueTryingToDecode::1","FetchQueueTryingToDecode::2","FetchQueueTryingToDecode::total","FetchQueueSendingToDecode::0","FetchQueueSendingToDecode::1","FetchQueueSendingToDecode::2","FetchQueueSendingToDecode::total"]


    decode_stats = ["idleCycles","idleCyclesPerThread::0","idleCyclesPerThread::1","idleCyclesPerThread::2","idleCyclesPerThread::3","idleCyclesPerThread::total","blockedCycles","blockedCyclesPerThread::0","blockedCyclesPerThread::1","blockedCyclesPerThread::2","blockedCyclesPerThread::3","blockedCyclesPerThread::total","runCycles","unblockCycles","branchMispred","branchMispredSThread","branchMispredWThread","controlMispred","stalledS","stalledW","notStalled","blocking","blockingS","DecodeWidthUtilization::0","DecodeWidthUtilization::1","DecodeWidthUtilization::2","DecodeWidthUtilization::3","DecodeWidthUtilization::4","DecodeWidthUtilization::5","DecodeWidthUtilization::6","DecodeWidthUtilization::7","DecodeWidthUtilization::8","squashCycles","squashCyclesPerThread::0","squashCyclesPerThread::1","squashCyclesPerThread::2","squashCyclesPerThread::3"]

    rename_stats = ["squashCycles","squashCyclesSThread","squashCyclesWThread","idleCycles","idleCyclesPerThread::0","idleCyclesPerThread::1","idleCyclesPerThread::2","idleCyclesPerThread::3","idleCyclesPerThread::total","idleCyclesSThread","idleCyclesWThread","blockCycles","blockCyclesPerThread::0","blockCyclesPerThread::1","blockCyclesPerThread::2","blockCyclesPerThread::3","blockCyclesPerThread::total","blockCyclesSThread","blockCyclesWThread","serializeStallCycles","runCycles","runCyclesSThread","runCyclesWThread","ROBFullEvents","ROBFullEventsS","ROBFullEventsW","IQFullEvents","IQFullEventsS","IQFullEventsW","LQFullEvents","LQFullEventsS","LQFullEventsW","SQFullEvents","SQFullEventsS","SQFullEventW","fullRegistersEvents","fullRegistersEventsS","fullRegistersEventsW","renamedOperands","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","blockingIQFull","blockingIQFullS","blockingIQFullW","blockingROBFull","blockingROBFullS","blockingROBFullW","blockingBandwidthFull","blockingBandwidthFullS","blockingBandwidthFullW","blockingRegFull","blockingRegFullS","blockingRegFullW","skidInsts","iewStallS","iewStallW","NoROBFreeS","NoROBFreeW","NoIQFreeS","NoIQFreeW","NoLSQFreeS","NoLSQFreeW","NoRenameFreeS","NoRenameFreeW","SerializeROBFullS","SerializeROBFullW","renameDeactivate","BlockedBecauseOneThread","resumeSerializeS","resumeSerializeW","resumeUnblockingS","resumeUnblockingW","RunningS","RunningW","IdleS","IdleW","StartSquashS","StartSquashW","SquashingS","SquashingW","BlockedS","BlockedW","UnblockingS","UnblockingW","SerializeStallS","SerializeStallW"]

    iew_stats = ["idleCyclesDispatch","ExecuteInstsIdle","squashCycles","squashCyclesS","squashCyclesW","blockCycles","blockCyclesS","blockCyclesW","unblockCycles","dispatchedInsts","dispSquashedInsts","dispSquashedInstsS","dispSquashedInstsW","iqFullEvents","iqFullEventsS","iqFullEventsW","lsqFullEvents","lsqFullEventsS","lsqFullEventsW","memOrderViolationEvents","branchMispredicts","instsToCommit","wbRateTotal","wbRateSTotal","wbRateWTotal","stalledS","stalledW","notStalled","blockingS","blockingW","blockingIQFull","blockingIQFullS","blockingLSQFull","blockingLSQFullS","blockingBandwidthFull","blockingBandwidthFullS","NoIntructionsAvailable::0","NoIntructionsAvailable::1","NoIntructionsAvailable::2","NoIntructionsAvailable::3","NoIntructionsAvailable::4","NoIntructionsAvailable::5","instsToCommit::0","NoSInstFromRename","NoWInstFromRename"]

    commit_stats = ["commitSquashedInsts","branchMispredicts","branchMispredictsS","branchMispredictsW","branches::0","branches::1","IQTimeTotal","ROBTimeTotal","IEWTimeTotal","totalReadyTimeTotal","renameTimeTotal","decodeTimeTotal","fetchTimeTotal","AvgInstInROB::1"]

    icache_stats = ["demandHits::cpu_cluster.cpus.inst","overallMissRate::total","overallMissLatency::total","demandHits::total","overallHits::total","demandMisses::total","overallMisses::total","overallAvgMissLatency::total","blockedCycles::no_mshrs","blockedCycles::no_targets","avgBlocked::no_mshrs","avgBlocked::no_targets","replacements","overallAvgMshrMissLatency::total","tags.warmupTick","tags.avgRefs","tags.totalRefs","tags.avgOccs::total","demandMissLatency::total","demandAccesses::total","demandAvgMissLatency::total","demandMshrMissLatency::total","demandAvgMshrMissLatency::total","tags.avgOccs::total","ReadReq.mshrMissLatency::cpu_cluster.cpus.inst","ReadReq.avgMissLatency::cpu_cluster.cpus.inst","overallAvgMissLatency::cpu_cluster.cpus.inst","overallMshrMisses::total","overallAccesses::total","demandMissRate::total","blockedCauses::no_mshrs","blockedCauses::no_targets","writebacks::writebacks","demandMshrHits::total","overallAvgMshrMissLatency::total","ReadReq.hits::total","ReadReq.misses::total","ReadReq.missLatency::total","ReadReq.accesses::total","ReadReq.missRate::total","tags.tagsInUse","tags.tagAccesses"]

    dcache_stats = ["overallMissRate::total","overallMissLatency::total"]

    cpu_stage_specfic_stats = [["rename",rename_stats]]
    cpu_stage_specfic_stats = [["commit",commit_stats],["iew",iew_stats],["rename",rename_stats],["decode",decode_stats],["fetch",fetch_stats],["dcache_strong",icache_stats],["icache_strong",icache_stats],["l2",l2_stats]]

    #cpu_stage_specfic_stats = [["fetch",fetch_stats],["dcache_strong",icache_stats],["icache_strong",icache_stats]]

    lsq = ["LoadQueueUtilization","StoreQueueUtilization"]

    cpus = []

    stage_specfic_stats = []
    stage_specfic_stats = [["l2",icache_stats],["cpus.lsq0",lsq],["cpus.lsq1",lsq],["cpus.lsq2",lsq],["cpus.lsq3",lsq]]

    output_file = "stats.csv"
    directory = os.getcwd()

    cpulist = []

    # m5_ops_stats: variable to show if m5_ops dumping is enabled. If yes, then we average over dumps which are collected over the same piece of code
    # loop_type: "main_loop": Main calculation loop
    # loop_type: "tile_setup": Next tile setup

    loop_type = "main_loop"

    get_stats(parent_folder, files_check, stat_names, output_file, directory, stage_specfic_stats, cpu_stage_specfic_stats, top_line, cpulist, SimStats, m5_ops_stats, loop_type)

    # backup data

    # where stats are per CPU
    # cpu_stage_specfic_stats = [["fetch",fetch_stats],["icache_strong",icache_stats],["icache_weak",icache_stats]]

    # stage_specfic_stats = [["l2",l2_stats],["lsq0",lsq]]

    # cpu_stage_specfic_stats = []
    # stage_specfic_stats = []

    # cpulist = ["cpus0","cpus1","cpus2"]

    # lsq = ["loadToUse::mean"]

    # cpu_stage_specfic_stats = [["commit",commit_stats],["iew",iew_stats],["rename",rename_stats],["decode",decode_stats],["fetch",fetch_stats],["dcache_strong",dcache_stats],["icache_strong",icache_stats],["icache_weak",icache_stats],["l2",l2_stats]]

    # stat_names = ["numCycles","issueRate","numIssuedDist::mean","statFuBusyPerThreadCollective","statStalledOnControlInstructionPerThread","statNumIssueNotPossiblePerThread","fuBusyS","fuBusyW","totalIpc", "cycleCountS", "cycleCountW","statFuNoFree::IntAlu","statFuNoFree::IntMult","statFuNoFree::IntDiv","statFuNoFree::MemRead","statFuNoFree::MemWrite","numIssuedDist::0","numIssuedDist::1","numIssuedDist::2","numIssuedDist::3","numIssuedDist::4","numIssuedDist::5","numIssuedDist::6","numIssuedDist::7","numIssuedDist::8","numIssuedDist::9","numIssuedDist::10","numIssuedDist::11","numIssuedDist::12","ipc::0"]


    # fetch_stats = ["cacheStallCycles","icacheStallCycles","icacheStallCyclesSThread","icacheStallCyclesWThread","noActiveThreadStallCycles","blockedCycles","insts","instsSThread","instsWThread","branches","cycles","blockedCycles","noActiveThreadStallCycles","icacheWaitRetryStallCycles","cacheLines","icacheSquashes","tlbSquashes","idleRate","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","multipleRunning","nisnDist::0","nisnDist::1","nisnDist::2","nisnDist::3","nisnDist::4","nisnDist::5","nisnDist::6","nisnDist::7","nisnDist::8","nisnDist::9","nisnDist::10","nisnDist::11","nisnDist::12","nisnDist::mean","squashCycles"]


    # decode_stats = ["idleCycles","blockedCycles","runCycles","unblockCycles","branchMispred","branchMispredSThread","branchMispredWThread","controlMispred","stalledS","stalledW","notStalled","blocking","blockingS"]

    # rename_stats = ["squashCycles","squashCyclesSThread","squashCyclesWThread","idleCycles","idleCyclesSThread","idleCyclesWThread","blockCycles","blockCyclesSThread","blockCyclesWThread","serializeStallCycles","runCycles","runCyclesSThread","runCyclesWThread","ROBFullEvents","ROBFullEventsS","ROBFullEventsW","IQFullEvents","IQFullEventsS","IQFullEventsW","LQFullEvents","LQFullEventsS","LQFullEventsW","SQFullEvents","SQFullEventsS","SQFullEventW","fullRegistersEvents","fullRegistersEventsS","fullRegistersEventsW","renamedOperands","stalledS","stalledW","stalledSNotW","stalledSAndW","notStalled","blockingIQFull","blockingIQFullS","blockingIQFullW","blockingROBFull","blockingROBFullS","blockingROBFullW","blockingBandwidthFull","blockingBandwidthFullS","blockingBandwidthFullW","blockingRegFull","blockingRegFullS","blockingRegFullW","skidInsts","iewStallS","iewStallW","NoROBFreeS","NoROBFreeW","NoIQFreeS","NoIQFreeW","NoLSQFreeS","NoLSQFreeW","NoRenameFreeS","NoRenameFreeW","SerializeROBFullS","SerializeROBFullW","renameDeactivate","BlockedBecauseOneThread","resumeSerializeS","resumeSerializeW","resumeUnblockingS","resumeUnblockingW","RunningS","RunningW","IdleS","IdleW","StartSquashS","StartSquashW","SquashingS","SquashingW","BlockedS","BlockedW","UnblockingS","UnblockingW","SerializeStallS","SerializeStallW"]

    # iew_stats = ["idleCycles","idleCyclesS","idleCyclesW","squashCycles","squashCyclesS","squashCyclesW","blockCycles","blockCyclesS","blockCyclesW","unblockCycles","dispatchedInsts","dispSquashedInsts","dispSquashedInstsS","dispSquashedInstsW","iqFullEvents","iqFullEventsS","iqFullEventsW","lsqFullEvents","lsqFullEventsS","lsqFullEventsW","memOrderViolationEvents","branchMispredicts","instsToCommit","wbRateTotal","wbRateSTotal","wbRateWTotal","stalledS","stalledW","notStalled","blockingS","blockingW","blockingIQFull","blockingIQFullS","blockingLSQFull","blockingLSQFullS","blockingBandwidthFull","blockingBandwidthFullS"]

    #commit_stats = ["commitSquashedInsts","branchMispredicts","branchMispredictsS","branchMispredictsW","totalTransitTimeTotal","totalTransitTimeSTotal","totalTransitTimeWTotal","totalInstructionTimeTotal","totalInstructionTimeSTotal","totalInstructionTimeWTotal","IEWTimeTotal","IQTimeTotal","ROBTimeTotal","totalReadyTimeTotal","renameTimeTotal","decodeTimeTotal","fetchTimeTotal","numCommittedDist::mean","totalInstructionTimeS::0","totalInstructionTimeS::1","totalInstructionTimeS::2","totalInstructionTimeS::3","totalInstructionTimeS::12","totalInstructionTimeS::14","totalInstructionTimeS::15","totalInstructionTimeS::47","totalInstructionTimeS::48","totalInstructionTimeS::total","IQTime::0","IQTime::1","IQTime::2","IQTime::3","IQTime::12","IQTime::14","IQTime::15","IQTime::47","IQTime::48","IQTime::total","ROBTime::0","ROBTime::1","ROBTime::2","ROBTime::3","ROBTime::12","ROBTime::14","ROBTime::15","ROBTime::47","ROBTime::48","ROBTime::total","IEWTime::0","IEWTime::1","IEWTime::2","IEWTime::3","IEWTime::12","IEWTime::14","IEWTime::15","IEWTime::47","IEWTime::48","IEWTime::total","totalReadyTime::0","totalReadyTime::1","totalReadyTime::2","totalReadyTime::3","totalReadyTime::12","totalReadyTime::14","totalReadyTime::15","totalReadyTime::47","totalReadyTime::48","totalReadyTime::total","renameTime::0","renameTime::1","renameTime::2","renameTime::3","renameTime::12","renameTime::14","renameTime::15","renameTime::47","renameTime::48","renameTime::total","decodeTime::0","decodeTime::1","decodeTime::2","decodeTime::3","decodeTime::12","decodeTime::14","decodeTime::15","decodeTime::47","decodeTime::48","decodeTime::total","fetchTime::0","fetchTime::1","fetchTime::2","fetchTime::3","fetchTime::12","fetchTime::14","fetchTime::15","fetchTime::47","fetchTime::48","fetchTime::total"]

    # commit_stats = []

    # dcache_stats = ["overallMissRate::total","overallMissLatency::total"]

    # icache_stats = ["overallMissRate::total","overallMissLatency::total","demandHits::total","overallHits::total","demandMisses::total","overallMisses::total","overallAvgMissLatency::total","blockedCycles::no_mshrs","blockedCycles::no_targets","avgBlocked::no_mshrs","avgBlocked::no_targets","replacements","overallAvgMshrMissLatency::total","tags.warmupTick","tags.avgRefs","tags.totalRefs","tags.avgOccs::total","demandMissLatency::total","demandAccesses::total","demandAvgMissLatency::total","demandMshrMissLatency::total","demandAvgMshrMissLatency::total","tags.avgOccs::total","ReadReq.mshrMissLatency::cpu_cluster.cpus.inst","ReadReq.avgMissLatency::cpu_cluster.cpus.inst","overallAvgMissLatency::cpu_cluster.cpus.inst","overallMshrMisses::total"]

    # icache_stats = ["overallMissRate::total","overallMissLatency::total","demandMisses::total","overallMisses::total"]

    # l2_stats = ["overallMissRate::total","overallMissLatency::total","demandHits::total","demandMisses::total","demandMissLatency::total","demandAccesses::total","demandAvgMissLatency::total","demandAvgMissLatency::cpu_cluster.cpus.inst","demandAvgMissLatency::cpu_cluster.cpus.data","overallMissRate::cpu_cluster.cpus.inst","overallMissRate::cpu_cluster.cpus.data","demandAccesses::cpu_cluster.cpus.inst",".overallMshrMissLatency::cpu_cluster.cpus.inst","overallMshrMissLatency::cpu_cluster.cpus.inst"]

    # %% plot a graph to see how many instructions is being fetched/decoded/.. etc 

    # files = [
    #     "bc_1tid.txt",
    #     "bc_2tid.txt",
    #     "matmul_1tid.txt",
    #     "matmul_2tid.txt"
    # ]

    # # files = [
    # #    "test3_f1.txt",
    # # #    "test22_f1.txt"
    # # ]

    # for file in files:
    #     plot_per_cycle_status(file)

if __name__ == "__main__":
    main()











# %%
