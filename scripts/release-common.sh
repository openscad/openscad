#!/usr/bin/env bash
#
# This script creates a binary release of OpenSCAD. This should work
# under Mac OS X, Linux 32bit, Linux 64bit, and Linux->Win32 MXE cross-build.
# Windows under msys has not been tested recently.
#
# The script will create a file called openscad-<versionstring>.<extension> in
# the current directory (or under ./mingw32 or ./mingw64)
#
# Usage: release-common.sh [-v <versionstring>] [-c] [-mingw[32|64]]
#  -v       Version string (e.g. -v 2010.01)
#  -d       Version date (e.g. -d 2010.01.23)
#  mingw32 Cross-compile for win32 using MXE
#  mingw64 Cross-compile for win64 using MXE
#  snapshot Build a snapshot binary (make e.g. experimental features available, build with commit info)
#
# If no version string or version date is given, today's date will be used (YYYY-MM-DD)
# If only version date is given, it will be used also as version string.
# If no make target is given, release will be used on Windows, none one Mac OS X
#
# The mingw cross compile depends on the MXE cross-build tools. Please
# see the README.md file on how to install these dependencies. To debug
# the mingw-cross build process, set env var FAKEMAKE=1 to fake-make the
# .exe files
#

set -e # exit when any command fails

# convert end-of-line in given file from unix \n to dos/windows(TM) \r\n
# see https://kb.iu.edu/data/acux.html
lf2crlf()
{
  fname=$1
  if [ "`command -v unix2dos`" ]; then
    unix2dos $fname
    return
  fi
  if [ "`command -v awk`" ]; then
    echo using awk to convert end of line markers in $fname
    awk 'sub("$", "\r")' $fname > $fname".temp"
    mv $fname".temp" $fname
    return
  fi
  echo "warning- can't change eol to cr eol"
}

printUsage()
{
  echo "Usage: $0 -v <versionstring> -d <versiondate> [mingw32|mingw64] [shared] [snapshot]"
  echo
  echo "  -d <versiondate>: YYYY.MM.DD format; defaults to today\'s date"
  echo "  -v <versionstring>: YYYY.MM format; defaults to <versiondate>"
  echo "  mingw32|mingw64: Override \$OSTYPE"
  echo "  shared:          Use shared libraries for mingw build"
  echo "  snapshot:        Build a development snapshot (-DSNAPSHOT=ON -DEXPERIMENTAL=ON)"
  echo
  echo "  Example: $0 -v 2021.01"
}

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/src/openscad.cc ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 1
fi

CMAKE_CONFIG=

if [[ "$OSTYPE" =~ "darwin" ]]; then
  OS=MACOSX
elif [[ $OSTYPE == "msys" ]]; then
  OS=WIN
elif [[ $OSTYPE == "linux-gnu" ]]; then
  OS=LINUX
  if [[ `uname -m` == "x86_64" ]]; then
    ARCH=64
  else
    ARCH=32
  fi
  echo "Detected build-machine ARCH: $ARCH"
fi

if [ "`echo $* | grep mingw`" ]; then
  OS=UNIX_CROSS_WIN
  ARCH=32
  if [ "`echo $* | grep mingw64`" ]; then
    ARCH=64
  fi
fi

if [ $OS ]; then
  echo "Detected OS: $OS"
else
  echo "Error: Couldn't detect OSTYPE"
  exit
fi

case $OS in
    MACOSX)
        . ./scripts/setenv-macos.sh
        CMAKE_CONFIG="$CMAKE_CONFIG -DUSE_BUILTIN_MANIFOLD=OFF -DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"
    ;;
    LINUX)
        TARGET=
        export QT_SELECT=5
    ;;
    WIN)
        export QTDIR=/c/devmingw/qt2009.03
        export QTMAKESPEC=win32-g++
        export PATH=$PATH:/c/devmingw/qt2009.03/bin:/c/devmingw/qt2009.03/qt/bin
        TARGET=release
        ZIP="/c/Program Files/7-Zip/7z.exe"
        ZIPARGS="a -tzip"
    ;;
    UNIX_CROSS_WIN)
        SHARED=
        if [ "`echo $* | grep shared`" ]; then
          SHARED=-shared
        fi
        MINGWCONFIG=mingw-cross-env$SHARED
        . ./scripts/setenv-mingw-xbuild.sh $ARCH $SHARED
        TARGET=
        ZIP="zip"
        ZIPARGS="-r -q"
        echo Mingw-cross build using ARCH=$ARCH MXELIBTYPE=$MXELIBTYPE
        CMAKE_CONFIG="$CMAKE_CONFIG -GNinja -DMXECROSS=ON -DALLOW_BUNDLED_HIDAPI=ON -DPACKAGE_ARCH=x86-$ARCH"
    ;;
esac

if [ "`echo $* | grep snapshot`" ]; then
  CMAKE_CONFIG="$CMAKE_CONFIG -DSNAPSHOT=ON -DEXPERIMENTAL=ON"
  BUILD_TYPE="Release"
  OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
else
  BUILD_TYPE="Release"
fi

while getopts 'v:d:' c
do
  case $c in
    v) VERSION=$OPTARG;;
    d) VERSIONDATE=$OPTARG;;
  esac
done

if test -z "$VERSIONDATE"; then
    VERSIONDATE=`date "+%Y.%m.%d"`
fi
if test -z "$VERSION"; then
    VERSION=$VERSIONDATE
fi

export VERSIONDATE
export VERSION

if [ $FAKEMAKE ]; then
  echo 'fake make on:' $FAKEMAKE
else
  FAKEMAKE=
fi

case $OS in
  UNIX_CROSS_WIN)
    CMAKE=$MXE_TARGETS-cmake
    ;;
  *)
    CMAKE=cmake
    ;;
esac

echo "Checking pre-requisites..."

git submodule update --init --recursive

echo "Building openscad-$VERSION ($VERSIONDATE)"
echo "  CMake args: $CMAKE_CONFIG"
echo "  DEPLOYDIR: " $DEPLOYDIR

if [ ! $NUMCPU ]; then
  echo "  Note: you can 'export NUMCPU=x' for multi-core compiles (x=number)";
  NUMCPU=1
fi
echo "  NUMCPU: $NUMCPU"

cd $DEPLOYDIR
CMAKE_CONFIG="${CMAKE_CONFIG}\
 -DCMAKE_BUILD_TYPE=${BUILD_TYPE}\
 -DOPENSCAD_VERSION=${VERSION}\
 -DOPENSCAD_COMMIT=${OPENSCAD_COMMIT}"

echo "Running CMake from ${DEPLOYDIR}"
echo "${CMAKE} .. ${CMAKE_CONFIG}"

"${CMAKE}" .. ${CMAKE_CONFIG}
cd $OPENSCADDIR

echo "Building Project..."

case $OS in
    UNIX_CROSS_WIN)
        # make main openscad.exe
        cd $DEPLOYDIR
        if [ $FAKEMAKE ]; then
            echo "notexe. debugging build process" > openscad.exe
        else
            ${CMAKE} --build . -j$NUMCPU
            echo "Creating packages with CPack..."
            ${MXE_TARGETS}-cpack
            echo "Packaging Complete!"
            exit
        fi
        if [ ! -e openscad.exe ]; then
            echo "can't find openscad.exe. build failed. stopping."
            exit 1
        fi
        if [ ! -e winconsole/openscad.com ]; then
            echo "can't find openscad.com. build failed. stopping."
            exit 1
        fi
        cd $OPENSCADDIR
    ;;
    LINUX)
        if [ $FAKEMAKE ]; then
            echo "notexe. debugging build process" > $TARGET/openscad
        else
            make $TARGET -j$NUMCPU
        fi
    ;;
    *)
        cd $DEPLOYDIR
        VERBOSE=1 make -j$NUMCPU $TARGET
        cd $OPENSCADDIR
    ;;
esac

echo "Creating directory structure..."

case $OS in
    MACOSX)
        cd $OPENSCADDIR
        RESOURCEDIR=$DEPLOYDIR/OpenSCAD.app/Contents/Resources
    ;;
    UNIX_CROSS_WIN)
        cd $OPENSCADDIR
        RESOURCEDIR=$DEPLOYDIR/openscad-$VERSION
        rm -rf $RESOURCEDIR
        mkdir $RESOURCEDIR
    ;;
    *)
        RESOURCEDIR=openscad-$VERSION
        rm -rf $RESOURCEDIR
        mkdir $RESOURCEDIR
    ;;
esac

EXAMPLESDIR=$RESOURCEDIR/examples
FONTDIR=$RESOURCEDIR/fonts
COLORSCHEMESDIR=$RESOURCEDIR/color-schemes
SHADERSDIR=$RESOURCEDIR/shaders
TEMPLATESDIR=$RESOURCEDIR/templates
LIBRARYDIR=$RESOURCEDIR/libraries
TRANSLATIONDIR=$RESOURCEDIR/locale

if [ -n $EXAMPLESDIR ]; then
    echo $EXAMPLESDIR
    mkdir -p $EXAMPLESDIR
    rm -f examples.tar
    tar cf examples.tar examples
    cd $EXAMPLESDIR/.. && tar xf $OPENSCADDIR/examples.tar && cd $OPENSCADDIR
    rm -f examples.tar
    chmod -R 644 $EXAMPLESDIR/*/*
fi
if [ -n $FONTDIR ]; then
  echo $FONTDIR
  mkdir -p $FONTDIR
  cp -a fonts/10-liberation.conf $FONTDIR
  cp -a fonts/Liberation-2.00.1 $FONTDIR
  case $OS in
    MACOSX) 
      cp -a fonts/05-osx-fonts.conf $FONTDIR
      cp -a fonts-osx/* $FONTDIR
      ;;
    UNIX_CROSS_WIN)
      cp -a "$DEPLOYDIR"/mingw-cross-env/etc/fonts/. "$FONTDIR"
      ;;
  esac
fi
if [ -n $COLORSCHEMESDIR ]; then
  echo $COLORSCHEMESDIR
  mkdir -p $COLORSCHEMESDIR
  cp -a color-schemes/* $COLORSCHEMESDIR
fi
if [ -n $SHADERSDIR ]; then
  echo $SHADERSDIR
  mkdir -p $SHADERSDIR
  cp -a shaders/* $SHADERSDIR
fi
if [ -n $TEMPLATESDIR ]; then
  echo $TEMPLATESDIR
  mkdir -p $TEMPLATESDIR
  cp -a templates/* $TEMPLATESDIR
fi
if [ -n $LIBRARYDIR ]; then
    echo $LIBRARYDIR
    mkdir -p $LIBRARYDIR
    # exclude the .git stuff from MCAD which is a git submodule.
    # tar is a relatively portable way to do exclusion, without the
    # risks of rm
    rm -f libraries.tar
    tar cf libraries.tar --exclude=.git* libraries
    cd $LIBRARYDIR/.. && tar xf $OPENSCADDIR/libraries.tar && cd $OPENSCADDIR
    rm -f libraries.tar
    chmod -R u=rwx,go=r,+X $LIBRARYDIR/*
fi
if [ -n $TRANSLATIONDIR ]; then
  echo $TRANSLATIONDIR
  mkdir -p $TRANSLATIONDIR
  cd locale && tar cvf $OPENSCADDIR/translations.tar */*/*.mo && cd $OPENSCADDIR
  cd $TRANSLATIONDIR && tar xvf $OPENSCADDIR/translations.tar && cd $OPENSCADDIR
  rm -f translations.tar
fi

case $OS in
    MACOSX)
        cd $DEPLOYDIR
        /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSIONDATE" OpenSCAD.app/Contents/Info.plist
        macdeployqt OpenSCAD.app -no-strip
        echo "Binary created: OpenSCAD.app"
        cd $OPENSCADDIR
    ;;
    WIN)
        #package
        cp win32deps/* openscad-$VERSION
        cp $TARGET/openscad.exe openscad-$VERSION
        cp $TARGET/openscad.com openscad-$VERSION
        rm -f openscad-$VERSION.x86-$ARCH.zip
        "$ZIP" $ZIPARGS openscad-$VERSION.x86-$ARCH.zip openscad-$VERSION
        rm -rf openscad-$VERSION
        echo "Binary created: openscad-$VERSION.zip"
    ;;
    LINUX)
        # Do stuff from release-linux.sh
        mkdir openscad-$VERSION/bin
        mkdir -p openscad-$VERSION/lib/openscad
        cp scripts/openscad-linux openscad-$VERSION/bin/openscad
        cp openscad openscad-$VERSION/lib/openscad/
        if [[ $ARCH == 64 ]]; then
              gcc -o chrpath_linux -DSIZEOF_VOID_P=8 scripts/chrpath_linux.c
        else
              gcc -o chrpath_linux -DSIZEOF_VOID_P=4 scripts/chrpath_linux.c
        fi
        ./chrpath_linux -d openscad-$VERSION/lib/openscad/openscad

        QTLIBDIR=$(dirname $(ldd openscad | grep Qt5Gui | head -n 1 | awk '{print $3;}'))
        ( ldd openscad ; ldd "$QTLIBDIR"/qt5/plugins/platforms/libqxcb.so ) \
          | sed -re 's,.* => ,,; s,[\t ].*,,;' -e '/^$/d' -e '/libc\.so|libm\.so|libdl\.so|libgcc_|libpthread\.so/d' \
          | sort -u \
          | xargs cp -vt "openscad-$VERSION/lib/openscad/"
        PLATFORMDIR="openscad-$VERSION/lib/openscad/platforms/"
        mkdir -p "$PLATFORMDIR"
        cp -av "$QTLIBDIR"/qt5/plugins/platforms/libqxcb.so "$PLATFORMDIR"
        DRIDRIVERDIR=$(find /usr/lib -xdev -type d -name dri)
        if [ -d "$DRIDRIVERDIR" ]
        then
          DRILIB="openscad-$VERSION/lib/openscad/dri/"
          mkdir -p "$DRILIB"
          cp -av "$DRIDRIVERDIR"/swrast_dri.so "$DRILIB"
        fi

        strip openscad-$VERSION/lib/openscad/*
        mkdir -p openscad-$VERSION/share/appdata
        cp icons/openscad.{desktop,png,xml} openscad-$VERSION/share/appdata
        cp scripts/installer-linux.sh openscad-$VERSION/install.sh
        chmod 755 -R openscad-$VERSION/
        PACKAGEFILE=openscad-$VERSION.x86-$ARCH.tar.gz
        tar cz openscad-$VERSION > $PACKAGEFILE
        echo
        echo "Binary created:" $PACKAGEFILE
        echo
    ;;
esac
