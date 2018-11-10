#!/bin/bash
#
# Generate list of files for translation. The output is saved to po/POTFILES
# which is needed for the xgettext call.

for ui in objects/ui_*.h
do
        for d in mingw64 mingw32 .
        do
            if [ -f "$d/$ui" ]
            then
                echo "$d/$ui"
            fi
        done
done

for src in src/*.h src/*.cc src/*.cpp src/*.mm
do
	echo $src
done

for src in src/parameter/*.h src/parameter/*.cc src/parameter/*.cpp
do
	echo $src
done

for src in src/input/*.h src/input/*.cc
do
	echo $src
done
