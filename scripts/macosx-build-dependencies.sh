#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit (and optionally 32-bit mode) mode
# and backwards compatible with 10.5 "Leopard".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-6lcd]
#  -6   Build only 64-bit binaries
#  -l   Force use of LLVM compiler
#  -c   Force use of clang compiler
#  -d   Build for deployment (if not specified, e.g. Sparkle won't be built)
#
# Prerequisites:
# - MacPorts: curl, cmake
#
# FIXME:
# o Verbose option
#

BASEDIR=$PWD/../libraries
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install
MAC_OSX_VERSION_MIN=10.5
OPTION_32BIT=true
OPTION_LLVM=false
OPTION_CLANG=false
OPTION_GCC=false
OPTION_DEPLOY=false
export QMAKESPEC=macx-g++

printUsage()
{
  echo "Usage: $0 [-6lcd]"
  echo
  echo "  -6   Build only 64-bit binaries"
  echo "  -l   Force use of LLVM compiler"
  echo "  -c   Force use of clang compiler"
  echo "  -d   Build for deployment"
}

# FIXME: Support gcc/llvm/clang flags. Use -platform <whatever> to make this work? kintel 20130117
build_qt()
{
  version=$1
  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
    curl -O http://releases.qt-project.org/qt4/source/qt-everywhere-opensource-src-$version.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  if $OPTION_CLANG; then
    # FIX for clang
    sed -i "" -e "s/::TabletProximityRec/TabletProximityRec/g"  src/gui/kernel/qt_cocoa_helpers_mac_p.h
  fi
  if $OPTION_32BIT; then
    QT_32BIT="-arch x86"
  fi
  ./configure -prefix $DEPLOYDIR -release $QT_32BIT -arch x86_64 -opensource -confirm-license -fast -no-qt3support -no-svg -no-phonon -no-audio-backend -no-multimedia -no-javascript-jit -no-script -no-scripttools -no-declarative -no-xmlpatterns -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit
  make -j6 install
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
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,chrono,system,regex
  if $OPTION_32BIT; then
    BOOST_EXTRA_FLAGS="-arch i386"
  fi
  if $OPTION_LLVM; then
    BOOST_TOOLSET="toolset=darwin-llvm"
    echo "using darwin : llvm : llvm-g++ ;" >> tools/build/v2/user-config.jam 
  elif $OPTION_CLANG; then
    BOOST_TOOLSET="toolset=clang"
    echo "using clang ;" >> tools/build/v2/user-config.jam 
  fi
  ./b2 -d+2 $BOOST_TOOLSET cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS" install
  install_name_tool -id $DEPLOYDIR/lib/libboost_thread.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -change libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -change libboost_chrono.dylib $DEPLOYDIR/lib/libboost_chrono.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
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
    # 4.2
    curl -O https://gforge.inria.fr/frs/download.php/32359/CGAL-$version.tar.gz
    # 4.1 curl -O https://gforge.inria.fr/frs/download.php/31641/CGAL-$version.tar.gz
    # 4.1-beta1 curl -O https://gforge.inria.fr/frs/download.php/31348/CGAL-$version.tar.gz
    # 4.0.2 curl -O https://gforge.inria.fr/frs/download.php/31175/CGAL-$version.tar.gz
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
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64$CGAL_EXTRA_FLAGS" -DBOOST_ROOT=$DEPLOYDIR -DBoost_USE_MULTITHREADED=false
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
  make GLEW_DEST=$DEPLOYDIR CC=$CC CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" LDFLAGS.EXTRA="-mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" STRIP= install
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
  if [ $version = "3.1.2" ]; then EIGENDIR=eigen-eigen-5097c01bcdc4;
  elif [ $version = "3.1.3" ]; then EIGENDIR=eigen-eigen-2249f9c22fe8; fi

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

build_sparkle()
{
  # Let Sparkle use the default compiler
  unset CC
  unset CXX
  version=$1
  echo "Building Sparkle" $version "..."
  cd $BASEDIR/src
  rm -rf Sparkle-$version
  if [ ! -f Sparkle-$version.zip ]; then
      curl -o Sparkle-$version.zip https://nodeload.github.com/andymatuschak/Sparkle/zip/$version
  fi
  unzip -q Sparkle-$version.zip
  cd Sparkle-$version
  patch -p1 < $OPENSCADDIR/patches/sparkle.patch
  if $OPTION_32BIT; then
    SPARKLE_EXTRA_FLAGS="-arch i386"
  fi
  xcodebuild clean
  xcodebuild -arch x86_64 $SPARKLE_EXTRA_FLAGS
  rm -rf $DEPLOYDIR/lib/Sparkle.framework
  cp -Rf build/Release/Sparkle.framework $DEPLOYDIR/lib/ 
  install_name_tool -id $DEPLOYDIR/lib/Sparkle.framework/Versions/A/Sparkle $DEPLOYDIR/lib/Sparkle.framework/Sparkle
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

while getopts '6lcd' c
do
  case $c in
    6) OPTION_32BIT=false;;
    l) OPTION_LLVM=true;;
    c) OPTION_CLANG=true;;
    d) OPTION_DEPLOY=true;;
  esac
done

OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_VERSION >= 8 )); then
  echo "Detected Mountain Lion (10.8) or later"
elif (( $OSX_VERSION >= 7 )); then
  echo "Detected Lion (10.7) or later"
else
  echo "Detected Snow Leopard (10.6) or earlier"
fi

USING_LLVM=false
USING_GCC=false
USING_CLANG=false
if $OPTION_LLVM; then
  USING_LLCM=true
elif $OPTION_GCC; then
  USING_GCC=true
elif $OPTION_CLANG; then
  USING_CLANG=true
elif (( $OSX_VERSION >= 7 )); then
  USING_GCC=true
fi

if $USING_LLVM; then
  echo "Using gcc LLVM compiler"
  export CC=llvm-gcc
  export CXX=llvm-g++
  export QMAKESPEC=macx-llvm
elif $USING_GCC; then
  echo "Using gcc compiler"
  export CC=gcc
  export CXX=g++
  export CPP=cpp
  # Somehow, qmake in Qt-4.8.2 doesn't detect Lion's gcc and falls back into
  # project file mode unless manually given a QMAKESPEC
  export QMAKESPEC=macx-llvm
elif $USING_CLANG; then
  echo "Using clang compiler"
  export CC=clang
  export CXX=clang++
  export QMAKESPEC=unsupported/macx-clang
fi

if (( $OSX_VERSION >= 8 )); then
  echo "Setting build target to 10.6 or later"
  MAC_OSX_VERSION_MIN=10.6
else
  echo "Setting build target to 10.5 or later"
  MAC_OSX_VERSION_MIN=10.5
fi

if $OPTION_DEPLOY; then
  echo "Building deployment version of libraries"
else
  OPTION_32BIT=false
fi

if $OPTION_32BIT; then
  echo "Building combined 32/64-bit binaries"
else
  echo "Building 64-bit binaries"
fi

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR
build_qt 4.8.4
# NB! For eigen, also update the path in the function
build_eigen 3.1.3
build_gmp 5.1.2
build_mpfr 3.1.2
build_boost 1.53.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.2
build_glew 1.9.0
build_opencsg 1.3.2
if $OPTION_DEPLOY; then
  build_sparkle 0ed83cf9f2eeb425d4fdd141c01a29d843970c20
fi
