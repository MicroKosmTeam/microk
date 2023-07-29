#!/bin/sh

make -C mkmi static

for i in module/*
do
	make -C $i module
done

make -C microk-kernel kernel

make buildimg

make run-x64-efi

