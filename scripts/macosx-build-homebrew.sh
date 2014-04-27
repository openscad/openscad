#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be built in 64-bit mode and backwards compatible with 10.7 "Lion".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-d]
#  -d   Build for deployment (if not specified, e.g. Sparkle won't be built)
#
# Prerequisites:
# - Xcode
#
# FIXME:
# o Verbose option
# o Force rebuild vs. only rebuild changes
#

OPENSCADDIR=$PWD
BASEDIR=$OPENSCADDIR/../libraries
DEPLOYDIR=$BASEDIR/homebrew
MAC_OSX_VERSION_MIN=10.7

OPTION_DEPLOY=false

printUsage()
{
  echo "Usage: $0 [-d]"
  echo
  echo "  -d   Build for deployment"
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

while getopts 'd' c
do
  case $c in
    d) OPTION_DEPLOY=true;;
  esac
done

OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_VERSION >= 9 )); then
  echo "Detected Mavericks (10.9) or later"
elif (( $OSX_VERSION >= 8 )); then
  echo "Detected Mountain Lion (10.8)"
elif (( $OSX_VERSION >= 7 )); then
  echo "Detected Lion (10.7)"
else
  echo "Detected Snow Leopard (10.6) or earlier"
fi

echo "Building for $MAC_OSX_VERSION_MIN or later"

echo "Using basedir:" $BASEDIR

# Homebrew doesn't support building for other OS X versions than the current,
# but we can do this with environment variables
export MACOSX_DEPLOYMENT_TARGET=$MAC_OSX_VERSION_MIN

# Don't use bottles, as they might be built with the wrong deployment target
export HOMEBREW_BUILD_FROM_SOURCE=1

for formula in qt5 eigen boost cgal glew glib opencsg freetype libxml2 fontconfig harfbuzz; do
  brew install openscad/tap/$formula
done
if $OPTION_DEPLOY; then
  brew install --HEAD openscad/tap/sparkle
fi
