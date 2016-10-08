#!/bin/bash
#
# This script creates a binary release of OpenSCAD. This should work
# under Mac OS X, Linux 32bit, Linux 64bit, and Linux->Win32 MXE cross-build.
#
# The script will create a file called openscad-<versionstring>.<extension> in
# the current directory.
#
# Usage: release-common.sh [-v <versionstring>]
#  -v       Version string (e.g. -v 2010.01)
#  -d       Version date (e.g. -d 2010.01.23)
#  -snapshot Build a snapshot binary (make e.g. experimental features available, build with commit info)
#
# If no version string or version date is given, todays date will be used (YYYY-MM-DD)
# If only version date is given, it will be used also as version string.

# convert end-of-line in given file from unix \n to dos/windows(TM) \r\n
# see https://kb.iu.edu/data/acux.html
lf2crlf()
{
	fname=$1
	if [ "`command -v awk`" ]; then
		echo using awk to convert end of line markers in $fname
		awk 'sub("$", "\r")' $fname > $fname".temp"
		mv $fname".temp" $fname
		return
	fi
}

printUsage()
{
  echo "Usage: $0 -v <versionstring> -d <versiondate>
  echo
  echo "  Example: $0 -v 2010.01
}

if [ ! $OPENSCADDIR ]; then
  OPENSCADDIR=$PWD
fi

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Cannot find OPENSCADDIR/openscad.pro, OPENSCADDIR should be src root "
  exit 1
fi

echo OPENSCADDIR:$OPENSCADDIR

CONFIG=deploy

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

if [ $MXE_TARGET ]; then
  echo MXE cross build environment variables detected
  echo MXE_TARGET_DIR $MXE_TARGET_DIR
  echo MXE_TARGET $MXE_TARGET
  echo MXE_LIB_TYPE $MXE_LIB_TYPE
  echo DEPLOYDIR $DEPLOYDIR
  OS=UNIX_CROSS_WIN
fi

if [ "`echo $* | grep snapshot`" ]; then
  CONFIG="$CONFIG snapshot experimental"
  OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
fi

if [ $OS ]; then
  echo "Detected OS: $OS"
else
  echo "Error: Couldn't detect OSTYPE"
  exit
fi

while getopts 'v:d:c' c
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

echo "Checking pre-requisites..."

case $OS in
    UNIX_CROSS_WIN)
        MAKENSIS=
        if [ "`command -v makensis`" ]; then
            MAKENSIS=makensis
        elif [ -e $MXE_DIR/usr/bin/i686-pc-mingw32-makensis`" ]; then
            # MXE has its own makensis, but its only available under
            # 32-bit MXE. it works the same as a native linux version so
            # its not really a 'cross' nsis
            MAKENSIS=i686-pc-mingw32-makensis
        else
            echo "makensis not found. please install nsis on your system."
            echo "(for example, on debian linux, try apt-get install nsis)"
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

echo "Building openscad-$VERSION ($VERSIONDATE) $CONFIG..."

if [ ! $NUMCPU ]; then
  echo "note: you can 'export NUMCPU=x' for multi-core compiles (x=number)";
  NUMCPU=1
fi
echo "NUMCPU: " $NUMCPU



case $OS in
    LINUX|MACOSX)
        TARGET=
        # for QT4 set QT_SELECT=4
        QT_SELECT=5
        export QT_SELECT
        ;;
    UNIX_CROSS_WIN)
        ZIP="zip"
        ZIPARGS="-r -q"
        ;;
esac

case $OS in
    UNIX_CROSS_WIN)
        cd $DEPLOYDIR
        qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug $OPENSCADDIR/openscad.pro
        cd $OPENSCADDIR
    ;;
    *)
	QMAKE="`command -v qmake-qt5`"
	if [ ! -x "$QMAKE" ]
	then
		QMAKE=qmake
	fi
	"$QMAKE" VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug openscad.pro
    ;;
esac

case $OS in
    UNIX_CROSS_WIN)
        cd $DEPLOYDIR
        make clean
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
    UNIX_CROSS_WIN)
        # kludge to enable paralell make
        touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.h
        touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.cpp
        touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.hpp
        touch -t 200012121010 $OPENSCADDIR/src/lexer_lex.cpp
        ;;
esac

echo "Building GUI binary..."

case $OS in
    UNIX_CROSS_WIN)
        # make main openscad.exe
        cd $DEPLOYDIR
        make -j$NUMCPU
        if [ ! -e ./release/openscad.exe ]; then
            echo "cant find release/openscad.exe. build failed. stopping."
            exit
        fi
        # make console pipe-able openscad.com - see winconsole.pro for info
        qmake $OPENSCADDIR/winconsole/winconsole.pro
        make
        if [ ! -e ./release/openscad.com ]; then
            echo "cant find $TARGET/openscad.com. build failed. stopping."
            exit
        fi
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
        FONTDIR=OpenSCAD.app/Contents/Resources/fonts
        TRANSLATIONDIR=OpenSCAD.app/Contents/Resources/locale
        COLORSCHEMESDIR=OpenSCAD.app/Contents/Resources/color-schemes
    ;;
    UNIX_CROSS_WIN)
        cd $OPENSCADDIR
        EXAMPLESDIR=$DEPLOYDIR/openscad-$VERSION/examples/
        LIBRARYDIR=$DEPLOYDIR/openscad-$VERSION/libraries/
        FONTDIR=$DEPLOYDIR/openscad-$VERSION/fonts/
        TRANSLATIONDIR=$DEPLOYDIR/openscad-$VERSION/locale/
        COLORSCHEMESDIR=$DEPLOYDIR/openscad-$VERSION/color-schemes/
        rm -rf $DEPLOYDIR/openscad-$VERSION
        mkdir $DEPLOYDIR/openscad-$VERSION
    ;;
    *)
        EXAMPLESDIR=openscad-$VERSION/examples/
        LIBRARYDIR=openscad-$VERSION/libraries/
        FONTDIR=openscad-$VERSION/fonts/
        TRANSLATIONDIR=openscad-$VERSION/locale/
        COLORSCHEMESDIR=openscad-$VERSION/color-schemes/
        rm -rf openscad-$VERSION
        mkdir openscad-$VERSION
    ;;
esac

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
      cp -a $MXE_TARGET_DIR/etc/fonts/. "$FONTDIR"
      ;;
  esac
fi
if [ -n $COLORSCHEMESDIR ]; then
  echo $COLORSCHEMESDIR
  mkdir -p $COLORSCHEMESDIR
  cp -a color-schemes/* $COLORSCHEMESDIR
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

echo "Creating archive.."

case $OS in
    MACOSX)
        /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSIONDATE" OpenSCAD.app/Contents/Info.plist
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
        rm -f openscad-$VERSION.x86-$ARCH.zip
        "$ZIP" $ZIPARGS openscad-$VERSION.x86-$ARCH.zip openscad-$VERSION
        rm -rf openscad-$VERSION
        echo "Binary created: openscad-$VERSION.zip"
        ;;
    UNIX_CROSS_WIN)
        cd $OPENSCADDIR
        cd $DEPLOYDIR
        ZIPFILE=$DEPLOYDIR/OpenSCAD-$VERSION-x86-$ARCH.zip
        INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-x86-$ARCH-Installer.exe

        #package
        if [ $MXE_LIB_TYPE = "shared" ]; then
          flprefix=$MXE_TARGET_DIR/bin
          echo Copying dlls for shared library build
          echo from $flprefix
          echo to $DEPLOYDIR/$TARGET
          flist=
          # fl="$fl opengl.dll" # use Windows version?
          # fl="$fl libmpfr.dll" # does not exist
          fl="$fl libgmp-10.dll"
          fl="$fl libgmpxx-4.dll"
          fl="$fl libboost_filesystem-mt.dll"
          fl="$fl libboost_program_options-mt.dll"
          fl="$fl libboost_regex-mt.dll"
          fl="$fl libboost_chrono-mt.dll"
          fl="$fl libboost_system-mt.dll"
          fl="$fl libboost_thread_win32-mt.dll"
          fl="$fl libCGAL.dll"
          fl="$fl libCGAL_Core.dll"
          fl="$fl GLEW.dll"
          fl="$fl libglib-2.0-0.dll"
          fl="$fl libopencsg-1.dll"
          fl="$fl libharfbuzz-0.dll"
          # fl="$fl libharfbuzz-gobject-0.dll" # ????
          fl="$fl libfontconfig-1.dll"
          fl="$fl libexpat-1.dll"
          fl="$fl libbz2.dll"
          fl="$fl libintl-8.dll"
          fl="$fl libiconv-2.dll"
          fl="$fl libfreetype-6.dll"
          fl="$fl libpcre16-0.dll"
          fl="$fl zlib1.dll"
          fl="$fl libpng16-16.dll"
          fl="$fl icudt54.dll"
          fl="$fl icudt.dll"
          fl="$fl icuin.dll"
          fl="$fl libstdc++-6.dll"
          fl="$fl ../qt5/lib/qscintilla2.dll"
          fl="$fl ../qt5/bin/Qt5PrintSupport.dll"
          fl="$fl ../qt5/bin/Qt5Core.dll"
          fl="$fl ../qt5/bin/Qt5Gui.dll"
          fl="$fl ../qt5/bin/Qt5OpenGL.dll"
          #  fl="$fl ../qt5/bin/QtSvg4.dll" # why is this here?
          fl="$fl ../qt5/bin/Qt5Widgets.dll"
          fl="$fl ../qt5/bin/Qt5PrintSupport.dll"
          fl="$fl ../qt5/bin/Qt5PrintSupport.dll"
          for dllfile in $fl; do
            if [ -e $flprefix/$dllfile ]; then
                echo $flprefix/$dllfile
                cp $flprefix/$dllfile $DEPLOYDIR/$TARGET/
            else
                echo cannot find $flprefix/$dllfile
                echo stopping build.
                exit 1
            fi
          done
        fi

        echo "Copying main binary .exe, .com, and dlls"
        echo "from $DEPLOYDIR/$TARGET"
        echo "to $DEPLOYDIR/openscad-$VERSION"
        TMPTAR=$DEPLOYDIR/tmpmingw.$ARCH.$MXE_LIB_TYPE.tar
        cd $DEPLOYDIR
        cd $TARGET
        tar cvf $TMPTAR --exclude=winconsole.o .
        cd $DEPLOYDIR
        cd ./openscad-$VERSION
        tar xvf $TMPTAR
        cd $DEPLOYDIR
        rm -f $TMPTAR


        echo "Creating binary zip package"
        rm -f OpenSCAD-$VERSION.x86-$ARCH.zip
        "$ZIP" $ZIPARGS $ZIPFILE openscad-$VERSION
        cd $OPENSCADDIR
        echo "Binary zip package created"

        echo "Creating installer"
        echo "Copying NSIS files to $DEPLOYDIR/openscad-$VERSION"
        cp ./scripts/installer$ARCH.nsi $DEPLOYDIR/openscad-$VERSION/installer_arch.nsi
        cp ./scripts/installer.nsi $DEPLOYDIR/openscad-$VERSION/
        cp ./scripts/mingw-file-association.nsh $DEPLOYDIR/openscad-$VERSION/
        cp ./scripts/x64.nsh $DEPLOYDIR/openscad-$VERSION/
        cp ./scripts/LogicLib.nsh $DEPLOYDIR/openscad-$VERSION/
        cd $DEPLOYDIR/openscad-$VERSION
        NSISDEBUG=-V2
        # NSISDEBUG=      # leave blank for full log
        echo $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
        $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
        cp $DEPLOYDIR/openscad-$VERSION/openscad_setup.exe $INSTFILE
        cd $OPENSCADDIR

        if [ -e $ZIPFILE ]; then
            echo "Zipfile created:" $ZIPFILE
        else
            echo "zipfile creation failed. stopping"
            exit 1
        fi
        if [ -e $INSTFILE ]; then
            echo "Installer created:" $INSTFILE
        else
            echo "installer creation failed. stopping"
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
