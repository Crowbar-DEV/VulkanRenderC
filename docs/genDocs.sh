#!/bin/bash

mapfile -t arrStructs < neededApiStructs.txt
mapfile -t arrFns < neededApiCalls.txt

for i in "${arrStructs[@]}"; do
    if [[ -e "structs/$i.txt" ]]; then
        rm "structs/$i.txt"
        awk "/$i -/,/See Also/" vulkanRef.txt > structs/$i.txt
    else
        awk "/$i -/,/See Also/" vulkanRef.txt > structs/$i.txt
    fi
done

for i in "${arrFns[@]}"; do
    if [[ -e "functions/$i.txt" ]]; then
        rm "functions/$i.txt"
        awk "/$i -/,/See Also/" vulkanRef.txt > functions/$i.txt
    else
        awk "/$i -/,/See Also/" vulkanRef.txt > functions/$i.txt
    fi
done