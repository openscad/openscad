#!/usr/bin/env bash

set -euo pipefail

DARK=-dark
ICONDIR=icons/svg-default
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

for i in $("$OPENSCAD" -o - --export-format echo -Dlist_icons=true "$ICONSCAD" | sed -e 's/ECHO: icon = //;' | tr -d '"')
do
        SVG="$ICONDIR"/svg/"$i.svg"
        SVGDARK="$ICONDIR$DARK"/svg/"$i.svg"
	echo "Generating icon $SVG and $SVGDARK..."
	"$OPENSCAD" -o "$SVG" --render -Dselected_icon="\"$i\"" "$ICONSCAD" 2>/dev/null

	sed -i'' -e's/<svg width="[0-9]*mm" height="[0-9]*mm" viewBox="[^"]*"/<svg width="100mm" height="100mm" viewBox="-8 -108 116 116"/' "$SVG"
	sed -e's/stroke="black" fill="lightgray"/stroke="white" fill="white"/' "$SVG" > "$SVGDARK"
	sed -i'' -e's/fill="lightgray"/fill="black"/' "$SVG"
done

for s in "" "$DARK"
do
cat > icons-svg-default${s}.qrc <<EOF
<RCC>
  <qresource>
    <file>icons/svg-default${s}/index.theme</file>
$(ls "${ICONDIR}${s}"/svg/*.svg | sed -E 's,.*,    <file>&</file>,')
  </qresource>
</RCC>
EOF
done

echo "Done."
