#!/bin/bash
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit mode and backwards compatible with 10.8 "Mountain Lion".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-16lcdfv] [<package>]
#  -d   Build for deployment (if not specified, e.g. Sparkle won't be built)
#  -f   Force build even if package is installed
#  -v   Verbose
#
# Prerequisites:
# - MacPorts: curl, cmake
#

set -e

if [ "`echo $* | grep \\\-v `" ]; then
  set -x
fi

BASEDIR=$PWD/../libraries
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install
MAC_OSX_VERSION_MIN=10.9
OPTION_DEPLOY=false
OPTION_FORCE=0

PACKAGES=(
    "double_conversion 3.1.5"
    "eigen 3.3.7"
    "gmp 6.1.2"
    "mpfr 4.0.2"
    "glew 2.1.0"
    "gettext 0.21"
    "libffi 3.2.1"
    "freetype 2.9.1"
    "ragel 6.10"
    "harfbuzz 2.3.1"
    "libzip 1.5.1"
    "libxml2 2.9.9"
    "fontconfig 2.13.1"
    "hidapi 0.9.0"
    "libuuid 1.6.2"
    "lib3mf 1.8.1"
    "glib2 2.56.3"
    "boost 1.74.0"
    "poppler 21.01.0"
    "pixman 0.40.0"
    "cairo 1.16.0"
    "cgal 4.14.3"
    "qt5 5.9.9"
    "opencsg 1.4.2"
    "qscintilla 2.11.6"
)
DEPLOY_PACKAGES=(
    "sparkle 1.21.3"
)

printUsage()
{
  echo "Usage: $0 [-cdfv] [<package>]"
  echo
  echo "  -d   Build for deployment"
  echo "  -f   Force build even if package is installed"
  echo "  -v   Verbose"
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

# Usage: check_version_file <package> <version>
# Checks if $DEPLOYDIR/fileshare/macosx-build-dependencies/$package.version exists
# and its contents equals $version
# Returns success (0) if it does
check_version_file()
{
    versionfile="$DEPLOYDIR/share/macosx-build-dependencies/$1.version"
    if [ -f $versionfile ]; then
	[[ $(cat $versionfile) == $2 ]]
	return $?
    else
	return 1
    fi
}

# Usage: is_installed <package> [<version>]
# Returns success (0) if the/a version of the package is already installed
is_installed()
{
    if check_version_file $1 $2; then
	echo "$1 $2 already installed - not building"
	return 0
    else
	return 1
    fi
}

# Usage: build <package> <version>
build()
{
    local package=$1
    local version=$2

    local should_install=$(( $OPTION_FORCE == 1 ))
    if [[ $should_install == 0 ]]; then
        if ! is_installed $package $version; then
            should_install=1
	fi
    fi
    if [[ $should_install == 1 ]]; then
        set -e
        build_$package $version
        set +e
    fi
}

build_double_conversion()
{
  version="$1"

  echo "Building double-conversion $version..."
  cd "$BASEDIR"/src
  rm -rf "double-conversion-$version"
  if [ ! -f "double-conversion-$version.tar.gz" ]; then
    curl -L "https://github.com/google/double-conversion/archive/v$version.tar.gz" -o double-conversion-$version.tar.gz
  fi
  tar xzf "double-conversion-$version.tar.gz"
  cd "double-conversion-$version"
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" .
  make -j$NUMCPU
  make install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/double_conversion.version
}

build_qt5()
{
  version=$1

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  v=(${version//./ }) # Split into array
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.xz ]; then
      curl -LO http://download.qt.io/official_releases/qt/${v[0]}.${v[1]}/$version/single/qt-everywhere-opensource-src-$version.tar.xz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.xz
  cd qt-everywhere-opensource-src-$version
  patch -p1 < $OPENSCADDIR/patches/qt5/qt-5.9.7-macos.patch
  ./configure -prefix $DEPLOYDIR -release -opensource -confirm-license \
		-nomake examples -nomake tests \
		-no-xcb -no-glib -no-harfbuzz -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc \
		-no-sql-psql -no-sql-sqlite -no-sql-sqlite2 -no-sql-tds -no-cups -no-assimp -no-qml-debug \
                -skip qtx11extras -skip qtandroidextras -skip qtserialport -skip qtserialbus \
                -skip qtactiveqt -skip qtxmlpatterns -skip qtdeclarative -skip qtscxml \
                -skip qtpurchasing -skip qtcanvas3d -skip qtwayland \
                -skip qtconnectivity -skip qtwebsockets -skip qtwebchannel -skip qtsensors \
                -skip qtdatavis3d -skip qtcharts -skip qtwinextras \
                -skip qtgraphicaleffects -skip qtquickcontrols2 -skip qtquickcontrols \
                -skip qtvirtualkeyboard -skip qtlocation -skip qtwebengine -skip qtwebview \
                -skip qtscript -skip qttranslations -skip qtdoc \
                -no-feature-openal -no-feature-avfoundation
  make -j"$NUMCPU" 
  make install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/qt5.version
}

build_qscintilla()
{
  version=$1
  echo "Building QScintilla" $version "..."
  cd $BASEDIR/src
  QSCINTILLA_FILENAME="QScintilla-$version.tar.gz"
  rm -rf "${QSCINTILLA_FILENAME}"
  if [ ! -f "${QSCINTILLA_FILENAME}" ]; then
      curl -LO https://www.riverbankcomputing.com/static/Downloads/QScintilla/$version/"${QSCINTILLA_FILENAME}"
  fi
  tar xzf "${QSCINTILLA_FILENAME}"
  cd QScintilla*/Qt4Qt5
  #patch -p2 < $OPENSCADDIR/patches/QScintilla-2.9.3-xcode8.patch
  qmake qscintilla.pro
  make -j"$NUMCPU" install
  install_name_tool -id @rpath/libqscintilla2_qt5.dylib $DEPLOYDIR/lib/libqscintilla2_qt5.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/qscintilla.version
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
  # Note: We're building against the core2 CPU profile as that's the minimum required hardware for running OS X 10.9
  ./configure --prefix=$DEPLOYDIR CXXFLAGS="$CXXSTDFLAGS" CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="$LDSTDFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN" --enable-cxx --host=core2-apple-darwin13.0.0
  make -j"$NUMCPU" install

  install_name_tool -id @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmp.dylib
  install_name_tool -id @rpath/libgmpxx.dylib $DEPLOYDIR/lib/libgmpxx.dylib
  install_name_tool -change $DEPLOYDIR/lib/libgmp.10.dylib @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmpxx.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/gmp.version
}

build_mpfr()
{
  version=$1

  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl -L -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version

  ./configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64"
  make -j"$NUMCPU" install

  install_name_tool -id @rpath/libmpfr.dylib $DEPLOYDIR/lib/libmpfr.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/mpfr.version
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
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,chrono,system,regex,date_time,atomic
  BOOST_TOOLSET="toolset=clang"
  echo "using clang ;" >> tools/build/user-config.jam 
  ./b2 -j"$NUMCPU" -d+2 $BOOST_TOOLSET cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 -headerpad_max_install_names" install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/boost.version
}

build_cgal()
{
  version=$1

  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.xz ]; then
    curl -LO https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-$version/CGAL-$version.tar.xz
  fi
  tar xzf CGAL-$version.tar.xz
  cd CGAL-$version
  cmake . -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Release -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_Qt5=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64" -DBOOST_ROOT=$DEPLOYDIR -DBoost_USE_MULTITHREADED=false
  make -j"$NUMCPU" install
  make install
  install_name_tool -id @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL.dylib
  install_name_tool -id @rpath/libCGAL_Core.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  install_name_tool -change libCGAL.11.dylib @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/cgal.version
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
  make GLEW_DEST=$DEPLOYDIR CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS.EXTRA="-install_name @rpath/libGLEW.dylib -mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" POPT="-Os" STRIP= install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/glew.version
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
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/opencsg.version
}

build_eigen()
{
  version=$1

  echo "Building eigen" $version "..."
  cd $BASEDIR/src
  rm -rf eigen-$version

  if [ ! -f eigen-$version.tar.bz2 ]; then
    curl -LO https://gitlab.com/libeigen/eigen/-/archive/$version/eigen-$version.tar.bz2
  fi
  EIGENDIR=`tar tjf eigen-$version.tar.bz2 | head -1 | cut -f1 -d"/"`
  rm -rf "./$EIGENDIR"
  tar xjf eigen-$version.tar.bz2
  ln -s "./$EIGENDIR" eigen-$version || true
  cd eigen-$version
  mkdir build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_TEST_NOQT=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64" ..
  make -j"$NUMCPU" install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/eigen.version
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

  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/sparkle.version
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
  install_name_tool -id @rpath/libfreetype.dylib $DEPLOYDIR/lib/libfreetype.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/freetype.version
}
 
build_libzip()
{
  version="$1"

  echo "Building libzip $version..."
  cd "$BASEDIR"/src
  rm -rf "libzip-$version"
  if [ ! -f "libzip-$version.tar.gz" ]; then
    curl -LO "https://libzip.org/download/libzip-$version.tar.gz"
  fi
  tar xzf "libzip-$version.tar.gz"
  cd "libzip-$version"
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" .
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libzip.dylib $DEPLOYDIR/lib/libzip.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/libzip.version
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
  install_name_tool -id @rpath/libxml2.dylib $DEPLOYDIR/lib/libxml2.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/libxml2.version
}

build_fontconfig()
{
  version=$1

  echo "Building fontconfig $version..."
  cd "$BASEDIR"/src
  rm -rf "fontconfig-$version"
  if [ ! -f "fontconfig-$version.tar.gz" ]; then
    curl -LO "https://www.freedesktop.org/software/fontconfig/release/fontconfig-$version.tar.gz"
  fi
  tar xzf "fontconfig-$version.tar.gz"
  cd "fontconfig-$version"
  # FIXME: The "ac_cv_func_mkostemp=no" is a workaround for fontconfig's autotools config not respecting any passed
  # -no_weak_imports linker flag. This may be improved in future versions of fontconfig
  ./configure --prefix="$DEPLOYDIR" --enable-libxml2 CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS="-Wl,-rpath,$DEPLOYDIR/lib -mmacosx-version-min=$MAC_OSX_VERSION_MIN" ac_cv_func_mkostemp=no
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libfontconfig.dylib $DEPLOYDIR/lib/libfontconfig.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/fontconfig.version
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
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/libffi.version
}

build_gettext()
{
  version="$1"

  echo "Building gettext $version..."
  cd "$BASEDIR"/src
  rm -rf "gettext-$version"
  if [ ! -f "gettext-$version.tar.gz" ]; then
    curl --insecure -LO "http://ftpmirror.gnu.org/gettext/gettext-$version.tar.gz"
  fi
  tar xzf "gettext-$version.tar.gz"
  cd "gettext-$version"
  #patch -p1 < $OPENSCADDIR/patches/gettext.patch
  ./configure --with-included-glib --disable-java --disable-csharp --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -Wl,-rpath,$DEPLOYDIR/lib"
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libintl.dylib $DEPLOYDIR/lib/libintl.dylib
  install_name_tool -id @rpath/libgettextlib.dylib $DEPLOYDIR/lib/libgettextlib-$version.dylib

  install_name_tool -change $DEPLOYDIR/lib/libintl.9.dylib @rpath/libintl.dylib $DEPLOYDIR/lib/libgettextlib-$version.dylib

  install_name_tool -change $DEPLOYDIR/lib/libgettextsrc-$version.dylib @rpath/libgettextsrc.dylib $DEPLOYDIR/bin/msgfmt
  install_name_tool -change $DEPLOYDIR/lib/libgettextlib-$version.dylib @rpath/libgettextlib.dylib $DEPLOYDIR/bin/msgfmt
  install_name_tool -change $DEPLOYDIR/lib/libintl.9.dylib @rpath/libintl.dylib $DEPLOYDIR/bin/msgfmt

  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/gettext.version
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

  ./configure --disable-gtk-doc --disable-man --without-pcre --prefix="$DEPLOYDIR" CFLAGS="-I$DEPLOYDIR/include -mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-Wl,-rpath,$DEPLOYDIR/lib -L$DEPLOYDIR/lib -mmacosx-version-min=$MAC_OSX_VERSION_MIN"
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libglib-2.0.dylib $DEPLOYDIR/lib/libglib-2.0.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/glib2.version
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
  ./configure --prefix="$DEPLOYDIR"
  make -j$NUMCPU
  make install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/ragel.version
}

build_harfbuzz()
{
    set -x
  version=$1
  extra_config_flags="--with-coretext=auto --with-glib=no --disable-gtk-doc-html"

  echo "Building harfbuzz $version..."
  cd "$BASEDIR"/src
  rm -rf "harfbuzz-$version"
  if [ ! -f "harfbuzz-$version.tar.gz" ]; then
    curl -LO "https://www.freedesktop.org/software/harfbuzz/release/harfbuzz-$version.tar.bz2"
  fi
  tar xzf "harfbuzz-$version.tar.bz2"
  cd "harfbuzz-$version"
  PKG_CONFIG_LIBDIR="$DEPLOYDIR/lib/pkgconfig" ./configure --prefix="$DEPLOYDIR" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN CXXFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" $extra_config_flags
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libharfbuzz.dylib $DEPLOYDIR/lib/libharfbuzz.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/harfbuzz.version
}

build_hidapi()
{
  version=$1

  echo "Building hidapi $version..."
  cd "$BASEDIR"/src
  rm -rf "hidapi-hidapi-$version"
  if [ ! -f "hidapi-$version.zip" ]; then
    curl --insecure -LO "https://github.com/libusb/hidapi/archive/hidapi-${version}.zip"
  fi
  unzip "hidapi-$version.zip"
  cd "hidapi-hidapi-$version"
  ./bootstrap # Needed when building from github sources
  ./configure --prefix=$DEPLOYDIR CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN"
  make -j"$NUMCPU" install
  install_name_tool -id @rpath/libhidapi.dylib $DEPLOYDIR/lib/libhidapi.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/hidapi.version
}

build_libuuid()
{
  version=$1
  cd $BASEDIR/src
  rm -rf uuid-$version
  if [ ! -f uuid-$version.tar.gz ]; then
    curl -L https://mirrors.ocf.berkeley.edu/debian/pool/main/o/ossp-uuid/ossp-uuid_$version.orig.tar.gz -o uuid-$version.tar.gz
  fi
  tar xzf uuid-$version.tar.gz
  cd uuid-$version
  patch -p1 < $OPENSCADDIR/patches/uuid-1.6.2.patch
  ./configure -prefix $DEPLOYDIR CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" --without-perl --without-php --without-pgsql
  make -j"$NUMCPU"
  make install
  install_name_tool -id @rpath/libuuid.dylib $DEPLOYDIR/lib/libuuid.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/libuuid.version
}

build_lib3mf()
{
  version=$1

  echo "Building lib3mf" $version "..."
  cd $BASEDIR/src
  rm -rf lib3mf-$version
  if [ ! -f $version.tar.gz ]; then
    curl -L https://github.com/3MFConsortium/lib3mf/archive/v$version.tar.gz -o lib3mf-$version.tar.gz
  fi
  tar xzf lib3mf-$version.tar.gz
  cd lib3mf-$version
  cmake -DLIB3MF_TESTS=false -DCMAKE_PREFIX_PATH=$DEPLOYDIR -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" .
  make -j"$NUMCPU" VERBOSE=1
  make -j"$NUMCPU" install
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/lib3mf.version
}

build_poppler()
{
  version=$1
  POPPLER_DIR="poppler-${version}"
  POPPLER_FILENAME="${POPPLER_DIR}.tar.xz"

  echo "Building poppler" $version "..."

  cd $BASEDIR/src
  rm -rf "$POPPLER_DIR"
  if [ ! -f "${POPPLER_FILENAME}" ]; then
    curl -LO https://poppler.freedesktop.org/"${POPPLER_FILENAME}"
  fi
  tar xzf "${POPPLER_FILENAME}"
  cd "$POPPLER_DIR"
  mkdir build
  cd build
  cmake .. \
        -DCMAKE_INSTALL_PREFIX="$DEPLOYDIR" \
        -DCMAKE_OSX_ARCHITECTURES="x86_64" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" \
        -DBUILD_GTK_TESTS=OFF -DBUILD_QT5_TESTS=OFF -DBUILD_QT6_TESTS=OFF \
        -DBUILD_CPP_TESTS=OFF -DENABLE_GTK_DOC=OFF -DENABLE_QT5=OFF \
        -DENABLE_QT6=OFF -DENABLE_LIBOPENJPEG=none -DENABLE_DCTDECODER=none \
        -DENABLE_UTILS=OFF
  make -j"$NUMCPU" install
  otool -L $DEPLOYDIR/lib/"libpoppler.dylib"
  install_name_tool -id @rpath/libpoppler.dylib $DEPLOYDIR/lib/"libpoppler.dylib"
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/poppler.version
}

build_pixman()
{
  version=$1
  PIXMAN_DIR="pixman-${version}"
  PIXMAN_FILENAME="${PIXMAN_DIR}.tar.gz"

  echo "Building pixman" $version "..."

  cd $BASEDIR/src
  rm -rf "$PIXMAN_DIR"
  if [ ! -f "${PIXMAN_FILENAME}" ]; then
    curl -LO https://www.cairographics.org/releases/"${PIXMAN_FILENAME}"
  fi
  tar xzf "${PIXMAN_FILENAME}"
  cd "$PIXMAN_DIR"
  ./configure --prefix=$DEPLOYDIR CXXFLAGS="$CXXSTDFLAGS" CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="$LDSTDFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN"
  make -j"$NUMCPU" install
  otool -L $DEPLOYDIR/lib/"libpixman-1.dylib"
  install_name_tool -id @rpath/libpixman-1.dylib $DEPLOYDIR/lib/"libpixman-1.dylib"
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/pixman.version
}

build_cairo()
{
  version=$1
  CAIRO_DIR="cairo-${version}"
  CAIRO_FILENAME="${CAIRO_DIR}.tar.xz"

  echo "Building cairo" $version "..."

  cd $BASEDIR/src
  rm -rf "$CAIRO_DIR"
  if [ ! -f "${CAIRO_FILENAME}" ]; then
    curl -LO https://www.cairographics.org/releases/"${CAIRO_FILENAME}"
  fi
  tar xzf "${CAIRO_FILENAME}"
  cd "$CAIRO_DIR"
  ./configure --prefix=$DEPLOYDIR \
        CXXFLAGS="$CXXSTDFLAGS" \
        CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN" \
        LDFLAGS="$LDSTDFLAGS -mmacosx-version-min=$MAC_OSX_VERSION_MIN" \
        --enable-xlib=no --enable-xlib-xrender=no --enable-xcb=no \
        --enable-xlib-xcb=no --enable-xcb-shm=no --enable-win32=no \
        --enable-win32-font=no --enable-png=no --enable-ps=no \
        --enable-svg=no
  make -j"$NUMCPU" install
  otool -L $DEPLOYDIR/lib/libcairo.dylib
  install_name_tool -id @rpath/libcairo.dylib $DEPLOYDIR/lib/libcairo.dylib
  install_name_tool -change @rpath/libpixman.dylib @rpath/libpixman-1.dylib $DEPLOYDIR/lib/libcairo.dylib
  echo $version > $DEPLOYDIR/share/macosx-build-dependencies/cairo.version
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi
OPENSCAD_SCRIPTDIR=$PWD/scripts

while getopts '3lcdfv' c
do
  case $c in
    d) OPTION_DEPLOY=true;;
    f) OPTION_FORCE=1;;
    v) echo verbose on;;
    *) printUsage;exit 1;;
  esac
done

OPTION_PACKAGES="${@:$OPTIND}"

OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_VERSION >= 14 )); then
  echo "Detected Mojave (10.14) or later"
elif (( $OSX_VERSION >= 13 )); then
  echo "Detected High Sierra (10.13) or later"
elif (( $OSX_VERSION >= 12 )); then
  echo "Detected Sierra (10.12) or later"
elif (( $OSX_VERSION >= 11 )); then
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
mkdir -p $SRCDIR $DEPLOYDIR $DEPLOYDIR/share/macosx-build-dependencies

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

if [ "`echo $* | grep \\\-v `" ]; then
  set +x
  echo verbose macosx dependency build finished running
fi
