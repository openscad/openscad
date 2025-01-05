#!/usr/bin/env bash

set -euo pipefail

DARK=-dark
NAME=chokusen
ICONDIR=icons/$NAME
ICONSCAD=icons/icons-svg-default.scad

if [ ! -d resources/"$ICONDIR" -o ! -f resources/"$ICONSCAD" ]; then
	echo "$0: script should be run from the source code top level folder"
	exit 1
fi

cd resources

if [[ "$OSTYPE" =~ "darwin" ]]; then
  OPENSCAD=/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD
else
  OPENSCAD=openscad
fi

set -e

generate_icons() {
	rm -f "$ICONDIR"/svg/*.svg
	rm -f "$ICONDIR$DARK"/svg/*.svg
	for i in $("$OPENSCAD" -o - --export-format echo -Dlist_icons=true "$ICONSCAD" | sed -e 's/ECHO: icon = //;' | tr -d '"')
	do
		SVG="$ICONDIR"/svg/"$NAME-$i.svg"
		SVGDARK="$ICONDIR$DARK"/svg/"$NAME-$i.svg"
		echo "Generating icon $SVG and $SVGDARK..."
		"$OPENSCAD" -o "$SVG" --render -Dselected_icon="\"$i\"" "$ICONSCAD" 2>/dev/null

		sed -i'' -e's/<svg width="[0-9]*mm" height="[0-9]*mm" viewBox="[^"]*"/<svg width="100mm" height="100mm" viewBox="-8 -108 116 116"/' "$SVG"
		sed -e's/stroke="black" fill="lightgray"/stroke="white" fill="white"/' "$SVG" > "$SVGDARK"
		sed -i'' -e's/fill="lightgray"/fill="black"/' "$SVG"
	done
}

write_qrc() {
	FILE="$1"
	PREFIX="$2"
	SUBDIR="$3"
	INDEX="$4"

	if [ -n "$INDEX" ]; then
		I="    <file>${PREFIX}${INDEX}</file>
"
	else
		I=""
	fi
	cat > "$FILE" <<EOF
<RCC>
  <qresource>
${I}$(cd "${ICONDIR}"/svg && ls *.svg | sed -E "s,.*,    <file>${PREFIX}${SUBDIR}&</file>,")
  </qresource>
</RCC>
EOF
}

generate_icons
write_qrc icons-$NAME.qrc		icons/$NAME/		svg/	index.theme
write_qrc icons-$NAME${DARK}.qrc	icons/$NAME${DARK}/	svg/	index.theme

echo "Done."
