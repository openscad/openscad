#!/bin/bash
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit mode and backwards compatible with 10.8 "Mountain Lion".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-16lcdf] [<package>]
#  -3   Build using C++03 and libstdc++ (default is C++11 and libc++)
#  -l   Force use of LLVM compiler
#  -c   Force use of clang compiler
#  -d   Build for deployment (if not specified, e.g. Sparkle won't be built)
#  -f   Force build even if package is installed
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
MAC_OSX_VERSION_MIN=10.8
OPTION_LLVM=false
OPTION_CLANG=false
OPTION_DEPLOY=false
OPTION_FORCE=0
OPTION_CXX11=true

PACKAGES=(
    # NB! For eigen, also update the path in the function
    "eigen 3.2.8"
    "gmp 6.1.1"
    "mpfr 3.1.4"
    "boost 1.61.0"
    "qt5 5.7.0"
    "qscintilla 2.9.3"
    # NB! For CGAL, also update the actual download URL in the function
    "cgal 4.8.1"
    "glew 1.13.0"
    "gettext 0.19.8"
    "libffi 3.2.1"
    "glib2 2.50.1"
    "opencsg 1.4.2"
    "freetype 2.6.3"
    "ragel 6.9"
    "harfbuzz 1.2.7"
    "libzip 1.1.3"
    "libxml2 2.9.4"
    "fontconfig 2.12.0"
)
DEPLOY_PACKAGES=(
    "sparkle 1.13.1"
)

printUsage()
{
  echo "Usage: $0 [-3lcdf] [<package>]"
  echo
  echo "  -3   Build using C++03 and libstdc++"
  echo "  -l   Force use of LLVM compiler"
  echo "  -c   Force use of clang compiler"
  echo "  -d   Build for deployment"
  echo "  -f   Force build even if package is installed"
  echo
  echo "  If <package> is not specified, builds all packages"
}

# Outputs all package names
all_packages()
{
    for i in $(seq 0 $(( ${#PACKAGES[@]} - 1 )) ); do
        local p=${PACKAGES[$i]}
        echo -n "${p%%\ *} " # Cut at first space
    done
}

# Usage: package_version <package>
# Outputs the package version for the given package
package_version()
{
    for i in $(seq 0 $(( ${#PACKAGES[@]} - 1 )) ); do
        local p=${PACKAGES[$i]}
        if [ "$1" = "${p%%\ *}" ]; then
            echo "${p#*\ }" # cut until first space
            return 0
        fi
    done
    return 1
}

# Usage: build <package> <version>
build()
{
    local package=$1
    local version=$2

    local should_install=$(( $OPTION_FORCE == 1 ))
    if [[ $should_install == 0 ]]; then
        is_installed $package $version
        should_install=$?
    fi
    if [[ $should_install == 1 ]]; then
        set -e
        build_$package $version
        set +e
    fi
}

# Usage: is_installed <package> [<version>]
# Returns success (0) if the/a version of the package is already installed
is_installed()
{
    if check_$1 $2; then
      echo "$1 already installed - not building"
      return 0
    fi
    return 1
}

# Usage: check_dir <dir>
# Checks if $DEPLOYDIR/<dir> exists and is a folder
# Returns success (0) if the folder exists
check_dir()
{
    test -d "$DEPLOYDIR/$1"
}

# Usage: check_file <file>
# Checks if $DEPLOYDIR/<file> exists and is a file
# Returns success (0) if the file exists
check_file()
{
    test -f "$DEPLOYDIR/$1"
}


patch_qt_disable_core_wlan()
{
  version="$1"

  patch -p1 <<END-OF-PATCH
--- qt-everywhere-opensource-src-4.8.5/src/plugins/bearer/bearer.pro.orig	2013-11-01 19:04:29.000000000 +0100
+++ qt-everywhere-opensource-src-4.8.5/src/plugins/bearer/bearer.pro	2013-10-31 21:53:00.000000000 +0100
@@ -12,7 +12,7 @@
 #win32:SUBDIRS += nla
 win32:SUBDIRS += generic
 win32:!wince*:SUBDIRS += nativewifi
-macx:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
+#macx:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
 macx:SUBDIRS += generic
 symbian:SUBDIRS += symbian
 blackberry:SUBDIRS += blackberry
END-OF-PATCH
}

# FIXME: Support gcc/llvm/clang flags. Use -platform <whatever> to make this work? kintel 20130117
build_qt()
{
  version=$1

  if [ -d $DEPLOYDIR/lib/QtCore.framework ]; then
    echo "qt already installed. not building"
    return
  fi

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
     curl -O -L http://download.qt-project.org/official_releases/qt/4.8/4.8.5/qt-everywhere-opensource-src-4.8.5.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-src_corelib_global_qglobal.h.diff
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-libtiff.diff
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-src_plugins_bearer_corewlan_qcorewlanengine.mm.diff
  if $USING_CLANG; then
    # FIX for clang
    sed -i "" -e "s/::TabletProximityRec/TabletProximityRec/g"  src/gui/kernel/qt_cocoa_helpers_mac_p.h
    PLATFORM="-platform unsupported/macx-clang"
  fi
  case "$OSX_VERSION" in
    9)
      # libtiff fails in the linker step with Mavericks / XCode 5.0.1
      MACOSX_RELEASE_OPTIONS=-no-libtiff
      # wlan support bails out with lots of compiler errors, disable it for the build
      patch_qt_disable_core_wlan "$version"
      ;;
    *)
      MACOSX_RELEASE_OPTIONS=
      ;;
  esac
  ./configure -prefix $DEPLOYDIR -release -arch x86_64 -opensource -confirm-license $PLATFORM -fast -no-qt3support -no-svg -no-phonon -no-audio-backend -no-multimedia -no-javascript-jit -no-script -no-scripttools -no-declarative -no-xmlpatterns -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit $MACOSX_RELEASE_OPTIONS
  make -j"$NUMCPU" install
}

check_qt5()
{
    check_dir lib/QtCore.framework
}

build_qt5()
{
  version=$1

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  v=(${version//./ }) # Split into array
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
     curl -O -L http://download.qt-project.org/official_releases/qt/${v[0]}.${v[1]}/$version/single/qt-everywhere-opensource-src-$version.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  patch -d qtbase -p1 < $OPENSCADDIR/patches/qt5/QTBUG-56004.patch
  patch -d qtbase -p1 < $OPENSCADDIR/patches/qt5/QTBUG-56004b.patch
  if ! $USING_CXX11; then
    QT_EXTRA_FLAGS="-no-c++11"
  fi
  CXXFLAGS="$CXXSTDFLAGS" ./configure -prefix $DEPLOYDIR $QT_EXTRA_FLAGS -release -opensource -confirm-license \
		-nomake examples -nomake tests \
		-no-xcb -no-glib -no-harfbuzz -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc \
		-no-sql-psql -no-sql-sqlite2 -no-sql-tds -no-cups -no-qml-debug \
                -skip qtx11extras -skip qtandroidextras -skip qtserialport -skip qtserialbus \
                -skip qtactiveqt -skip qtxmlpatterns -skip qtdeclarative -skip qtscxml \
                -skip qtpurchasing -skip qtcanvas3d -skip qtgamepad -skip qtwayland \
                -skip qtconnectivity -skip qtwebsockets -skip qtwebchannel -skip qtsensors \
                -skip qtmultimedia -skip qtdatavis3d -skip qtcharts -skip qtwinextras \
                -skip qtgraphicaleffects -skip qtquickcontrols2 -skip qtquickcontrols \
                -skip qtvirtualkeyboard -skip qtlocation -skip qtwebengine -skip qtwebview \
                -skip qtscript -skip qttranslations -skip qtdoc
  make -j"$NUMCPU" 
  make install
}

check_qscintilla()
{
    check_file include/Qsci/qsciscintilla.h 
}

build_qscintilla()
{
  version=$1
  echo "Building QScintilla" $version "..."
  cd $BASEDIR/src
  rm -rf QScintilla_gpl-$version
  if [ ! -f QScintilla_gpl-$version.tar.gz ]; then
    curl -LO http://downloads.sourceforge.net/project/pyqt/QScintilla2/QScintilla-$version/QScintilla_gpl-$version.tar.gz
  fi
  tar xzf QScintilla_gpl-$version.tar.gz
  cd QScintilla_gpl-$version/Qt4Qt5
  patch -p2 < $OPENSCADDIR/patches/QScintilla-2.9.3-xcode8.patch
  qmake QMAKE_CXXFLAGS+="$CXXSTDFLAGS" QMAKE_LFLAGS+="$CXXSTDFLAGS" qscintilla.pro
  make -j"$NUMCPU" install
  install_name_tool -id @rpath/libqscintilla2.dylib $DEPLOYDIR/lib/libqscintilla2.dylib
}

check_gmp()
{
    check_file lib/libgmp.dylib
}

build_gmp()
{
  version=$1

  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp-$version
  if [ ! -f gmp-$version.tar.bz2 ]; then
    curl -O https://gmplib.org/download/gmp/gmp-$version.tar.bz2
  fi
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  # Note: We're building against the core2 CPU profile as that's the minimum required hardware for running OS X 10.8
  ./configure --prefix=$DEPLOYDIR CXXFLAGS="$CXXSTDFLAGS" CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="$LDSTDFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN" --enable-cxx --host=core2-apple-darwin12.0.0
  make -j"$NUMCPU" install

  install_name_tool -id @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmp.dylib
  install_name_tool -id @rpath/libgmpxx.dylib $DEPLOYDIR/lib/libgmpxx.dylib
  install_name_tool -change $DEPLOYDIR/lib/libgmp.10.dylib @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmpxx.dylib
}

check_mpfr()
{
    check_file include/mpfr.h
}

# As with gmplib, mpfr is built separately in 32-bit and 64-bit mode and then merged
# afterwards.
check_mpfr()
{
    check_file include/mpfr.h
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

  ./configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64"
  make -j"$NUMCPU" install

  install_name_tool -id @rpath/libmpfr.dylib $DEPLOYDIR/lib/libmpfr.dylib
}

check_boost()
{
    check_file lib/libboost_system.dylib
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
  if $USING_LLVM; then
    BOOST_TOOLSET="toolset=darwin-llvm"
    echo "using darwin : llvm : llvm-g++ ;" >> tools/build/user-config.jam 
  elif $USING_CLANG; then
    BOOST_TOOLSET="toolset=clang"
    echo "using clang ;" >> tools/build/user-config.jam 
  fi
  ./b2 -j"$NUMCPU" -d+2 $BOOST_TOOLSET cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $CXXSTDFLAGS" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64  $LDSTDFLAGS -headerpad_max_install_names" install
}

check_cgal()
{
    check_file lib/libCGAL.dylib
}

build_cgal()
{
  version=$1

  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.xz ]; then
    # 4.8  
    curl -LO https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-$version/CGAL-$version.tar.xz
    # 4.6.3 curl -O https://gforge.inria.fr/frs/download.php/file/35138/CGAL-$version.tar.gz
    # 4.5.2 curl -O https://gforge.inria.fr/frs/download.php/file/34512/CGAL-$version.tar.gz
    # 4.5.1 curl -O https://gforge.inria.fr/frs/download.php/file/34400/CGAL-$version.tar.gz
    # 4.5 curl -O https://gforge.inria.fr/frs/download.php/file/34149/CGAL-$version.tar.gz
    # 4.4 curl -O https://gforge.inria.fr/frs/download.php/file/33525/CGAL-$version.tar.gz
    # 4.3 curl -O https://gforge.inria.fr/frs/download.php/32994/CGAL-$version.tar.gz
    # 4.2 curl -O https://gforge.inria.fr/frs/download.php/32359/CGAL-$version.tar.gz
    # 4.1 curl -O https://gforge.inria.fr/frs/download.php/31641/CGAL-$version.tar.gz
    # 4.1-beta1 curl -O https://gforge.inria.fr/frs/download.php/31348/CGAL-$version.tar.gz
    # 4.0.2 curl -O https://gforge.inria.fr/frs/download.php/31175/CGAL-$version.tar.gz
    # 4.0 curl -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar xzf CGAL-$version.tar.xz
  cd CGAL-$version
  CXXFLAGS="$CXXSTDFLAGS" cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64" -DBOOST_ROOT=$DEPLOYDIR -DBoost_USE_MULTITHREADED=false
  make -j"$NUMCPU" install
  make install
  install_name_tool -id @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL.dylib
  install_name_tool -id @rpath/libCGAL_Core.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  install_name_tool -change libCGAL.11.dylib @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
}

check_glew()
{
    check_file lib/libGLEW.dylib
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
  make GLEW_DEST=$DEPLOYDIR CC=$CC CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS.EXTRA="-install_name @rpath/libGLEW.dylib -mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" POPT="-Os" STRIP= install
}

check_opencsg()
{
    check_file lib/libopencsg.dylib
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
  qmake -r INSTALLDIR=$DEPLOYDIR
  make install
  install_name_tool -id @rpath/libopencsg.dylib $DEPLOYDIR/lib/libopencsg.dylib
}

# Usage: func [<version>]
check_eigen()
{
    # To check version:
    # include/eigen3/Eigen/src/Core/util/Macros.h:
    #  #define EIGEN_WORLD_VERSION 3
    #  #define EIGEN_MAJOR_VERSION 2
    #  #define EIGEN_MINOR_VERSION 3

    check_dir include/eigen3
}

# Usage: func <version>
build_eigen()
{
  version=$1

  echo "Building eigen" $version "..."
  cd $BASEDIR/src
  rm -rf eigen-$version

  EIGENDIR="none"
  if [ $version = "3.1.2" ]; then EIGENDIR=eigen-eigen-5097c01bcdc4;
  elif [ $version = "3.1.3" ]; then EIGENDIR=eigen-eigen-2249f9c22fe8;
  elif [ $version = "3.1.4" ]; then EIGENDIR=eigen-eigen-36bf2ceaf8f5;
  elif [ $version = "3.2.0" ]; then EIGENDIR=eigen-eigen-ffa86ffb5570;
  elif [ $version = "3.2.1" ]; then EIGENDIR=eigen-eigen-6b38706d90a9;
  elif [ $version = "3.2.2" ]; then EIGENDIR=eigen-eigen-1306d75b4a21;
  elif [ $version = "3.2.3" ]; then EIGENDIR=eigen-eigen-36fd1ba04c12;
  elif [ $version = "3.2.4" ]; then EIGENDIR=eigen-eigen-10219c95fe65;
  elif [ $version = "3.2.6" ]; then EIGENDIR=eigen-eigen-c58038c56923;
  elif [ $version = "3.2.8" ]; then EIGENDIR=eigen-eigen-07105f7124f9;
  fi  
  
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
  CXXFLAGS="$CXXSTDFLAGS" cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_TEST_NOQT=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64" ..
  make -j"$NUMCPU" install
}

check_sparkle()
{
    check_file lib/Sparkle.framework/Sparkle
}

# Usage:
#   build_sparkle <githubuser>:<commitID>
#   build_sparkle <version>

build_sparkle()
{
# Binary install:
  version=$1
  cd $BASEDIR/src
  rm -rf Sparkle-$version
  if [ ! -f Sparkle-$version.tar.bz2 ]; then
    curl -LO https://github.com/sparkle-project/Sparkle/releases/download/$version/Sparkle-$version.tar.bz2
  fi
  mkdir Sparkle-$version
  cd Sparkle-$version
  tar xjf ../Sparkle-$version.tar.bz2
  cp -Rf Sparkle.framework $DEPLOYDIR/lib/ 

# Build from source:
#  v=$1
#  github=${1%%:*}  # Cut at first colon
#  version=${1#*:}  # cut until first colon
#
#  echo "Building Sparkle" $version "..."
#
#  # Let Sparkle use the default compiler
#  unset CC
#  unset CXX
#
#  cd $BASEDIR/src
#  rm -rf Sparkle-$version
#  if [ ! -f Sparkle-$version.zip ]; then
#      curl -o Sparkle-$version.zip https://nodeload.github.com/$github/Sparkle/zip/$version
#  fi
#  unzip -q Sparkle-$version.zip
#  cd Sparkle-$version
#  patch -p1 < $OPENSCADDIR/patches/sparkle.patch
#  xcodebuild clean
#  xcodebuild -arch x86_64
#  rm -rf $DEPLOYDIR/lib/Sparkle.framework
#  cp -Rf build/Release/Sparkle.framework $DEPLOYDIR/lib/ 
#  Install_name_tool -id $DEPLOYDIR/lib/Sparkle.framework/Versions/A/Sparkle $DEPLOYDIR/lib/Sparkle.framework/Sparkle
}

check_freetype()
{
    check_file lib/libfreetype.dylib
}

build_freetype()
{
  version="$1"
  extra_config_flags="--without-png"

  echo "Building freetype $version..."
  cd "$BASEDIR"/src
  rm -rf "freetype-$version"
  if [ ! -f "freetype-$version.tar.gz" ]; then
    curl --insecure -LO "http://downloads.sourceforge.net/project/freetype/freetype2/$version/freetype-$version.tar.gz"
  fi
  tar xzf "freetype-$version.tar.gz"
  cd "freetype-$version"

  export FREETYPE_CFLAGS="-I$DEPLOYDIR/include -I$DEPLOYDIR/include/freetype2"
  export FREETYPE_LIBS="-L$DEPLOYDIR/lib -lfreetype"
  PKG_CONFIG_LIBDIR="$DEPLOYDOR/lib/pkgconfig" ./configure --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN $extra_config_flags
  make -j"$NUMCPU"
  make install
  install_name_tool -id $DEPLOYDIR/lib/libfreetype.dylib $DEPLOYDIR/lib/libfreetype.dylib
}
 
check_libzip()
{
    check_file lib/libzip.dylib
}

build_libzip()
{
  version="$1"

  echo "Building libzip $version..."
  cd "$BASEDIR"/src
  rm -rf "libzip-$version"
  if [ ! -f "libxml2-$version.tar.gz" ]; then
    curl --insecure -LO "https://nih.at/libzip/libzip-1.1.3.tar.gz"
  fi
  tar xzf "libzip-$version.tar.gz"
  cd "libzip-$version"
  ./configure --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libzip.dylib $DEPLOYDIR/lib/libzip.dylib
}

check_libxml2()
{
    check_file lib/libxml2.dylib
}

build_libxml2()
{
  version="$1"

  echo "Building libxml2 $version..."
  cd "$BASEDIR"/src
  rm -rf "libxml2-$version"
  if [ ! -f "libxml2-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://xmlsoft.org/libxml2/libxml2-$version.tar.gz"
  fi
  tar xzf "libxml2-$version.tar.gz"
  cd "libxml2-$version"
  ./configure --prefix="$DEPLOYDIR" --with-zlib=/usr --without-lzma --without-ftp --without-http --without-python CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  make -j$NUMCPU
  make install
  install_name_tool -id $DEPLOYDIR/lib/libxml2.dylib $DEPLOYDIR/lib/libxml2.dylib
}

check_fontconfig()
{
    check_file lib/libfontconfig.dylib
}

build_fontconfig()
{
  version=$1

  echo "Building fontconfig $version..."
  cd "$BASEDIR"/src
  rm -rf "fontconfig-$version"
  if [ ! -f "fontconfig-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.freedesktop.org/software/fontconfig/release/fontconfig-$version.tar.gz"
  fi
  tar xzf "fontconfig-$version.tar.gz"
  cd "fontconfig-$version"
  export PKG_CONFIG_PATH="$DEPLOYDIR/lib/pkgconfig"
  ./configure --prefix="$DEPLOYDIR" --enable-libxml2 CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  unset PKG_CONFIG_PATH
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libfontconfig.dylib $DEPLOYDIR/lib/libfontconfig.dylib
}

check_libffi()
{
    check_file lib/libffi.dylib
}

build_libffi()
{
  version="$1"

  echo "Building libffi $version..."
  cd "$BASEDIR"/src
  rm -rf "libffi-$version"
  if [ ! -f "libffi-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://sourceware.org/pub/libffi/libffi-$version.tar.gz"
  fi
  tar xzf "libffi-$version.tar.gz"
  cd "libffi-$version"
  ./configure --prefix="$DEPLOYDIR"
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libffi.dylib $DEPLOYDIR/lib/libffi.dylib
}

check_gettext()
{
    check_file lib/libgettextlib.dylib
}

build_gettext()
{
  version="$1"

  echo "Building gettext $version..."
  cd "$BASEDIR"/src
  rm -rf "gettext-$version"
  if [ ! -f "gettext-$version.tar.xz" ]; then
    curl --insecure -LO "http://ftpmirror.gnu.org/gettext/gettext-$version.tar.gz"
  fi
  tar xzf "gettext-$version.tar.gz"
  cd "gettext-$version"

  ./configure --with-included-glib --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  make -j$NUMCPU
  make install
  install_name_tool -id $DEPLOYDIR/lib/libintl.dylib $DEPLOYDIR/lib/libintl.dylib
}

check_glib2()
{
    check_file lib/libglib-2.0.dylib
}

build_glib2()
{
  version="$1"

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
  ./configure --disable-gtk-doc --disable-man --without-pcre --prefix="$DEPLOYDIR" CFLAGS="-I$DEPLOYDIR/include -mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-L$DEPLOYDIR/lib -mmacosx-version-min=$MAC_OSX_VERSION_MIN"
  unset PKG_CONFIG_PATH
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libglib-2.0.dylib $DEPLOYDIR/lib/libglib-2.0.dylib
}

check_ragel()
{
    check_file bin/ragel
}

build_ragel()
{
  version=$1

  echo "Building ragel $version..."
  cd "$BASEDIR"/src
  rm -rf "ragel-$version"
  if [ ! -f "ragel-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.colm.net/files/ragel/ragel-$version.tar.gz"
  fi
  tar xzf "ragel-$version.tar.gz"
  cd "ragel-$version"
  sed -e "s/setiosflags(ios::right)/std::&/g" ragel/javacodegen.cpp > ragel/javacodegen.cpp.new && mv ragel/javacodegen.cpp.new ragel/javacodegen.cpp
  ./configure --prefix="$DEPLOYDIR"
  make -j$NUMCPU
  make install
}

check_harfbuzz()
{
    check_file lib/libharfbuzz.dylib
}

build_harfbuzz()
{
  version=$1
  extra_config_flags="--with-coretext=auto --with-glib=no"

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
  PKG_CONFIG_LIBDIR="$DEPLOYDIR/lib/pkgconfig" ./autogen.sh --prefix="$DEPLOYDIR" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN CXXFLAGS="$CXXFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="$CXXFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN" $extra_config_flags
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libharfbuzz.dylib $DEPLOYDIR/lib/libharfbuzz.dylib
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi
OPENSCAD_SCRIPTDIR=$PWD/scripts

while getopts '3lcdf' c
do
  case $c in
    3) USING_CXX11=false;;
    l) OPTION_LLVM=true;;
    c) OPTION_CLANG=true;;
    d) OPTION_DEPLOY=true;;
    f) OPTION_FORCE=1;;
    *) printUsage;exit 1;;
  esac
done

OPTION_PACKAGES="${@:$OPTIND}"

OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_VERSION >= 11 )); then
  echo "Detected El Capitan (10.11) or later"
elif (( $OSX_VERSION >= 10 )); then
  echo "Detected Yosemite (10.10) or later"
elif (( $OSX_VERSION >= 9 )); then
  echo "Detected Mavericks (10.9)"
elif (( $OSX_VERSION >= 8 )); then
  echo "Detected Mountain Lion (10.8)"
else
  echo "Detected Lion (10.7) or earlier"
fi

USING_LLVM=false
USING_CLANG=false
if $OPTION_LLVM; then
  USING_LLVM=true
elif $OPTION_CLANG; then
  USING_CLANG=true
elif (( $OSX_VERSION >= 7 )); then
  USING_CLANG=true
fi

if $USING_LLVM; then
  echo "Using gcc LLVM compiler"
  export CC=llvm-gcc
  export CXX=llvm-g++
  export QMAKESPEC=macx-llvm
elif $USING_CLANG; then
  echo "Using clang compiler"
  export CC=clang
  export CXX=clang++
fi

if $USING_CXX11; then
  export CXXSTDFLAGS="-std=c++11 -stdlib=libc++"
  export LDSTDFLAGS="-stdlib=libc++"
else
  export CXXSTDFLAGS="-std=c++03 -stdlib=libstdc++"
  export LDSTDFLAGS="-stdlib=libstdc++"
fi

echo "Building for $MAC_OSX_VERSION_MIN or later"

if [ ! $NUMCPU ]; then
  NUMCPU=$(sysctl -n hw.ncpu)
  echo "Setting number of CPUs to $NUMCPU"
fi

if $OPTION_DEPLOY; then
  echo "Building deployment version of libraries"
fi

if (( $OPTION_FORCE )); then
  echo "Forcing rebuild"
fi

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR

# Only build deploy packages in deploy mode
if $OPTION_DEPLOY; then
  # Array concatenation
  PACKAGES=("${PACKAGES[@]}" "${DEPLOY_PACKAGES[@]}")
fi

# Build specified (or all) packages
ALL_PACKAGES=$(all_packages)
echo $ALL_PACKAGES
if [ -z "$OPTION_PACKAGES" ]; then
  OPTION_PACKAGES=$ALL_PACKAGES
fi

echo "Building packages: $OPTION_PACKAGES"
echo

for package in $OPTION_PACKAGES; do
  if [[ $ALL_PACKAGES =~ $package ]]; then
    build $package $(package_version $package)
  else
    echo "Skipping unknown package $package"
  fi
done
