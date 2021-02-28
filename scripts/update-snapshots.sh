#!/bin/bash -e

NAME=( "OpenSCAD-*.AppImage" "OpenSCAD-*.dmg"     "OpenSCAD-*-x86-32.zip" "OpenSCAD-*-x86-32-Installer.exe" "OpenSCAD-*-x86-64.zip" "OpenSCAD-*-x86-64-Installer.exe" )
KEY=(  "LIN64_SNAPSHOT"      "MAC_SNAPSHOT"       "WIN32_SNAPSHOT_ZIP"    "WIN32_SNAPSHOT_INSTALLER"        "WIN64_SNAPSHOT_ZIP"    "WIN64_SNAPSHOT_INSTALLER"        )
OUT=(  ".snapshot_linux.js"  ".snapshot_macos.js" ".snapshot_win32.js"    ".snapshot_win32.js"              ".snapshot_win64.js"    ".snapshot_win64.js"              )

for o in ${OUT[*]}
do
	rm -f "$o".tmp
	echo '/**/' > "$o".tmp
done

for n in $(seq 0 $((${#NAME[@]} - 1)))
do
	FILE="$(ls -t ${NAME[$n]} | head -n 1)"
        DATE="$(echo "$FILE" | cut -b 1-19)"
	SIZE="$((($(stat --format=%s "$FILE") / 1024 + 512) / 1024)) MB"
	echo "setSnapshotFileInfo('${KEY[$n]}', '$DATE', '$SIZE', 'https://openscad.thisistheremix.dev/snapshots/$FILE');" >> "${OUT[$n]}".tmp
done

for o in ${OUT[*]}
do
	if [ -f "$o".tmp ]
	then
		mv -vfb "$o".tmp "$o"
		chmod 444 "$o"
	fi
done
