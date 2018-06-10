#!/bin/sh

# Script for use from qmake to generate the translation
# related files.
#

SCRIPTDIR="`dirname \"$0\"`"
TOPDIR="`dirname \"$SCRIPTDIR\"`"

SCRIPT=./scripts/translation-update.sh

if [ ! -f "$SCRIPT" ]
then
	cd "$TOPDIR" || exit 1
fi

echo "Compiling language files (CWD = `pwd`)..."
"$SCRIPT" updatemo
