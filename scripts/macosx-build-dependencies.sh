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
DEPLOYDIR=$BASEDIR/deploy

build_gmp()
{
  version=$1
  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp*
  curl -O ftp://ftp.gmplib.org/pub/gmp-$version/gmp-$version.tar.bz2
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  mkdir build-i386
  cd build-i386
  ../configure --prefix=$DEPLOYDIR "CFLAGS=-mmacosx-version-min=10.5 -arch i386" LDFLAGS="-mmacosx-version-min=10.5 -arch i386" ABI=32 --libdir=$DEPLOYDIR/lib-i386
  make install
  cd ..
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR "CFLAGS=-mmacosx-version-min=10.5" LDFLAGS="-mmacosx-version-min=10.5" --libdir=$DEPLOYDIR/lib-x86_64
  make install
  cd $DEPLOYDIR
  mkdir -p lib
  lipo -create lib-i386/libgmp.dylib lib-x86_64/libgmp.dylib -output lib/libgmp.dylib
  install_name_tool -id $DEPLOYDIR/lib/libgmp.dylib lib/libgmp.dylib
}

build_mpfr()
{
  version=$1
  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr*
  curl -O http://www.mpfr.org/mpfr-current/mpfr-$version.tar.bz2
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version
  mkdir build-i386
  cd build-i386
  ../configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR CFLAGS="-mmacosx-version-min=10.5 -arch i386" LDFLAGS="-mmacosx-version-min=10.5 -arch i386"  --libdir=$DEPLOYDIR/lib-i386
  make install
  cd ..
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR CFLAGS="-mmacosx-version-min=10.5 -arch x86_64" LDFLAGS="-mmacosx-version-min=10.5 -arch x86_64"  --libdir=$DEPLOYDIR/lib-x86_64
  make install
  cd $DEPLOYDIR
  lipo -create lib-i386/libmpfr.dylib lib-x86_64/libmpfr.dylib -output lib/libmpfr.dylib
  install_name_tool -id $DEPLOYDIR/lib/libmpfr.dylib lib/libmpfr.dylib
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
  curl -O https://gforge.inria.fr/frs/download.php/26688/CGAL-$version.tar.gz
  tar xzf CGAL-$version.tar.gz
  cd CGAL-$version
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
build_gmp 5.0.1
build_mpfr 2.4.2
build_boost 1.43.0
build_cgal 3.6
build_glew 1.5.4
build_opencsg 1.3.0
