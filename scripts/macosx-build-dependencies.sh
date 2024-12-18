#!/usr/bin/env bash
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit mode and backwards compatible
# with 10.13 "High Sierra".
#
# This script must be run from the OpenSCAD source root directory.
# By default, dependencies will be built for the local architecture.
#
# Usage: macosx-build-dependencies.sh [-dflaxv] [<package>]
#  -d         Build for deployment (if not specified, e.g. Sparkle won't be built)
#  -f         Force build even if package is installed
#  -l MINUTES Build time limit in minutes
#  -a         Build arm64 binaries
#  -x         Build x86_64 binaries
#  -v         Verbose
#
# Prerequisites: automake, libtool, cmake, pkg-config, wget, meson, python-packaging
#
# meson and python-packaging is required by glib.
#

set -e

if [ "`echo $* | grep \\\-v `" ]; then
  set -x
fi

BASEDIR=$PWD/../libraries
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install
MAC_OSX_VERSION_MIN=11.0
OPTION_DEPLOY=false
OPTION_FORCE=0
OPTION_ARM64=false
OPTION_X86_64=false

PACKAGES=(
    "double_conversion 3.2.1"
    "boost 1.86.0"
    "eigen 3.4.0"
    "gmp 6.3.0"
    "mpfr 4.2.0"
    "gettext 0.22.5"
    "freetype 2.12.1"
    "harfbuzz 6.0.0"

    "libzip 1.9.2"
    "fontconfig 2.14.1"
    "hidapi 0.12.0"
    "lib3mf 2.3.1"
    "pcre2 10.44"
    "glib2 2.83.0"
    "pixman 0.42.2"
    "cairo 1.18.0"
    "cgal 6.0"
    # Using Qt6 going forward, leaving Qt5 config just in case
    # "qt5 5.15.16"
    "qt6 6.5.3"
    "opencsg 1.6.0"
    "qscintilla 2.14.1"
    "onetbb 2021.12.0"
    "manifold 3.0.0"
)
DEPLOY_PACKAGES=(
    "sparkle 1.27.1"
)

printUsage()
{
  echo "Usage: $0 [-dflaxv] [<package>]"
  echo
  echo "  -d          Build for deployment"
  echo "  -f          Force build even if package is installed"
  echo "  -l MINUTES  Build time limit in minutes"
  echo "  -a          Build arm64 binaries"
  echo "  -x          Build x86_64 binaries"
  echo "  -v          Verbose"
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

# Usage: check_version_file <package> <version> [<patch>]
# Checks if $DEPLOYDIR/fileshare/macosx-build-dependencies/$package.version exists
# and its contents equals $version[.$patch]
# Returns success (0) if it does
check_version_file()
{
    versionfile="$DEPLOYDIR/share/macosx-build-dependencies/$1.version"
    if [ -f $versionfile ]; then
	if [ -z "$2" ]; then
	    return 0
	else
	    [[ $(cat $versionfile) == $2${3:+.}$3 ]]
	    return $?
	fi
    else
	return 1
    fi
}

# Usage: is_installed <package> [<version>]
# Returns success (0) if the/a version of the package is already installed
is_installed()
{
    if check_version_file $1 $2 $3; then
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
    local patch=$3

    if [[ $version == "REMOVE" ]]; then
	local should_remove=$(( $OPTION_FORCE == 1 ))
	if [[ $should_remove == 0 ]]; then
            if is_installed $package; then
		should_remove=1
	    else
   		echo "$package not installed - not removing"
	    fi
	fi
	if [[ $should_remove == 1 ]]; then
            set -e
            remove_$package
            set +e
	fi
    else
	local should_install=$(( $OPTION_FORCE == 1 ))
	if [[ $should_install == 0 ]]; then
            if ! is_installed $package $version $patch; then
		should_install=1
	    else
   		echo "$package $version $patch already installed - not building"
	    fi
	fi
	if [[ $should_install == 1 ]]; then
            set -e
	    echo "Building $package $version $patch..."
            build_$package $version $patch
	    echo $version${patch:+.}$patch > $DEPLOYDIR/share/macosx-build-dependencies/$package.version
            set +e
	fi
    fi
}

build_double_conversion()
{
  version="$1"
  cd "$BASEDIR"/src
  rm -rf "double-conversion-$version"
  if [ ! -f "double-conversion-$version.tar.gz" ]; then
    curl -L "https://github.com/google/double-conversion/archive/v$version.tar.gz" -o double-conversion-$version.tar.gz
  fi
  tar xzf "double-conversion-$version.tar.gz"
  cd "double-conversion-$version"
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" .
  make -j$NUMCPU
  make install
}

build_qt5()
{
  version=$1
  cd $BASEDIR/src
  v=(${version//./ }) # Split into array
  rm -rf qt-everywhere-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.xz ]; then
    curl -LO --insecure https://download.qt.io/official_releases/qt/${v[0]}.${v[1]}/$version/single/qt-everywhere-opensource-src-$version.tar.xz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.xz
  cd qt-everywhere-src-$version
  patch -p1 < $OPENSCADDIR/patches/qt5/qt-5.15-memory_resource.patch

  # Build each arch separately
  for arch in ${ARCHS[*]}; do
    mkdir build-$arch
    cd build-$arch
    ../configure -prefix $DEPLOYDIR -release -no-static -opensource -confirm-license \
		-nomake examples -nomake tests \
		-no-xcb -no-glib -no-harfbuzz -no-cups \
		-skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcharts -skip qtconnectivity -skip qtdatavis3d \
		-skip qtdeclarative -skip qtdoc -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtlottie \
		-skip qtnetworkauth -skip qtpurchasing -skip qtquick3d -skip qtquickcontrols \
		-skip qtquickcontrols2 -skip qtquicktimeline -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors \
		-skip qtserialbus -skip qtserialport -skip qtspeech -skip qttranslations -skip qtvirtualkeyboard \
		-skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview \
		-skip qtwinextras -skip qtx11extras -skip qtxmlpatterns \
		-no-feature-assistant -no-feature-designer -no-feature-distancefieldgenerator -no-feature-kmap2qmap \
		-no-feature-linguist -no-feature-makeqpf -no-feature-qev -no-feature-qtattributionsscanner \
		-no-feature-qtdiag -no-feature-qtpaths -no-feature-qtplugininfo \
		-no-feature-openal -no-feature-avfoundation -no-feature-gstreamer -no-feature-qdoc \
		-device-option QMAKE_APPLE_DEVICE_ARCHS=$arch
    make -j"$NUMCPU"
    make -j"$NUMCPU" install INSTALL_ROOT=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    frameworks="QtConcurrent QtCore QtDBus QtGamepad QtGui QtMacExtras QtMultimedia QtMultimediaWidgets QtNetwork QtOpenGL QtPrintSupport QtSql QtSvg QtTest QtWidgets QtXml"
    for framework in $frameworks; do
	LIBS=()
	for arch in ${ARCHS[*]}; do
	    LIBS+=(build-$arch/install/$DEPLOYDIR/lib/$framework.framework/Versions/Current/$framework)
	done
	lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/$framework.framework/Versions/Current/$framework
    done
    # We also need to merge plugins into universal binaries
    for plugin in $(find $DEPLOYDIR/plugins -name "*.dylib"); do
	libname=$(basename $plugin)
	lipo -create $(find build-*/install -name $libname) -output $plugin
    done
  fi
}

build_qt6()
{
  version=$1
  cd $BASEDIR/src
  v=(${version//./ }) # Split into array
  rm -rf qt-everywhere-src-$version
  if [ ! -f qt-everywhere-src-$version.tar.xz ]; then
    curl -LO --insecure https://download.qt.io/official_releases/qt/${v[0]}.${v[1]}/$version/single/qt-everywhere-src-$version.tar.xz
  fi
  tar xjf qt-everywhere-src-$version.tar.xz
  cd qt-everywhere-src-$version

  mkdir build
  cd build
  ../configure -prefix $DEPLOYDIR -release -opensource -confirm-license -nomake tests -nomake examples \
    -submodules qtbase,qt5compat,qtmultimedia,qtsvg -skip qtquick3d,qtquicktimeline,qtdeclarative \
    -no-feature-sql -no-feature-testlib -no-feature-glib \
		-no-feature-linguist -no-feature-designer -no-feature-pixeltool -no-feature-assistant \
    -no-feature-distancefieldgenerator -no-feature-qtattributionsscanner -no-feature-qtplugininfo \
    -no-feature-qtdiag \
    -- -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED_REV"
  ninja
  ninja install
}

build_qscintilla()
{
  version=$1
  cd $BASEDIR/src
  rm -rf QScintilla_src-$version
  QSCINTILLA_FILENAME="QScintilla_src-$version.tar.gz"
  if [ ! -f "${QSCINTILLA_FILENAME}" ]; then
      curl -LO https://www.riverbankcomputing.com/static/Downloads/QScintilla/$version/"${QSCINTILLA_FILENAME}"
  fi
  tar xzf "${QSCINTILLA_FILENAME}"
  cd QScintilla_src-$version/src
  qmake qscintilla.pro QMAKE_APPLE_DEVICE_ARCHS="${ARCHS[*]}"
  make -j"$NUMCPU" install
}


build_gmp()
{
  version=$1
  cd $BASEDIR/src
  rm -rf gmp-$version
  if [ ! -f gmp-$version.tar.bz2 ]; then
    # FIXME: -k is only to ignore libgmp's expired SSL certificate
    curl -kO https://gmplib.org/download/gmp/gmp-$version.tar.bz2
  fi
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version

  # Build each arch separately
  for arch in ${ARCHS[*]}; do
    mkdir build-$arch
    cd build-$arch
    M4=gm4 ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --enable-cxx --disable-static --build=$LOCAL_ARCH-apple-darwin --host=$arch-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    GMPLIBS=()
    GMPXXLIBS=()
    for arch in ${ARCHS[*]}; do
      GMPLIBS+=(build-$arch/install/$DEPLOYDIR/lib/libgmp.dylib)
      GMPXXLIBS+=(build-$arch/install/$DEPLOYDIR/lib/libgmpxx.dylib)
    done
    lipo -create ${GMPLIBS[@]} -output $DEPLOYDIR/lib/libgmp.dylib
    lipo -create ${GMPXXLIBS[@]} -output $DEPLOYDIR/lib/libgmpxx.dylib
  fi

  install_name_tool -id @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmp.dylib
  install_name_tool -id @rpath/libgmpxx.dylib $DEPLOYDIR/lib/libgmpxx.dylib
  install_name_tool -change $DEPLOYDIR/lib/libgmp.10.dylib @rpath/libgmp.dylib $DEPLOYDIR/lib/libgmpxx.dylib
}

build_mpfr()
{
  version=$1
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl -L -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    ../configure --prefix=$DEPLOYDIR --with-gmp=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --disable-static --build=$LOCAL_GNU_ARCH-apple-darwin --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libmpfr.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libmpfr.dylib
  fi

  install_name_tool -id @rpath/libmpfr.dylib $DEPLOYDIR/lib/libmpfr.dylib
}

build_boost()
{
  version=$1
  bversion=`echo $version | tr "." "_"`
  cd $BASEDIR/src
  rm -rf boost_$bversion
  if [ ! -f boost_$bversion.tar.bz2 ]; then
    curl -LO http://downloads.sourceforge.net/project/boost/boost/$version/boost_$bversion.tar.bz2
  fi
  tar xjf boost_$bversion.tar.bz2
  cd boost_$bversion

  ARCH_FLAGS=()
  for arch in ${ARCHS[*]}; do
    ARCH_FLAGS+=(-arch $arch)
  done

  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,chrono,system,regex,date_time,atomic
  ./b2 -j"$NUMCPU" -d+2 $BOOST_TOOLSET link=shared cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN ${ARCH_FLAGS[*]}" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN ${ARCH_FLAGS[*]} -headerpad_max_install_names" install
}

build_cgal()
{
  version=$1
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.xz ]; then
      curl -L https://github.com/CGAL/cgal/releases/download/v${version}/CGAL-${version}-library.tar.xz --output CGAL-${version}.tar.xz
  fi
  tar xzf CGAL-$version.tar.xz
  cd CGAL-$version
  cmake . -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Release -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt5=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" -DBOOST_ROOT=$DEPLOYDIR -DBoost_USE_MULTITHREADED=false
  make -j"$NUMCPU" install
  make install
  if [[ $version =~ 4.* ]]; then
    install_name_tool -id @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL.dylib
    install_name_tool -id @rpath/libCGAL_Core.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
    install_name_tool -change libCGAL.11.dylib @rpath/libCGAL.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  fi
}

build_onetbb()
{
  version=$1
  cd $BASEDIR/src
  rm -rf oneTBB-$version
  if [ ! -f oneTBB-$version.tar.gz ]; then
      curl -L https://github.com/oneapi-src/oneTBB/archive/refs/tags/v${version}.tar.gz --output oneTBB-$version.tar.gz
  fi
  tar xzf oneTBB-$version.tar.gz
  cd oneTBB-$version
  cmake . -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=OFF -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED"
  make -j"$NUMCPU" install
}

build_opencsg()
{
  version=$1
  cd $BASEDIR/src
  rm -rf OpenCSG-$version
  if [ ! -f OpenCSG-$version.tar.gz ]; then
    curl -O http://www.opencsg.org/OpenCSG-$version.tar.gz
  fi
  tar xzf OpenCSG-$version.tar.gz
  cd OpenCSG-$version
  patch -p1 < $OPENSCADDIR/patches/OpenCSG-$version-MacOSX-port.patch
  qmake -r INSTALLDIR=$DEPLOYDIR QMAKE_APPLE_DEVICE_ARCHS="${ARCHS[*]}"
  make install
  install_name_tool -id @rpath/libopencsg.dylib $DEPLOYDIR/lib/libopencsg.dylib
}

build_eigen()
{
  version=$1
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
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_TEST_NOQT=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" ..
  make -j"$NUMCPU" install
}

# Usage:
#   build_sparkle <githubuser>:<commitID>
#   build_sparkle <version>

build_sparkle()
{
# Binary install:
  version=$1

  echo "Installing sparkle" $version "..."
  cd $BASEDIR/src
  rm -rf Sparkle-$version
  if [ ! -f Sparkle-$version.tar.xz ]; then
    curl -LO https://github.com/sparkle-project/Sparkle/releases/download/$version/Sparkle-$version.tar.xz
  fi
  mkdir Sparkle-$version
  cd Sparkle-$version
  tar xjf ../Sparkle-$version.tar.xz
  # Make sure the destination dir is clean before overwriting
  rm -rf $DEPLOYDIR/lib/Sparkle.framework
  cp -Rf Sparkle.framework $DEPLOYDIR/lib/

# Build from source:
#  v=$1
#  github=${1%%:*}  # Cut at first colon
#  version=${1#*:}  # cut until first colon
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

build_freetype()
{
  version="$1"
  cd "$BASEDIR"/src
  rm -rf "freetype-$version"
  if [ ! -f "freetype-$version.tar.gz" ]; then
    curl --insecure -LO "http://downloads.sourceforge.net/project/freetype/freetype2/$version/freetype-$version.tar.gz"
  fi
  tar xzf "freetype-$version.tar.gz"
  cd "freetype-$version"

  export FREETYPE_CFLAGS="-I$DEPLOYDIR/include -I$DEPLOYDIR/include/freetype2"
  export FREETYPE_LIBS="-L$DEPLOYDIR/lib -lfreetype"

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    PKG_CONFIG_LIBDIR="$DEPLOYDIR/lib/pkgconfig" ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --without-png --without-harfbuzz --disable-static --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libfreetype.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libfreetype.dylib
  fi

  install_name_tool -id @rpath/libfreetype.dylib $DEPLOYDIR/lib/libfreetype.dylib
}

build_libzip()
{
  version="$1"
  cd "$BASEDIR"/src
  rm -rf "libzip-$version"
  if [ ! -f "libzip-$version.tar.gz" ]; then
    # Using wget instead of curl for now, due to a macOS 12 OpenSSL bug:
    # curl: (35) error:06FFF089:digital envelope routines:CRYPTO_internal:bad key length
    wget "https://libzip.org/download/libzip-$version.tar.gz"
  fi
  tar xzf "libzip-$version.tar.gz"
  cd "libzip-$version"
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" -DENABLE_GNUTLS=OFF -DENABLE_ZSTD=OFF .
  make -j$NUMCPU
  make install
  install_name_tool -id @rpath/libzip.dylib $DEPLOYDIR/lib/libzip.dylib
}

build_fontconfig()
{
  version=$1
  cd "$BASEDIR"/src
  rm -rf "fontconfig-$version"
  if [ ! -f "fontconfig-$version.tar.gz" ]; then
    curl -LO "https://www.freedesktop.org/software/fontconfig/release/fontconfig-$version.tar.gz"
  fi
  tar xzf "fontconfig-$version.tar.gz"
  cd "fontconfig-$version"
#  patch -p1 < $OPENSCADDIR/patches/fontconfig-arm64.patch

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    # FIXME: The "ac_cv_func_mkostemp=no" is a workaround for fontconfig's autotools config not respecting any passed
    # -no_weak_imports linker flag. This may be improved in future versions of fontconfig
    ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN -Wl,-rpath,$DEPLOYDIR/lib" --enable-libxml2  --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0 ac_cv_func_mkostemp=no
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libfontconfig.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libfontconfig.dylib
  fi

  install_name_tool -id @rpath/libfontconfig.dylib $DEPLOYDIR/lib/libfontconfig.dylib
}

build_gettext()
{
  version="$1"
  cd "$BASEDIR"/src
  rm -rf "gettext-$version"
  if [ ! -f "gettext-$version.tar.gz" ]; then
    curl --insecure -LO "http://ftp.gnu.org/pub/gnu/gettext/gettext-$version.tar.gz"
  fi
  tar xzf "gettext-$version.tar.gz"
  cd "gettext-$version"

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" CXXFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN -Wl,-rpath,$DEPLOYDIR/lib" --disable-shared --with-included-glib --with-included-gettext --with-included-libunistring --disable-java --disable-csharp --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU"
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libintl.a)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libintl.a
  fi
}

build_pcre2()
{
  version=$1
  cd "$BASEDIR"/src
  rm -rf "pcre2-$version"
  if [ ! -f "pcre2-$version.tar.bz2" ]; then
    curl -LO "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-$version/pcre2-$version.tar.bz2"
  fi
  tar xzf "pcre2-$version.tar.bz2"
  cd "pcre2-$version"

  cmake . -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_BUILD_TESTS=OFF -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED"
  make -j"$NUMCPU" install
  make install
}

build_glib2()
{
  version="$1"
  cd "$BASEDIR"/src
  rm -rf "glib-$version"
  maj_min_version="${version%.*}" #Drop micro
  if [ ! -f "glib-$version.tar.xz" ]; then
    curl -LO "https://ftp.gnome.org/pub/gnome/sources/glib/$maj_min_version/glib-$version.tar.xz"
  fi
  tar xJf "glib-$version.tar.xz"
  cd "glib-$version"

  # Build each arch separately
  for arch in ${ARCHS[*]}; do
    sed -e "s,@MAC_OSX_VERSION_MIN@,$MAC_OSX_VERSION_MIN,g" -e "s,@DEPLOYDIR@,$DEPLOYDIR,g" $OPENSCADDIR/scripts/macos-$arch.txt.in > macos-$arch.txt
    meson setup --prefix $DEPLOYDIR --cross-file macos-$arch.txt -Ddocumentation=false -Dman-pages=disabled -Ddtrace=disabled -Dtests=false build-$arch
    meson compile -C build-$arch
    DESTDIR=install/ meson install -C build-$arch
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libglib-2.0.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libglib-2.0.dylib
  fi

  install_name_tool -id @rpath/libglib-2.0.dylib $DEPLOYDIR/lib/libglib-2.0.dylib
}

build_harfbuzz()
{
  version=$1
  cd "$BASEDIR"/src
  rm -rf "harfbuzz-$version"
  if [ ! -f "harfbuzz-$version.tar.xz" ]; then
      curl -LO "https://github.com/harfbuzz/harfbuzz/releases/download/$version/harfbuzz-$version.tar.xz"
  fi
  tar xzf "harfbuzz-$version.tar.xz"
  cd "harfbuzz-$version"

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    PKG_CONFIG_LIBDIR="$DEPLOYDIR/lib/pkgconfig" ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" CXXFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no --with-coretext=auto --with-glib=no --disable-gtk-doc-html --disable-static --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libharfbuzz.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libharfbuzz.dylib
  fi

  install_name_tool -id @rpath/libharfbuzz.dylib $DEPLOYDIR/lib/libharfbuzz.dylib
}

build_hidapi()
{
  version=$1
  cd "$BASEDIR"/src
  rm -rf "hidapi-hidapi-$version"
  if [ ! -f "hidapi-$version.zip" ]; then
    curl --insecure -LO "https://github.com/libusb/hidapi/archive/hidapi-${version}.zip"
  fi
  unzip "hidapi-$version.zip"
  cd "hidapi-hidapi-$version"
  ./bootstrap # Needed when building from github sources

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --disable-static --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libhidapi.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libhidapi.dylib
  fi

  install_name_tool -id @rpath/libhidapi.dylib $DEPLOYDIR/lib/libhidapi.dylib
}

build_lib3mf()
{
  version=$1
  cd $BASEDIR/src
 rm -rf lib3mf-$version
 if [ ! -f $version.tar.gz ]; then
   curl -L https://github.com/3MFConsortium/lib3mf/archive/v$version.tar.gz -o lib3mf-$version.tar.gz
 fi
 tar xzf lib3mf-$version.tar.gz
  cd lib3mf-$version
  patch -p1 < $OPENSCADDIR/patches/lib3mf-macos.patch
  cmake -DLIB3MF_TESTS=false -DCMAKE_PREFIX_PATH=$DEPLOYDIR -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_INSTALL_INCLUDEDIR=include/lib3mf -DUSE_INCLUDED_ZLIB=OFF -DUSE_INCLUDED_LIBZIP=OFF -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" .
  make -j"$NUMCPU" VERBOSE=1
  make -j"$NUMCPU" install
}

build_pixman()
{
  version=$1
  PIXMAN_DIR="pixman-${version}"
  PIXMAN_FILENAME="${PIXMAN_DIR}.tar.gz"

  cd $BASEDIR/src
  rm -rf "$PIXMAN_DIR"
  if [ ! -f "${PIXMAN_FILENAME}" ]; then
    curl -LO https://www.cairographics.org/releases/"${PIXMAN_FILENAME}"
  fi
  tar xzf "${PIXMAN_FILENAME}"
  cd "$PIXMAN_DIR"

  # Build each arch separately
  for i in ${!ARCHS[@]}; do
    arch=${ARCHS[$i]}
    mkdir build-$arch
    cd build-$arch
    # libpng is only used for tests, disabling to kill linker warnings since we don't build libpng ourselves
    # --disable-arm-a64-neon is due to https://gitlab.freedesktop.org/pixman/pixman/-/issues/59 and
    #   https://gitlab.freedesktop.org/pixman/pixman/-/issues/69
    ../configure --prefix=$DEPLOYDIR CFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-arch $arch -mmacos-version-min=$MAC_OSX_VERSION_MIN" --disable-gtk --disable-libpng --disable-arm-a64-neon --disable-static --host=${GNU_ARCHS[$i]}-apple-darwin17.0.0
    make -j"$NUMCPU" install DESTDIR=$PWD/install/
    cd ..
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libpixman-1.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libpixman-1.dylib
  fi

  install_name_tool -id @rpath/libpixman-1.dylib $DEPLOYDIR/lib/"libpixman-1.dylib"
}

build_cairo()
{
  version=$1
  CAIRO_DIR="cairo-${version}"
  CAIRO_FILENAME="${CAIRO_DIR}.tar.xz"

  cd $BASEDIR/src
  rm -rf "$CAIRO_DIR"
  if [ ! -f "${CAIRO_FILENAME}" ]; then
    curl -LO https://www.cairographics.org/releases/"${CAIRO_FILENAME}"
  fi
  tar xzf "${CAIRO_FILENAME}"
  cd "$CAIRO_DIR"

  # FIXME: Cairo cannot disable lzo2, so we patch it
  patch -p1 < $OPENSCADDIR/patches/cairo-lzo2-macos.patch
  
  # Fix for cairo-1.18 build issue against freetype-2.13
  # https://lore.kernel.org/buildroot/20231116145113.1828682-1-thomas.devoogdt@barco.com/T/
  patch -p1 < $OPENSCADDIR/patches/cairo-ft-private.h-fix-missing-FT_Color-error.patch
  
  # Build each arch separately
  for arch in ${ARCHS[*]}; do
    sed -e "s,@MAC_OSX_VERSION_MIN@,$MAC_OSX_VERSION_MIN,g" -e "s,@DEPLOYDIR@,$DEPLOYDIR,g" $OPENSCADDIR/scripts/macos-$arch.txt.in > macos-$arch.txt
    meson setup --prefix $DEPLOYDIR --cross-file macos-$arch.txt -Dfreetype=enabled -Dfontconfig=enabled -Dxlib=disabled -Dxcb=disabled -Dpng=disabled -Dglib=disabled -Dtests=disabled -Dquartz=disabled build-$arch
    meson compile -C build-$arch
    DESTDIR=install/ meson install -C build-$arch
  done

  # Install the first arch
  cp -R build-${ARCHS[0]}/install/$DEPLOYDIR/* $DEPLOYDIR

  # If we're building for multiple archs, create fat binaries
  if (( ${#ARCHS[@]} > 1 )); then
    LIBS=()
    for arch in ${ARCHS[*]}; do
      LIBS+=(build-$arch/install/$DEPLOYDIR/lib/libcairo.dylib)
    done
    lipo -create ${LIBS[@]} -output $DEPLOYDIR/lib/libcairo.dylib
  fi

  install_name_tool -id @rpath/libcairo.dylib $DEPLOYDIR/lib/libcairo.dylib
  install_name_tool -change @rpath/libpixman.dylib @rpath/libpixman-1.dylib $DEPLOYDIR/lib/libcairo.dylib
}

build_manifold()
{
  cd $BASEDIR/src
  rm -rf "manifold-$version"
  if [ ! -f "manifold-$version.tar.gz" ]; then
    curl -LO https://github.com/elalish/manifold/releases/download/v$version/manifold-$version.tar.gz
  fi
  tar xzf "manifold-$version.tar.gz"
  cd "manifold-$version"

  mkdir build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="$ARCHS_COMBINED" -DMANIFOLD_CBIND=OFF -DMANIFOLD_TEST=OFF -DMANIFOLD_PAR=ON ..
  make -j$NUMCPU
  make install
}


if [ ! -f $OPENSCADDIR/openscad.appdata.xml.in ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi
OPENSCAD_SCRIPTDIR=$PWD/scripts

TIME_LIMIT=1440 # one day
while getopts 'dfl:axv' c
do
  case $c in
    d) OPTION_DEPLOY=true;;
    f) OPTION_FORCE=1;;
    l) TIME_LIMIT=${OPTARG}; if [ "$TIME_LIMIT" -gt 0 ]; then echo time limit $TIME_LIMIT minutes; else printUsage;exit 1; fi;;
    a) OPTION_ARM64=true;;
    x) OPTION_X86_64=true;;
    v) echo verbose on;;
    *) printUsage;exit 1;;
  esac
done

START_TIME=$(( $(date +%s) / 60 ))
STOP_TIME=$(( $START_TIME + $TIME_LIMIT ))
OPTION_PACKAGES="${@:$OPTIND}"

OSX_MAJOR_VERSION=`sw_vers -productVersion | cut -d. -f1`
OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_MAJOR_VERSION >= 14 )); then
  echo "Detected Sonoma (14.x) or later"
elif (( $OSX_MAJOR_VERSION >= 13 )); then
  echo "Detected Ventura (13.x) or later"
elif (( $OSX_MAJOR_VERSION >= 11 )); then
  echo "Detected BigSur (11.x) or later"
elif (( $OSX_VERSION >= 15 )); then
  echo "Detected Catalina (10.15) or later"
elif (( $OSX_VERSION >= 14 )); then
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

LOCAL_ARCH=`uname -m`

# Some older autotools doesn't recognize 'arm64', so we set
# LOCAL_GNU_ARCH and GNU_ARCHS to 'aarch64' for usage with those tools.
if [[ $LOCAL_ARCH == "arm64" ]]; then
    LOCAL_GNU_ARCH=aarch64
else
    LOCAL_GNU_ARCH=$LOCAL_ARCH
fi

ARCHS=()
GNU_ARCHS=()
if $OPTION_ARM64 || $OPTION_X86_64; then
  if $OPTION_ARM64; then
    ARCHS+=(arm64)
    GNU_ARCHS+=(aarch64)
  fi
  if $OPTION_X86_64; then
    ARCHS+=(x86_64)
	  GNU_ARCHS+=(x86_64)
  fi
else
    ARCHS+=($LOCAL_ARCH)
    GNU_ARCHS+=($LOCAL_GNU_ARCH)
fi
ARCHS_COMBINED=$(IFS=\; ; echo "${ARCHS[*]}")
# Some libraries needs to build arm64 first, while others need x86_64 first (e.g. Qt6)
ARCHS_REV=()
for ((i=${#ARCHS[@]}-1; i>=0; i--)); do
  ARCHS_REV+=("${ARCHS[i]}");
done
ARCHS_COMBINED_REV=$(IFS=\; ; echo "${ARCHS_REV[*]}")
echo "Building on $LOCAL_ARCH for $ARCHS_COMBINED"

echo "Building for $MAC_OSX_VERSION_MIN or later"

if [ ! $NUMCPU ]; then
  NUMCPU=$(($(sysctl -n hw.ncpu) * 3 / 2))
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
# Convert DEPLOYDIR to canonical path as "make install" doesn't always like ..s in folder names
DEPLOYDIR=$(cd "$DEPLOYDIR" ; pwd -P)
echo "Using deploydir:" $DEPLOYDIR

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

rm -f .timeout
for package in $OPTION_PACKAGES; do
  ELAPSED=$(( $(date +%s) / 60 - $START_TIME ))
  echo "Elapsed build time: $ELAPSED minutes"
  if [ "qt5" = $package -a $TIME_LIMIT -le 60 -a $ELAPSED -gt 2 ]; then
    touch .timeout
    echo "Timeout before building package $package"
    exit 0
  fi
  if [[ $ALL_PACKAGES =~ $package ]]; then
    build $package $(package_version $package)
    CURRENT_TIME=$(( $(date +%s) / 60 ))
    if [ $CURRENT_TIME -ge $STOP_TIME ]; then
      touch .timeout
      echo "Timeout after building package $package"
      exit 0
    fi
  else
    echo "Skipping unknown package $package"
  fi
done

if [ "`echo $* | grep \\\-v `" ]; then
  set +x
  echo verbose macosx dependency build finished running
fi
