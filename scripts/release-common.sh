#!/bin/bash
#
# This script creates a binary release of OpenSCAD. This should work
# under Mac OS X, Linux 32, Linux 64, and Linux->Win32 MXE cross-build.
# Windows under msys has not been tested recently.
#
# The script will create a file called openscad-<versionstring>.<extension> in
# the current directory (or under ./mingw32)
#
# Usage: release-common.sh [-v <versionstring>] [-c] [-x32]
#  -v       Version string (e.g. -v 2010.01)
#  -c       Build with commit info
#  -mingw32 Cross-compile for win32 using MXE
#
# If no version string is given, todays date will be used (YYYY-MM-DD)
# If no make target is given, release will be used on Windows, none one Mac OS X
#
# The commit info will extracted from git and be passed to qmake as OPENSCAD_COMMIT
# to identify a build in the about box.
#
# The mingw32 cross compile depends on the MXE cross-build tools. Please
# see the README.md file on how to install these dependencies.

printUsage()
{
  echo "Usage: $0 -v <versionstring> -c -mingw32
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
fi

if [ "`echo $* | grep mingw32`" ]; then
  OS=LINXWIN
fi

if [ $OS ]; then
  echo "Detected OS: $OS"
else
  echo "Error: Couldn't detect OSTYPE"
  exit
fi

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


echo "Checking pre-requisites..."

case $OS in
    LINXWIN)
        MAKENSIS=

        if [ "`command -v makensis`" ]; then
            MAKENSIS=makensis
        elif [ "`command -v i686-pc-mingw32-makensis`" ]; then
            MAKENSIS=i686-pc-mingw32-makensis
        else
            echo "makensis not found. please install nsis"
            exit 1
        fi
        echo NSIS makensis found: $MAKENSIS
    ;;
esac

if [ ! -e $OPENSCADDIR/libraries/MCAD/__init__.py ]; then
  echo "Downloading MCAD"
  git submodule init
  git submodule update
else
  echo "MCAD found:" $OPENSCADDIR/libraries/MCAD
fi

if [ -d .git ]; then
  git submodule update
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

if [ ! $NUMCPU ]; then
  echo "note: you can 'export NUMCPU=x' for multi-core compiles (x=number)";
  NUMCPU=2
fi

case $OS in
    LINXWIN)
        # dont use paralell builds, it can error-out on parser_yacc.

        # make main openscad.exe
        cd $DEPLOYDIR && make $TARGET

        # make console pipe-able openscad.com - see winconsole.pri for info
        i686-pc-mingw32-qmake CONFIG+=winconsole ../openscad.pro
        make

        cd $OPENSCADDIR
    ;;
    *)
        make -j$NUMCPU $TARGET
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

if [ -n $EXAMPLESDIR ]; then
  echo $EXAMPLESDIR
  mkdir -p $EXAMPLESDIR
  cp examples/* $EXAMPLESDIR
  chmod -R 644 $EXAMPLESDIR/*
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
        cp $TARGET/openscad.com openscad-$VERSION
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
        cp $TARGET/openscad.com openscad-$VERSION
        rm -f OpenSCAD-$VERSION.zip
        "$ZIP" $ZIPARGS OpenSCAD-$VERSION.zip openscad-$VERSION
        cd $OPENSCADDIR
        echo "Binary package created"

        echo "Creating installer"
        echo "Copying NSIS files to $DEPLOYDIR/openscad-$VERSION"
        cp ./scripts/installer.nsi $DEPLOYDIR/openscad-$VERSION
        cp ./scripts/mingw-file-association.nsh $DEPLOYDIR/openscad-$VERSION
        cd $DEPLOYDIR/openscad-$VERSION
        NSISDEBUG=-V2
        # NSISDEBUG=      # leave blank for full log
        echo $MAKENSIS $NSISDEBUG installer.nsi
        $MAKENSIS $NSISDEBUG installer.nsi
        cp $DEPLOYDIR/openscad-$VERSION/openscad_setup.exe $DEPLOYDIR/OpenSCAD-$VERSION-Installer.exe
        cd $OPENSCADDIR

        BINFILE=$DEPLOYDIR/OpenSCAD-$VERSION.zip
        INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-Installer.exe
        if [ -e $BINFILE ]; then
            if [ -e $INSTFILE ]; then
                echo
                echo "Binary created:" $BINFILE
                echo "Installer created:" $INSTFILE
                echo
            else
              echo "Build failed. Cannot find" $INSTFILE
              exit 1
            fi
        else
          echo "Build failed. Cannot find" $BINFILE
          exit 1
        fi
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
