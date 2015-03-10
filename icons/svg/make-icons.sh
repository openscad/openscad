#!/bin/bash

set -x 

if [[ "$OSTYPE" =~ "darwin" ]]; then
  EXE=OpenSCAD.app/Contents/MacOS/OpenSCAD
else
  EXE=openscad
fi
OPENSCAD=../../$EXE
T=`mktemp icons-XXXX`
rm $T
ECHO=$T.echo

"$OPENSCAD" -o "$ECHO" icons.scad

for i in `cat "$ECHO" | sed -e 's/ECHO: icon = //;' | tr -d '"'`
do
	echo "Generating $i icon..."
        SVG="$i.svg"
	"$OPENSCAD" -o "$SVG" -Dicon="\"$i\"" icons.scad 2>/dev/null

	sed -e 's/stroke="black" fill="lightgray"/stroke="white" fill="white"/' "$SVG" > "$i-white.svg"
	sed -i '' -e 's/fill="lightgray"/fill="black"/' "$SVG"
done
rm "$ECHO"
echo "Done."
