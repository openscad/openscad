#!/bin/bash
#
# This script creates a binary release of OpenSCAD.
# This should work under Mac OS X, Windows (msys), and Linux cross-compiling
# for windows using mingw-cross-env (use like: OSTYPE=mingw-cross-env release-common.sh).
# Linux support pending.
# The script will create a file called openscad-<versionstring>.zip
# in the current directory (or in the $DEPLOYDIR of a mingw cross build)
#
# Usage: release-common.sh [-v <versionstring>] [-c]
#  -v   Version string (e.g. -v 2010.01)
#  -c   Build with commit info
#
# If no version string is given, todays date will be used (YYYY-MM-DD)
# If no make target is given, release will be used on Windows, none one Mac OS X
#
# The commit info will extracted from git and be passed to qmake as OPENSCAD_COMMIT
# to identify a build in the about box.

printUsage()
{
  echo "Usage: $0 -v <versionstring> -c
  echo
  echo "  Example: $0 -v 2010.01
}

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 1
fi

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
  echo "Detected ARCH: $ARCH"
elif [[ $OSTYPE == "mingw-cross-env" ]]; then
  OS=LINXWIN
fi

echo "Detected OS: $OS"

while getopts 'v:c' c
do
  case $c in
    v) VERSION=$OPTARG;;
    c) OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
  esac
done

if test -z "$VERSION"; then
    VERSION=`date "+%Y.%m.%d"`
fi

echo "Building openscad-$VERSION $CONFIGURATION..."

case $OS in
    LINUX|MACOSX) 
        CONFIG=deploy
        TARGET=
        ;;
    WIN) 
        unset CONFIG
        export QTDIR=/c/devmingw/qt2009.03
        export QTMAKESPEC=win32-g++
        export PATH=$PATH:/c/devmingw/qt2009.03/bin:/c/devmingw/qt2009.03/qt/bin
        ZIP="/c/Program Files/7-Zip/7z.exe"
        ZIPARGS="a -tzip"
        TARGET=release
        ;;
    LINXWIN) 
        unset CONFIG
        . ./scripts/setenv-mingw-xbuild.sh
        TARGET=release
        ZIP="zip"
        ZIPARGS="-r"
        ;;
esac


case $OS in
    LINXWIN)
        cd $DEPLOYDIR && i686-pc-mingw32-qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+=$CONFIG CONFIG+=mingw-cross-env CONFIG-=debug ../openscad.pro
        cd $OPENSCADDIR
    ;;
    *)
        qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+=$CONFIG CONFIG-=debug openscad.pro
    ;;
esac

case $OS in
    LINXWIN)
        cd $DEPLOYDIR && make -s clean
        cd $OPENSCADDIR
    ;;
    *)
        make -s clean
    ;;
esac

case $OS in
    MACOSX) 
        rm -rf OpenSCAD.app
        ;;
    WIN)
        #if the following files are missing their tried removal stops the build process on msys
        touch -t 200012121010 parser_yacc.h parser_yacc.cpp lexer_lex.cpp
        ;;
esac

case $OS in
    LINXWIN)
        # make -j2 sometimes has problems with parser_yacc
        cd $DEPLOYDIR && make $TARGET
        cd $OPENSCADDIR
    ;;
    *)
        make -j2 $TARGET
    ;;
esac

if [[ $? != 0 ]]; then
  echo "Error building OpenSCAD. Aborting."
  exit 1
fi

echo "Creating directory structure..."

case $OS in
    MACOSX)
        EXAMPLESDIR=OpenSCAD.app/Contents/Resources/examples
        LIBRARYDIR=OpenSCAD.app/Contents/Resources/libraries
    ;;
    LINXWIN)
        EXAMPLESDIR=$DEPLOYDIR/openscad-$VERSION/examples/
        LIBRARYDIR=$DEPLOYDIR/openscad-$VERSION/libraries/
        rm -rf $DEPLOYDIR/openscad-$VERSION
        mkdir $DEPLOYDIR/openscad-$VERSION
		;;
    *)
        EXAMPLESDIR=openscad-$VERSION/examples/
        LIBRARYDIR=openscad-$VERSION/libraries/
        rm -rf openscad-$VERSION
        mkdir openscad-$VERSION
    ;;
esac

if [ ! -e $OPENSCADDIR/libraries/MCAD/__init__.py ]; then
  echo "Downloading MCAD"
  git submodule init
  git submodule update
fi

if [ -n $EXAMPLESDIR ]; then
  echo $EXAMPLESDIR
  mkdir -p $EXAMPLESDIR
  cp examples/* $EXAMPLESDIR
  chmod -R 644 $EXAMPLESDIR/*
fi
if [ -n $LIBRARYDIR ]; then
  echo $LIBRARYDIR
  mkdir -p $LIBRARYDIR
  cp -R libraries/* $LIBRARYDIR
  chmod -R u=rwx,go=r,+X $LIBRARYDIR/*
  rm -rf `find $LIBRARYDIR -name ".git"`
fi

echo "Creating archive.."

case $OS in
    MACOSX)
        macdeployqt OpenSCAD.app -dmg -no-strip
        mv OpenSCAD.dmg OpenSCAD-$VERSION.dmg
        hdiutil internet-enable -yes -quiet OpenSCAD-$VERSION.dmg
        echo "Binary created: OpenSCAD-$VERSION.dmg"
    ;;
    WIN)
        #package
        cp win32deps/* openscad-$VERSION
        cp $TARGET/openscad.exe openscad-$VERSION
        rm -f openscad-$VERSION.zip
        "$ZIP" $ZIPARGS openscad-$VERSION.zip openscad-$VERSION
        rm -rf openscad-$VERSION
        echo "Binary created: openscad-$VERSION.zip"
        ;;
    LINXWIN)
        #package
        echo "Creating binary package"
        cd $DEPLOYDIR
        cp $TARGET/openscad.exe openscad-$VERSION
        rm -f OpenSCAD-$VERSION.zip
        "$ZIP" $ZIPARGS OpenSCAD-$VERSION.zip openscad-$VERSION
        rm -rf ./openscad-$VERSION
        cd $OPENSCADDIR
        echo "Binary package created"

        echo "Creating installer"
        ./scripts/mingw-x-build-installer.sh
        cp $DEPLOYDIR/openscad_setup.exe $DEPLOYDIR/OpenSCAD-$VERSION-Installer.exe
				echo "Installer created"

        echo
        echo "Binary created: $DEPLOYDIR/OpenSCAD-$VERSION.zip"
        echo "Installer created: $DEPLOYDIR/OpenSCAD-$VERSION-Installer.exe"
        echo
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
        ldd openscad | sed -re 's,.* => ,,; s,[\t ].*,,;' -e '/Qt|boost/ { p; d; };' \
            -e '/lib(icu.*|stdc.*|audio|CGAL|GLEW|opencsg|png|gmp|gmpxx|mpfr)\.so/ { p; d; };' \
            -e 'd;' | xargs cp -vt openscad-$VERSION/lib/openscad/
        strip openscad-$VERSION/lib/openscad/*
        cp scripts/installer-linux.sh openscad-$VERSION/install.sh
        chmod 755 -R openscad-$VERSION/
        PACKAGEFILE=openscad-$VERSION.x86-$ARCH.tar.gz
        tar cz openscad-$VERSION > $PACKAGEFILE
        echo
        echo "Binary created:" $PACKAGEFILE
        echo
        ;;
esac
