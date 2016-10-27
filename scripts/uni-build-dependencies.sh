#!/bin/sh -e
#
# Build openscad without root access
#
#   . /path/to/openscad/scripts/setenv-unibuild.sh
#   /path/to/openscad/scripts/uni-build-dependencies.sh
#
# Prerequisites:
#
# - see http://linuxbrew.sh/
#

OPENSCADDIR=$PWD

printUsage()
{
  echo "Usage: $0"
<<<<<<< HEAD
  echo
}

check_env()
{
  SLEEP=0
  if [ x != x"$CFLAGS" ]
  then
    echo "*** WARNING: You have CFLAGS set to '$CFLAGS'"
    SLEEP=2
  fi
  if [ x != x"$CXXFLAGS" ]
  then
    echo "*** WARNING: You have CXXFLAGS set to '$CXXFLAGS'"
    SLEEP=2
  fi
  if [ x != x"$LDFLAGS" ]
  then
    echo "*** WARNING: You have LDFLAGS set to '$LDFLAGS'"
    SLEEP=2
  fi
  [ $SLEEP -gt 0 ] && sleep $SLEEP || true
}

detect_glu()
{
  detect_glu_result=
  if [ -e $DEPLOYDIR/include/GL/glu.h ]; then
    detect_glu_include=$DEPLOYDIR/include
    detect_glu_result=1;
  fi
  if [ -e /usr/include/GL/glu.h ]; then
    detect_glu_include=/usr/include
    detect_glu_result=1;
  fi
  if [ -e /usr/local/include/GL/glu.h ]; then
    detect_glu_include=/usr/local/include
    detect_glu_result=1;
  fi
  if [ -e /usr/pkg/X11R7/include/GL/glu.h ]; then
    detect_glu_include=/usr/pkg/X11R7/include
    detect_glu_result=1;
  fi
  return
}

build_glu()
{
  version=$1
  if [ -e $DEPLOYDIR/lib/libGLU.so ]; then
    echo "GLU already installed. not building"
    return
  fi
  echo "Building GLU" $version "..."
  cd $BASEDIR/src
  rm -rf glu-$version
  if [ ! -f glu-$version.tar.gz ]; then
    curl -O http://cgit.freedesktop.org/mesa/glu/snapshot/glu-$version.tar.gz
  fi
  tar xzf glu-$version.tar.gz
  cd glu-$version
  ./autogen.sh --prefix=$DEPLOYDIR
  make -j$NUMCPU
  make install
}

build_qt4()
{
  version=$1
  if [ -e $DEPLOYDIR/include/Qt ]; then
    echo "qt already installed. not building"
    return
  fi
  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
    curl -O http://releases.qt-project.org/qt4/source/qt-everywhere-opensource-src-$version.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  ./configure -prefix $DEPLOYDIR -opensource -confirm-license -fast -no-qt3support -no-svg -no-phonon -no-audio-backend -no-multimedia -no-javascript-jit -no-script -no-scripttools -no-declarative -no-xmlpatterns -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit
  make -j$NUMCPU
  make install
  QTDIR=$DEPLOYDIR
  export QTDIR
  echo "----------"
  echo " Please set QTDIR to $DEPLOYDIR ( or run '. scripts/setenv-unibuild.sh' )"
  echo "----------"
}

build_qt5()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libQt5Core.a ]; then
    echo "Qt5 already installed. not building"
    return
  fi

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  rm -rf qt-everywhere-opensource-src-$version
  v=`echo "$version" | sed -e 's/\.[0-9]$//'`
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
     curl -O -L http://download.qt-project.org/official_releases/qt/$v/$version/single/qt-everywhere-opensource-src-$version.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  ./configure -prefix $DEPLOYDIR -release -static -opensource -confirm-license \
                -nomake examples -nomake tests \
                -qt-xcb -no-c++11 -no-glib -no-harfbuzz -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc \
                -no-sql-psql -no-sql-sqlite2 -no-sql-tds -no-cups -no-qml-debug \
                -skip activeqt -skip connectivity -skip declarative -skip doc \
                -skip enginio -skip graphicaleffects -skip location -skip multimedia \
                -skip quick1 -skip quickcontrols -skip script -skip sensors -skip serialport \
                -skip svg -skip webkit -skip webkit-examples -skip websockets -skip xmlpatterns
  make -j"$NUMCPU" install
}

build_qt5scintilla2()
{
  version=$1

  if [ -d $DEPLOYDIR/lib/libqt5scintilla2.a ]; then
    echo "Qt5Scintilla2 already installed. not building"
    return
  fi

  echo "Building Qt5Scintilla2" $version "..."
  cd $BASEDIR/src
  rm -rf ./QScintilla-gpl-$version.tar.gz
  if [ ! -f QScintilla-gpl-$version.tar.gz ]; then
     curl -L -o "QScintilla-gpl-$version.tar.gz" "http://downloads.sourceforge.net/project/pyqt/QScintilla2/QScintilla-$version/QScintilla-gpl-$version.tar.gz?use_mirror=switch"
  fi
  tar xzf QScintilla-gpl-$version.tar.gz
  cd QScintilla-gpl-$version/Qt4Qt5/
  qmake CONFIG+=staticlib
  tmpinstalldir=$DEPLOYDIR/tmp/qsci$version
  INSTALL_ROOT=$tmpinstalldir make -j"$NUMCPU" install

  if [ -d $tmpinstalldir/usr/share ]; then
    cp -av $tmpinstalldir/usr/share $DEPLOYDIR/
    cp -av $tmpinstalldir/usr/include $DEPLOYDIR/
    cp -av $tmpinstalldir/usr/lib $DEPLOYDIR/
  fi

  if [ ! -e $DEPLOYDIR/include/Qsci ]; then
    # workaround numerous bugs in qscintilla build system, see 
    # ../qscintilla2.prf and ../scintilla.pri for more info
    qsci_staticlib=`find $tmpinstalldir -name libqscintilla2.a`
    qsci_include=`find $tmpinstalldir -name Qsci`
    if [ -e $qsci_staticlib ]; then
      cp -av $qsci_include $DEPLOYDIR/include/
      cp -av $qsci_staticlib $DEPLOYDIR/lib/
    else
      echo problems finding built qscintilla libraries and include headers
    fi
    if [ -e $DEPLOYDIR/lib/libqscintilla2.a ]; then
      cp $DEPLOYDIR/lib/libqscintilla2.a $DEPLOYDIR/lib/libqt5scintilla2.a
    fi
  fi
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
    curl --insecure -O https://gmplib.org/download/gmp/gmp-$version.tar.bz2
  fi
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  mkdir build
  cd build
  ../configure --prefix=$DEPLOYDIR --enable-cxx
  make -j$NUMCPU
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
  make -j$NUMCPU
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
  if [ ! $? -eq 0 ]; then
    echo download failed. 
    exit 1
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
  if [ -e ./bootstrap.sh ]; then
    BSTRAPBIN=./bootstrap.sh
  else
    BSTRAPBIN=./configure
  fi
  $BSTRAPBIN --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,system,regex
	if [ -e ./b2 ]; then
    BJAMBIN=./b2;
  elif [ -e ./bjam ]; then
    BJAMBIN=./bjam
  elif [ -e ./Makefile ]; then
    BJAMBIN=make
  fi
  if [ $CXX ]; then
    if [ $CXX = "clang++" ]; then
      $BJAMBIN -j$NUMCPU toolset=clang
    fi
  else
    $BJAMBIN -j$NUMCPU
  fi
  if [ $? = 0 ]; then
    $BJAMBIN install
  else
    echo boost build failed
    exit 1
  fi
  if [ "`ls $DEPLOYDIR/include/ | grep boost.[0-9]`" ]; then
    if [ ! -e $DEPLOYDIR/include/boost ]; then
      echo "boost is old, make a symlink to $DEPLOYDIR/include/boost & rerun"
      exit 1
    fi
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
  ver4_8="curl -L --insecure -O https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-4.8/CGAL-4.8.tar.xz"
  ver4_7="curl -L --insecure -O https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-4.7/CGAL-4.7.tar.gz"
  ver4_4="curl --insecure -O https://gforge.inria.fr/frs/download.php/file/33524/CGAL-4.4.tar.bz2"
  ver4_2="curl --insecure -O https://gforge.inria.fr/frs/download.php/32360/CGAL-4.2.tar.bz2"
  ver4_1="curl --insecure -O https://gforge.inria.fr/frs/download.php/31640/CGAL-4.1.tar.bz2"
  ver4_0_2="curl --insecure -O https://gforge.inria.fr/frs/download.php/31174/CGAL-4.0.2.tar.bz2"
  ver4_0="curl --insecure -O https://gforge.inria.fr/frs/download.php/30387/CGAL-4.0.tar.gz"
  ver3_9="curl --insecure -O https://gforge.inria.fr/frs/download.php/29125/CGAL-3.9.tar.gz"
  ver3_8="curl --insecure -O https://gforge.inria.fr/frs/download.php/28500/CGAL-3.8.tar.gz"
  ver3_7="curl --insecure -O https://gforge.inria.fr/frs/download.php/27641/CGAL-3.7.tar.gz"
  vernull="echo already downloaded..skipping"
  download_cmd=ver`echo $version | sed s/"\."/"_"/ | sed s/"\."/"_"/`

  if [ -e CGAL-$version.tar.gz ]; then
    download_cmd=vernull;
  fi
  if [ -e CGAL-$version.tar.bz2 ]; then
    download_cmd=vernull;
  fi
  if [ -e CGAL-$version.tar.xz ]; then
    download_cmd=vernull;
  fi

  eval echo "$"$download_cmd
  `eval echo "$"$download_cmd`

  zipper=gzip
  suffix=gz
  if [ -e CGAL-$version.tar.bz2 ]; then
    zipper=bzip2
    suffix=bz2
  fi
  if [ -e CGAL-$version.tar.xz ]; then
    zipper=xz
    suffix=xz
  fi

  $zipper -f -d CGAL-$version.tar.$suffix;
  tar xf CGAL-$version.tar

  cd CGAL-$version

  # older cmakes have buggy FindBoost that can result in
  # finding the system libraries but OPENSCAD_LIBRARIES include paths
  # NB! This was removed 2015-12-02 - if this problem resurfaces, fix it only for the relevant platforms as this
  # messes up more recent installations of cmake and CGAL.
  # FINDBOOST_CMAKE=$OPENSCAD_SCRIPTDIR/../tests/FindBoost.cmake
  # cp $FINDBOOST_CMAKE ./cmake/modules/

  mkdir bin
  cd bin
  rm -rf ./*
  if [ "`uname -a| grep ppc64`" ]; then
    CGAL_BUILDTYPE="Release" # avoid assertion violation
  else
    CGAL_BUILDTYPE="Debug"
  fi

  DEBUGBOOSTFIND=0 # for debugging FindBoost.cmake (not for debugging boost)
  Boost_NO_SYSTEM_PATHS=1
  if [ "`echo $2 | grep use-sys-libs`" ]; then
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND ..
  else
    cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.so -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.so -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.so -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_LIBRARYDIR=$DEPLOYDIR/lib -DBOOST_INCLUDEDIR=$DEPLOYDIR/include -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND -DBoost_NO_SYSTEM_PATHS=1 ..
  fi
  make -j$NUMCPU
  make install
=======
>>>>>>> fbsdbuild
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi

if [ ! "`uname -m`|grep x86_64" ]; then
  echo "requires x86_64 bit cpu sorry, please see linuxbrew.sh"
fi

. ./scripts/setenv-unibuild.sh

cd $HOME

brewurl=https://raw.githubusercontent.com/Linuxbrew/install/master/install
if [ ! -e ~/.linuxbrew ]; then
  ruby -e "$(curl -fsSL "$brewurl")"
fi

brew update
pkgs='eigen boost cgal glew glib opencsg freetype libxml2 fontconfig'
pkgs=$pkgs' harfbuzz qt5 qscintilla2 imagemagick'
for formula in $pkgs; do
  brew install $formula
  brew outdated $formula || brew upgrade $formula
done
brew link --force gettext
brew link --force qt5
brew link --force qscintilla2

<<<<<<< HEAD
# todo - cgal 4.02 for gcc<4.7, gcc 4.2 for above

#
# Main build of libraries
# edit version numbers here as needed.
# This is only for libraries most systems won't have new enough versions of.
# For big things like Qt4, see the notes at the head of this file on
# building individual dependencies.
# 
# Some of these are defined in scripts/common-build-dependencies.sh

build_eigen 3.2.2
build_gmp 6.0.0
build_mpfr 3.1.1
build_boost 1.56.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.7
build_glew 1.9.0
build_opencsg 1.3.2
build_gettext 0.18.3.1
build_glib2 2.38.2

# the following are only needed for text()
build_freetype 2.6.1 --without-png
build_libxml2 2.9.1
build_fontconfig 2.11.0 --with-add-fonts=/usr/X11R6/lib/X11/fonts,/usr/local/share/fonts
build_ragel 6.9
build_harfbuzz 0.9.35 --with-glib=yes
=======
>>>>>>> fbsdbuild

