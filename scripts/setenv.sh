#!/bin/sh -e
#
# set environment variables for various builds
#
# Usage:
#
#  source ./scripts/setenv.sh               # standard darwin/linux/msys2 build
#
# MXE Linux->Windows cross building:
#
#  source ./scripts/setenv.sh i686-w64-mingw32.static   # Static link 32bit
#  source ./scripts/setenv.sh i686-w64-mingw32.shared   # Dynamic link 32bit
#  source ./scripts/setenv.sh x86_64-w64-mingw32.static # Static link 64bit
#  source ./scripts/setenv.sh x86_64-w64-mingw32.shared # Dynamic link 64bit
#
# Special linux builds:
#
#  source ./scripts/setenv.sh brew          # use linuxbrew from $HOME/.linuxbrew
#  source ./scripts/setenv.sh clang         # build linux using clang compiler
#
# See Also:
#
#  ARCH,SUB,SYS,ABI - http://clang.llvn.org/docs/CrossCompilation
#  https://gcc.gnu.org/onlinedocs/gccint/Configure-Terms.html
#  HOST = machine openscad will be run on
#  BUILD = machine openscad is being built on
#
# Notes:
#
#  This script works by using function naming and run()
#  Ex, using 'run setup_mypath' will run setup_mypath, but if you
#  are on darwin, it will instead run 'setup_mypath_darwin', which
#  may or may not call the main setup_mypath

setup_host()
{
  if [ "`echo $1 | grep mingw`" ]; then
    HOST_TRIPLE=$1
    MXE_TARGET=$1
  elif [ "`command -v gcc`" ]; then
    HOST_TRIPLE=`gcc -dumpmachine`
  elif [ "`command -v clang`" ]; then
    HOST_TRIPLE=`clang -dumpmachine`
  fi
  export_and_print_vars HOST_TRIPLE MXE_TARGET
}

setup_host_darwin()
{
  . ./setenv_mac-qt5.sh
}

setup_buildtype()
{
  BUILDTYPE=$OSTYPE
  case
    $BUILDTYPE in darwin*)
      BUILDTYPE=darwin
    ;;
    $BUILDTYPE in linux*)
      BUILDTYPE=linux
    ;;
  esac
  if [ "`echo $1 | grep mingw`" ]; then
    BUILDTYPE=mxe
  fi
  export_and_print_vars BUILDTYPE
}

def setup_dirvars()
{
  OPENSCADDIR=$PWD

  if [ ! -f $OPENSCADDIR/openscad.pro ]; then
    echo "Cannot find OPENSCADDIR/openscad.pro, OPENSCADDIR should be src root "
    exit 1
  fi

  DEPLOYDIR=$OPENSCADDIR/bin/$HOST_TRIPLE

  export_and_print_vars OPENSCADDIR DEPLOYDIR
}

setup_dirvars_mxe()
{
  setup_dirvars
  if [ ! $MXEDIR ]; then
    MXEDIR=$HOME/openscad_deps/mxe
  fi
  if [ ! -e $MXEDIR ]; then
    if [ -e /opt/mxe ]; then
       MXEDIR=/opt/mxe # mxe on custom build machines
    fi
  fi
  if [ ! -e $MXEDIR ]; then
    if [ -e /usr/lib/mxe ]; then
       MXEDIR=/usr/lib/mxe  # mxe dpkg binary on debian
    fi
  fi
  if [ ! -e $MXEDIR ]; then
    echo can't find mxe. please install, see README.md
  fi
  OPENSCAD_LIBRARIES=$MXEDIR/usr/$MXE_TARGET
  PATH=$MXEDIR/usr/bin:$PATH
  PKG_CONFIG_PATH=$MXEDIR/lib/pkgconfig
  export_and_print_vars OPENSCAD_LIBRARIES PATH PKG_CONFIG_PATH
}

setup_dirvars_linuxbrew()
{
  setup_dirvars
  OPENSCAD_LIBRARIES=$HOME/.linuxbrew
  PATH=$HOME/.linuxbrew/bin:$PATH
  PATH=$HOME/.linuxbrew/sbin:$PATH
  PATH=$HOME/.linuxbrew/opt/lib/qt5/bin:$PATH
  PKG_CONFIG_PATH=$HOME/.linuxbrew/lib/pkgconfig
  export_and_print_vars OPENSCAD_LIBRARIES PATH PKG_CONFIG_PATH
}

def setup_dirvars_darwin()
{
  if [ ! $OPENSCAD_LIBRARIES ]; then
    OPENSCAD_LIBRARIES=/usr/local/Cellar
    if [ ! -e $OPENSCAD_LIBRARIES ]; then
      OPENSCAD_LIBRARIES=/opt/local
    fi
    if [ ! -e $OPENSCAD_LIBRARIES ]; then
      echo cannot find /usr/local/Cellar nor /opt/local, please see README
    fi
  fi
  export_and_print_vars OPENSCAD_LIBRARIES
}

export_and_print_vars()
{
  if [ "`echo $*`" ]; then
    for varname in $*; do
      export $varname
      echo "$varname: "`eval echo "$"$varname`
    done
  fi
}

run()
{
  # run() calls function $1, or a specialized version $1_$ostype.
  # also it exits the build if there is an error.
  # stackoverflow.com/questions/85880/determine-if-a-function-exists-in-bash
  runfunc1=`echo $1"_"$BUILDTYPE`
  runfunc2=`echo $1`
  if [ "`type -t $runfunc1 | grep function`" ]; then
    echo "calling $runfunc1"
    eval $runfunc1
  elif [ "`type -t $runfunc2 | grep function`" ]; then
    echo "calling $runfunc2"
    eval $runfunc2
  else
    eval $1
  fi
  if [[ $? != 0 ]]; then
    echo "Error while running $1 ...Stopping"
    exit 1
  fi
}

setup_version()
{
  OPENSCAD_VERSION=`date "+%Y.%m.%d"`
  export_and_print_vars OPENSCAD_VERSION
}

run setup_host
run setup_buildtype
run setup_dirvars
run setup_version

#run setup_dirs
#run save_path
#run setup_path
#if [ "`echo $ARGS | grep clang`" ]; then
#  run setup_clang
#fi
