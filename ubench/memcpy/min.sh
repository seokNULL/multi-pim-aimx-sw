#!/bin/bash

# Check if the count and size are provided
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <count> <size>"
    exit 1
fi

count="$1"
size="$2"

# Initialize variable to store minimum time
min_time=
min_cmd=

# Function to extract QDMA Memcpy time from output
extract_memcpy_time() {
    logfile="$1"
    time=$(awk '/CDMA Memcpy time:/ {print $4}' "$logfile")
    echo "$time" | grep -o '[0-9]\+'
}

# Execute the command and capture minimum time
for (( i=1; i<=$count; i++ )); do
    logfile="log_$i.txt"
    cmd="./mem-cpy -d cdma -s $size"
    # cmd="./mem-cpy -d qdma -b -c 15 -s $size"
    $cmd > "$logfile"
    memcpy_time=$(extract_memcpy_time "$logfile")
    
    # Check if memcpy_time is a valid integer
    if [[ "$memcpy_time" =~ ^[0-9]+$ ]]; then
        # If min_time is not set or current memcpy_time is smaller, update min_time and min_cmd
        if [ -z "$min_time" ] || [ "$memcpy_time" -lt "$min_time" ]; then
            min_time="$memcpy_time"
            min_cmd="$cmd"
        fi
    else
        echo "Error: Unable to extract valid QDMA Memcpy time from log file $logfile"
        exit 1
    fi

    # Clean up the log file
    rm "$logfile"
done

# Output the minimum QDMA Memcpy time found and the corresponding command
echo "Minimum CDMA Memcpy time: $min_time nanoseconds"
echo "Command: $min_cmd"
