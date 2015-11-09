#!/bin/sh -e
#
# set environment variables for MSYS2 build
#
# Usage:
#
#  source ./scripts/setenv-msys2.sh           # Default build
#  source ./scripts/setenv-msys2.sh clang     # Use clang compiler, not gcc
#  source ./scripts/setenv-msys2.sh clean     # Clean up exported variables
#
# 32 or 64 bit is selected by starting the appropriate "MINGW64" or
# "MINGW32" shell on the system and runnning these commands from within it.
#
# Prerequisites:
#
# Please download and install msys2 from http://msys2.github.io
#
# Note - this uses a lot of unpleasant global variables, take care if editing

setup_target()
{
  ARCH=i686
  SUB=w64
  SYS=windows
  ABI=gnu
  if [ "`uname -a | grep -i x86_64`" ]; then
    ARCH=x86_64
  fi

  OPENSCAD_BUILD_TARGET_OSTYPE=msys
  OPENSCAD_BUILD_TARGET_ARCH=$ARCH
  OPENSCAD_BUILD_TARGET_TRIPLE=$ARCH-$SUB-$SYS.$ABI
}

setup_deploydir()
{
  DEPLOYDIR=$OPENSCADDIR/$OPENSCAD_BUILD_TARGET_TRIPLE
  if [ ! -e $DEPLOYDIR ]; then
    mkdir -p $DEPLOYDIR
  fi
}

setup_path()
{
  if [ ! $MSYS2_SAVED_ORIGINAL_PATH ]; then
    echo "current PATH saved in MSYS2_SAVED_ORIGINAL_PATH"
    MSYS2_SAVED_ORIGINAL_PATH=$PATH
  fi

  MWBITS=32
  if [ $ARCH = x86_64 ]; then MWBITS=64; fi
  PATH=/mingw$MWBITS/bin:$PATH
}

setup_clang()
{
  CC=clang
  CXX=clang++
  echo if you have not already installed clang try this:
  echo   pacman -Sy mingw-w64-x86_64-clang or
  echo   pacman -Sy mingw-w64-i686-clang
}

clean_variables()
{
  if [ $MSYS2_SAVED_ORIGINAL_PATH ]; then
    PATH=$MSYS2_SAVED_ORIGINAL_PATH
    echo "PATH restored from MSYS2_SAVED_ORIGINAL_PATH"
  fi
  for varname in $vl; do
    eval $varname"="
  done
  echo "MSYS2 build environment variables cleared"
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
  if [ "`echo $* | grep clang`" ]; then
    setup_clang
  fi
  OPENSCADDIR=$PWD
  setup_target $*
  setup_path $*
  setup_deploydir $*
  echo "MSYS2 build environment variables setup.."
}

vl=
vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
vl="$vl OPENSCAD_BUILD_TARGET_ABI OPENSCAD_BUILD_TARGET_TRIPLE DEPLOYDIR"
vl="$vl CC CXX"

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
