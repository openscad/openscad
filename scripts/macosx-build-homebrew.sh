#!/usr/bin/env bash
#
# This script builds library dependencies of OpenSCAD for Mac OS X using Homebrew.
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

log()
{
  echo "$(date):" "$@"
}

if [ ! -f $OPENSCADDIR/openscad.appdata.xml.in ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

log "Updating homebrew"
time brew update

log "Listing homebrew configuration"
time brew config

# Install special packages not yet in upstream homebrew repo.
# Check if there's already an active openscad tap and skip
# tap/untap in that case.
TAP=:
if ! brew tap | grep ^openscad/ >/dev/null 2>/dev/null
then
	log "Tapping openscad homebrew repo"
	TAP=brew
fi
$TAP tap openscad/homebrew-tap
$TAP trust openscad/homebrew-tap

for formula in pkg-config boost eigen cgal glew glib opencsg freetype libzip libxml2 fontconfig harfbuzz lib3mf osmesa double-conversion imagemagick ccache ghostscript tbb catch2 qt qscintilla2; do
  log "Installing formula $formula"
  brew ls --versions $formula
  time brew install $formula
done
