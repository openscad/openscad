#!/bin/sh -e

# emcc-build-dependencies
# based on uni-build-dependencies by don bright 2012. copyright assigned to
# Marius Kintel and Clifford Wolf, 2012. released under the GPL 2, or
# later, as described in the file named 'COPYING' in OpenSCAD's project root.

# This script builds most dependencies, both libraries and binary tools,
# of OpenSCAD for emcscripten on Linux/BSD. It is based on macosx-build-dependencies.sh
#
# By default it builds under $HOME/openscad_deps. You can alter this by
# setting the BASEDIR environment variable or with the 'out of tree'
# feature
#
# Usage:
#   cd openscad
#   . ./scripts/setenv-emscriptenbuild.sh
#   ./scripts/emcc-build-dependencies.sh
#
# Out-of-tree usage:
#
#   cd somepath
#   . /path/to/openscad/scripts/setenv-emscriptenbuild.sh
#   /path/to/openscad/scripts/emcc-build-dependencies.sh
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
#   ./scripts/uemcc-build-dependencies.sh qt4
#   . ./scripts/setenv-unibuild.sh #(Rerun to re-detect qt4)
#
# If your system lacks glu, gettext, or glib2, you can build them as well:
#
#   ./scripts/emcc-build-dependencies.sh glu
#   ./scripts/emcc-build-dependencies.sh glib2
#   ./scripts/emcc-build-dependencies.sh gettext
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
  if [ -e $DEPLOYDIR/include/GL/glu.h ]; then detect_glu_result=1; fi
  if [ -e /usr/include/GL/glu.h ]; then detect_glu_result=1; fi
  if [ -e /usr/local/include/GL/glu.h ]; then detect_glu_result=1; fi
  if [ -e /usr/pkg/X11R7/include/GL/glu.h ]; then detect_glu_result=1; fi
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
  emconfigure ./autogen.sh --prefix=$DEPLOYDIR
  emmake make -j$NUMCPU
  emmake make install
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
  emconfigure ./configure -prefix $DEPLOYDIR -opensource -confirm-license -fast -no-qt3support -no-svg -no-phonon -no-audio-backend -no-multimedia -no-javascript-jit -no-script -no-scripttools -no-declarative -no-xmlpatterns -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit
  emmake make -j$NUMCPU
  emmake make install
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
  emconfigure ./configure -prefix $DEPLOYDIR -release -static -opensource -confirm-license \
                -nomake examples -nomake tests \
                -qt-xcb -no-c++11 -no-glib -no-harfbuzz -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc \
                -no-sql-psql -no-sql-sqlite2 -no-sql-tds -no-cups -no-qml-debug \
                -skip activeqt -skip connectivity -skip declarative -skip doc \
                -skip enginio -skip graphicaleffects -skip location -skip multimedia \
                -skip quick1 -skip quickcontrols -skip script -skip sensors -skip serialport \
                -skip svg -skip webkit -skip webkit-examples -skip websockets -skip xmlpatterns
  emmake make -j"$NUMCPU" install
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
  #rm -rf QScintilla-gpl-$version.tar.gz
  if [ ! -f QScintilla-gpl-$version.tar.gz ]; then
     curl -L -o "QScintilla-gpl-$version.tar.gz" "http://downloads.sourceforge.net/project/pyqt/QScintilla2/QScintilla-$version/QScintilla-gpl-$version.tar.gz?use_mirror=switch"
  fi
  tar xzf QScintilla-gpl-$version.tar.gz
  cd QScintilla-gpl-$version/Qt4Qt5/
  emconfigure qmake CONFIG+=staticlib
  emmake make -j"$NUMCPU" install
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
  emmake make -j$NUMCPU
  emmake make install
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
  emmake make -j$NUMCPU
  emmake make install
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
  emmake make -j$NUMCPU
  emmake make install
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
  emmake make -j$NUMCPU
  emmake make install
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
  EMCONFIGURE_JS=1 emconfigure ../configure --prefix=$DEPLOYDIR --build=none --host=none
  emmake make -j$NUMCPU
  emmake make install
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
  EMCONFIGURE_JS=1 emconfigure ../configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR --build=none --host=none
  emmake make -j$NUMCPU
  emmake make install
  cd ..
}

build_boost()
{
  if [ -e $DEPLOYDIR/include/boost ]; then
    echo "boost already installed. not building"
    return
  fi
  
  # Since boost has to be special and use its own build tools, and those
  # don't know about emscripten, let's just get the cmake boost fork and use that.
  cd $BASEDIR/src
  rm -rf boost
  git clone https://github.com/starseeker/boost.git
  cd boost
  #don't build locale -it fails when ICU isn't found, and it can't dynamically link to libicu anyways.
  mv libs/locale/CMakeLists.txt libs/locale/CMakeLists.ignore
  mkdir build && cd build

#  #old way that works but isn't as versatile ---------
#  emconfigure cmake ../ -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR
#
#  emmake make -j$NUMCPU #might as well build all of them, it's not like the unused ones will get linked in.
#  #if I want to only build the parts we're actually going to use, then do this instead:
#  # emmake make -j$NUMCPU boost_thread boost_program_options boost_filesystem boost_system boost_regex
#
#  #now install it to our $DEPLOYDIR
#  emmake make install #probably could do without emmake, but just to be safe...
#  #End old way -----------

  #This way might be better, since it teaches cmake that we're cross-compiling, and it works:
  cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DENABLE_SINGLE_THREADED=ON ../
  make -j$NUMCPU
  #if I want to only build the parts we're actually going to use, then do this instead:
  #make -j$NUMCPU boost_thread boost_program_options boost_filesystem boost_system boost_regex
  make install

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

  eval echo "$"$download_cmd
  `eval echo "$"$download_cmd`

  zipper=gzip
  suffix=gz
  if [ -e CGAL-$version.tar.bz2 ]; then
    zipper=bzip2
    suffix=bz2
  fi

  $zipper -f -d CGAL-$version.tar.$suffix;
  tar xf CGAL-$version.tar

  cd CGAL-$version

  # older cmakes have buggy FindBoost that can result in
  # finding the system libraries but OPENSCAD_LIBRARIES include paths
  FINDBOOST_CMAKE=$OPENSCAD_SCRIPTDIR/../tests/FindBoost.cmake
  cp $FINDBOOST_CMAKE ./cmake/modules/
  #Also copy in our alternative CGAL_CheckCXXFileRuns.cmake
  cp $OPENSCAD_SCRIPTDIR/../tests/CGAL_CheckCXXFileRuns.cmake ./cmake/modules/
  #and CGAL_Macros.cmake
  cp $OPENSCAD_SCRIPTDIR/../tests/CGAL_Macros.cmake ./cmake/modules

  mkdir bin &&  cd bin &&  rm -rf ./*
  
  if [ "`uname -a| grep ppc64`" ]; then
    CGAL_BUILDTYPE="Release" # avoid assertion violation
  else
    CGAL_BUILDTYPE="Debug"
  fi

  DEBUGBOOSTFIND=1 # for debugging FindBoost.cmake (not for debugging boost)
  Boost_NO_SYSTEM_PATHS=1
  if [ "`echo $2 | grep use-sys-libs`" ]; then
    emconfigure cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND ..
  else
    #Gargh why is it so bad at finding boost?!
    EMCONFIGURE_JS=1 emconfigure cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.a -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.a -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.a -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_LIBRARYDIR=$DEPLOYDIR/lib -DBOOST_INCLUDEDIR=$DEPLOYDIR/include -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND -DBoost_NO_SYSTEM_PATHS=1 ..
    cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.a -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.a -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.a -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_LIBRARYDIR=$DEPLOYDIR/lib -DBOOST_INCLUDEDIR=$DEPLOYDIR/include -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND -DBoost_NO_SYSTEM_PATHS=1 ..
   #EMCONFIGURE_JS=1 emconfigure cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.a -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.a -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.a -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBOOST_LIBRARYDIR=$DEPLOYDIR/lib -DBOOST_INCLUDEDIR=$DEPLOYDIR/include -DCMAKE_BUILD_TYPE=$CGAL_BUILDTYPE -DBoost_DEBUG=$DEBUGBOOSTFIND -DBoost_NO_SYSTEM_PATHS=1 ..
  fi
  emmake make -j$NUMCPU
  emmake make install
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

  MAKER=emmake make
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

  #if [ "`command -v qmake-qt4`" ]; then
  #  OPENCSG_QMAKE=emmake qmake-qt4
  #elif [ "`command -v qmake4`" ]; then
  #  OPENCSG_QMAKE=emmake qmake4
  #elif [ "`command -v qmake`" ]; then
  #  OPENCSG_QMAKE=emmake qmake
  #else
  #  echo qmake not found... using standard OpenCSG makefiles
    echo not using qmake because reasons
    OPENCSG_QMAKE=emmake make
    cp Makefile Makefile.bak
    cp src/Makefile src/Makefile.bak

    cat Makefile.bak | sed s/example// |sed s/glew// > Makefile
    cat src/Makefile.bak | sed s@^INCPATH.*@INCPATH\ =\ -I$BASEDIR/include\ -I../include\ -I..\ -I.@ > src/Makefile
    cp src/Makefile src/Makefile.bak2
    #cat src/Makefile.bak2 | sed s@^LIBS.*@LIBS\ =\ -L$BASEDIR/lib\ -L/usr/X11R6/lib\ -lGLU\ -lGL@ > src/Makefile
    cat src/Makefile.bak2 | sed s@^LIBS.*@LIBS\ =\ -L$BASEDIR/lib\ -lGLU\ -lGL@ > src/Makefile
    tmp=$version
    detect_glu
    if [ ! $detect_glu_result ]; then 
      echo building glu
      build_glu 9.0.0 ; fi
    version=$tmp
  #fi

  cd $BASEDIR/src/OpenCSG-$version/src
  $OPENCSG_QMAKE

  cd $BASEDIR/src/OpenCSG-$version
  $OPENCSG_QMAKE

  emmake make

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
  emmake cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_TEST_NO_OPENGL=1 ..
  emmake make -j$NUMCPU
  emmake make install
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

  emconfigure ./configure --prefix="$DEPLOYDIR" --with-internal-glib
  emmake make -j$NUMCPU
  emmake make install
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
  emconfigure ./configure --prefix="$DEPLOYDIR"
  emmake make -j$NUMCPU
  emmake make install
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

. $OPENSCAD_SCRIPTDIR/setenv-emscriptenbuild.sh # '.' is equivalent to 'source'
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
  build_curl 7.26.0
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
##
#
#
#
#
#
#Moved these from common-build-dependencies temporarily so that we can test building each one.
build_freetype()
{
  version="$1"
  extra_config_flags="$2"

  if [ -e "$DEPLOYDIR/include/freetype2" ]; then
    echo "freetype already installed. not building"
    return
  fi

  echo "Building freetype $version..."
  cd "$BASEDIR"/src
  rm -rf "freetype-$version"
  if [ ! -f "freetype-$version.tar.gz" ]; then
    curl --insecure -LO "http://download.savannah.gnu.org/releases/freetype/freetype-$version.tar.gz"
  fi
  tar xzf "freetype-$version.tar.gz"
  cd "freetype-$version"
  emconfigure ./configure --prefix="$DEPLOYDIR" $extra_config_flags
  emmake make -j"$NUMCPU"
  emmake make install
}
 
build_libxml2()
{
  version="$1"

  if [ -e $DEPLOYDIR/include/libxml2 ]; then
    echo "libxml2 already installed. not building"
    return
  fi

  echo "Building libxml2 $version..."
  cd "$BASEDIR"/src
  rm -rf "libxml2-$version"
  if [ ! -f "libxml2-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://xmlsoft.org/libxml2/libxml2-$version.tar.gz"
  fi
  tar xzf "libxml2-$version.tar.gz"
  cd "libxml2-$version"
  emconfigure ./configure --prefix="$DEPLOYDIR" --without-ftp --without-http --without-python
  emmake make -j$NUMCPU
  emmake make install
}

build_fontconfig()
{
  version=$1
  extra_config_flags="$2"

  if [ -e $DEPLOYDIR/include/fontconfig ]; then
    echo "fontconfig already installed. not building"
    return
  fi

  echo "Building fontconfig $version..."
  cd "$BASEDIR"/src
  rm -rf "fontconfig-$version"
  if [ ! -f "fontconfig-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.freedesktop.org/software/fontconfig/release/fontconfig-$version.tar.gz"
  fi
  tar xzf "fontconfig-$version.tar.gz"
  cd "fontconfig-$version"
  export PKG_CONFIG_PATH="$DEPLOYDIR/lib/pkgconfig"
  emconfigure ./configure --prefix=/ --enable-libxml2 --disable-docs $extra_config_flags
  unset PKG_CONFIG_PATH
  DESTDIR="$DEPLOYDIR" emmake make -j$NUMCPU
  DESTDIR="$DEPLOYDIR" emmake make install
}

build_libffi()
{
  version="$1"

  if [ -e "$DEPLOYDIR/lib/libffi.a" ]; then
    echo "libffi already installed. not building"
    return
  fi

  echo "Building libffi $version..."
  cd "$BASEDIR"/src
  rm -rf "libffi-$version"
  if [ ! -f "libffi-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://sourceware.org/pub/libffi/libffi-$version.tar.gz"
  fi
  tar xzf "libffi-$version.tar.gz"
  cd "libffi-$version"
  emconfigure ./configure --prefix="$DEPLOYDIR"
  emmake make -j$NUMCPU
  emmake make install
}

build_gettext()
{
  version="$1"

  if [ -f "$DEPLOYDIR"/lib/libgettextpo.a ]; then
    echo "gettext already installed. not building"
    return
  fi

  echo "Building gettext $version..."
  cd "$BASEDIR"/src
  rm -rf "gettext-$version"
  if [ ! -f "gettext-$version.tar.gz" ]; then
    curl --insecure -LO "http://ftpmirror.gnu.org/gettext/gettext-$version.tar.gz"
  fi
  tar xzf "gettext-$version.tar.gz"
  cd "gettext-$version"

  emconfigure ./configure --prefix="$DEPLOYDIR" --disable-java --disable-native-java
  emmake make -j$NUMCPU
  emmake make install
}

build_glib2()
{
  version="$1"
  if [ -f "$DEPLOYDIR/include/glib-2.0/glib.h" ]; then
    echo "glib2 already installed. not building"
    return
  fi

  echo "Building glib2 $version..."

  cd "$BASEDIR"/src
  rm -rf "glib-$version"
  maj_min_version="${version%.*}" #Drop micro
  if [ ! -f "glib-$version.tar.xz" ]; then
    curl --insecure -LO "http://ftp.gnome.org/pub/gnome/sources/glib/$maj_min_version/glib-$version.tar.xz"
  fi
  tar xJf "glib-$version.tar.xz"
  cd "glib-$version"

  export PKG_CONFIG_PATH="$DEPLOYDIR/lib/pkgconfig"
  emconfigure ./configure --disable-gtk-doc --disable-man --prefix="$DEPLOYDIR" CFLAGS="-I$DEPLOYDIR/include" LDFLAGS="-L$DEPLOYDIR/lib"
  unset PKG_CONFIG_PATH
  emmake make -j$NUMCPU
  emmake make install
}

build_ragel()
{
  version=$1

  if [ -f $DEPLOYDIR/bin/ragel ]; then
    echo "ragel already installed. not building"
    return
  fi

  echo "Building ragel $version..."
  cd "$BASEDIR"/src
  rm -rf "ragel-$version"
  if [ ! -f "ragel-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.colm.net/files/ragel/ragel-$version.tar.gz"
  fi
  tar xzf "ragel-$version.tar.gz"
  cd "ragel-$version"
  sed -e "s/setiosflags(ios::right)/std::&/g" ragel/javacodegen.cpp > ragel/javacodegen.cpp.new && mv ragel/javacodegen.cpp.new ragel/javacodegen.cpp
  emconfigure ./configure --prefix="$DEPLOYDIR"
  emmake make -j$NUMCPU
  emmake make install
}

build_harfbuzz()
{
  version=$1
  extra_config_flags="$2"

  if [ -e $DEPLOYDIR/include/harfbuzz ]; then
    echo "harfbuzz already installed. not building"
    return
  fi

  echo "Building harfbuzz $version..."
  cd "$BASEDIR"/src
  rm -rf "harfbuzz-$version"
  if [ ! -f "harfbuzz-$version.tar.gz" ]; then
    curl --insecure -LO "http://cgit.freedesktop.org/harfbuzz/snapshot/harfbuzz-$version.tar.gz"
  fi
  tar xzf "harfbuzz-$version.tar.gz"
  cd "harfbuzz-$version"
  # disable doc directories as they make problems on Mac OS Build
  sed -e "s/SUBDIRS = src util test docs/SUBDIRS = src util test/g" Makefile.am > Makefile.am.bak && mv Makefile.am.bak Makefile.am
  sed -e "s/^docs.*$//" configure.ac > configure.ac.bak && mv configure.ac.bak configure.ac
  ./autogen.sh --prefix="$DEPLOYDIR" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no $extra_config_flags
  emmake make -j$NUMCPU
  emmake make install
}

# Singly build certain tools or libraries
if [ $1 ]; then
  if [ $1 = "git" ]; then
    build_git 1.7.10.3
    exit $?
  fi
  if [ $1 = "cgal" ]; then
    #immediate failure in configure
    build_cgal 4.4 use-sys-libs
    exit $?
  fi
  if [ $1 = "opencsg" ]; then
    #errors with GL required
    build_opencsg 1.3.2
    exit $?
  fi
  if [ $1 = "qt4" ]; then
    # such a huge build, put here by itself
    build_qt4 4.8.4
    exit $?
  fi
  if [ $1 = "qt5" ]; then
    build_qt5 5.3.1
    build_qt5scintilla2 2.8.3
    exit $?
  fi
  if [ $1 = "glu" ]; then
    # Mesa and GLU split in late 2012, so it's not on some systems
    #build fails for GL
    build_glu 9.0.0
    exit $?
  fi
  if [ $1 = "gettext" ]; then
    # such a huge build, put here by itself
    #fails with 
    #    ./l10nflist.c:214:7: error: implicit declaration of function 'argz_stringify' is invalid in C99
    #      [-Werror,-Wimplicit-function-declaration]
    #      __argz_stringify (cp, dirlist_len, PATH_SEPARATOR);
    #      ^
    #./l10nflist.c:112:27: note: expanded from macro '__argz_stringify'
    ## define __argz_stringify argz_stringify
    build_gettext 0.18.3.1
    exit $?
  fi
  if [ $1 = "glib2" ]; then
    # such a huge build, put here by itself
    #error: 'regparm' is not valid on this platform
    build_pkgconfig 0.28
    build_libffi 3.0.13
    #build_gettext 0.18.3.1
    build_glib2 2.38.2
    exit $?
  fi
  #Matt's additional options:
  if [ $1 = "glew" ]; then
    build_glew 1.9.0
    exit $?
  fi
  if [ $1 = "eigen" ]; then
    #THIS ONE COMPILES!!!
    build_eigen 3.2.2
    exit $?
  fi
  if [ $1 = "gmp" ]; then
    #THIS ONE COMPILES!!!
    build_gmp 5.0.5
    exit $?
  fi
  if [ $1 = "mpfr" ]; then
    #RTHIS ONE COMPILES!!!
    build_mpfr 3.1.1
    exit $?
  fi
  if [ $1 = "boost" ]; then
    #THIS ONE COMPILES!!!
    build_boost 1.56.0
    exit $?
  fi
  if [ $1 = "freetype" ]; then
    #THIS ONE COMPILES!!!
    build_freetype 2.5.0.1 --without-png
    exit $?
  fi
  if [ $1 = "libxml2" ]; then
    #THIS ONE COMPILES!!!
    build_libxml2 2.9.1
    exit $?
  fi
  if [ $1 = "fontconfig" ]; then
    #Can't find freetype
    build_fontconfig 2.11.0 --with-add-fonts=/usr/X11R6/lib/X11/fonts,/usr/local/share/fonts
    exit $?
  fi
  if [ $1 = "ragel" ]; then
    #THIS ONE COMPILES!!!
    build_ragel 6.9
    exit $?
  fi
  if [ $1 = "harfbuzz" ]; then
    build_harfbuzz 0.9.23 --with-glib=yes
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
build_gmp 5.0.5
build_mpfr 3.1.1
build_boost 1.56.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.4
build_glew 1.9.0
build_opencsg 1.3.2
build_gettext 0.18.3.1
build_glib2 2.38.2

# the following are only needed for text()
build_freetype 2.5.0.1 --without-png
build_libxml2 2.9.1
build_fontconfig 2.11.0 --with-add-fonts=/usr/X11R6/lib/X11/fonts,/usr/local/share/fonts
build_ragel 6.9
build_harfbuzz 0.9.23 --with-glib=yes

echo "OpenSCAD dependencies built and installed to " $BASEDIR
