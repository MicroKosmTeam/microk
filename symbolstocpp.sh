#!/bin/bash

SYMBOLDIR="microk-kernel/src"

#Extract the symbol table information
nm microk.elf -n > symbols.txt
symbol_count=$(wc -l symbols.txt | awk '{print $1}')

#Create a new file to store the C array
echo "#include <sys/symbol.hpp>" > ${SYMBOLDIR}/sys/symbolMap.cpp
echo "const uint64_t symbolCount = $symbol_count;" >> ${SYMBOLDIR}/sys/symbolMap.cpp
echo "const Symbol symbols[] = {" >> ${SYMBOLDIR}/sys/symbolMap.cpp

#Parse the symbols.txt file and generate the C array
while read -r line; do
    symbol=($line)
    echo "   {{0x${symbol[0]}}, {\"${symbol[2]}\"}}, " >> ${SYMBOLDIR}/sys/symbolMap.cpp
done < symbols.txt

#Close the C array
echo "};" >> ${SYMBOLDIR}/sys/symbolMap.cpp
