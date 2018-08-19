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

log()
{
  echo "$(date):" "$@"
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

log "Listing homebrew configuration"
time brew config

log "Updating homebrew"
time brew update
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

# FIXME: We used to require unlinking boost, but doing so also causes us to lose boost.
# Disabling until we can figure out why we unlinked in the first place
# brew unlink boost
for formula in eigen boost cgal glew glib opencsg freetype libzip libxml2 fontconfig harfbuzz qt5 qscintilla2 imagemagick ccache; do
  log "Installing formula $formula"
  brew ls --versions $formula
  time brew install $formula
done

for formula in gettext qt5 qscintilla2; do
  log "Linking formula $formula"
  time brew link --force $formula
done

$TAP untap openscad/homebrew-tap
