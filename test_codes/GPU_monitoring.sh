#!/bin/bash

# Define the log file name
LOG_FILE="gpu_utilization_log.csv"

# Write headers to the log file
rocm-smi --showuse --csv | head -n 1 > "$LOG_FILE"

echo "Starting GPU monitoring..."

# Run the monitoring loop
while true; do
    # Append GPU usage data to the log file
    rocm-smi --showuse --csv | sed '1d;/^$/d' >> "$LOG_FILE"
    sleep 5  # Adjust the frequency of logging as needed
done