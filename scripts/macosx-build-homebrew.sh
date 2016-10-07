#!/bin/bash
#
# This script builds  library dependencies of OpenSCAD for Mac OS X using Homebrew.
# 
# This script must be run from the OpenSCAD source root directory
#
# Prerequisites:
# - Homebrew (http://brew.sh)
#

set -x

OPENSCADDIR=$PWD

printUsage()
{
  echo "Usage: $0"
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

brew -v ls

brew -v config

brew -v update
# FIXME: We used to require unlinking boost, but doing so also causes us to lose boost.
# Disabling until we can figure out why we unlinked in the first place
# brew unlink boost
for formula in eigen boost cgal glew glib opencsg freetype libxml2 fontconfig harfbuzz qt5 qscintilla2 imagemagick; do
  brew -v install $formula
  brew -v outdated $formula || brew -v upgrade $formula
done
brew -v link --force gettext
brew -v link --force qt5
brew -v link --force qscintilla2

