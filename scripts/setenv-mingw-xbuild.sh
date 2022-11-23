#!/bin/sh -e
#
# set environment variables for mingw/mxe cross-build
#
# Usage:
#
#  source ./scripts/setenv-mingw-xbuild.sh           # 32 bit build
#  source ./scripts/setenv-mingw-xbuild.sh shared    # 32 bit build, shared libs
#  source ./scripts/setenv-mingw-xbuild.sh 64        # 64 bit build
#  source ./scripts/setenv-mingw-xbuild.sh 64 shared # 64 bit build, shared libs
#  source ./scripts/setenv-mingw-xbuild.sh clean     # Clean up exported variables
#  source ./scripts/setenv-mingw-xbuild.sh qt5       # use qt5 (experimental)
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#

OPENSCADDIR=$PWD

if [ ! $BASEDIR ]; then
	BASEDIR=$HOME/openscad_deps
fi

MXELIBTYPE=static.posix
if [ "`echo $* | grep shared `" ]; then
	MXELIBTYPE=shared.posix
fi

if [ ! $DEPLOYDIR ]; then
	if [ "`echo $* | grep 64 `" ]; then
		DEPLOYDIR=$OPENSCADDIR/mingw64.$MXELIBTYPE
	else
		DEPLOYDIR=$OPENSCADDIR/mingw32.$MXELIBTYPE
	fi
fi

if [ ! $MXEDIR ]; then
	if [ "`echo $* | grep 64 `" ]; then
		MXEDIR=$BASEDIR/mxe
	else
		MXEDIR=$BASEDIR/mxe
	fi
	if [ ! -e $MXEDIR ]; then
		if [ -e /opt/mxe ]; then
			MXEDIR=/opt/mxe
		fi
	fi
fi

if [ ! $MXEQTSUBDIR ]; then
	# default is qt5 see issue #252
	MXEQTSUBDIR=qt5
fi

if [ ! -e $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi

if [ "`echo $* | grep 64 `" ]; then
  MXE_TARGETS=x86_64-w64-mingw32.$MXELIBTYPE
else
  MXE_TARGETS=i686-w64-mingw32.$MXELIBTYPE
fi
MXETARGETDIR=$MXEDIR/usr/$MXE_TARGETS

if [ ! $MINGWX_SAVED_ORIGINAL_PATH ]; then
  MINGWX_SAVED_ORIGINAL_PATH=$PATH
  echo current path saved
fi

PATH=$MXEDIR/usr/bin:$PATH
PATH=$MXETARGETDIR/$MXEQTSUBDIR/bin:$PATH

if [ "`echo $* | grep clean`" ]; then
  BASEDIR=
  MXEDIR=
  MXE_TARGETS=
  MXETARGETDIR=
  MXELIBTYPE=
  DEPLOYDIR=
  PATH=$MINGWX_SAVED_ORIGINAL_PATH
  MINGWX_SAVED_ORIGINAL_PATH=
  MXEQTSUBDIR=
else
  echo 'linking' $MXETARGETDIR
  echo '     to' $DEPLOYDIR/mingw-cross-env
  rm -f $DEPLOYDIR/mingw-cross-env
  ln -s $MXETARGETDIR $DEPLOYDIR/mingw-cross-env
fi

export OPENSCAD_LIBRARIES
export BASEDIR
export MXEDIR
export MXE_TARGETS
export MXETARGETDIR
export MXELIBTYPE
export DEPLOYDIR
export PATH
export MINGWX_SAVED_ORIGINAL_PATH
export MXEQTSUBDIR

echo OPENSCAD_LIBRARIES: $OPENSCAD_LIBRARIES
echo BASEDIR: $BASEDIR
echo MXEDIR: $MXEDIR
echo MXE_TARGETS: $MXE_TARGETS
echo MXETARGETDIR: $MXETARGETDIR
echo MXELIBTYPE: $MXELIBTYPE
echo DEPLOYDIR: $DEPLOYDIR
echo MXEQTSUBDIR: $MXEQTSUBDIR
if [ "`echo $* | grep clean`" ]; then
  echo PATH restored to pre-setenv-mingw-x state
else
  echo PATH modified: $MXEDIR/usr/bin
  echo PATH modified: $MXETARGETDIR/$MXEQTSUBDIR/bin
fi

if [ "`echo $PATH | grep anaconda.*bin`" ]; then
  echo please remove python anaconda/bin from your PATH, exit, and rerun this
fi

