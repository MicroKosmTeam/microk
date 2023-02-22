#!/bin/bash

#Extract the symbol table information
nm bin/kernel.elf -n > symbols.txt
symbol_count=$(wc -l symbols.txt | awk '{print $1}')

#Create a new file to store the C array
echo "#include <sys/symbols.h>" > src/kernel/sys/symbols_map.cpp
echo "const uint64_t symbolCount = $symbol_count;" >> src/kernel/sys/symbols_map.cpp
echo "const symbol_t symbols[] = {" >> src/kernel/sys/symbols_map.cpp

#Parse the symbols.txt file and generate the C array
while read -r line; do
    symbol=($line)
    echo "   {{0x${symbol[0]}}, {\"${symbol[2]}\"}}, " >> src/kernel/sys/symbols_map.cpp
done < symbols.txt

#Close the C array
echo "};" >> src/kernel/sys/symbols_map.cpp
