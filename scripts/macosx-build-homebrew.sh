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
  echo "Usage: $0 [qt6]"
}

log()
{
  echo "$(date):" "$@"
}

# Qt5 is default
if [ "`echo $* | grep qt6`" ]; then
  USE_QT6=1
fi

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

# FIXME: We used to require unlinking boost, but doing so also causes us to lose boost.
# Disabling until we can figure out why we unlinked in the first place
# brew unlink boost

# Python 2 conflicts with Python 3 links
brew unlink python@2

for formula in boost; do
  log "Installing or updating formula $formula"
  if brew ls --versions $formula; then
    time brew upgrade $formula
  else
    time brew install $formula
  fi
done

for formula in pkg-config eigen cgal glew glib opencsg freetype libzip libxml2 fontconfig harfbuzz lib3mf double-conversion imagemagick ccache ghostscript tbb; do
  log "Installing formula $formula"
  brew ls --versions $formula
  time brew install $formula
done

if [ $USE_QT6 ]; then 
  for formula in qt qscintilla2; do
    log "Installing formula $formula"
    brew ls --versions $formula
    time brew install $formula
  done
else
  for formula in qt5; do
    log "Installing formula $formula"
    brew ls --versions $formula
    time brew install $formula
  done
  # FIXME: Workaround for https://github.com/openscad/openscad/issues/5058
  curl -o qscintilla2.rb https://raw.githubusercontent.com/Homebrew/homebrew-core/da59bcdf7f1dadf70e30240394ddc0bd6014affe/Formula/q/qscintilla2.rb
  brew unlink qscintilla2
  brew install qscintilla2.rb
fi

$TAP untap openscad/homebrew-tap || true
