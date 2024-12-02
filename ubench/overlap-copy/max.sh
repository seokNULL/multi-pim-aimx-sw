#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <count> <size> <cl_size>"
    exit 1
fi

# Assign arguments to variables
count="$1"
size="$2"
cl_size="$3"

# Initialize variable to store maximum time
max_time=
max_cmd=

# Function to extract CDMA Memcpy time from output
extract_memcpy_time() {
    logfile="$1"
    time=$(awk '/CDMA Memcpy time:/ {print $4}' "$logfile")
    echo "$time" | grep -o '[0-9]\+'
}

# Execute the command and capture maximum time
for (( i=1; i<=$count; i++ )); do
    logfile="log_$i.txt"
    cmd="./mem-cpy -d qdma -s $size -c $cl_size"
    $cmd > "$logfile"
    memcpy_time=$(extract_memcpy_time "$logfile")
    
    # Check if memcpy_time is a valid integer
    if [[ "$memcpy_time" =~ ^[0-9]+$ ]]; then
        # If max_time is not set or current memcpy_time is larger, update max_time and max_cmd
        if [ -z "$max_time" ] || [ "$memcpy_time" -gt "$max_time" ]; then
            max_time="$memcpy_time"
            max_cmd="$cmd"
        fi
    else
        echo "Error: Unable to extract valid CDMA Memcpy time from log file $logfile"
        exit 1
    fi

    # Clean up the log file
    rm "$logfile"
done

# Output the maximum CDMA Memcpy time found and the corresponding command
echo "Maximum CDMA Memcpy time: $max_time nanoseconds"
echo "Command: $max_cmd"
