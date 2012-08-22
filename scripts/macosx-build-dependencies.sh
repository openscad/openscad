#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit (and optionally 32-bit mode) mode
# and backwards compatible with 10.5 "Leopard".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-6l]
#  -6   Build only 64-bit binaries
#  -l   Force use of LLVM compiler
#
# Prerequisites:
# - MacPorts: curl, cmake
# - Qt4
#
# FIXME:
# o Verbose option
# o Port to other platforms?
#

BASEDIR=$PWD/../libraries
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install
MAC_OSX_VERSION_MIN=10.5
OPTION_32BIT=true
OPTION_LLVM=false
export QMAKESPEC=macx-g++

printUsage()
{
  echo "Usage: $0 [-6l]"
  echo
  echo "  -6   Build only 64-bit binaries"
  echo "  -l   Force use of LLVM compiler"
}

# Hack warning: gmplib is built separately in 32-bit and 64-bit mode
# and then merged afterwards. gmplib's header files are dependent on
# the CPU architecture on which configure was run and will be patched accordingly.
build_gmp()
{
  version=$1
  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp-$version
  if [ ! -f gmp-$version.tar.bz2 ]; then
    curl -O ftp://ftp.gmplib.org/pub/gmp-$version/gmp-$version.tar.bz2
  fi
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  if $OPTION_32BIT; then
    mkdir build-i386
    cd build-i386
    ../configure --prefix=$DEPLOYDIR/i386 "CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" ABI=32 --enable-cxx
    make install
    cd ..
  fi

  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 "CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" ABI=64 --enable-cxx
  make install

  # merge
  cd $DEPLOYDIR
  mkdir -p lib
  if $OPTION_32BIT; then
    lipo -create i386/lib/libgmp.dylib x86_64/lib/libgmp.dylib -output lib/libgmp.dylib
    lipo -create i386/lib/libgmpxx.dylib x86_64/lib/libgmpxx.dylib -output lib/libgmpxx.dylib
  else
    cp x86_64/lib/libgmp.dylib lib/libgmp.dylib
    cp x86_64/lib/libgmpxx.dylib lib/libgmpxx.dylib
  fi
  install_name_tool -id $DEPLOYDIR/lib/libgmp.dylib lib/libgmp.dylib
  install_name_tool -id $DEPLOYDIR/lib/libgmpxx.dylib lib/libgmpxx.dylib
  install_name_tool -change $DEPLOYDIR/x86_64/lib/libgmp.10.dylib $DEPLOYDIR/lib/libgmp.dylib lib/libgmpxx.dylib
  if $OPTION_32BIT; then
    cp lib/libgmp.dylib i386/lib/
    cp lib/libgmp.dylib x86_64/lib/
    cp lib/libgmpxx.dylib i386/lib/
    cp lib/libgmpxx.dylib x86_64/lib/
  fi
  mkdir -p include
  cp x86_64/include/gmp.h include/
  cp x86_64/include/gmpxx.h include/

  patch -p0 include/gmp.h << EOF
--- gmp.h.orig	2011-11-08 01:03:41.000000000 +0100
+++ gmp.h	2011-11-08 01:06:21.000000000 +0100
@@ -26,12 +26,28 @@
 #endif
 
 
-/* Instantiated by configure. */
-#if ! defined (__GMP_WITHIN_CONFIGURE)
+#if defined(__i386__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 0
+#define GMP_LIMB_BITS                      32
+#define GMP_NAIL_BITS                      0
+#elif defined(__x86_64__)
 #define __GMP_HAVE_HOST_CPU_FAMILY_power   0
 #define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 0
 #define GMP_LIMB_BITS                      64
 #define GMP_NAIL_BITS                      0
+#elif defined(__ppc__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 1
+#define GMP_LIMB_BITS                      32
+#define GMP_NAIL_BITS                      0
+#elif defined(__powerpc64__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 1
+#define GMP_LIMB_BITS                      64
+#define GMP_NAIL_BITS                      0
+#else
+#error Unsupported architecture
 #endif
 #define GMP_NUMB_BITS     (GMP_LIMB_BITS - GMP_NAIL_BITS)
 #define GMP_NUMB_MASK     ((~ __GMP_CAST (mp_limb_t, 0)) >> GMP_NAIL_BITS)
EOF
}

# As with gmplib, mpfr is built separately in 32-bit and 64-bit mode and then merged
# afterwards.
build_mpfr()
{
  version=$1
  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version
#  curl -O http://www.mpfr.org/mpfr-$version/allpatches
#  patch -N -Z -p1 < allpatches 
  if $OPTION_32BIT; then
    mkdir build-i386
    cd build-i386
    ../configure --prefix=$DEPLOYDIR/i386 --with-gmp=$DEPLOYDIR/i386 CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386"
    make install
    cd ..
  fi

  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 --with-gmp=$DEPLOYDIR/x86_64 CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64"
  make install

  # merge
  cd $DEPLOYDIR
  if $OPTION_32BIT; then
    lipo -create i386/lib/libmpfr.dylib x86_64/lib/libmpfr.dylib -output lib/libmpfr.dylib
  else
    cp x86_64/lib/libmpfr.dylib lib/libmpfr.dylib
  fi
  install_name_tool -id $DEPLOYDIR/lib/libmpfr.dylib lib/libmpfr.dylib
  mkdir -p include
  cp x86_64/include/mpfr.h include/
  cp x86_64/include/mpf2mpfr.h include/
}


build_boost()
{
  version=$1
  bversion=`echo $version | tr "." "_"`
  echo "Building boost" $version "..."
  cd $BASEDIR/src
  rm -rf boost_$bversion
  if [ ! -f boost_$bversion.tar.bz2 ]; then
    curl -LO http://downloads.sourceforge.net/project/boost/boost/$version/boost_$bversion.tar.bz2
  fi
  tar xjf boost_$bversion.tar.bz2
  cd boost_$bversion
  # We only need the thread and program_options libraries
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,system,regex
  if $OPTION_32BIT; then
    BOOST_EXTRA_FLAGS="-arch i386"
  fi
  if $OPTION_LLVM; then
    BOOST_TOOLSET="toolset=darwin-llvm"
    echo "using darwin : llvm : llvm-g++ ;" >> tools/build/v2/user-config.jam 
  fi
  ./b2 -d+2 $BOOST_TOOLSET cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS" install
  install_name_tool -id $DEPLOYDIR/lib/libboost_thread.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_program_options.dylib $DEPLOYDIR/lib/libboost_program_options.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_filesystem.dylib $DEPLOYDIR/lib/libboost_filesystem.dylib 
  install_name_tool -change libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_filesystem.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_regex.dylib $DEPLOYDIR/lib/libboost_regex.dylib 


}

build_cgal()
{
  version=$1
  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.gz ]; then
      # 4.0.2
      curl -O https://gforge.inria.fr/frs/download.php/31175/CGAL-$version.tar.gz
    # 4.0 curl -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar xzf CGAL-$version.tar.gz
  cd CGAL-$version
  if $OPTION_32BIT; then
    CGAL_EXTRA_FLAGS=";i386"
  fi
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64$CGAL_EXTRA_FLAGS" -DBOOST_ROOT=$DEPLOYDIR
  make -j4
  make install
  install_name_tool -id $DEPLOYDIR/lib/libCGAL.dylib $DEPLOYDIR/lib/libCGAL.dylib
  install_name_tool -id $DEPLOYDIR/lib/libCGAL_Core.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  install_name_tool -change $PWD/lib/libCGAL.9.dylib $DEPLOYDIR/lib/libCGAL.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
}

build_glew()
{
  version=$1
  echo "Building GLEW" $version "..."
  cd $BASEDIR/src
  rm -rf glew-$version
  if [ ! -f glew-$version.tgz ]; then
    curl -LO http://downloads.sourceforge.net/project/glew/glew/$version/glew-$version.tgz
  fi
  tar xzf glew-$version.tgz
  cd glew-$version
  mkdir -p $DEPLOYDIR/lib/pkgconfig
  if $OPTION_32BIT; then
    GLEW_EXTRA_FLAGS="-arch i386"
  fi
  make GLEW_DEST=$DEPLOYDIR CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" LDFLAGS.EXTRA="-mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" STRIP= install
}

build_opencsg()
{
  version=$1
  echo "Building OpenCSG" $version "..."
  cd $BASEDIR/src
  rm -rf OpenCSG-$version
  if [ ! -f OpenCSG-$version.tar.gz ]; then
    curl -O http://www.opencsg.org/OpenCSG-$version.tar.gz
  fi
  tar xzf OpenCSG-$version.tar.gz
  cd OpenCSG-$version
  patch -p1 < $OPENSCADDIR/patches/OpenCSG-$version-MacOSX-port.patch
  if $OPTION_32BIT; then
    OPENCSG_EXTRA_FLAGS="x86"
  fi
  OPENSCAD_LIBRARIES=$DEPLOYDIR qmake -r CONFIG+="x86_64 $OPENCSG_EXTRA_FLAGS"
  make install
}

build_eigen()
{
  version=$1
  echo "Building eigen" $version "..."
  cd $BASEDIR/src
  rm -rf eigen-$version

  EIGENDIR="none"
  if [ $version = "2.0.17" ]; then EIGENDIR=eigen-eigen-b23437e61a07; fi
  if [ $version = "3.1.1" ]; then EIGENDIR=eigen-eigen-43d9075b23ef; fi
  if [ $EIGENDIR = "none" ]; then
    echo Unknown eigen version. Please edit script.
    exit 1
  fi
  rm -rf ./$EIGENDIR

  if [ ! -f eigen-$version.tar.bz2 ]; then
    curl -LO http://bitbucket.org/eigen/eigen/get/$version.tar.bz2
    mv $version.tar.bz2 eigen-$version.tar.bz2
  fi
  tar xjf eigen-$version.tar.bz2
  ln -s ./$EIGENDIR eigen-$version
  cd eigen-$version
  mkdir build
  cd build
  if $OPTION_32BIT; then
    EIGEN_EXTRA_FLAGS=";i386"
  fi
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_BUILD_LIB=ON -DBUILD_SHARED_LIBS=FALSE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64$EIGEN_EXTRA_FLAGS" ..
  make -j4
  make install
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

while getopts '6l' c
do
  case $c in
    6) OPTION_32BIT=false;;
    l) OPTION_LLVM=true;;
  esac
done

OSVERSION=`sw_vers -productVersion | cut -d. -f2`
if [[ $OSVERSION -ge 7 ]]; then
  echo "Detected Lion or later"
  export LION=1
  export CC=gcc
  export CXX=g++
  export CPP=cpp
  # Somehow, qmake in Qt-4.8.2 doesn't detect Lion's gcc and falls back into
  # project file mode unless manually given a QMAKESPEC
  export QMAKESPEC=macx-llvm
else
  echo "Detected Snow Leopard or earlier"
fi

if $OPTION_LLVM; then
  echo "Using LLVM compiler"
  export CC=llvm-gcc
  export CXX=llvm-g++
  export QMAKESPEC=macx-llvm
fi

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR
build_eigen 3.1.1
build_gmp 5.0.5
build_mpfr 3.1.1
build_boost 1.50.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.0.2
build_glew 1.9.0
build_opencsg 1.3.2
