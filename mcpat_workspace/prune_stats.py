import argparse

def extract_middle_section(input_file, output_file):
    # Open the input file for reading
    with open(input_file, 'r') as infile:
        lines = infile.readlines()

    # Initialize variables to keep track of the markers
    begin_count = 0
    middle_section_started = False
    middle_section = []

    # Iterate through the lines to find the sections
    for line in lines:
        if "---------- Begin Simulation Statistics ----------" in line:
            begin_count += 1

            # When the second "Begin" marker is found, start collecting data
            if begin_count == 2:
                middle_section_started = True

            # When the third "Begin" marker is found, stop collecting data
            elif begin_count == 3:
                break

        # If we're in the middle section, start adding the lines to the list
        if middle_section_started:
            middle_section.append(line)

    # Write the middle section to the output file
    if middle_section:
        with open(output_file, 'w') as outfile:
            outfile.writelines(middle_section)
        print(f"Middle section has been saved to {output_file}")
    else:
        print("Could not find the correct begin and end markers in the file.")

def main():
    # Set up argument parsing
    parser = argparse.ArgumentParser(description="Extract the middle section of stats.txt")
    parser.add_argument("input_file", help="Path to the input stats.txt file")
    parser.add_argument("output_file", help="Path to save the pruned stats file")

    # Parse arguments
    args = parser.parse_args()

    # Call the function to extract the middle section
    extract_middle_section(args.input_file, args.output_file)

if __name__ == "__main__":
    main()
