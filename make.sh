#!/bin/sh

set -e

make -C mkmi static

cd module
directories=$(find . -type d)
cd ..

for dir in $directories; do
	if [ -f "module/$dir/Makefile" ]; then
		echo $dir
		make -C "module/$dir" module
	fi
done

make -C microk-kernel kernel

make buildimg

set +e
