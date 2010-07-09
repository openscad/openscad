#!/bin/sh
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 32- and 64-bit mode and backwards compatible with 
# 10.5 "Leopard".
# 
# Usage:
# - Edit the BASEDIR variable. This is where libraries will be built and installed
# - Edit the OPENSCADDIR variable. This is where patches are fetched from
#
# Prerequisites:
# - MacPorts: curl eigen
# - Qt4
#
# FIXME:
# o Verbose option
# o Port to other platforms?
#

BASEDIR=/Users/kintel/code/metalab/checkout/OpenSCAD/libraries
OPENSCADDIR=/Users/kintel/code/metalab/checkout/OpenSCAD/openscad-release
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install

# Hack warning: gmplib is built separately in 32-bit and 64-bit mode
# and then merged afterwards.  Somehow, gmplib's header files appear
# to be dependant on the CPU architecture on which configure was
# run. Not nice, but as long as we also build mpfr in two separate
# steps and nobody else uses these architecture-dependent macros, we
# should be fine.
build_gmp()
{
  version=$1
  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp*
  curl -O ftp://ftp.gmplib.org/pub/gmp-$version/gmp-$version.tar.bz2
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  # 32-bit version
  mkdir build-i386
  cd build-i386
  ../configure --prefix=$DEPLOYDIR/i386 "CFLAGS=-mmacosx-version-min=10.5 -arch i386" LDFLAGS="-mmacosx-version-min=10.5 -arch i386" ABI=32 --enable-cxx
  make install
  cd ..
  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 "CFLAGS=-mmacosx-version-min=10.5" LDFLAGS="-mmacosx-version-min=10.5" --enable-cxx
  make install

  # merge
  cd $DEPLOYDIR
  mkdir -p lib
  lipo -create i386/lib/libgmp.dylib x86_64/lib/libgmp.dylib -output lib/libgmp.dylib
  install_name_tool -id $DEPLOYDIR/lib/libgmp.dylib lib/libgmp.dylib
  cp lib/libgmp.dylib i386/lib/
  cp lib/libgmp.dylib x86_64/lib/
  mkdir -p include
  cp x86_64/include/gmp.h include/
  cp x86_64/include/gmpxx.h include/
}

# As with gmplib, mpfr is built separately in 32-bit and 64-bit mode and then merged
# afterwards.
build_mpfr()
{
  version=$1
  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr*
  curl -O http://www.mpfr.org/mpfr-current/mpfr-$version.tar.bz2
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version

  # 32-bit version
  mkdir build-i386
  cd build-i386
  ../configure --prefix=$DEPLOYDIR/i386 --with-gmp=$DEPLOYDIR/i386 CFLAGS="-mmacosx-version-min=10.5 -arch i386" LDFLAGS="-mmacosx-version-min=10.5 -arch i386"
  make install
  cd ..

  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 --with-gmp=$DEPLOYDIR/x86_64 CFLAGS="-mmacosx-version-min=10.5 -arch x86_64" LDFLAGS="-mmacosx-version-min=10.5 -arch x86_64"
  make install

  # merge
  cd $DEPLOYDIR
  lipo -create i386/lib/libmpfr.dylib x86_64/lib/libmpfr.dylib -output lib/libmpfr.dylib
  install_name_tool -id $DEPLOYDIR/lib/libmpfr.dylib lib/libmpfr.dylib
  mkdir -p include
  cp x86_64/include/mpfr.h include/
  cp x86_64/include/mpf2mpfr.h include/
}


build_boost()
{
  version=$1
  bversion=`echo $version | tr "." "_"`
  echo "Building boost::thread" $version "..."
  cd $BASEDIR/src
  rm -rf boost*
  curl -LO http://downloads.sourceforge.net/project/boost/boost/$version/boost_$bversion.tar.bz2
  tar xjf boost_$bversion.tar.bz2
  cd boost_$bversion
  # We only need the thread library for now
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread
  ./bjam cflags="-mmacosx-version-min=10.5 -arch i386 -arch x86_64" linkflags="-mmacosx-version-min=10.5 -arch i386 -arch x86_64"
  ./bjam install
  install_name_tool -id $DEPLOYDIR/lib/libboost_thread.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
}

build_cgal()
{
  version=$1
  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  curl -O https://gforge.inria.fr/frs/download.php/27222/CGAL-$version.tar.gz
  tar xzf CGAL-$version.tar.gz
  cd CGAL-$version
  # We build a static lib. Not really necessary, but it's well tested.
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DBUILD_SHARED_LIBS=FALSE -DCMAKE_OSX_DEPLOYMENT_TARGET="10.5" -DCMAKE_OSX_ARCHITECTURES="i386;x86_64"
  make -j4
  make install
}

build_glew()
{
  version=$1
  echo "Building GLEW" $version "..."
  cd $BASEDIR/src
  curl -LO http://downloads.sourceforge.net/project/glew/glew/$version/glew-$version.tgz
  tar xzf glew-$version.tgz
  cd glew-$version
  mkdir -p $DEPLOYDIR/lib/pkgconfig
  # To avoid running strip on a fat archive as this is not supported by strip
  sed -i bak -e "s/\$(STRIP) -x lib\/\$(LIB.STATIC)//" Makefile 
  make GLEW_DEST=$DEPLOYDIR CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=10.5 -arch i386 -arch x86_64" LDFLAGS.EXTRA="-mmacosx-version-min=10.5 -arch i386 -arch x86_64" install
}

build_opencsg()
{
  version=$1
  echo "Building OpenCSG" $version "..."
  cd $BASEDIR/src
  curl -O http://www.opencsg.org/OpenCSG-$version.tar.gz
  tar xzf OpenCSG-$version.tar.gz
  cd OpenCSG-$version
  patch -p1 < $OPENSCADDIR/patches/OpenCSG-$version-MacOSX-port.patch
  MACOSX_DEPLOY_DIR=$DEPLOYDIR qmake -r CONFIG+="x86 x86_64"
  make install
}

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR
#build_gmp 5.0.1
#build_mpfr 3.0.0
#build_boost 1.43.0
#build_cgal 3.6.1
#build_glew 1.5.4
build_opencsg 1.3.0
