def extract_pc(line):
    """Extract the PC value from a given line."""
    pc_start = line.find('PC (') + 4
    pc_end = line.find('),', pc_start) + 1
    return line[pc_start:pc_end]


def extract_sn(line):
    """Extract the sn value from a given line."""
    sn_start = line.find('[sn:') + 4
    sn_end = line.find(']', sn_start)
    return line[sn_start:sn_end]

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
    sn = line[sn_start:sn_end]
    
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

        for line in f4:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
                squashed_inst1.append(sn)
        for line in f1:
            if "REGOUTVALS for DEST" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                file1_data.setdefault(pc, []).append((data_values,sn))
            if("Retiring head instruction" in line):
                sn = extract_sn(line)
                if sn not in squashed_inst1:
                    pc = extract_pc(line)
                    pc_values_file1.append(pc)
                    sn_values_file1.append(sn)
        for line in f3:
            if "instruction_squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REGOUTVALS for DEST" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    file2_data.setdefault(pc, []).append((data_values,sn))
            if("Retiring head instruction" in line):
                sn = extract_sn(line)
                if sn not in squashed_inst2:
                    pc = extract_pc(line)
                    pc_values_file2.append(pc)
                    sn_values_file2.append(sn)
    print("************")

    # Compare the data values
    for pc in file1_data:
        #print("LOOKING AT PC ",pc,end=" ")
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                #print("length",min_length,"idx",i,"Considering SN",file1_data[pc][i][1],": File1 has data",file1_data[pc][i][0],"SN",file2_data[pc][i][1],": File2 has data",file2_data[pc][i][0])
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    print(f"Mismatch for PC {pc} at SN {file1_data[pc][i][1]}: File1 has data {file1_data[pc][i][0]}, SN {file2_data[pc][i][1]}: File2 has data {file2_data[pc][i][0]}")
                    #break
        else:
            print(f"PC {pc} found in File1 but not in File2")

    # Compare the commited instructions
    min_length = min(len(pc_values_file1), len(pc_values_file2))
    print("************Looking at len ",min_length)
    for i in range(min_length):
        print(f"idx {i} Looking at file1 PC {pc_values_file1[i]} at SN {sn_values_file1[i]}: file2 PC {pc_values_file2[i]} at SN {sn_values_file2[i]}")
        if(pc_values_file1[i] != pc_values_file2[i]):
            print(f"idx {i} Mismatch for file1 PC {pc_values_file1[i]} at SN {sn_values_file1[i]}: file2 PC {pc_values_file2[i]} at SN {sn_values_file2[i]}")

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
                sn = line[sn_start:sn_end]
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
                sn = line[sn_start:sn_end]
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REG_ISSUE1_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    #print("LINE IN ",line)
                    file2_data.setdefault(pc, []).append((data_values,sn))

    print("************")
    # Compare the data values
    for pc in file1_data:
        #print("LOOKING AT PC ",pc,end=" ")
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                #print("length",min_length," idx ",i,"Considering SN ",file1_data[pc][i][1],": File1 has data ",file1_data[pc][i][0]," SN ",file2_data[pc][i][1],": File2 has data ",file2_data[pc][i][0])
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    print(f"Mismatch for PC {pc} at SN {file1_data[pc][i][1]}: File1 has data {file1_data[pc][i][0]}, SN {file2_data[pc][i][1]}: File2 has data {file2_data[pc][i][0]}")
                    #break
        else:
            print(f"PC {pc} found in File1 but not in File2")

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
                sn = line[sn_start:sn_end]
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
                sn = line[sn_start:sn_end]
                squashed_inst2.append(sn)
            if "Instruction was squashed" in line:
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
                if sn not in squashed_inst2:
                    squashed_inst2.append(sn)
        for line in f2:
            if "REG_ISSUE2_OUTVALS for SRC" in line:
                pc, data_values, sn = parse_line_REGOUTVALS(line)
                if sn not in squashed_inst2:
                    #print("LINE IN ",line)
                    file2_data.setdefault(pc, []).append((data_values,sn))

    print("************")
    # Compare the data values
    for pc in file1_data:
        #print("LOOKING AT PC ",pc,end=" ")
        if pc in file2_data:
            min_length = min(len(file1_data[pc]), len(file2_data[pc]))
            for i in range(min_length):
                #print("length",min_length," idx ",i,"Considering SN ",file1_data[pc][i][1],": File1 has data ",file1_data[pc][i][0]," SN ",file2_data[pc][i][1],": File2 has data ",file2_data[pc][i][0])
                if file1_data[pc][i][0] != file2_data[pc][i][0]:  # Compare data values
                    print(f"Mismatch for PC {pc} at SN {file1_data[pc][i][1]}: File1 has data {file1_data[pc][i][0]}, SN {file2_data[pc][i][1]}: File2 has data {file2_data[pc][i][0]}")
                    #break
        else:
            print(f"PC {pc} found in File1 but not in File2")

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
                sn = line[sn_start:sn_end]
                destroyed_sn_list.append(sn)
            elif "Instruction created" in line:
                # Extract the serial number from the line and add it to the created list
                sn_start = line.find('[sn:') + 4
                sn_end = line.find(']', sn_start)
                sn = line[sn_start:sn_end]
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
compare_files_REGOUTVALS('out_err', 'outS')
print("REG_ISSUE1_OUTVALS SRC")
compare_files_REG_ISSUE1_OUTVALS('out_err', 'outS')
print("REG_ISSUE2_OUTVALS SRC")
compare_files_REG_ISSUE2_OUTVALS('out_err', 'outS')

#compareCreatedAndDestroyedInst('out')