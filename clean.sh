#!/bin/sh

make -C mkmi clean 

cd module
directories=$(find . -type d)
cd ..

for dir in $directories; do
	if [ -f "module/$dir/Makefile" ]; then
		echo $dir
		make -C "module/$dir" clean
	fi
done

make -C microk-kernel clean
