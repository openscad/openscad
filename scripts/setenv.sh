#!/bin/sh -e
#
# set environment variables for various builds
#
# Usage:
#
#  source ./scripts/setenv.sh               # standard darwin/linux/msys2 build
#  source ./scripts/setenv.sh linuxbrew     # use brew from $HOME/.linuxbrew
#  source ./scripts/setenv.sh i686-w64-mingw32.static   # crossbuild Win 32bit
#  source ./scripts/setenv.sh i686-w64-mingw32.shared   # cross DLLs Win 32bit
#  source ./scripts/setenv.sh x86_64-w64-mingw32.static # crossbuild Win 64bit
#  source ./scripts/setenv.sh x86_64-w64-mingw32.shared # cross DLLs Win 64bit
#  source ./scripts/setenv.sh clang         # build linux using clang compiler
#  source ./scripts/setenv.sh clean         # unset all exported variables
#
#  Use the 'machine triple' modeled on GNU. (gcc -dumpmachine)
#  ARCH,SUB,SYS,ABI - http://clang.llvn.org/docs/CrossCompilation
#  HOST = machine openscad will be run on
#  BUILD = machine openscad is being built on
#
# Notes:
#
#  This script works by using function naming and run() for portability.
#  "_generic" is a generic linux/bsd build. _msys / _mxe etc are specialized.
#  Only variables in the 'varexportlist' are exported.
#  This script uses a lot of global variables, take care if editing.


setup_host_generic()
{
  if [ "`echo $1 | grep mingw`" ]; then
    HOST_TRIPLE=$1
    MXE_TARGET=$1
  elif [ "`command -v gcc`" ]; then
    HOST_TRIPLE=`gcc -dumpmachine`
  elif [ "`command -v clang`" ]; then
    HOST_TRIPLE=`clang -dumpmachine`
  fi
}

setup_host_darwin()
{
  . ./setenv_mac-qt5.sh
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
  BUILDDIR=$OPENSCADDIR/bin/$OPENSCAD_BUILD_host_TRIPLE
}

setup_dirs_msys()
{
  OPENSCADDIR=$PWD
  BUILDDIR=$OPENSCADDIR/bin/$OPENSCAD_BUILD_host_TRIPLE
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
  BUILDDIR=$OPENSCADDIR/bin/$MXE_TARGET
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

setup_dirs_darwin()
{
  echo
  # noop, already done in setup_host_darwin() (. ../setenv_mac-qt5.sh)
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

setup_path_darwin()
{
  echo
  # noop, already done in setup_host_darwin() (. ../setenv_mac-qt5.sh)
}

setup_clang_generic()
{
  CC=clang
  CXX=clang++
}

setup_clang_darwin()
{
  echo
  # noop, already done in setup_host_darwin() (. ../setenv_mac-qt5.sh)
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
  vel="$vel SETENV_SAVED_ORIGINAL_PATH OPENSCAD_BUILD_host_OSTYPE"
  vel="$vel OPENSCAD_BUILD_host_TRIPLE"
  vel="$vel OPENSCAD_BUILD_host_ARCH"
  vel="$vel OPENSCAD_BUILD_host_ABI"
  vel="$vel BUILDDIR BASEDIR OPENSCAD_LIBRARIES"
}

setup_varexportlist_generic()
{
  setup_varexportlist_common
  vel="$vel LD_LIBRARY_PATH LD_RUN_PATH CC CXX"
  vel="$vel GLEWDIR QMAKESPEC QTDIR"
}

setup_varexportlist_darwin()
{
  # dont use common list here. look at ../setenv_mac-qt5.sh
  vel=
  vel="DYLD_LIBRARY_PATH DYLD_FRAMEWORK_PATH QMAKESPEC CCACHE_BASEDIR"
}

setup_varexportlist_mxe()
{
  setup_varexportlist_common
  vel="$vel MXEDIR"
  vel="$vel MXE_TARGET MXE_SYS_DIR MXE_SYS_DIR_SHARED MXE_SYS_DIR_STATIC"
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

export_and_print_vars()
{
  if [ "`echo $*`" ]; then
    for varname in $*; do
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
  runfunc1=`echo $1"_"$OPENSCAD_BUILD_host_OSTYPE`
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

if [ SETENV_SAVED_ORIGINAL_PATH ]; then
    echo "$OPENSCAD_BUILD_host_OSTYPE environment was previously setup"
fi

OPENSCAD_QMAKE_CONFIG=experimental
export_and_print_vars OPENSCAD_QMAKE_CONFIG

#run setup_host
#run setup_dirs
#run save_path
#run setup_path
#if [ "`echo $ARGS | grep clang`" ]; then
#  run setup_clang
#fi
