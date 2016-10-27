#!/usr/bin/env bash
#
# This script creates a binary release of OpenSCAD. This should work
# under Mac OS X, Linux 32bit, Linux 64bit, and Linux->Win32 MXE cross-build.
#
# The script will create a file called openscad-<versionstring>.<extension>
# in the current directory.
#
# For cross build, do 'source scripts/setenv-mingw-xbuild.sh [32|64]' before
# running this. The result will be under bin/machine-triple aka $DEPLOYDIR
#
# Usage: release-common.sh [-v <versionstring>] [-dryrun] [-snapshot]
#  -v       Version string (e.g. -v 2010.01)
#  -d       Version date (e.g. -d 2010.01.23)
#  -dryrun  Quickly build a dummy openscad.exe file to test this release script
#  -snapshot Build a snapshot binary (make e.g. experimental features available, build with commit info)
#
# If no version string or version date is given, todays date will be used (YYYY-MM-DD)
# If only version date is given, it will be used also as version string.

paralell_note()
{
  if [ ! $NUMCPU ]; then
    echo "note: you can 'export NUMCPU=x' for multi-core compiles (x=number)";
    NUMCPU=1
  fi
}

run()
{
  # run() calls function $1_generic, or a specialized version $1_$ostype
  # stackoverflow.com/questions/85880/determine-if-a-function-exists-in-bash
  runfunc1=`echo $1"_"$OPENSCAD_BUILD_TARGET_OSTYPE`
  runfunc2=`echo $1_generic`
  if [ "`type -t $runfunc1 | grep function`" ]; then
    echo "calling $runfunc1"
    eval $runfunc1
  elif [ "`type -t $runfunc2 | grep function`" ]; then
    echo "calling $runfunc2"
    eval $runfunc2
  else
    echo "neither $runfunc2 nor $runfunc1 were defined, skipping."
  fi
}

check_prereq_mxe()
{
  MAKENSIS=
  if [ "`command -v makensis`" ]; then
    MAKENSIS=makensis
  elif [ "`command -v i686-pc-mingw32-makensis`" ]; then
    # we cant find systems nsis so look for the MXE's 32 bit version.
    MAKENSIS=i686-pc-mingw32-makensis
  else
    echo "makensis not found. please install nsis on your system."
    echo "(for example, on debian linux, try apt-get install nsis)"
    exit 1
  fi
}

update_mcad_generic()
{
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
}

verify_binary_generic()
{
  cd $BUILDDIR
  if [ ! -e ./$MAKE_TARGET/openscad ]; then
    echo "cant find ./$MAKE_TARGET/openscad. build failed. stopping."
    exit 1
  fi
  cd $OPENSCADDIR
}

verify_binary_darwin()
{
  cd $BUILDDIR
  if [ ! -f ./OpenSCAD.app/Contents/MacOS/OpenSCAD ]; then
    echo "cant find ./OpenSCAD.app/Contents/MacOS/OpenSCAD. build failed. stopping."
    exit 1
  fi
  cd $OPENSCADDIR
}

verify_binary_mxe()
{
  cd $BUILDDIR
  if [ ! -e $MAKE_TARGET/openscad.com ]; then
    echo "cant find $MAKE_TARGET/openscad.com. build failed. stopping."
    exit 1
  fi
  if [ ! -e $MAKE_TARGET/openscad.exe ]; then
    echo "cant find $MAKE_TARGET/openscad.exe. build failed. stopping."
    exit 1
  fi
  cd $OPENSCADDIR
}

verify_binary_linux()
{
  if [ ! -e $BUILDDIR/$MAKE_TARGET/openscad ]; then
    echo "cant find $MAKE_TARGET/openscad. build failed. stopping."
    exit 1
  fi
}

setup_directories_generic()
{
  bprefix=$BUILDDIR/openscad-$VERSION
  EXAMPLESDIR=$bprefix/examples/
  LIBRARYDIR=$bprefix/libraries/
  FONTDIR=$bprefix/fonts/
  TRANSLATIONDIR=$bprefix/locale/
  COLORSCHEMESDIR=$bprefix/color-schemes/
  if [ -e $bprefix ]; then
    rm -rf $bprefix
  fi
  mkdir $bprefix
}

setup_directories_darwin()
{
  EXAMPLESDIR=OpenSCAD.app/Contents/Resources/examples
  LIBRARYDIR=OpenSCAD.app/Contents/Resources/libraries
  FONTDIR=OpenSCAD.app/Contents/Resources/fonts
  TRANSLATIONDIR=OpenSCAD.app/Contents/Resources/locale
  COLORSCHEMESDIR=OpenSCAD.app/Contents/Resources/color-schemes
}

copy_examples_generic()
{
  cd $OPENSCADDIR
  echo $EXAMPLESDIR
  mkdir -p $EXAMPLESDIR
  rm -f examples.tar
  tar cf examples.tar examples
  ls -l examples.tar
  cd $EXAMPLESDIR/.. && tar xf $OPENSCADDIR/examples.tar && cd $OPENSCADDIR
  rm -f examples.tar
  chmod -R 644 $EXAMPLESDIR/*/*
}

copy_fonts_generic()
{
  echo $FONTDIR
  mkdir -p $FONTDIR
  cp -a fonts/10-liberation.conf $FONTDIR
  cp -a fonts/Liberation-2.00.1 $FONTDIR
}

copy_fonts_darwin()
{
  copy_fonts_generic
  cp -a fonts/05-osx-fonts.conf $FONTDIR
  cp -a fonts-osx/* $FONTDIR
}

copy_fonts_mxe()
{
  copy_fonts_generic
  cp -a $MXETARGETDIR/etc/fonts/ "$FONTDIR"
}

copy_colorschemes_generic()
{
  echo $COLORSCHEMESDIR
  mkdir -p $COLORSCHEMESDIR
  cp -a color-schemes/* $COLORSCHEMESDIR
}

copy_mcad_generic()
{
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
}

copy_translations_generic()
{
  echo $TRANSLATIONDIR
  mkdir -p $TRANSLATIONDIR
  cd locale && tar cvf $OPENSCADDIR/translations.tar */*/*.mo && cd $OPENSCADDIR
  cd $TRANSLATIONDIR && tar xvf $OPENSCADDIR/translations.tar && cd $OPENSCADDIR
  rm -f translations.tar
}

create_archive_darwin()
{
  /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSIONDATE" OpenSCAD.app/Contents/Info.plist
  macdeployqt OpenSCAD.app -dmg -no-strip
  mv OpenSCAD.dmg OpenSCAD-$VERSION.dmg
  hdiutil internet-enable -yes -quiet OpenSCAD-$VERSION.dmg
  echo "Binary created: OpenSCAD-$VERSION.dmg"
}

copyfail()
{
  if [ ! -e $1 ]; then
    echo "$1 not found"
    echo stopping build.
    exit 1
  fi
  if [ ! -d $2 ]; then
    echo "$2 not an existing directory"
    echo stopping build.
    exit 1
  fi
  echo $1 $2
  cp $1 $2
}

create_archive_msys()
{
  cd $OPENSCADDIR
  cd $BUILDDIR

  echo "QT5 deployment, dll and other files copying..."
  windeployqt $MAKE_TARGET/openscad.exe

  bits=64
  if [ $OPENSCAD_BUILD_TARGET_ARCH = i686 ]; then
    bits=32
  fi

  flprefix=/mingw$bits/bin/
  echo MSYS2, dll copying...
  echo from $flprefix
  echo to $BUILDDIR/$MAKE_TARGET
  fl=
  boostlist="filesystem program_options regex system thread"
  liblist="mpfr-4 gmp-10 gmpxx-4 opencsg-1 harfbuzz-0 harfbuzz-gobject-0 glib-2.0-0"
  liblist="$liblist CGAL CGAL_Core fontconfig-1 expat-1 bz2-1 intl-8 iconv-2"
  liblist="$liblist pcre16-0 png16-16 icudt55 freetype-6"
  dlist="glew32 opengl qscintilla2 zlib1 jsiosdjfiosdjf Qt5PrintSupport"
  for file in $boostlist; do fl="$fl libboost_"$file"-mt.dll"; done
  for file in $liblist;   do fl="$fl lib"$file".dll"; done
  for file in $dlist;     do fl="$fl "$file".dll"; done

  for dllfile in $fl; do
    copyfail $flprefix/$dllfile /$BUILDDIR/$MAKE_TARGET/
  done


  ARCH_INDICATOR=Msys2-x86-64
  if [ $OPENSCAD_BUILD_TARGET_ARCH = i686 ]; then
    ARCH_INDICATOR=Msys2-x86-32
  fi
  BINFILE=$BUILDDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR.zip
  INSTFILE=$BUILDDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR-Installer.exe

  echo
  echo "Copying main binary .exe, .com, and dlls"
  echo "from $BUILDDIR/$MAKE_TARGET"
  echo "to $BUILDDIR/openscad-$VERSION"
  TMPTAR=$BUILDDIR/windeployqt.tar
  cd $BUILDDIR
  cd $MAKE_TARGET
  tar cvf $TMPTAR --exclude=winconsole.o .
  cd $BUILDDIR
  cd ./openscad-$VERSION
  tar xvf $TMPTAR
  cd $BUILDDIR
  rm -f $TMPTAR

  echo "Creating zipfile..."
  rm -f OpenSCAD-$VERSION.x86-$ARCH.zip
  "$ZIP" $ZIPARGS $BINFILE openscad-$VERSION
  mv $BINFILE $OPENSCADDIR/
  cd $OPENSCADDIR
  echo "Binary zip package created:"
  echo "  $BINFILE"
  echo "Not creating installable .msi/.exe package"
}

create_archive_mxe()
{
  cd $OPENSCADDIR
  cd $BUILDDIR

  # try to use a package filename that is not confusing (i686-w64-mingw32 is)
  ARCH_INDICATOR=MingW-x86-32-$OPENSCAD_BUILD_TARGET_ABI
  if [ $OPENSCAD_BUILD_TARGET_ARCH = x86_64 ]; then
    ARCH_INDICATOR=MingW-x86-64-$OPENSCAD_BUILD_TARGET_ABI
  fi

  BINFILE=$BUILDDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR.zip
  INSTFILE=$BUILDDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR-Installer.exe

  #package
  if [ $OPENSCAD_BUILD_TARGET_ABI = "shared" ]; then
    flprefix=$MXE_SYS_DIR/bin
    echo Copying dlls for shared library build
    echo from $flprefix
    echo to $BUILDDIR/$MAKE_TARGET
    flist=
    fl=

    qtlist="PrintSupport Core Gui OpenGL Widgets"
    boostlist="filesystem program_options regex system thread_win32 chrono"
    dlist="icuin icudt icudt54 zlib1 GLEW ../qt5/lib/qscintilla2"
    liblist="stdc++-6 png16-16 pcre16-0 freetype-6 iconv-2 intl-8 bz2 expat-1"
    liblist="$liblist fontconfig-1 harfbuzz-0 opencsg-1 glib-2.0-0"
    liblist="$liblist CGAL_Core CGAL gmpxx-4 gmp-10 mpfr-4 pcre-1"
    if [ $OPENSCAD_BUILD_TARGET_ARCH = i686 ]; then
      liblist="$liblist gcc_s_sjlj-1"
    else
      liblist="$liblist gcc_s_seh-1"
    fi
    fl=
    for file in $qtlist;    do fl="$fl ../qt5/bin/Qt5"$file".dll"; done
    for file in $boostlist; do fl="$fl libboost_"$file"-mt.dll"; done
    for file in $liblist;   do fl="$fl lib"$file".dll"; done
    for file in $dlist;     do fl="$fl "$file".dll"; done
    for dllfile in $fl; do
      copyfail $flprefix/$dllfile $BUILDDIR/$MAKE_TARGET/
    done
    # replicate windeployqt behavior. as of writing, theres no mxe windeployqt
    dqt=$BUILDDIR/$MAKE_TARGET/
    for subdir in platforms iconengines imageformats translations; do
      echo mkdir $dqt/$subdir
      mkdir $dqt/$subdir
    done
    copyfail $MXE_SYS_DIR/qt5/plugins/platforms/qwindows.dll $dqt/platforms/
    copyfail $MXE_SYS_DIR/qt/plugins/iconengines/qsvgicon4.dll $dqt/iconengines/
    for idll in `ls $MXE_SYS_DIR/qt/plugins/imageformats/`; do
      copyfail $MXE_SYS_DIR/qt/plugins/imageformats/$idll $dqt/imageformats/
    done
    # dont know how windeployqt does these .qm files in 'translations'. skip it 
  fi # shared

  echo "Copying main binary .exe, .com, and other stuff"
  echo "from $BUILDDIR/$MAKE_TARGET"
  echo "to $BUILDDIR/openscad-$VERSION"
  TMPTAR=$BUILDDIR/tmpmingw.$OPENSCAD_BUILD_TARGET_TRIPLE.tar
  cd $BUILDDIR
  cd $MAKE_TARGET
  tar cvf $TMPTAR --exclude=winconsole.o .
  cd $BUILDDIR
  cd ./openscad-$VERSION
  tar xf $TMPTAR
  cd $BUILDDIR
  rm -f $TMPTAR

  echo "Creating binary zip package `basename $BINFILE`"
  rm -f $BINFILE
  "$ZIP" $ZIPARGS $BINFILE openscad-$VERSION
  cd $OPENSCADDIR

  echo "Creating installer `basename $INSTFILE`"
  echo "Copying NSIS files to $BUILDDIR/openscad-$VERSION"
  cp ./scripts/installer$OPENSCAD_BUILD_TARGET_ARCH.nsi $BUILDDIR/openscad-$VERSION/installer_arch.nsi
  cp ./scripts/installer.nsi $BUILDDIR/openscad-$VERSION/
  cp ./scripts/mingw-file-association.nsh $BUILDDIR/openscad-$VERSION/
  cp ./scripts/x64.nsh $BUILDDIR/openscad-$VERSION/
  cp ./scripts/LogicLib.nsh $BUILDDIR/openscad-$VERSION/
  cd $BUILDDIR/openscad-$VERSION
  NSISDEBUG=-V2
  # NSISDEBUG=    # leave blank for full log
  echo $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
  $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
  cp $BUILDDIR/openscad-$VERSION/openscad_setup.exe $INSTFILE
  cd $OPENSCADDIR

  mv $BINFILE $OPENSCADDIR/
  mv $INSTFILE $OPENSCADDIR/
}

create_archive_netbsd()
{
  echo "Usage: $0 -v <versionstring> -d <versiondate>
  echo
  echo "Binary created:" $PACKAGEFILE
  echo
}

setup_BUILDDIR()
{
  if [ ! -d $BUILDDIR ]; then
    mkdir -p $BUILDDIR
  fi
  if [ ! -d $BUILDDIR ]; then
    exit 1
  fi
}

setup_misc_generic()
{
  cd $OPENSCADDIR
  setup_BUILDDIR
  MAKE_TARGET=
  # for QT4 set QT_SELECT=4
  QT_SELECT=5
  export QT_SELECT
}

setup_misc_mxe()
{
  setup_BUILDDIR
  MAKE_TARGET=release
  ZIP="zip"
  ZIPARGS="-r -q"
}

setup_misc_msys()
{
  setup_misc_mxe
}

qmaker_generic()
{
  cd $BUILDDIR
  qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug ../openscad.pro
  cd $OPENSCADDIR
}

qmaker_darwin()
{
  QMAKE="`command -v qmake-qt5`"
  if [ ! -x "$QMAKE" ]; then
    QMAKE=qmake
  fi
  "$QMAKE" VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug openscad.pro
}

make_clean_generic()
{
  cd $BUILDDIR
  make clean
  rm -f ./release/*
  rm -f ./debug/*
  cd $OPENSCADDIR
}

make_clean_darwin()
{
  sed -i.bak s/.Volumes.Macintosh.HD//g Makefile
  make -s clean
  rm -rf OpenSCAD.app
}

touch_parser_lexer_mxe()
{
  # kludge to enable paralell make
  touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.h
  touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.cpp
  touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.hpp
  touch -t 200012121010 $OPENSCADDIR/src/lexer_lex.cpp
}

touch_parser_lexer_msys()
{
  touch_parser_lexer_mxe
}

make_gui_binary_generic()
{
  cd $BUILDDIR
  if [ $FAKEMAKE ]; then build_fake_gui_binary_generic ; return ; fi
  make -j$NUMCPU $MAKE_TARGET
  if [[ $? != 0 ]]; then
    echo "Error building OpenSCAD. Aborting."
    exit 1
  fi
  cd $OPENSCADDIR
}

make_gui_binary_mxe()
{
  if [ $FAKEMAKE ]; then build_fake_gui_binary_mxe ; return ; fi
  # make main openscad.exe
  cd $BUILDDIR
  make $MAKE_TARGET -j$NUMCPU
  # make console pipe-able openscad.com - see winconsole.pro for info
  qmake ../winconsole/winconsole.pro
  make
  cd $OPENSCADDIR
}

make_gui_binary_msys()
{
  make_gui_binary_mxe
}

build_fake_gui_binary_generic()
{
  cd $BUILDDIR
  touch ./$MAKE_TARGET/openscad
  cd $OPENSCADDIR
}

build_fake_gui_binary_mxe()
{
  cd $BUILDDIR
  touch $MAKE_TARGET/openscad.exe
  touch $MAKE_TARGET/openscad.com
  cd $OPENSCADDIR
}

build_fake_gui_binary_msys()
{
  build_fake_gui_binary_mxe
}

if [ ! $OPENSCADDIR ]; then
  OPENSCADDIR=$PWD
fi

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Cannot find OPENSCADDIR/openscad.pro, OPENSCADDIR should be src root "
  exit 1
fi

echo OPENSCADDIR:$OPENSCADDIR

if [ ! -d $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi
echo DEPLOYDIR: $DEPLOYDIR

CONFIG=deploy

if [ $MXE_TARGET ]; then
  OS=UNIX_CROSS_WIN
elif [[ "$OSTYPE" =~ "darwin" ]]; then
  OS=MACOSX
elif [[ $OSTYPE == "msys" ]]; then
  OS=WIN
elif [[ $OSTYPE == "linux-gnu" ]]; then
  OS=LINUX
  ARCH=`uname -m`
fi

if [ $OS = UNIX_CROSS_WIN ]; then
  echo MXE cross build environment variables detected
  echo MXE_TARGET_DIR $MXE_TARGET_DIR
  echo MXE_TARGET $MXE_TARGET
  echo MXE_LIB_TYPE $MXE_LIB_TYPE
  echo DEPLOYDIR $DEPLOYDIR
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
        elif [ -e $MXE_DIR/usr/bin/i686-pc-mingw32-makensis ]; then
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
  if [ ! -e $OPENSCADDIR/libraries/MCAD/__init__.py ]; then
    echo "Downloading MCAD failed. exiting"
    exit
  fi
else
  echo "MCAD found:" $OPENSCADDIR/libraries/MCAD
fi


if [ -d .git ]; then
  git submodule update
fi

echo "Building openscad-$VERSION ($VERSIONDATE) $CONFIG..."
run check_prereq
paralell_note
echo "NUMCPU: " $NUMCPU



case $OS in
    LINUX|MACOSX)
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
        QPROFILE=$OPENSCADDIR/openscad.pro
        if [ "`echo $* | grep dryrun`" ]; then
          QPROFILE=$OPENSCADDIR/scripts/fakescad.pro
        fi
        cd $DEPLOYDIR
        qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug $QPROFILE
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
        cd $DEPLOYDIR
        make -j$NUMCPU
    ;;
    *)
        make -j$NUMCPU
    ;;
esac

if [[ $? != 0 ]]; then
  echo "Error building OpenSCAD. Stopping."
  exit 1
fi


case $OS in
  UNIX_CROSS_WIN)
    # make console pipe-able openscad.com - see winconsole.pro for info
    cd $DEPLOYDIR
    qmake $OPENSCADDIR/winconsole/winconsole.pro
    make
    if [[ $? != 0 ]]; then
      echo "Error building $DEPLOYDIR/openscad.com. Stopping."
      exit 1
    fi
    cd $OPENSCADDIR
  ;;
esac

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

mxe_shared()
{
  flprefix=$MXE_TARGET_DIR/bin
  echo Copying dlls for shared library build
  echo from $flprefix
  echo to $DEPLOYDIR/release
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
  cp $flprefix/$dllfile $DEPLOYDIR/release/
    else
  echo cannot find $flprefix/$dllfile
  echo stopping build.
  exit 1
    fi
  done
}

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
        cp release/openscad.exe openscad-$VERSION
        cp release/openscad.com openscad-$VERSION
        rm -f openscad-$VERSION.x86-$ARCH.zip
        "$ZIP" $ZIPARGS openscad-$VERSION.x86-$ARCH.zip openscad-$VERSION
        rm -rf openscad-$VERSION
        echo "Binary created: openscad-$VERSION.zip"
        ;;
    UNIX_CROSS_WIN)
        cd $OPENSCADDIR
        cd $DEPLOYDIR
        ARCH=x86_64
        if [ "`echo $MXE_TARGET | grep i686`" ]; then ARCH=x86_32 ; fi
        ZIPFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH.zip
        INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH-Installer.exe

        #package
        if [ $MXE_LIB_TYPE = "shared" ]; then
          mxe_shared
        fi

        echo "Copying main binary .exe, .com, and dlls"
        echo "from $DEPLOYDIR/release"
        echo "to $DEPLOYDIR/openscad-$VERSION"
        TMPTAR=$DEPLOYDIR/tmpmingw.$ARCH.$MXE_LIB_TYPE.tar
        cd $DEPLOYDIR
        cd release
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
        BITS=64
        if [ "`echo $MXE_TARGET | grep i686`" ]; then BITS=32 ; fi
        cp ./scripts/installer$BITS.nsi $DEPLOYDIR/openscad-$VERSION/installer_arch.nsi
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
