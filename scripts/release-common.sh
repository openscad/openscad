#!/bin/bash
#
# This script creates a binary release of OpenSCAD. This should work
# under Mac OS X, Linux 32bit, Linux 64bit, and Linux->Win32 MXE cross-build.
# Windows under msys has not been tested recently.
#
# The script will create a file called openscad-<versionstring>.<extension> in
# the current directory (or under ./mingw32 or ./mingw64)
#
# Usage: release-common.sh [-v <versionstring>] [-c] [-mingw[32|64]] [-tests]
#  -v       Version string (e.g. -v 2010.01)
#  -d       Version date (e.g. -d 2010.01.23)
#  -mingw32 Cross-compile for win32 using MXE
#  -mingw64 Cross-compile for win64 using MXE
#  -snapshot Build a snapshot binary (make e.g. experimental features available, build with commit info)
#  -tests   Build additional package containing the regression tests
#
# If no version string or version date is given, todays date will be used (YYYY-MM-DD)
# If only version date is given, it will be used also as version string.
# If no make target is given, release will be used on Windows, none one Mac OS X
#
# The mingw cross compile depends on the MXE cross-build tools. Please
# see the README.md file on how to install these dependencies. To debug
# the mingw-cross build process, set env var FAKEMAKE=1 to fake-make the
# .exe files
#

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
	echo 'warning- cant change eol to cr eol'
}

printUsage()
{
  echo "Usage: $0 -v <versionstring> -d <versiondate> -c -mingw32
  echo
  echo "  Example: $0 -v 2010.01
}

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 1
fi

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

if [ "`echo $* | grep mingw32`" ]; then
  OS=UNIX_CROSS_WIN
  ARCH=32
  echo Mingw-cross build using ARCH=32
fi

if [ "`echo $* | grep mingw64`" ]; then
  OS=UNIX_CROSS_WIN
  ARCH=64
  echo Mingw-cross build using ARCH=64
fi

if [ "`echo $* | grep snapshot`" ]; then
  CONFIG="$CONFIG experimental"
  OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
fi

BUILD_TESTS=
if [ "`echo $* | grep tests`" ]; then
  BUILD_TESTS=1
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

if [ $FAKEMAKE ]; then
  echo 'fake make on:' $FAKEMAKE
else
  FAKEMAKE=
fi

echo "Checking pre-requisites..."

case $OS in
    UNIX_CROSS_WIN)
        MAKENSIS=
        if [ "`command -v makensis`" ]; then
            MAKENSIS=makensis
        elif [ "`command -v i686-pc-mingw32-makensis`" ]; then
            # we cant find systems nsis so look for the MXE's version.
            # MXE has its own makensis, but its only available under
            # 32-bit MXE. note that the cross-version in theory works
            # the same as the linux version so we can use them, in
            # theory, interchangeably. its not really a 'cross' nsis
            # todo - when doing 64 bit mingw build, see if we can call
            # 32bit nsis here.
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
    WIN) 
        export QTDIR=/c/devmingw/qt2009.03
        export QTMAKESPEC=win32-g++
        export PATH=$PATH:/c/devmingw/qt2009.03/bin:/c/devmingw/qt2009.03/qt/bin
        ZIP="/c/Program Files/7-Zip/7z.exe"
        ZIPARGS="a -tzip"
        TARGET=release
        ;;
    UNIX_CROSS_WIN) 
        . ./scripts/setenv-mingw-xbuild.sh $ARCH
        TARGET=release
        ZIP="zip"
        ZIPARGS="-r -q"
        ;;
esac


case $OS in
    UNIX_CROSS_WIN)
        cd $DEPLOYDIR && qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG+=mingw-cross-env CONFIG-=debug ../openscad.pro
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
        make clean ## comment out for test-run
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
        if [ $FAKEMAKE ]; then
            echo "notexe. debugging build process" > $TARGET/openscad.exe
        else
            make $TARGET -j$NUMCPU
        fi
        if [ ! -e $TARGET/openscad.exe ]; then
            echo "cant find $TARGET/openscad.exe. build failed. stopping."
            exit
        fi
        # make console pipe-able openscad.com - see winconsole.pro for info
        qmake ../winconsole/winconsole.pro
        make
        if [ ! -e $TARGET/openscad.com ]; then
            echo "cant find $TARGET/openscad.com. build failed. stopping."
            exit
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
        make -j$NUMCPU $TARGET
    ;;
esac

if [[ $? != 0 ]]; then
  echo "Error building OpenSCAD. Aborting."
  exit 1
fi

echo "Building test suite..."

if [ $BUILD_TESTS ]; then
  case $OS in
    UNIX_CROSS_WIN)
        TESTBUILD_MACHINE=x86_64-w64-mingw32
        # dont use build-machine trilpe in TESTBINDIR because the 'mingw32'
        # will confuse people who are on 64 bit machines
        TESTBINDIR=tests-build
        export TESTBUILD_MACHINE
        export TESTBINDIR
        if [[ $ARCH == 32 ]]; then
            TESTBUILD_MACHINE=i686-pc-mingw32
        fi
        cd $DEPLOYDIR
        mkdir $TESTBINDIR
        cd $TESTBINDIR
        cmake $OPENSCADDIR/tests/ \
          -DCMAKE_TOOLCHAIN_FILE=../tests/CMingw-cross-env.cmake \
          -DMINGW_CROSS_ENV_DIR=$MXEDIR \
          -DMACHINE=$TESTBUILD_MACHINE
        if [ $FAKEMAKE ]; then
            echo "notexe. debugging build process" > openscad_nogui.exe
        else
            make -j$NUMCPU
        fi
        if [ ! -e openscad_nogui.exe ]; then
            echo 'test cross-build failed'
            exit 1
        fi
        cd $OPENSCADDIR
    ;;
    *)
        echo 'test suite build not implemented for osx/linux'
    ;;
  esac
fi # BUILD_TESTS

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
  cp -a fonts/* $FONTDIR
  case $OS in
    MACOSX) 
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
        BINFILE=$DEPLOYDIR/OpenSCAD-$VERSION-x86-$ARCH.zip
        INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-x86-$ARCH-Installer.exe

        #package
        echo "Creating binary zip package"
        cp $TARGET/openscad.exe openscad-$VERSION
        cp $TARGET/openscad.com openscad-$VERSION
        rm -f OpenSCAD-$VERSION.x86-$ARCH.zip
        "$ZIP" $ZIPARGS $BINFILE openscad-$VERSION
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
        echo $MAKENSIS $NSISDEBUG installer.nsi
        $MAKENSIS $NSISDEBUG installer.nsi
        cp $DEPLOYDIR/openscad-$VERSION/openscad_setup.exe $INSTFILE
        cd $OPENSCADDIR

        if [ -e $BINFILE ]; then
            if [ -e $INSTFILE ]; then
                echo
                echo "Binary created:" $BINFILE
                echo "Installer created:" $INSTFILE
                echo
            else
                echo "Build failed. Cannot find" $INSTFILE
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




if [ $BUILD_TESTS ]; then
  echo "Creating regression tests package..."
  case $OS in
    MACOSX)
        echo 'building regression test package on OSX not implemented'
    ;;
    WIN)
        echo 'building regression test package on Win not implemented'
        ;;
    UNIX_CROSS_WIN)
        # Build a .zip file containing all the files we need to run a
        # ctest on Windows(TM). For the sake of simplicity, we do not
        # create an installer for the tests.

        echo "Copying files..."
        cd $OPENSCADDIR
        # This copies a lot of unnecessary stuff but that's OK.
        # as above, we use tar as a somewhat portable way to do 'exclude'
        # while copying.
        rm -f ./ostests.tar
       	for subdir in tests testdata libraries examples doc; do
          tar prvf ./ostests.tar --exclude=.git* --exclude=*/mingw64/* --exclude=*/mingw32/* --exclude=*.cc.obj --exclude=*.a $subdir
        done
        cd $DEPLOYDIR
        tar prvf $OPENSCADDIR/ostests.tar --exclude=.git* --exclude=*/mingw* --exclude=*.cc.obj --exclude=*.a $TESTBINDIR

        cd $DEPLOYDIR
        if [ -e ./OpenSCAD-Tests-$VERSION ]; then
          rm -rf ./OpenSCAD-Tests-$VERSION
        fi
        mkdir OpenSCAD-Tests-$VERSION
        cd OpenSCAD-Tests-$VERSION
        tar pxf $OPENSCADDIR/ostests.tar
        rm -f $OPENSCADDIR/ostests.tar

        # Now we have the basic files copied into our tree that will become
        # our .zip file. We also want to move some files around for easier
        # access for the user:
        cd $DEPLOYDIR
        cd ./OpenSCAD-Tests-$VERSION
        echo "Copying files for ease of use when running from cmdline"
        cp -v ./tests/OpenSCAD_Test_Console.py .
        cp -v ./tests/WinReadme.txt .
        cp -v ./tests/mingw_convert_ctest.py ./$TESTBINDIR
	cp -v ./tests/mingwcon.bat ./$TESTBINDIR

        echo "Creating mingw_cross_info.py file"
        cd $DEPLOYDIR
        cd ./OpenSCAD-Tests-$VERSION
        cd $TESTBINDIR
        if [ -e ./mingw_cross_info.py ]; then
          rm -f ./mingw_cross_info.py
        fi
        echo "# created automatically by release-common.sh from within linux " >> mingw_cross_info.py
        echo "linux_abs_basedir='"$OPENSCADDIR"'" >> mingw_cross_info.py
        echo "linux_abs_builddir='"$DEPLOYDIR/$TESTBINDIR"'" >> mingw_cross_info.py
        echo "bindir='"$TESTBINDIR"'" >> mingw_cross_info.py
        # fixme .. parse CTestTestfiles to find linux+convert python strings
        # or have CMake itself dump them during it's cross build cmake call
        echo "linux_python='"`which python`"'" >> mingw_cross_info.py
        # note- this has to match the CMakeLists.txt line that sets the
        # convert executable... and CMingw-cross-env.cmake's skip-imagemagick
        # setting. what a kludge!
        echo "linux_convert='/bin/echo'" >> mingw_cross_info.py
        echo "win_installdir='OpenSCAD_Tests_"$VERSIONDATE"'" >> mingw_cross_info.py

	echo 'Converting linefeed to carriage-return+linefeed'
	for textfile in `find . | grep txt$`; do lf2crlf $textfile; done
	for textfile in `find . | grep py$`; do lf2crlf $textfile; done
	for textfile in `find . | grep cmake$`; do lf2crlf $textfile; done
	for textfile in `find . | grep bat$`; do lf2crlf $textfile; done

        # Test binaries can be hundreds of megabytes due to debugging info.
        # By default, we strip that. In most cases we wont need it and it
        # causes too many problems to have >100MB files.
        echo "stripping .exe binaries"
        cd $DEPLOYDIR
        cd ./OpenSCAD-Tests-$VERSION
        cd $TESTBINDIR
        if [ "`command -v $TESTBUILD_MACHINE'-strip' `" ]; then
            for exefile in *exe; do
                ls -sh $exefile
                echo $TESTBUILD_MACHINE'-strip' $exefile
                $TESTBUILD_MACHINE'-strip' $exefile
                ls -sh $exefile
            done
        fi

        # Build the actual .zip archive based on the file tree we've built above
        cd $DEPLOYDIR
        ZIPFILE=OpenSCAD-Tests-$VERSION-x86-$ARCH.zip
        echo "Creating binary zip package for Tests:" $ZIPFILE
        rm -f ./$ZIPFILE
        "$ZIP" $ZIPARGS $ZIPFILE OpenSCAD-Tests-$VERSION

        if [ -e $ZIPFILE ]; then
            echo "ZIP package created:" `pwd`/$ZIPFILE
        else
            echo "Build of Regression Tests package failed. Cannot find" `pwd`/$ZIPFILE
            exit 1
        fi
        cd $OPENSCADDIR
        ;;
    LINUX)
        echo 'building regression test package on linux not implemented'
        ;;
  esac
else
  echo "Not building regression tests package"
fi # BUILD_TESTS

