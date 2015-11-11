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
#  source ./scripts/setenv.sh clean         # unset all exported variables
#
# Notes:
#
# Linux/BSD:
#
#  Please see 'scripts/uni-build-dependencies.sh'
#
# MXE (Cross-build Linux->Windows):
#
#  Please see http://mxe.cc/#requirements
#
#  Also see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Cross-compiling_for_Windows_on_Linux_or_Mac_OS_X
#
# Msys2 (WindowsTM):
#
#  32 or 64 bit is selected by starting the appropriate "MINGW64" or
#  "MINGW32" shell on the system and runnning these commands from within it.
#
#  Please download and install msys2 from http://msys2.github.io
#
# General:
#
#  This script works by using function naming and run() for portability.
#  "_generic" is a generic linux/bsd build. _msys / _mxe etc are specialized.
#  Only variables in the 'varexportlist' are exported.
#  This script uses a lot of global variables, take care if editing.

setup_target_generic()
{
  ARCH=`uname -m`
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_TRIPLE=$ARCH-$OSTYPE
  if [ "`echo $ARGS | grep qt4`" ]; then
    OPENSCAD_USEQT4=1
  fi
}

setup_target_mxe()
{
  ARCH=i686
  SUB=w64
  SYS=mingw32
  ABI=static
  if [ "`echo $ARGS | grep 64 `" ]; then ARCH=x86_64 ; fi
  if [ "`echo $ARGS | grep shared `" ]; then ABI=shared ; fi
  MXE_TARGET=$ARCH-$SUB-$SYS.$ABI
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_ABI=$ABI
  OPENSCAD_BUILD_TARGET_TRIPLE=$MXE_TARGET
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

setup_dirs_generic()
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
  OPENSCAD_LIBRARIES=$BASEDIR
  GLEWDIR=$BASEDIR
  if [ -e $BASEDIR/include/Qt ]; then
    echo "Qt found under $BASEDIR ... "
    QTDIR=$BASEDIR
  fi
  OPENSCADDIR=$PWD
  DEPLOYDIR=$OPENSCADDIR/$OPENSCAD_BUILD_TARGET_TRIPLE
}

setup_dirs_msys()
{
  OPENSCADDIR=$PWD
  DEPLOYDIR=$OPENSCADDIR/$OPENSCAD_BUILD_TARGET_TRIPLE
}

setup_dirs_mxe()
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
  MXE_SYS_DIR=$MXEDIR/usr/$MXE_TARGET
  MXE_SYS_DIR_STATIC=$MXEDIR/usr/$ARCH-$SUB-$SYS.static
  MXE_SYS_DIR_SHARED=$MXEDIR/usr/$ARCH-$SUB-$SYS.shared
  OPENSCAD_LIBRARIES=$MXE_SYS_DIR
  OPENSCADDIR=$PWD
  DEPLOYDIR=$OPENSCADDIR/$MXE_TARGET
}

setup_dirs_freebsd()
{
  setup_dirs_generic
  QTDIR=/usr/local/share/qt4
  QMAKESPEC=freebsd-g++
  #QTDIR=/usr/local/share/qt5
  #QMAKESPEC=freebsd-clang
}

setup_dirs_netbsd()
{
  setup_dirs_generic
  QTDIR=/usr/pkg/qt4
  QMAKESPEC=netbsd-g++
}

save_path_generic()
{
  if [ ! $SETENV_SAVED_ORIGINAL_PATH ]; then
    echo "current PATH saved in SETENV_SAVED_ORIGINAL_PATH"
    SETENV_SAVED_ORIGINAL_PATH=$PATH
  fi
}

setup_path_generic()
{
  PATH=$BASEDIR/bin:$PATH
  LD_LIBRARY_PATH=$BASEDIR/lib:$BASEDIR/lib64
  LD_RUN_PATH=$BASEDIR/lib:$BASEDIR/lib64
}

setup_path_msys()
{
  MWBITS=32
  if [ $ARCH = x86_64 ]; then MWBITS=64; fi
  PATH=/mingw$MWBITS/bin:$PATH
}

setup_path_mxe()
{
  PATH=$MXEDIR/usr/bin:$PATH
  if [ $OPENSCAD_USEQT4 ]; then
    PATH=$MXE_SYS_DIR/qt/bin:$PATH
  else
    PATH=$MXE_SYS_DIR/qt5/bin:$PATH
  fi
}

setup_path_netbsd()
{
  setup_path_generic
  PATH=/usr/pkg/qt4/bin:$PATH
  LD_LIBRARY_PATH=/usr/pkg/qt4/lib:$LD_LIBRARY_PATH
  LD_LIBRARY_PATH=/usr/X11R7/lib:$LD_LIBRARY_PATH
  LD_LIBRARY_PATH=/usr/pkg/lib:$LD_LIBRARY_PATH
}

setup_clang_generic()
{
  CC=clang
  CXX=clang++
}

setup_clang_msys()
{
  setup_clang_generic
  echo if you have not already installed clang try this:
  echo   pacman -Sy mingw-w64-x86_64-clang or
  echo   pacman -Sy mingw-w64-i686-clang
}

setup_clang_linux()
{
  setup_clang_generic
  if [ $OPENSCAD_USEQT4 ]; then
    QMAKESPEC=unsupported/linux-clang
  else
    QMAKESPEC=linux-clang
  fi
}

setup_clang_freebsd()
{
  setup_clang_generic
  QMAKESPEC=unsupported/freebsd-clang
}

setup_varexportlist_common()
{
  vel=
  vel="$vel SETENV_SAVED_ORIGINAL_PATH OPENSCAD_BUILD_TARGET_OSTYPE CC CXX"
}

setup_varexportlist_generic()
{
  setup_varexportlist_common
  vel="$vel OPENSCAD_BUILD_TARGET_TRIPLE"
  vel="$vel OPENSCAD_BUILD_TARGET_ARCH"
  vel="$vel OPENSCAD_BUILD_TARGET_ABI"
  vel="$vel DEPLOYDIR BASEDIR LD_LIBRARY_PATH LD_RUN_PATH OPENSCAD_LIBRARIES"
  vel="$vel GLEWDIR QMAKESPEC QTDIR"
}

setup_varexportlist_msys()
{
  setup_varexportlist_common
  vel="$vel OPENSCAD_BUILD_TARGET_ARCH"
  vel="$vel OPENSCAD_BUILD_TARGET_ABI OPENSCAD_BUILD_TARGET_TRIPLE DEPLOYDIR"
}

setup_varexportlist_mxe()
{
  setup_varexportlist_common
  vel="$vel OPENSCAD_BUILD_TARGET_TRIPLE"
  vel="$vel OPENSCAD_BUILD_TARGET_ARCH"
  vel="$vel OPENSCAD_BUILD_TARGET_ABI OPENSCAD_LIBRARIES BASEDIR MXEDIR"
  vel="$vel MXE_TARGET MXE_SYS_DIR MXE_SYS_DIR_SHARED MXE_SYS_DIR_STATIC"
  vel="$vel DEPLOYDIR"
}

clean_variables_generic()
{
  if [ $SETENV_SAVED_ORIGINAL_PATH ]; then
    PATH=$SETENV_SAVED_ORIGINAL_PATH
    echo "PATH restored from SETENV_SAVED_ORIGINAL_PATH"
  fi
  if [ "`echo $vel`" ]; then
    for varname in $vel; do
      eval $varname"="
    done
  fi
  echo "SETENV build environment variables cleared"
}

export_and_print_vars_generic()
{
  if [ "`echo $vel`" ]; then
    for varname in $vel; do
      export $varname
      echo "$varname: "`eval echo "$"$varname`
    done
  fi
  echo "PATH: $PATH"
}


run()
{
  # run() calls function $1_generic, or a specialized version $1_$ostype
  # stackoverflow.com/questions/85880/determine-if-a-function-exists-in-bash
  runfunc1=`echo $1"_"$OPENSCAD_BUILD_TARGET_OSTYPE`
  runfunc2=`echo $1_generic`
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
  else
    OPENSCAD_BUILD_TARGET_OSTYPE=unknownos
  fi
}

ARGS=$*
detect_target_ostype $ARGS
run setup_varexportlist
if [ "`echo $ARGS | grep clean`" ]; then
  run clean_variables
  run export_and_print_vars
else
  if [ $SETENV_SAVED_ORIGINAL_PATH ]; then
    echo "$OPENSCAD_BUILD_TARGET_OSTYPE environment was previously setup"
    echo "Please run this script with 'clean' before use, or logout/login"
  else
    run setup_target
    run setup_dirs
    run save_path
    run setup_path
    if [ "`echo $ARGS | grep clang`" ]; then
      run setup_clang
    fi
    run export_and_print_vars
  fi
fi
