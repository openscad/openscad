#!/usr/local/bin/bash -e

echo "Tested on FreeBSD 9. Please see README.md for info on older systems."

if [ "`pkg_info | grep -i cgal | echo hi`" ]; then
	echo Stopping. Please remove any CGAL packages you have installed and restart
	exit
fi

if [ "`pkg_info | grep -i opencsg`" ]; then
	echo Stopping. Please remove any OpenCSG packages you have installed and restart
	exit
fi

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

. ./scripts/setenv-freebsdbuild.sh

pkg_add -r bison boost-libs cmake git bash eigen2 flex gmake gmp mpfr 
pkg_add -r xorg libGLU libXmu libXi xorg-vfbserver glew
pkg_add -r qt4-corelib qt4-gui qt4-moc qt4-opengl qt4-qmake qt4-rcc qt4-uic

BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh cgal-use-sys-libs
BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg
