#!/bin/sh -e
#
# Build openscad without root access
#
#   . /path/to/openscad/scripts/setenv-unibuild.sh
#   /path/to/openscad/scripts/uni-build-dependencies.sh
#
# Prerequisites:
#
# - see http://linuxbrew.sh/
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

if [ ! "`uname -m`|grep x86_64" ]; then
  echo "requires x86_64 bit cpu sorry, please see linuxbrew.sh"
fi

. ./scripts/setenv-unibuild.sh

cd $HOME

brewurl=https://raw.githubusercontent.com/Linuxbrew/install/master/install
if [ ! -e ~/.linuxbrew ]; then
  ruby -e "$(curl -fsSL "$brewurl")"
fi

brew update
pkgs='eigen boost cgal glew glib opencsg freetype libxml2 fontconfig'
pkgs=$pkgs' harfbuzz qt5 qscintilla2 imagemagick'
for formula in $pkgs; do
  brew install $formula
  brew outdated $formula || brew upgrade $formula
done
brew link --force gettext
brew link --force qt5
brew link --force qscintilla2


