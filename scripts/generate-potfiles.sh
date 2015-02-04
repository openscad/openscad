#!/bin/bash
#
# Generate list of files for translation. The output is saved to po/POTFILES
# which is needed for the xgettext call.

for ui in src/*.ui
do
        UI="${ui#src/}"
        UI="${UI%.ui}"
        for d in mingw64 mingw32 .
        do
            if [ -f "$d/objects/ui_$UI.h" ]
            then
                echo "$d/objects/ui_$UI.h"
            fi
        done
done

for src in src/*.h src/*.cc
do
	echo $src
done
