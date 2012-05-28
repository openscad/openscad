#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for Linux
#
# This script must be run from the OpenSCAD source root directory
#
# Usage: linux-build-dependencies.sh
#
# Prerequisites:
# - curl
# -- if you dont have curl, but do have wget, uncomment 'build_curl')
# -- and add $BASEDIR/bin to your PATH, i.e. in .bash_profile
# - Qt4
#

BASEDIR=$HOME
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR
NUMCPU=4 # paralell builds for some libraries

printUsage()
{
  echo "Usage: $0"
  echo
}

build_curl()
{
  version=$1
  echo "Building curl" $version "..."
  cd $BASEDIR/src
  rm -rf curl-$version
  if [ ! -f curl-$version.tar.bz2 ]; then
    wget http://curl.haxx.se/download/curl-$version.tar.bz2
  fi
  tar xjf curl-$version.tar.bz2
  cd curl-$version
  mkdir build
  cd build
  ../configure --prefix=$DEPLOYDIR
  make -j$NUMCPU install
}

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
  mkdir build
  cd build
  ../configure --prefix=$DEPLOYDIR --enable-cxx
  make install
}

build_mpfr()
{
  version=$1
  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl -O http://www.mpfr.org/mpfr-current/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version
  curl -O http://www.mpfr.org/mpfr-current/allpatches
  patch -N -Z -p1 < allpatches
  mkdir build
  cd build
  ../configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR
  make install
  cd ..
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
  # We only need certain portions of boost
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,system,regex
  ./bjam
  ./bjam install
}

build_cgal()
{
  version=$1
  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.gz ]; then
    #4.0
    curl -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar xzf CGAL-$version.tar.gz
  cd CGAL-$version
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.so -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.so -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.so -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_ROOT=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Debug
  make -j$NUMCPU
  make install
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
  make GLEW_DEST=$DEPLOYDIR install
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
  sed -i s/example// opencsg.pro # examples might be broken without GLUT
  OPENSCAD_LIBRARIES=$DEPLOYDIR qmake
  make install
}

build_eigen()
{
  version=$1
  echo "Building eigen" $version "..."
  cd $BASEDIR/src
  rm -rf eigen-$version
  ## Directory name for v2.0.17
  rm -rf eigen-eigen-b23437e61a07
  if [ ! -f eigen-$version.tar.bz2 ]; then
    curl -LO http://bitbucket.org/eigen/eigen/get/$version.tar.bz2
    mv $version.tar.bz2 eigen-$version.tar.bz2
  fi
  tar xjf eigen-$version.tar.bz2
  ## File name for v2.0.17
  ln -s eigen-eigen-b23437e61a07 eigen-$version
  cd eigen-$version
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR
  make -j$NUMCPU
  make install
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

if [ ! -d $BASEDIR/bin ]; then
  mkdir --parents $BASEDIR/bin
fi

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR
#build_curl 7.26.0
build_eigen 2.0.17
build_gmp 5.0.5
build_mpfr 3.1.0
build_boost 1.47.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.0
build_glew 1.7.0
build_opencsg 1.3.2
