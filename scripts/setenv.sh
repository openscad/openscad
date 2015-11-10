#!/bin/sh -e
#
# set environment variables for various builds
#
# Usage:
#
#  source ./scripts/setenv.sh               # standard linux/bsd/msys2 build
#  source ./scripts/setenv.sh mxe           # mxe cross-build for Win, 32 bit
#  source ./scripts/setenv.sh mxe shared    # mxe with shared libraries (DLLs)
#  source ./scripts/setenv.sh mxe 64        # mxe 64 bit static link
#  source ./scripts/setenv.sh mxe 64 shared # mxe 64 bit shared libraries (DLLs)
#  source ./scripts/setenv.sh clang         # build using clang compiler
#  source ./scripts/setenv.sh clean         # Clean up exported variables
#
# Notes:
#
# Linux/BSD:
#
# Please see 'scripts/uni-build-dependencies.sh'
#
# MXE:
#
# Please see http://mxe.cc/#requirements
#
# Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Msys2:
#
# 32 or 64 bit is selected by starting the appropriate "MINGW64" or
# "MINGW32" shell on the system and runnning these commands from within it.
#
# Please download and install msys2 from http://msys2.github.io
#
# General:
#
# This script uses a lot of global variables, take care if editing

setup_target()
{
  ARCH=`uname -a`
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_TRIPLE=$ARCH-$OSTYPE
}

setup_target_mxe()
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
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_ABI=$ABI
  OPENSCAD_BUILD_TARGET_TRIPLE=$MXE_TARGET
  OPENSCAD_LIBRARIES=$MXE_TARGET_DIR
}

setup_target_msys()
{
  ARCH=i686
  SUB=w64
  SYS=windows
  ABI=gnu
  if [ "`uname -a | grep -i x86_64`" ]; then
    ARCH=x86_64
  fi

  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_TRIPLE=$ARCH-$SUB-$SYS.$ABI
}

setup_base()
{
  if [ ! $BASEDIR ]; then
    if [ -f openscad.pro ]; then
      # if in main openscad dir, put under $HOME
      BASEDIR=$HOME/openscad_deps
    else
      # otherwise, assume its being run 'out of tree'. treat it somewhat like
      # "configure" or "cmake", so you can build dependencies where u wish.
      echo "Warning. Using current directory as base of dependency build"
      BASEDIR=$PWD/openscad_deps
    fi
  fi
}

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

save_path()
{
  if [ ! $SETENV_SAVED_ORIGINAL_PATH ]; then
    echo "current PATH saved in SETENV_SAVED_ORIGINAL_PATH"
    SETENV_SAVED_ORIGINAL_PATH=$PATH
  fi
}

setup_path()
{
  run save_path
  PATH=$BASEDIR/bin:$PATH
}

setup_path_mxe()
{
  run save_path
  PATH=$MXEDIR/usr/bin:$PATH
  if [ "`echo $* | grep qt4 `" ]; then
    PATH=$MXE_TARGET_DIR/qt/bin:$PATH
  else
    PATH=$MXE_TARGET_DIR/qt5/bin:$PATH
  fi
}

setup_path_msys()
{
  run save_path
  MWBITS=32
  if [ $ARCH = x86_64 ]; then MWBITS=64; fi
  PATH=/mingw$MWBITS/bin:$PATH
}

setup_variables_common_unx()
{
  LD_LIBRARY_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
  LD_RUN_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
  OPENSCAD_LIBRARIES=$DEPLOYDIR
  GLEWDIR=$DEPLOYDIR
  if [ -e $DEPLOYDIR/include/Qt ]; then
    echo "Qt found under $DEPLOYDIR ... "
    QTDIR=$DEPLOYDIR
  fi
}

setup_variables_mxe()
{
  OPENSCADDIR=$PWD
  DEPLOYDIR=$OPENSCADDIR/$MXE_TARGET
  run setup_base
  run setup_path
}

setup_variables_freebsd()
{
  #QMAKESPEC=freebsd-clang
  #QTDIR=/usr/local/share/qt5
  QMAKESPEC=freebsd-g++
  QTDIR=/usr/local/share/qt4
  DEPLOYDIR=$BASEDIR
  setup_variables_common_unx
}

setup_variables_netbsd()
{
  run save_path
  QMAKESPEC=netbsd-g++
  QTDIR=/usr/pkg/qt4
  PATH=/usr/pkg/qt4/bin:$PATH
  LD_LIBRARY_PATH=/usr/pkg/qt4/lib:$LD_LIBRARY_PATH
  LD_LIBRARY_PATH=/usr/X11R7/lib:$LD_LIBRARY_PATH
  LD_LIBRARY_PATH=/usr/pkg/lib:$LD_LIBRARY_PATH
  DEPLOYDIR=$BASEDIR
  setup_variables_common_unx
}

setup_variables_msys()
{
  if [ "`echo $* | grep clang`" ]; then
    run setup_clang
  fi
  OPENSCADDIR=$PWD
  DEPLOYDIR=$OPENSCADDIR/$OPENSCAD_BUILD_TARGET_TRIPLE
  run setup_path
}

setup_clang_msys()
{
  CC=clang
  CXX=clang++
  echo if you have not already installed clang try this:
  echo   pacman -Sy mingw-w64-x86_64-clang or
  echo   pacman -Sy mingw-w64-i686-clang
}

setup_clang_linux()
{
  CC=clang
  CXX=clang++
  # for qt4 this would be unsupported/linux-clang
  QMAKESPEC=linux-clang
}

setup_varlist()
{
  vl=
  vl="$vl OPENSCAD_BUILD_TARGET_TRIPLE"
  vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
  vl="$vl OPENSCAD_BUILD_TARGET_ABI"
  vl="$vl DEPLOYDIR BASEDIR LD_LIBRARY_PATH LD_RUN_PATH OPENSCAD_LIBRARIES"
  vl="$vl GLEWDIR QMAKESPEC QTDIR CC CXX"
}

setup_varlist_mxe()
{
  vl=
  vl="$vl OPENSCAD_BUILD_TARGET_TRIPLE"
  vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
  vl="$vl OPENSCAD_BUILD_TARGET_ABI OPENSCAD_LIBRARIES BASEDIR MXEDIR"
  vl="$vl MXE_TARGET MXE_TARGET_DIR MXE_TARGET_DIR_SHARED MXE_TARGET_DIR_STATIC"
  vl="$vl DEPLOYDIR SETENV_SAVED_ORIGINAL_PATH"
}

setup_varlist_msys()
{
  vl=
  vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
  vl="$vl OPENSCAD_BUILD_TARGET_ABI OPENSCAD_BUILD_TARGET_TRIPLE DEPLOYDIR"
  vl="$vl CC CXX"
}

clean_variables()
{
  if [ $SETENV_SAVED_ORIGINAL_PATH ]; then
    PATH=$SETENV_SAVED_ORIGINAL_PATH
    echo "PATH restored from SETENV_SAVED_ORIGINAL_PATH"
  fi
  if [ "`echo $vl`" ]; then
    for varname in $vl; do
      eval $varname"="
    done
  fi
  echo "SETENV cross build environment variables cleared"
}

export_and_print_vars()
{
  if [ "`echo $vl`" ]; then
    for varname in $vl; do
      export $varname
      echo "$varname: "`eval echo "$"$varname`
    done
  fi
  echo "PATH: $PATH"
}


run()
{
  # run() calls function $1, possibly a specialized version for our target $2
  # stackoverflow.com/questions/85880/determine-if-a-function-exists-in-bash
  runfunc1=`echo $1"_"$OPENSCAD_BUILD_TARGET_OSTYPE`
  runfunc2=`echo $1`
  if [ "`type -t $runfunc1 | grep function`" ]; then
    echo "calling $runfunc1"
    eval $runfunc1
  elif [ "`type -t $runfunc2 | grep function`" ]; then
    echo "calling $runfunc2"
    eval $runfunc2
  else
    echo "neither $runfunc2 nor $runfunc1 were defined, skipping."
  fi
}

detect_target_ostype()
{
  if [ "`echo $1 | grep mxe`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=mxe
  elif [ "`uname | grep -i linux`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=linux
  elif [ "`uname | grep -i debian`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=linux
  elif [ "`uname | grep -i freebsd`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=freebsd
  elif [ "`uname | grep -i netbsd`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=netbsd
  elif [ "`uname | grep -i msys`" ]; then
    OPENSCAD_BUILD_TARGET_OSTYPE=msys
  fi
}


if [ "`echo $* | grep clean`" ]; then
  run clean_variables
  run export_and_print_vars
else
  if [ $OPENSCAD_BUILD_TARGET_OSTYPE ]; then
    echo "OPENSCAD_BUILD_TARGET_OSTYPE environment was previously setup"
    echo "Please run this script with 'clean' before use, or logout/login"
  else
    detect_target_ostype $*
    run setup_target
    run setup_variables
    run export_and_print_vars
  fi
fi
