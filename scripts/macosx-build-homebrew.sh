#!/bin/bash
#
# This script builds  library dependencies of OpenSCAD for Mac OS X using Homebrew.
# 
# This script must be run from the OpenSCAD source root directory
#
# Prerequisites:
# - Homebrew (http://brew.sh)
#

OPENSCADDIR=$PWD

printUsage()
{
  echo "Usage: $0"
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

brew update
# FIXME: We used to require unlinking boost, but doing so also causes us to lose boost.
# Disabling until we can figure out why we unlinked in the first place
# brew unlink boost
for formula in eigen boost cgal glew glib opencsg freetype libxml2 fontconfig harfbuzz qt5 qscintilla2 imagemagick; do
  brew ls --versions $formula && brew install $formula
  brew outdated $formula || brew upgrade $formula
done
brew link --force gettext
brew link --force qt5
brew link --force qscintilla2

