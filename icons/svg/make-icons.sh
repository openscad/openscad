#!/bin/bash

O=../../openscad
T=`mktemp`.echo

"$O" -o "$T" icons.scad

for i in `cat "$T" | sed -e 's/ECHO: icon = //;' | tr -d '"'`
do
	SVG=`mktemp`.svg
	echo "Generating $i icon..."
	"$O" -o "$SVG" -Dicon="\"$i\"" icons.scad 2>/dev/null

	sed -e 's/fill="lightgray"/fill="black"/' "$SVG" > "$i.svg"
	sed -e 's/stroke="black" fill="lightgray"/stroke="white" fill="white"/' "$SVG" > "$i-white.svg"
	rm -f "$SVG"
done
echo "Done."
