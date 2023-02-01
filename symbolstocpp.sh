#!/bin/bash

#Extract the symbol table information
objdump -t bin/kernel-tmp.elf | awk '{if ($1 !~ /SYMBOL/) print}' |grep -v "^bin/kernel-tmp.elf:     file format elf64-x86-64"  | sed 's/^[ \t]*//' > symbols_unordered.txt
sort -k1,1n symbols_unordered.txt > symbols.txt
#grep "^[0-9a-f]" | sort -k1,1n > symbols.txt
symbol_count=$(wc -l symbols.txt | awk '{print $1}')

#Create a new file to store the C array
echo "#include <sys/symbols.h>" > src/kernel/sys/symbols_map.cpp
echo "const uint64_t symbolCount = $symbol_count;" >> src/kernel/sys/symbols_map.cpp
echo "const symbol_t symbols[] = {" >> src/kernel/sys/symbols_map.cpp

#Parse the symbols.txt file and generate the C array
while read -r line; do
    symbol=($line)
    if [ ${#symbol[@]} -eq 6 ]; then
        printf "    {\"%s\", \"%s\"},\n" ${symbol[0]} ${symbol[5]} >> src/kernel/sys/symbols_map.cpp
    fi
done < symbols.txt

#Close the C array
echo "};" >> src/kernel/sys/symbols_map.cpp
