#!/bin/bash
#
# Generate list of files for translation. The output is saved to po/POTFILES
# which is needed for the xgettext call.

for ui in src/*.ui
do
        UI="${ui#src/}"
        UI="${UI%.ui}"
	echo "objects/ui_$UI.h"
done

for src in src/*.h src/*.cc
do
	echo $src
done
