#!/usr/bin/env bash
#
# Generate list of files for translation. The output is saved to po/POTFILES
# which is needed for the xgettext call.
# Call from project root, with build directory as first parameter.

for ui in {$1,ming64,mingw32}/OpenSCAD_autogen/include/ui_*.h
do
    if [ -f "$ui" ]
    then
        echo "$ui"
    fi
done

for src in src/{,gui/,gui/input/,gui/parameter/}*.{h,cc,cpp,mm}
do
    if [ -f "$src" ]
    then
	    echo $src
    fi
done
