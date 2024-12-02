#!/bin/bash

value=1024


# 7회 반복 실행
for (( i=1; i<=7; i++ )); do
    echo "Running run.sh with argument: $((value * 4))"
    ./mem-cpy -d cdma -s $value -b -c $1
    
    # value를 2배로 증가시킴
    (( value *= 4 ))
done