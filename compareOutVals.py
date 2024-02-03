def extract_pc(line):
    """Extract the PC value from a given line."""
    pc_start = line.find('PC (') + 4
    pc_end = line.find('),', pc_start) + 1
    return line[pc_start:pc_end]


def extract_sn(line):
    """Extract the sn value from a given line."""
    sn_start = line.find('[sn:') + 4
    sn_end = line.find(']', sn_start)
    return int(line[sn_start:sn_end])

def parse_line_REGOUTVALS(line):
    """Parse the line to extract the PC value and the 'has data' value."""
    pc_start = line.find('PC (') + 4
    #pc_end = line.find('=>', pc_start)
    pc_end = line.find(') ', pc_start) + 1
    pc = line[pc_start:pc_end]

    data_start = line.find('has data ')
    if data_start != -1:
        data_start += 9  # Length of 'has data '
        data_end = line.find(':', data_start)  # Find end of the 'has data' value
        data = line[data_start:data_end].strip()
    else:
        data = '0'  # Default value if 'has data' is not found

    sn_start = line.find('[sn:') + 4
    sn_end = line.find(']', sn_start)
    sn = int(line[sn_start:sn_end])
    
    return pc, data, sn

# src input check
def compare_files_REGOUTVALS(file1, file2):
    """Compare 'has data' values for PCs between two files."""
    with open(file1, 'r') as f1, open(file2, 'r') as f2, open(file2, 'r') as f3, open(file1, 'r') as f4:
        file1_data = {}
        file2_data = {}
        squashed_inst2 = []
        squashed_inst1 = []
        pc_values_file1 = []
        pc_values_file2 = []
        sn_values_file1 = []
        sn_values_file2 = []

        pc_matching_file1 = []
        sn_matching_file1 = []
        pc_matching_file2 = []
        sn_matching_file2 = []

        for line in f4:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst1.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst1.append(sn)
            if("Squashing due to" in line):
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst1.append(sn)
            if("Branch at PC" in line): # dont compare branch inst PC
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst1.append(sn)
        for line in f1:
            if "REGOUTVALS for DEST" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                file1_data.setdefault(pc, []).append((data_values,sn))
                if(int(sn) == 24354):
                    print("OUTS LINE ",line," ",(sn == 24354))
            if("Retiring head instruction" in line):
                sn = extract_sn(line)
                if sn not in squashed_inst1:
                    pc = extract_pc(line)
                    pc_values_file1.append(pc)
                    sn_values_file1.append(sn)
                    pc_matching_file1.append(pc)
                    sn_matching_file1.append(sn)
        for line in f3:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
            if("Squashing due to" in line):
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
            if("Branch at PC" in line): # dont compare branch inst PC
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REGOUTVALS for DEST" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    file2_data.setdefault(pc, []).append((data_values,sn))
                if(int(sn) == 20515):
                    print("OUTS LINE ",line," ",(sn == 20515))
            if("Retiring head instruction" in line):
                sn = extract_sn(line)
                if sn not in squashed_inst2:
                    pc = extract_pc(line)
                    pc_values_file2.append(pc)
                    sn_values_file2.append(sn)
                    pc_matching_file2.append(pc)
                    sn_matching_file2.append(sn)
                    
    print("************")

    # Get the correct order of retirement 
    combined_lists = list(zip(
        pc_matching_file1,
        sn_matching_file1,
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        pc_matching_file1,
        sn_matching_file1,
    ) = zip(*sorted_combined_lists)

    combined_lists = list(zip(
        pc_matching_file2,
        sn_matching_file2
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        pc_matching_file2,
        sn_matching_file2
    ) = zip(*sorted_combined_lists)

    print("*********SN pairs to check")

    min_length = min(len(sn_matching_file1), len(sn_matching_file2))
    for i in range(min_length):
        if(pc_matching_file1[i] == pc_matching_file2[i]):
            print("good-pair file1 PC:",pc_matching_file1[i],"sn",sn_matching_file1[i],"file2 PC:",pc_matching_file2[i],"sn",sn_matching_file2[i])
        else:
            print("bad-pair file1 PC:",pc_matching_file1[i],"sn",sn_matching_file1[i],"file2 PC:",pc_matching_file2[i],"sn",sn_matching_file2[i])
        if (pc_matching_file1[i] not in pc_matching_file2):
            print("ERROR11! ",pc_matching_file1[i],"not in file2 sn",sn_matching_file1[i])
        elif (pc_matching_file2[i] not in pc_matching_file1):
            print("ERROR22! ",pc_matching_file2[i],"not in file1 sn",sn_matching_file2[i])

    Mismatch_pc_REGOUTVALS = []
    Notfound_pc_REGOUTVALS = []
    mismatchSN_file1_REGOUTVALS = []
    mismatchSN_file2_REGOUTVALS = []
    mismatchData_file1_REGOUTVALS = []
    mismatchData_file2_REGOUTVALS = []

    match_pc_REGOUTVALS = []
    matchSN_file1_REGOUTVALS = []
    matchSN_file2_REGOUTVALS = []
    matchData_file1_REGOUTVALS = []
    matchData_file2_REGOUTVALS = []

    # Compare the data values
    for pc in file1_data:
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    Mismatch_pc_REGOUTVALS.append(pc)
                    mismatchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    mismatchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    mismatchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    mismatchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
                else:
                    match_pc_REGOUTVALS.append(pc)
                    matchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    matchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    matchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    matchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
        else:
            Notfound_pc_REGOUTVALS.append(pc)


    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)

    print("*********REGOUTVALS DEST INCORRECT")
    for i in range(len(Mismatch_pc_REGOUTVALS)):
        print(f"Mismatch for PC {Mismatch_pc_REGOUTVALS[i]} at SN {mismatchSN_file1_REGOUTVALS[i]}: File1 has data {mismatchData_file1_REGOUTVALS[i]}, SN {mismatchSN_file2_REGOUTVALS[i]}: File2 has data {mismatchData_file2_REGOUTVALS[i]}")

    print("*********REGOUTVALS PC NOT FOUND")
    for i in range(len(Notfound_pc_REGOUTVALS)):
        print(f"PC {Notfound_pc_REGOUTVALS[i]} fount in file1 but not in file2")

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)


    print("*********REGOUTVALS DEST Match")
    for i in range(len(match_pc_REGOUTVALS)):
        print(f"Allmatch for PC {match_pc_REGOUTVALS[i]} at SN {matchSN_file1_REGOUTVALS[i]}: File1 has data {matchData_file1_REGOUTVALS[i]}, SN {matchSN_file2_REGOUTVALS[i]}: File2 has data {matchData_file2_REGOUTVALS[i]}")


    Mismatch_pc_file1_Retiring = []
    Mismatch_pc_file2_Retiring = []
    mismatchSN_file1_Retiring = []
    mismatchSN_file2_Retiring = []

    # Compare the commited instructions
    min_length = min(len(pc_values_file1), len(pc_values_file2))
    for i in range(min_length):
        if(pc_values_file1[i] != pc_values_file2[i]):
            Mismatch_pc_file1_Retiring.append(pc_values_file1[i])
            Mismatch_pc_file2_Retiring.append(pc_values_file2[i])
            mismatchSN_file1_Retiring.append(sn_values_file1[i])
            mismatchSN_file2_Retiring.append(sn_values_file2[i])

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        Mismatch_pc_file1_Retiring,
        Mismatch_pc_file2_Retiring,
        mismatchSN_file1_Retiring,
        mismatchSN_file2_Retiring
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[2])

    (
        Mismatch_pc_file1_Retiring,
        Mismatch_pc_file2_Retiring,
        mismatchSN_file1_Retiring,
        mismatchSN_file2_Retiring
    ) = zip(*sorted_combined_lists)

    print("*********REGOUTVALS PC mismatch")
    for i in range(len(Mismatch_pc_file1_Retiring)):
        print(f"Mismatch for file1 PC {Mismatch_pc_file1_Retiring[i]} at SN {mismatchSN_file1_Retiring[i]}: file2 PC {Mismatch_pc_file2_Retiring[i]} at SN {mismatchSN_file2_Retiring[i]}")


# dest output check
def compare_files_REG_ISSUE1_OUTVALS(file1, file2):
    """Compare 'has data' values for PCs between two files."""
    with open(file1, 'r') as f1, open(file2, 'r') as f2, open(file2, 'r') as f3, open(file1, 'r') as f4:
        file1_data = {}
        file2_data = {}
        squashed_inst2 = []
        squashed_inst1 = []

        for line in f4:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst1.append(sn)
        for line in f1:
            if "REG_ISSUE1_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst1:
                    file1_data.setdefault(pc, []).append((data_values,sn))
        for line in f3:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REG_ISSUE1_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    file2_data.setdefault(pc, []).append((data_values,sn))
                

    Mismatch_pc_REGOUTVALS = []
    Notfound_pc_REGOUTVALS = []
    mismatchSN_file1_REGOUTVALS = []
    mismatchSN_file2_REGOUTVALS = []
    mismatchData_file1_REGOUTVALS = []
    mismatchData_file2_REGOUTVALS = []

    match_pc_REGOUTVALS = []
    matchSN_file1_REGOUTVALS = []
    matchSN_file2_REGOUTVALS = []
    matchData_file1_REGOUTVALS = []
    matchData_file2_REGOUTVALS = []

    print("************")
    for pc in file1_data:
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    Mismatch_pc_REGOUTVALS.append(pc)
                    mismatchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    mismatchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    mismatchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    mismatchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
                else:
                    match_pc_REGOUTVALS.append(pc)
                    matchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    matchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    matchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    matchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
        else:
            #print(f"PC {pc} found in File1 but not in File2")
            Notfound_pc_REGOUTVALS.append(pc)

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)

    print("*********REG_ISSUE1_OUTVALS DEST INCORRECT")
    for i in range(len(Mismatch_pc_REGOUTVALS)):
        print(f"Mismatch for PC {Mismatch_pc_REGOUTVALS[i]} at SN {mismatchSN_file1_REGOUTVALS[i]}: File1 has data {mismatchData_file1_REGOUTVALS[i]}, SN {mismatchSN_file2_REGOUTVALS[i]}: File2 has data {mismatchData_file2_REGOUTVALS[i]}")

    print("*********REG_ISSUE1_OUTVALS PC NOT FOUND")
    for i in range(len(Notfound_pc_REGOUTVALS)):
        print(f"PC {Notfound_pc_REGOUTVALS[i]} fount in file1 but not in file2")

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)


    print("*********REG_ISSUE1_OUTVALS DEST Match")
    for i in range(len(match_pc_REGOUTVALS)):
        print(f"Allmatch for PC {match_pc_REGOUTVALS[i]} at SN {matchSN_file1_REGOUTVALS[i]}: File1 has data {matchData_file1_REGOUTVALS[i]}, SN {matchSN_file2_REGOUTVALS[i]}: File2 has data {matchData_file2_REGOUTVALS[i]}")

# dest output check
def compare_files_REG_ISSUE2_OUTVALS(file1, file2):
    """Compare 'has data' values for PCs between two files."""
    with open(file1, 'r') as f1, open(file2, 'r') as f2, open(file2, 'r') as f3, open(file1, 'r') as f4:
        file1_data = {}
        file2_data = {}
        squashed_inst2 = []
        squashed_inst1 = []

        for line in f4:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst1.append(sn)
        for line in f1:
            if "REG_ISSUE2_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst1:
                    file1_data.setdefault(pc, []).append((data_values,sn))
        for line in f3:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REG_ISSUE2_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    #print("LINE IN ",line)
                    file2_data.setdefault(pc, []).append((data_values,sn))

    Mismatch_pc_REGOUTVALS = []
    Notfound_pc_REGOUTVALS = []
    mismatchSN_file1_REGOUTVALS = []
    mismatchSN_file2_REGOUTVALS = []
    mismatchData_file1_REGOUTVALS = []
    mismatchData_file2_REGOUTVALS = []

    match_pc_REGOUTVALS = []
    matchSN_file1_REGOUTVALS = []
    matchSN_file2_REGOUTVALS = []
    matchData_file1_REGOUTVALS = []
    matchData_file2_REGOUTVALS = []


    print("************")
    for pc in file1_data:
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    Mismatch_pc_REGOUTVALS.append(pc)
                    mismatchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    mismatchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    mismatchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    mismatchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
                else:
                    match_pc_REGOUTVALS.append(pc)
                    matchSN_file1_REGOUTVALS.append(file1_data[pc][i][1])
                    matchSN_file2_REGOUTVALS.append(file2_data[pc][i][1])
                    matchData_file1_REGOUTVALS.append(file1_data[pc][i][0])
                    matchData_file2_REGOUTVALS.append(file2_data[pc][i][0])
        else:
            Notfound_pc_REGOUTVALS.append(pc)

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        Mismatch_pc_REGOUTVALS,
        mismatchSN_file1_REGOUTVALS,
        mismatchSN_file2_REGOUTVALS,
        mismatchData_file1_REGOUTVALS,
        mismatchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)


    print("*********REG_ISSUE2_OUTVALS DEST INCORRECT")
    for i in range(len(Mismatch_pc_REGOUTVALS)):
        print(f"Mismatch for PC {Mismatch_pc_REGOUTVALS[i]} at SN {mismatchSN_file1_REGOUTVALS[i]}: File1 has data {mismatchData_file1_REGOUTVALS[i]}, SN {mismatchSN_file2_REGOUTVALS[i]}: File2 has data {mismatchData_file2_REGOUTVALS[i]}")

    print("*********REG_ISSUE2_OUTVALS PC NOT FOUND")
    for i in range(len(Notfound_pc_REGOUTVALS)):
        print(f"PC {Notfound_pc_REGOUTVALS[i]} fount in file1 but not in file2")

    # Combine the lists into a list of tuples
    combined_lists = list(zip(
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ))

    sorted_combined_lists = sorted(combined_lists, key=lambda x: x[1])

    (
        match_pc_REGOUTVALS,
        matchSN_file1_REGOUTVALS,
        matchSN_file2_REGOUTVALS,
        matchData_file1_REGOUTVALS,
        matchData_file2_REGOUTVALS
    ) = zip(*sorted_combined_lists)


    print("*********REG_ISSUE1_OUTVALS DEST Match")
    for i in range(len(match_pc_REGOUTVALS)):
        print(f"Allmatch for PC {match_pc_REGOUTVALS[i]} at SN {matchSN_file1_REGOUTVALS[i]}: File1 has data {matchData_file1_REGOUTVALS[i]}, SN {matchSN_file2_REGOUTVALS[i]}: File2 has data {matchData_file2_REGOUTVALS[i]}")

def parse_pc_full_pc(line):
    """Parse the line to extract the full PC value."""
    pc_start = line.find('PC (') + 4
    pc_end = line.find(').', pc_start)
    return line[pc_start:pc_end]

def compare_files_full_PC(file1, file2):
    """Compare the order of PC values between two files."""
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        file1_pcs = [parse_pc_full_pc(line) for line in f1 if "Committing instruction with PC" in line]
        file2_pcs = [parse_pc_full_pc(line) for line in f2 if "Committing instruction with PC" in line]

    # Compare the PC sequences
    for i, (pc1, pc2) in enumerate(zip(file1_pcs, file2_pcs)):
        if pc1 != pc2:
            print(f"Mismatch found at line {i + 1}: File1 PC={pc1}, File2 PC={pc2} total length ",len(file1_pcs))
            break
    else:
        print("All matching PCs in sequence are the same in both files.")

def parse_pc_half_pc(line):
    """Parse the line to extract the initial part of the PC value."""
    pc_start = line.find('PC (') + 4
    pc_end = line.find('=>', pc_start)
    return line[pc_start:pc_end]

def compare_files_part_PC(file1, file2):
    """Compare the order of initial PC values between two files."""
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        file1_pcs = [parse_pc_half_pc(line) for line in f1 if "Committing instruction with PC" in line]
        file2_pcs = [parse_pc_half_pc(line) for line in f2 if "Committing instruction with PC" in line]

    # Compare the PC sequences
    for i, (pc1, pc2) in enumerate(zip(file1_pcs, file2_pcs)):
        if pc1 != pc2:
            print(f"Mismatch found at line {i + 1}: File1 PC={pc1}, File2 PC={pc2} total length ",len(file1_pcs))
            break
    else:
        if len(file1_pcs) != len(file2_pcs):
            print("Mismatch in the number of PCs.")
        else:
            print("All matching PCs in sequence are the same in both files.")


def parse_file_dest(filename):
    data_by_sn = {}
    with open(filename, 'r') as file:
        for line in file:
            if 'REGOUTVALS for DEST' in line and 'has data' in line:
                sn = line.split('[sn:')[1].split(']')[0]
                data = line.split('has data ')[1].strip()
                if sn not in data_by_sn:
                    data_by_sn[sn] = []
                data_by_sn[sn].append(data)
    return data_by_sn

def parse_file_src(filename):
    data_by_sn = {}
    with open(filename, 'r') as file:
        for line in file:
            if 'REGVALS for SRC' in line and 'has data' in line:
                sn = line.split('[sn:')[1].split(']')[0]
                data = line.split('has data ')[1].strip()
                if sn not in data_by_sn:
                    data_by_sn[sn] = []
                data_by_sn[sn].append(data)
    return data_by_sn

def compare_files_dest(file1, file2):
    data1 = parse_file_dest(file1)
    data2 = parse_file_dest(file2)

    for sn in data1:
        if sn in data2:
            if set(data1[sn]) != set(data2[sn]):
                print(f"Mismatch for sn:{sn}. File1 data: {data1[sn]}, File2 data: {data2[sn]}")

def compare_files_src(file1, file2):
    data1 = parse_file_src(file1)
    data2 = parse_file_src(file2)

    for sn in data1:
        if sn in data2:
            if set(data1[sn]) != set(data2[sn]):
                print(f"Mismatch for sn:{sn}. File1 data: {data1[sn]}, File2 data: {data2[sn]}")

def compareCreatedAndDestroyedInst(file1):

    destroyed_sn_list = []
    created_sn_list = []

    with open(file1, 'r') as f1:
        for line in f1:
            parts = line.split()
            if "Instruction destroyed" in line:
                # Extract the serial number from the line and add it to the destroyed list
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                destroyed_sn_list.append(sn)
            elif "Instruction created" in line:
                # Extract the serial number from the line and add it to the created list
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = int(line[sn_start:sn_end])
                created_sn_list.append(sn)

    not_destroyed_sn_list = []

    print("Created SNs not destroyed:")
    for sn in created_sn_list:
        if sn not in destroyed_sn_list:
           print(sn)

# Replace 'file1.txt' and 'file2.txt' with your actual file names
# print("compare dest")
# compare_files_dest('out1', 'outS1')
# print("compare src")
# compare_files_src('out2', 'outS2')

# print("full pc")
# compare_files_full_PC('out', 'outS')
# print("partial pc")
# compare_files_part_PC('out', 'outS')

print("REGOUTVALS DEST")
compare_files_REGOUTVALS('out', 'outS')
#print("REG_ISSUE1_OUTVALS SRC")
#compare_files_REG_ISSUE1_OUTVALS('out', 'outS')
# print("REG_ISSUE2_OUTVALS SRC")
# compare_files_REG_ISSUE2_OUTVALS('out', 'outS')

#compareCreatedAndDestroyedInst('out')