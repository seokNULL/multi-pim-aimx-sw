#!/bin/bash

count=0
max_runs=1000

while [ $count -lt $max_runs ]; do
    # 실행 명령어
    output=$(./elewise 2 2048 1)

    # 실행 결과 검사
    if echo "$output" | grep -q "success"; then
        echo "Run $((count+1)): Success"
        count=$((count+1))
    elif echo "$output" | grep -q "error"; then
        # count=$((count+1))
        echo "Run $((count+1)): Fail"
        echo "Execution log:"
        echo "$output"
        break
    else
        echo "Run $((count+1)): Unexpected output"
        break
    fi
done

echo "Program finished after $count runs."

