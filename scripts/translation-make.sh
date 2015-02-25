#!/bin/sh

# Script for use from qmake to generate the translation
# related files.
#

SCRIPTDIR="`dirname \"$0\"`"
TOPDIR="`dirname \"$SCRIPTDIR\"`"

cd "$TOPDIR" || exit 1

echo "Compiling language files..."
./scripts/translation-update.sh updatemo
