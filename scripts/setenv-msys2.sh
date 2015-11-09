#!/bin/sh -e
#
# set environment variables for MSYS2 build
#
# Usage:
#
#  source ./scripts/setenv-msys2.sh 32        # 32 bit version
#  source ./scripts/setenv-msys2.sh 64        # 64 bit version
#  source ./scripts/setenv-msys2.sh clean     # Clean up exported variables
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
  if [ "`echo $* | grep 64 `" ]; then ARCH=x86_64 ; fi

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

clean_variables()
{
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
  OPENSCADDIR=$PWD
  setup_target $*
  setup_path $*
  setup_deploydir $*
  echo "MSYS2 build environment variables setup.."
}

vl=
vl="$vl OPENSCAD_BUILD_TARGET_ARCH OPENSCAD_BUILD_TARGET_OSTYPE"
vl="$vl OPENSCAD_BUILD_TARGET_ABI OPENSCAD_BUILD_TARGET_TRIPLE DEPLOYDIR"

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
