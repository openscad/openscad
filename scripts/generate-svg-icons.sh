#!/usr/bin/env bash

ICONDIR=resources/icons/svg-default
ICONSCAD="$ICONDIR"/icons.scad

if [ ! -d "$ICONDIR" ]; then
	echo "$0: script should be run from the source code top level folder"
	exit 1
fi

if [[ "$OSTYPE" =~ "darwin" ]]; then
  OPENSCAD=/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD
else
  OPENSCAD=openscad
fi

set -e

for i in `"$OPENSCAD" -o - --export-format echo -Dlist_icons=true "$ICONSCAD" | sed -e 's/ECHO: icon = //;' | tr -d '"'`
do
	echo "Generating $i icon..."
        SVG="$ICONDIR"/"$i.svg"
	"$OPENSCAD" -o "$SVG" --render -Dselected_icon="\"$i\"" "$ICONSCAD" 2>/dev/null

	sed -i'' -e's/<svg width="[0-9]*mm" height="[0-9]*mm" viewBox="[^"]*"/<svg width="100mm" height="100mm" viewBox="-8 -108 116 116"/' "$SVG"
	sed -e's/stroke="black" fill="lightgray"/stroke="white" fill="white"/' "$SVG" > "$ICONDIR"/"$i-white.svg"
	sed -i'' -e's/fill="lightgray"/fill="black"/' "$SVG"
done

echo "Done."
