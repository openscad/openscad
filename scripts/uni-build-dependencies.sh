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
# - OpenGL (GL/gl.h)
# - GLU (GL/glu.h)
# - gcc
# - Qt4
#
# If your system lacks qt4, build like this:
#
#   ./scripts/uni-build-dependencies.sh qt4
#   . ./scripts/setenv-unibuild.sh #(Rerun to re-detect qt4)
#
# If your system lacks glu, gettext, or glib2, you can build them as well:
#
#   ./scripts/uni-build-dependencies.sh glu
#   ./scripts/uni-build-dependencies.sh glib2
#   ./scripts/uni-build-dependencies.sh gettext
#
# If you want to try Clang compiler (experimental, only works on linux):
#
#   . ./scripts/setenv-unibuild.sh clang
#
# If you want to try Qt5 (experimental)
#
#   . ./scripts/setenv-unibuild.sh qt5
#

printUsage()
{
  echo "Usage: $0"
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
}

build_glew()
{
  GLEW_INSTALLED=
  if [ -e $DEPLOYDIR/lib64/libGLEW.so ]; then
    GLEW_INSTALLED=1
  fi
  if [ -e $DEPLOYDIR/lib/libGLEW.so ]; then
    GLEW_INSTALLED=1
  fi
  if [ $GLEW_INSTALLED ]; then
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
  if [ -e $DEPLOYDIR/lib/libopencsg.so ]; then
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

  detect_glu
  GLU_INCLUDE=$detect_glu_include
  if [ ! $detect_glu_result ]; then
    build_glu 9.0.0
  fi

  if [ "`command -v qmake-qt4`" ]; then
    OPENCSG_QMAKE=qmake-qt4
  elif [ "`command -v qmake4`" ]; then
    OPENCSG_QMAKE=qmake4
  elif [ "`command -v qmake-qt5`" ]; then
    OPENCSG_QMAKE=qmake-qt5
  elif [ "`command -v qmake5`" ]; then
    OPENCSG_QMAKE=qmake5
  elif [ "`command -v qmake`" ]; then
    OPENCSG_QMAKE=qmake
  else
    echo qmake not found... using standard OpenCSG makefiles
    OPENCSG_QMAKE=make
    cp Makefile Makefile.bak
    cp src/Makefile src/Makefile.bak

    cat Makefile.bak | sed s/example// |sed s/glew// > Makefile
    cat src/Makefile.bak | sed s@^INCPATH.*@INCPATH\ =\ -I$BASEDIR/include\ -I../include\ -I..\ -I$GLU_INCLUDE -I.@ > src/Makefile
    cp src/Makefile src/Makefile.bak2
    cat src/Makefile.bak2 | sed s@^LIBS.*@LIBS\ =\ -L$BASEDIR/lib\ -L/usr/X11R6/lib\ -lGLU\ -lGL@ > src/Makefile
    tmp=$version
    version=$tmp
  fi

  if [ ! $OPENCSG_QMAKE = "make" ]; then
    OPENCSG_QMAKE=$OPENCSG_QMAKE' "QMAKE_CXXFLAGS+=-I'$GLU_INCLUDE'"'
  fi
  echo OPENCSG_QMAKE: $OPENCSG_QMAKE

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
  if [ $version = "3.2.2" ]; then EIGENDIR=eigen-eigen-1306d75b4a21; fi
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


# glib2 and dependencies

#build_gettext()
#{
#  version=$1
#  ls -l $DEPLOYDIR/include/gettext-po.h
#  if [ -e $DEPLOYDIR/include/gettext-po.h ]; then
#    echo "gettext already installed. not building"
#    return
#  fi
#
#  echo "Building gettext $version..."
#
#  cd "$BASEDIR"/src
#  rm -rf "gettext-$version"
#  if [ ! -f "glib-$version.tar.gz" ]; then
#    curl --insecure -LO "http://ftpmirror.gnu.org/gettext/gettext-$version.tar.gz"
#  fi
#  tar xzf "gettext-$version.tar.gz"
#  cd "gettext-$version"
#
#  ./configure --prefix="$DEPLOYDIR"
#  make -j$NUMCPU
#  make install
#}

build_pkgconfig()
{
  if [ "`command -v pkg-config`" ]; then
    echo "pkg-config already installed. not building"
    return
  fi
  version=$1
  echo "Building pkg-config $version..."

  cd "$BASEDIR"/src
  rm -rf "pkg-config-$version"
  if [ ! -f "pkg-config-$version.tar.gz" ]; then
    curl --insecure -LO "http://pkgconfig.freedesktop.org/releases/pkg-config-$version.tar.gz"
  fi
  tar xzf "pkg-config-$version.tar.gz"
  cd "pkg-config-$version"

  ./configure --prefix="$DEPLOYDIR" --with-internal-glib
  make -j$NUMCPU
  make install
}

build_libffi()
{
  if [ -e $DEPLOYDIR/include/ffi.h ]; then
    echo "libffi already installed. not building"
    return
  fi
  version=$1
  echo "Building libffi $version..."

  cd "$BASEDIR"/src
  rm -rf "libffi-$version"
  if [ ! -f "libffi-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://sourceware.org/pub/libffi/libffi-$version.tar.gz"
    curl --insecure -LO "http://www.linuxfromscratch.org/patches/blfs/svn/libffi-$version-includedir-1.patch"
  fi
  tar xzf "libffi-$version.tar.gz"
  cd "libffi-$version"
  if [ ! "`command -v patch`" ]; then
    echo cannot proceed, need 'patch' program
    exit 1
  fi
  patch -Np1 -i ../libffi-3.0.13-includedir-1.patch
  ./configure --prefix="$DEPLOYDIR"
  make -j$NUMCPU
  make install
}

#build_glib2()
#{
#  version="$1"
#  maj_min_version="${version%.*}" #Drop micro#
#
#  if [ -e $DEPLOYDIR/lib/glib-2.0 ]; then
#    echo "glib2 already installed. not building"
#    return
#  fi
#
# echo "Building glib2 $version..."
#  cd "$BASEDIR"/src
#  rm -rf "glib-$version"
#  if [ ! -f "glib-$version.tar.xz" ]; then
#    curl --insecure -LO "http://ftp.gnome.org/pub/gnome/sources/glib/$maj_min_version/glib-$version.tar.xz"
#  fi
#  tar xJf "glib-$version.tar.xz"
#  cd "glib-$version"

#  ./configure --disable-gtk-doc --disable-man --prefix="$DEPLOYDIR" CFLAGS="-I$DEPLOYDIR/include" LDFLAGS="-L$DEPLOYDIR/lib"
#  make -j$NUMCPU
#  make install
#}

## end of glib2 stuff

# this section allows 'out of tree' builds, as long as the system has
# the 'dirname' command installed

if [ "`command -v dirname`" ]; then
  RUNDIR=$PWD
  OPENSCAD_SCRIPTDIR=`dirname $0`
  cd $OPENSCAD_SCRIPTDIR
  OPENSCAD_SCRIPTDIR=$PWD
  cd $RUNDIR
else
  if [ ! -f openscad.pro ]; then
    echo "Must be run from the OpenSCAD source root directory (dont have 'dirname')"
    exit 1
  else
    OPENSCAD_SCRIPTDIR=$PWD
  fi
fi

check_env

. $OPENSCAD_SCRIPTDIR/setenv-unibuild.sh # '.' is equivalent to 'source'
. $OPENSCAD_SCRIPTDIR/common-build-dependencies.sh
SRCDIR=$BASEDIR/src

if [ ! $NUMCPU ]; then
  echo "Note: The NUMCPU environment variable can be set for parallel builds"
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
  # to prevent "end of file" NSS error -5938 (ssl) use a newer version of curl
  build_curl 7.49.0
fi

if [ ! "`command -v bison`" ]; then
  build_bison 2.6.1
fi

# NB! For cmake, also update the actual download URL in the function
if [ ! "`command -v cmake`" ]; then
  build_cmake 2.8.8
fi
# see README for needed version (this should match 1<minimum)
if [ "`cmake --version | grep 'version 2.[1-8][^0-9][1-4] '`" ]; then
  build_cmake 2.8.8
fi

# Singly build certain tools or libraries
if [ $1 ]; then
  if [ $1 = "git" ]; then
    build_git 1.7.10.3
    exit $?
  fi
  if [ $1 = "cgal" ]; then
    build_cgal 4.4 use-sys-libs
    exit $?
  fi
  if [ $1 = "opencsg" ]; then
    build_opencsg 1.3.2
    exit $?
  fi
  if [ $1 = "qt4" ]; then
    # such a huge build, put here by itself
    build_qt4 4.8.4
    exit $?
  fi
  if [ $1 = "qt5scintilla2" ]; then
    build_qt5scintilla2 2.8.3
    exit $?
  fi
  if [ $1 = "qt5" ]; then
    build_qt5 5.3.1
    build_qt5scintilla2 2.8.3
    exit $?
  fi
  if [ $1 = "glu" ]; then
    # Mesa and GLU split in late 2012, so it's not on some systems
    build_glu 9.0.0
    exit $?
  fi
  if [ $1 = "gettext" ]; then
    # such a huge build, put here by itself
    build_gettext 0.18.3.1
    exit $?
  fi
  if [ $1 = "harfbuzz" ]; then
    # debian 7 lacks only harfbuzz
    build_harfbuzz 0.9.35 --with-glib=yes
    exit $?
  fi
  if [ $1 = "glib2" ]; then
    # such a huge build, put here by itself
    build_pkgconfig 0.28
    build_libffi 3.0.13
    #build_gettext 0.18.3.1
    build_glib2 2.38.2
    exit $?
  fi
fi


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

echo "OpenSCAD dependencies built and installed to " $BASEDIR
