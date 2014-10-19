#!/bin/sh

# Script for use from qmake to generate the translation
# related files.
#
# Step 1) call generate-potfiles.sh to collect input files
# Step 2) call translation-update.sh to generate *.mo files
#

SCRIPTDIR="`dirname \"$0\"`"
TOPDIR="`dirname \"$SCRIPTDIR\"`"

cd "$TOPDIR"

echo "Generating POTFILES..."
./scripts/generate-potfiles.sh > po/POTFILES

echo "Updating translation files..."
./scripts/translation-update.sh

echo "Done."