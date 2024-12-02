#!/bin/bash

value=1


# 7회 반복 실행
for (( i=1; i<=5; i++ )); do
    echo "Running run.sh with argument: $((value * 4))"
    ./mem-cpy -d cdma -s 4194304 
    # ./mem-cpy -d cdma -s 4194304 -b -c $((value-1))
    
    # value를 2배로 증가시킴
    (( value *= 4 ))
done