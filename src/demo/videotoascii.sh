#!/bin/sh
echo Converting frames...
for i in video/*.bmp
do
        echo Converting image $i...
        convert -resize 640x480 $i ${i%.bmp}.ppm
        convert ${i%.bmp}.ppm -compress none ${i%.bmp}.ppm
done
