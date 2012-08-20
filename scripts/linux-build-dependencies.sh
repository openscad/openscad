#!/bin/sh -e

# test_pretty_print copyright 2012 don bright. released under the GPL 2, or
# later, as described in the file named 'COPYING' in OpenSCAD's project root.
# permission to change this license is given to Marius Kintel & Clifford Wolf

#
# This script builds all library dependencies of OpenSCAD for Linux
#
# This script must be run from the OpenSCAD source root directory
#
# Usage: linux-build-dependencies.sh
#
# Prerequisites:
# - wget or curl
# - Qt4
#

printUsage()
{
  echo "Usage: $0"
  echo
}

build_git()
{
  version=$1
  echo "Building git" $version "..."
  cd $BASEDIR/src
  rm -rf git-$version
  if [ ! -f git-$version.tar.gz ]; then
    curl -O http://git-core.googlecode.com/files/git-$version.tar.gz
  fi
  tar zxf git-$version.tar.gz
  cd git-$version
  ./configure --prefix=$DEPLOYDIR
  make -j$NUMCPU
  make install
}

build_cmake()
{
  version=$1
  echo "Building cmake" $version "..."
  cd $BASEDIR/src
  rm -rf cmake-$version
  if [ ! -f cmake-$version.tar.gz ]; then
    curl -O http://www.cmake.org/files/v2.8/cmake-$version.tar.gz
  fi
  tar zxf cmake-$version.tar.gz
  cd cmake-$version
  mkdir build
  cd build
  ../configure --prefix=$DEPLOYDIR
  make -j$NUMCPU
  make install
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
  make -j$NUMCPU
  make install
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
    curl -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version
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
	if [ $CXX ]; then
		if [ $CXX = "clang++" ]; then
		  ./b2 -j$NUMCPU toolset=clang install
		  # ./b2 -j$NUMCPU toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" install
		fi
	else
	  ./b2 -j$NUMCPU
	  ./b2 install
	fi
}

build_cgal()
{
  version=$1
  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.gz ]; then
    #4.0.2
    curl -O https://gforge.inria.fr/frs/download.php/31174/CGAL-$version.tar.bz2
    # 4.0 curl -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar jxf CGAL-$version.tar.bz2
  cd CGAL-$version
  if [ $2 = use-sys-libs ]; then
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DCMAKE_BUILD_TYPE=Debug
  else
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.so -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.so -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.so -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_ROOT=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Debug
  fi
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

  # Fedora 64-bit
	if [ -e /usr/lib64 ]; then
	  if [ "`ls /usr/lib64 | grep Xmu`" ]; then
	    echo "modifying glew makefile for 64 bit machine"
	    sed -ibak s/"\-lXmu"/"\-L\/usr\/lib64\/libXmu.so.6"/ config/Makefile.linux
	  fi
	fi

	if [ $CC ]; then
		if [ $CC = "clang" ]; then
			echo "modifying glew makefile for clang"
			sed -i s/\$\(CC\)/clang/ Makefile
		fi
	fi

	GLEW_DEST=$DEPLOYDIR make -j$NUMCPU
  GLEW_DEST=$DEPLOYDIR make install
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
  sed -ibak s/example// opencsg.pro # examples might be broken without GLUT

  # Fedora 64-bit
	if [ -e /usr/lib64 ]; then
	  if [ "`ls /usr/lib64 | grep Xmu`" ]; then
	    echo "modifying opencsg makefile for 64 bit machine"
	    sed -ibak s/"\-lXmu"/"\-L\/usr\/lib64\/libXmu.so.6"/ src/Makefile 
	  fi
	fi

  if [ `uname | grep FreeBSD` ]; then
    sed -ibak s/X11R6/local/g src/Makefile
   fi

  if [ "`command -v qmake-qt4`" ]; then
    OPENCSG_QMAKE=qmake-qt4
  else
    OPENCSG_QMAKE=qmake
  fi

	if [ $CXX ]; then
		if [ $CXX = "clang++" ]; then
		  cd $BASEDIR/src/OpenCSG-$version/src
			$OPENCSG_QMAKE
		  cd $BASEDIR/src/OpenCSG-$version
			$OPENCSG_QMAKE
		fi
	else
		$OPENCSG_QMAKE
	fi

  make

  cp -av lib/* $DEPLOYDIR/lib
  cp -av include/* $DEPLOYDIR/include
  cd $OPENSCADDIR
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
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR ..
  make -j$NUMCPU
  make install
}


OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

. ./scripts/setenv-linbuild.sh # '.' is equivalent to 'source'
SRCDIR=$BASEDIR/src

if [ ! $NUMCPU ]; then
	echo "Note: The NUMCPU environment variable can be set for paralell builds"
	NUMCPU=1 
fi

if [ ! -d $BASEDIR/bin ]; then
  mkdir -p $BASEDIR/bin
fi

echo "Using basedir:" $BASEDIR
echo "Using deploydir:" $DEPLOYDIR
echo "Using srcdir:" $SRCDIR
echo "Number of CPUs for parallel builds:" $NUMCPU
mkdir -p $SRCDIR $DEPLOYDIR

if [ ! "`command -v curl`" ]; then
	build_curl 7.26.0
fi

# NB! For cmake, also update the actual download URL in the function
if [ ! "`command -v cmake`" ]; then
	build_cmake 2.8.8
fi
if [ "`cmake --version | grep 'version 2.[1-6][^0-9]'`" ]; then
	build_cmake 2.8.8
fi

# build_git 1.7.10.3

# Singly build CGAL or OpenCSG
# (Most systems have all libraries available as packages except CGAL/OpenCSG)
# (They can be built singly here by passing a command line arg to the script)
if [ $1 ]; then
  if [ $1 = "cgal-use-sys-libs" ]; then
    build_cgal 4.0.2 use-sys-libs
    exit
  fi
  if [ $1 = "opencsg" ]; then
    build_opencsg 1.3.2
    exit
  fi
fi


#
# Main build of libraries
# edit version numbers here as needed.
#

build_eigen 3.1.1
build_gmp 5.0.5
build_mpfr 3.1.1
build_boost 1.47.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.0.2
build_glew 1.7.0
build_opencsg 1.3.2

echo "OpenSCAD dependencies built and installed to " $BASEDIR
