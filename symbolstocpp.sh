#!/bin/bash

#Extract the symbol table information
nm microk.elf -n > symbols.txt
symbol_count=$(wc -l symbols.txt | awk '{print $1}')

#Create a new file to store the C array
echo "#include <sys/symbol.hpp>" > kernel/sys/symbolMap.cpp
echo "const uint64_t symbolCount = $symbol_count;" >> kernel/sys/symbolMap.cpp
echo "const Symbol symbols[] = {" >> kernel/sys/symbolMap.cpp

#Parse the symbols.txt file and generate the C array
while read -r line; do
    symbol=($line)
    echo "   {{0x${symbol[0]}}, {\"${symbol[2]}\"}}, " >> kernel/sys/symbolMap.cpp
done < symbols.txt

#Close the C array
echo "};" >> kernel/sys/symbolMap.cpp
