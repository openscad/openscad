 #!/bin/sh -e

# uni-build-dependencies by don bright 2012. copyright assigned to
# Marius Kintel and Clifford Wolf, 2012. released under the GPL 2, or
# later, as described in the file named 'COPYING' in OpenSCAD's project root.

# This script builds most dependencies, both libraries and binary tools,
# of OpenSCAD for Linux/BSD. It is based on macosx-build-dependencies.sh
#
# By default it builds under $HOME/openscad_deps. You can alter this by
# setting the BASEDIR environment variable or with the 'out of tree'
# feature
#
# Usage:
#   cd openscad
#   . ./scripts/setenv-unibuild.sh
#   ./scripts/uni-build-dependencies.sh
#
# Out-of-tree usage:
#
#   cd somepath
#   . /path/to/openscad/scripts/setenv-unibuild.sh
#   /path/to/openscad/scripts/uni-build-dependencies.sh
#
# Prerequisites:
# - wget or curl
# - Qt4
# - gcc
#
# Enable Clang (experimental, only works on linux):
#
#   . ./scripts/setenv-unibuild.sh clang
#

printUsage()
{
  echo "Usage: $0"
  echo
}

build_bison()
{
  version=$1
  echo "Building bison" $version
  cd $BASEDIR/src
  rm -rf bison-$version
  if [ ! -f bison-$version.tar.gz ]; then
    curl --insecure -O http://ftp.gnu.org/gnu/bison/bison-$version.tar.gz
  fi
  tar zxf bison-$version.tar.gz
  cd bison-$version
  ./configure --prefix=$DEPLOYDIR
  make -j$NUMCPU
  make install
}

build_git()
{
  version=$1
  echo "Building git" $version "..."
  cd $BASEDIR/src
  rm -rf git-$version
  if [ ! -f git-$version.tar.gz ]; then
    curl --insecure -O http://git-core.googlecode.com/files/git-$version.tar.gz
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
    curl --insecure -O http://www.cmake.org/files/v2.8/cmake-$version.tar.gz
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
  if [ -e $DEPLOYDIR/include/gmp.h ]; then
    echo "gmp already installed. not building"
    return
  fi
  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp-$version
  if [ ! -f gmp-$version.tar.bz2 ]; then
    curl --insecure -O ftp://ftp.gmplib.org/pub/gmp-$version/gmp-$version.tar.bz2
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
  if [ -e $DEPLOYDIR/include/mpfr.h ]; then
    echo "mpfr already installed. not building"
    return
  fi
  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl --insecure -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
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
  if [ -e $DEPLOYDIR/include/boost ]; then
    echo "boost already installed. not building"
    return
  fi
  version=$1
  bversion=`echo $version | tr "." "_"`
  echo "Building boost" $version "..."
  cd $BASEDIR/src
  rm -rf boost_$bversion
  if [ ! -f boost_$bversion.tar.bz2 ]; then
    curl --insecure -LO http://downloads.sourceforge.net/project/boost/boost/$version/boost_$bversion.tar.bz2
  fi
  tar xjf boost_$bversion.tar.bz2
  cd boost_$bversion
  if [ "`gcc --version|grep 4.7`" ]; then
    if [ "`echo $version | grep 1.47`" ]; then
      echo gcc 4.7 incompatible with boost 1.47. edit boost version in $0
      exit
    fi
  fi
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
  if [ -e $DEPLOYDIR/include/CGAL/version.h ]; then
    echo "CGAL already installed. not building"
    return
  fi
  version=$1
  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.* ]; then
    #4.0.2
    curl --insecure -O https://gforge.inria.fr/frs/download.php/31174/CGAL-$version.tar.bz2
    # 4.0 curl --insecure -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl --insecure -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl --insecure -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl --insecure -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar jxf CGAL-$version.tar.bz2
  cd CGAL-$version
  if [ "`echo $2 | grep use-sys-libs`" ]; then
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DCMAKE_BUILD_TYPE=Debug
  else
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.so -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.so -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.so -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_ROOT=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Debug
  fi
  make -j$NUMCPU
  make install
}

build_glew()
{
  if [ -e $DEPLOYDIR/include/GL/glew.h ]; then
    echo "glew already installed. not building"
    return
  fi
  version=$1
  echo "Building GLEW" $version "..."
  cd $BASEDIR/src
  rm -rf glew-$version
  if [ ! -f glew-$version.tgz ]; then
    curl --insecure -LO http://downloads.sourceforge.net/project/glew/glew/$version/glew-$version.tgz
  fi
  tar xzf glew-$version.tgz
  cd glew-$version
  mkdir -p $DEPLOYDIR/lib/pkgconfig

  # Glew's makefile is not built for Linux Multiarch. We aren't trying
  # to fix everything here, just the test machines OScad normally runs on

  # Fedora 64-bit
  if [ "`uname -m | grep 64`" ]; then
    if [ -e /usr/lib64/libXmu.so.6 ]; then
      sed -ibak s/"\-lXmu"/"\-L\/usr\/lib64\/libXmu.so.6"/ config/Makefile.linux
    fi
  fi

  # debian hurd i386
  if [ "`uname -m | grep 386`" ]; then
    if [ -e /usr/lib/i386-gnu/libXi.so.6 ]; then
      sed -ibak s/"-lXi"/"\-L\/usr\/lib\/i386-gnu\/libXi.so.6"/ config/Makefile.gnu
    fi
  fi

  # clang linux
  if [ $CC ]; then
    sed -ibak s/"CC = cc"/"# CC = cc"/ config/Makefile.linux
  fi

  MAKER=make
  if [ "`uname | grep BSD`" ]; then
    if [ "`command -v gmake`" ]; then
      MAKER=gmake
    else
      echo "building glew on BSD requires gmake (gnu make)"
      exit
    fi
  fi

  GLEW_DEST=$DEPLOYDIR $MAKER -j$NUMCPU
  GLEW_DEST=$DEPLOYDIR $MAKER install
}

build_opencsg()
{
  if [ -e $DEPLOYDIR/include/opencsg.h ]; then
    echo "OpenCSG already installed. not building"
    return
  fi
  version=$1
  echo "Building OpenCSG" $version "..."
  cd $BASEDIR/src
  rm -rf OpenCSG-$version
  if [ ! -f OpenCSG-$version.tar.gz ]; then
    curl --insecure -O http://www.opencsg.org/OpenCSG-$version.tar.gz
  fi
  tar xzf OpenCSG-$version.tar.gz
  cd OpenCSG-$version

  # modify the .pro file for qmake, then use qmake to
  # manually rebuild the src/Makefile (some systems don't auto-rebuild it)

  cp opencsg.pro opencsg.pro.bak
  cat opencsg.pro.bak | sed s/example// > opencsg.pro

  if [ "`command -v qmake-qt4`" ]; then
    OPENCSG_QMAKE=qmake-qt4
  elif [ "`command -v qmake4`" ]; then
    OPENCSG_QMAKE=qmake4
  else
    OPENCSG_QMAKE=qmake
  fi

  cd $BASEDIR/src/OpenCSG-$version/src
  $OPENCSG_QMAKE

  cd $BASEDIR/src/OpenCSG-$version
  $OPENCSG_QMAKE

  make

  ls lib/* include/*
  if [ -e lib/.libs ]; then ls lib/.libs/*; fi # netbsd
  echo "installing to -->" $DEPLOYDIR
  mkdir -p $DEPLOYDIR/lib
  mkdir -p $DEPLOYDIR/include
  install lib/* $DEPLOYDIR/lib
  install include/* $DEPLOYDIR/include
  if [ -e lib/.libs ]; then install lib/.libs/* $DEPLOYDIR/lib; fi #netbsd

  cd $BASEDIR
}

build_eigen()
{
  version=$1
  if [ -e $DEPLOYDIR/include/eigen2 ]; then
    if [ `echo $version | grep 2....` ]; then
      echo "Eigen2 already installed. not building"
      return
    fi
  fi
  if [ -e $DEPLOYDIR/include/eigen3 ]; then
    if [ `echo $version | grep 3....` ]; then
      echo "Eigen3 already installed. not building"
      return
    fi
  fi
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
    curl --insecure -LO http://bitbucket.org/eigen/eigen/get/$version.tar.bz2
    mv $version.tar.bz2 eigen-$version.tar.bz2
  fi
  tar xjf eigen-$version.tar.bz2
  ln -s ./$EIGENDIR eigen-$version
  cd eigen-$version
  mkdir build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_TEST_NO_OPENGL=1 ..
  make -j$NUMCPU
  make install
}


# this section allows 'out of tree' builds, as long as the system has
# the 'dirname' command installed

if [ "`command -v dirname`" ]; then
  OPENSCAD_SCRIPTDIR=`dirname $0`
else
  if [ ! -f openscad.pro ]; then
    echo "Must be run from the OpenSCAD source root directory (dont have 'dirname')"
    exit 1
  else
    OPENSCAD_SCRIPTDIR=$PWD
  fi
fi

. $OPENSCAD_SCRIPTDIR/setenv-unibuild.sh # '.' is equivalent to 'source'
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

# this section builds some basic tools, if they are missing or outdated
# they are installed under $BASEDIR/bin which we have added to our PATH

if [ ! "`command -v curl`" ]; then
  build_curl 7.26.0
fi

if [ ! "`command -v bison`" ]; then
  build_bison 2.6.1
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
  if [ $1 = "cgal" ]; then
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
build_boost 1.49.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.0.2
build_glew 1.9.0
build_opencsg 1.3.2

echo "OpenSCAD dependencies built and installed to " $BASEDIR
