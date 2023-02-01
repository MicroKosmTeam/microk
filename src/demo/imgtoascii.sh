#!/bin/sh
echo Converting image...
convert -resize 640x480 image.jpg image2.jpg
convert image2.jpg image.ppm
rm image2.jpg
convert image.ppm -compress none image2.ppm
rm image.ppm
