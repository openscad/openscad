#!/usr/bin/env -S bash -e

NAME=( "OpenSCAD-*-x86_64.AppImage" "OpenSCAD-*-aarch64.AppImage" "OpenSCAD-*-WebAssembly.zip" "OpenSCAD-*.dmg"     "OpenSCAD-*-x86-32.zip" "OpenSCAD-*-x86-32-Installer.exe" "OpenSCAD-*-x86-64.zip" "OpenSCAD-*-x86-64-Installer.exe" )
KEY=(  "LIN64_SNAPSHOT"             "LIN_AARCH64_SNAPSHOT"        "WASM_SNAPSHOT"              "MAC_SNAPSHOT"       "WIN32_SNAPSHOT_ZIP"    "WIN32_SNAPSHOT_INSTALLER"        "WIN64_SNAPSHOT_ZIP"    "WIN64_SNAPSHOT_INSTALLER"        )
OUT=(  ".snapshot_linux_x86_64.js"  ".snapshot_linux_aarch64.js"  ".snapshot_wasm.js"          ".snapshot_macos.js" ".snapshot_win32.js"    ".snapshot_win32.js"              ".snapshot_win64.js"    ".snapshot_win64.js"              )

#NAME=( "OpenSCAD-*-x86_64.AppImage" "OpenSCAD-*-aarch64.AppImage" "OpenSCAD-*.dmg"     )
#KEY=(  "LIN64_SNAPSHOT"             "LIN_AARCH64_SNAPSHOT"        "MAC_SNAPSHOT"       )
#OUT=(  ".snapshot_linux_x86_64.js"  ".snapshot_linux_aarch64.js"  ".snapshot_macos.js" )

for o in ${OUT[*]}
do
	rm -f "$o".tmp
	echo '/**/' > "$o".tmp
done

for n in $(seq 0 $((${#NAME[@]} - 1)))
do
	FILE="$(ls -t ${NAME[$n]} 2>/dev/null | head -n 1)"
	if [ -f "$FILE" ]; then
        	DATE="$(echo "$FILE" | cut -b 1-19)"
		SIZE="$((($(stat --format=%s "$FILE") / 1024 + 512) / 1024)) MB"
		echo "setSnapshotFileInfo('${KEY[$n]}', '$DATE', '$SIZE', 'https://files.openscad.org/snapshots/$FILE');" >> "${OUT[$n]}".tmp
	else
		echo "File not found ${NAME[$n]}!"
	fi
done

for o in ${OUT[*]}
do
	if [ -f "$o".tmp ]
	then
		mv -vfb "$o".tmp "$o"
		chmod 444 "$o"
	fi
done
