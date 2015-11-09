#!/bin/sh -e
#
# set environment variables for mingw/mxe cross-build
#
# Usage:
#
#  source ./scripts/setenv-mingw-xbuild.sh           # 32 bit build, static libs
#  source ./scripts/setenv-mingw-xbuild.sh 32 shared # 32 bit build, shared libs
#  source ./scripts/setenv-mingw-xbuild.sh 64        # 64 bit build, static libs
#  source ./scripts/setenv-mingw-xbuild.sh 64 shared # 64 bit build, shared libs
#  source ./scripts/setenv-mingw-xbuild.sh clean     # Clean up exported variables
#
# Prerequisites:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Note - this uses a lot of unpleasant global variables, take care if editing

setup_base_mxe()
{
  if [ ! $BASEDIR ]; then
    BASEDIR=$HOME/openscad_deps
  fi

  if [ ! $MXEDIR ]; then
    if [ -e /opt/mxe ]; then
      MXEDIR=/opt/mxe
    fi
    if [ -e $BASEDIR/mxe ]; then
      MXEDIR=$BASEDIR/mxe
    fi
    if [ ! -e $MXEDIR ]; then
      echo mxe not found in $MXEDIR.
      echo please set MXEDIR to base of mxe install
      exit
    fi
  fi
}

setup_target()
{
  ARCH=i686
  SUB=w64
  SYS=mingw32
  ABI=static
  if [ "`echo $* | grep 64 `" ]; then ARCH=x86_64 ; fi
  if [ "`echo $* | grep shared `" ]; then ABI=shared ; fi

  MXE_TARGET=$ARCH-$SUB-$SYS.$ABI
  MXE_TARGET_DIR=$MXEDIR/usr/$MXE_TARGET
  MXE_TARGET_DIR_STATIC=$MXEDIR/usr/$ARCH-$SUB-$SYS.static
  MXE_TARGET_DIR_SHARED=$MXEDIR/usr/$ARCH-$SUB-$SYS.shared
  OPENSCAD_BUILD_TARGET_OSTYPE=mxe
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_ABI=$ABI
  OPENSCAD_BUILD_TARGET_TRIPLE=$MXE_TARGET
  OPENSCAD_LIBRARIES=$MXE_TARGET_DIR
}

setup_path()
{
  if [ ! $MINGWX_SAVED_ORIGINAL_PATH ]; then
    echo "current PATH saved in MINGWX_SAVED_ORIGINAL_PATH"
    MINGWX_SAVED_ORIGINAL_PATH=$PATH
  fi

  PATH=$MXEDIR/usr/bin:$PATH

  if [ "`echo $* | grep qt4 `" ]; then
    PATH=$MXE_TARGET_DIR/qt/bin:$PATH
  else
    PATH=$MXE_TARGET_DIR/qt5/bin:$PATH
  fi
}

setup_deploydir()
{
  DEPLOYDIR=$OPENSCADDIR/$MXE_TARGET
  if [ ! -e $DEPLOYDIR ]; then
    mkdir -p $DEPLOYDIR
  fi
}


clean_variables()
{
  if [ $MINGWX_SAVED_ORIGINAL_PATH ]; then
    PATH=$MINGWX_SAVED_ORIGINAL_PATH
    echo "PATH restored from MINGWX_SAVED_ORIGINAL_PATH"
  fi
  for varname in $vl; do
    eval $varname"="
  done
  echo "MXE cross build environment variables cleared"
}


export_and_print_vars()
{
  for varname in $vl; do
    export $varname
    echo "$varname: `eval echo "$"$varname`"
  done
  echo PATH: $PATH
}

setup_variables()
{
  OPENSCADDIR=$PWD
  setup_base_mxe $*
  setup_target $*
  setup_path $*
  setup_deploydir $*
  echo "MXE cross build environment variables setup.."
}

vl=
vl="$vl OPENSCAD_BUILD_TARGET_TRIPLE"
vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
vl="$vl OPENSCAD_BUILD_TARGET_ABI OPENSCAD_LIBRARIES BASEDIR MXEDIR"
vl="$vl MXE_TARGET MXE_TARGET_DIR MXE_TARGET_DIR_SHARED MXE_TARGET_DIR_STATIC"
vl="$vl DEPLOYDIR MINGWX_SAVED_ORIGINAL_PATH"

if [ "`echo $* | grep clean`" ]; then
  clean_variables
  export_and_print_vars
else
  if [ $OPENSCAD_BUILD_TARGET_OSTYPE ]; then
    echo "OPENSCAD_BUILD_TARGET_OSTYPE environment was previously setup"
    echo "Please run this script with 'clean' before use, or logout/login"
  else
    setup_variables $*
    export_and_print_vars
  fi
fi
