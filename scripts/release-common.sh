#!/bin/bash
#
# This script creates a binary release of OpenSCAD.
#
# On OSX / Darwin the script will create a file called
# openscad-<versionstring>.<extension> in the current directory. For
# other platforms, including cross-builds, the file will be under a
# directory with the triple-name of the target system, for example
# ./x86_64-w64-mingw32 or ./i686-linux-gnu, as setup by 'scripts/setenv.h'
#
# Portability works by using bash function naming and run().
# "_generic" is a generic build, _darwin _linux, _msys, etc are specialized

printUsage()
{
  echo "Usage: release-common.sh [-v <versionstring>] [-d <versiondate>] -c"
  echo ""
  echo " -v  Version string (e.g. -v 2010.01)"
  echo " -d  Version date (e.g. -d 2010.01.23)"
  echo " -snapshot Build a snapshot binary (make e.g. experimental features "
  echo "     available, build with commit info)"
  echo " -fakebinary Debug the packaging process by skipping 'make' "
  echo
  echo "If no version string or version date is given, todays date will be used"
  echo "(YYYY-MM-DD) If only version date is given, it will be used also as"
  echo "version string. If no make target is given, none will be used"
  echo "on Mac OS X"
  echo
  echo "  Example: $0 -v 2010.01"
  paralell_note
}

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

verify_binary_mxe()
{
  cd $DEPLOYDIR
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
  if [ ! -e $MAKE_TARGET/openscad ]; then
    echo "cant find $MAKE_TARGET/openscad. build failed. stopping."
    exit 1
  fi
}

setup_directories_darwin()
{
  EXAMPLESDIR=OpenSCAD.app/Contents/Resources/examples
  LIBRARYDIR=OpenSCAD.app/Contents/Resources/libraries
  FONTDIR=OpenSCAD.app/Contents/Resources/fonts
  TRANSLATIONDIR=OpenSCAD.app/Contents/Resources/locale
  COLORSCHEMESDIR=OpenSCAD.app/Contents/Resources/color-schemes
}

setup_directories_mxe()
{
  cd $OPENSCADDIR
  if [ ! -e $DEPLOYDIR ]; then
    mkdir -p $DEPLOYDIR
  fi
  EXAMPLESDIR=$DEPLOYDIR/openscad-$VERSION/examples/
  LIBRARYDIR=$DEPLOYDIR/openscad-$VERSION/libraries/
  FONTDIR=$DEPLOYDIR/openscad-$VERSION/fonts/
  TRANSLATIONDIR=$DEPLOYDIR/openscad-$VERSION/locale/
  COLORSCHEMESDIR=$DEPLOYDIR/openscad-$VERSION/color-schemes/
  rm -rf $DEPLOYDIR/openscad-$VERSION
  mkdir $DEPLOYDIR/openscad-$VERSION
}

setup_directories_msys()
{
  setup_directories_mxe
}

setup_directories_linux()
{
  EXAMPLESDIR=openscad-$VERSION/examples/
  LIBRARYDIR=openscad-$VERSION/libraries/
  FONTDIR=openscad-$VERSION/fonts/
  TRANSLATIONDIR=openscad-$VERSION/locale/
  COLORSCHEMESDIR=openscad-$VERSION/color-schemes/
  rm -rf openscad-$VERSION
  mkdir openscad-$VERSION
}

copy_examples_generic()
{
  echo $EXAMPLESDIR
  mkdir -p $EXAMPLESDIR
  rm -f examples.tar
  tar cf examples.tar examples
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
  cd $DEPLOYDIR

  echo "QT5 deployment, dll and other files copying..."
  windeployqt $MAKE_TARGET/openscad.exe

  bits=64
  if [ $OPENSCAD_BUILD_TARGET_ARCH = i686 ]; then
    bits=32
  fi

  flprefix=/mingw$bits/bin/
  echo MSYS2, dll copying...
  echo from $flprefix
  echo to $DEPLOYDIR/$MAKE_TARGET
  fl=
  boostlist="filesystem program_options regex system thread"
  liblist="gmp-10 gmpxx-4 opencsg-1 harfbuzz-0 harfbuzz-gobject-0 glib-2.0-0"
  liblist="$liblist CGAL CGAL_Core fontconfig-1 expat-1 bz2-1 intl-8 iconv-2"
  liblist="$liblist pcre16-0 png16-16 icudt55 freetype-6"
  dlist="glew32 opengl qscintilla2 zlib1 jsiosdjfiosdjf Qt5PrintSupport"
  for file in $boostlist; do fl="$fl libboost_"$file"-mt.dll"; done
  for file in $liblist;   do fl="$fl lib"$file".dll"; done
  for file in $dlist;     do fl="$fl "$file".dll"; done

  for dllfile in $fl; do
    copyfail $flprefix/$dllfile /$DEPLOYDIR/$MAKE_TARGET/
  done


  ARCH_INDICATOR=Msys2-x86-64
  if [ $OPENSCAD_BUILD_TARGET_ARCH = i686 ]; then
    ARCH_INDICATOR=Msys2-x86-32
  fi
  BINFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR.zip
  INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR-Installer.exe

  echo
  echo "Copying main binary .exe, .com, and dlls"
  echo "from $DEPLOYDIR/$MAKE_TARGET"
  echo "to $DEPLOYDIR/openscad-$VERSION"
  TMPTAR=$DEPLOYDIR/windeployqt.tar
  cd $DEPLOYDIR
  cd $MAKE_TARGET
  tar cvf $TMPTAR --exclude=winconsole.o .
  cd $DEPLOYDIR
  cd ./openscad-$VERSION
  tar xvf $TMPTAR
  cd $DEPLOYDIR
  rm -f $TMPTAR

  echo "Creating zipfile..."
  rm -f OpenSCAD-$VERSION.x86-$ARCH.zip
  "$ZIP" $ZIPARGS $BINFILE openscad-$VERSION
  cd $OPENSCADDIR
  echo "Binary zip package created:"
  echo "  $BINFILE"
  echo "Not creating installable .msi/.exe package"
}

create_archive_mxe()
{
  cd $OPENSCADDIR
  cd $DEPLOYDIR

  # try to use a package filename that is not confusing (i686-w64-mingw32 is)
  ARCH_INDICATOR=MingW-x86-32
  if [ $OPENSCAD_BUILD_TARGET_ARCH = x86_64 ]; then
    ARCH_INDICATOR=MingW-x86-64
  fi
  if [ $OPENSCAD_BUILD_TARGET_ABI = "shared" ]; then
    ARCH_INDICTATOR=$ARCH_INDICATOR"-shared"
  else
    ARCH_INDICTATOR=$ARCH_INDICATOR"-static"
  fi

  BINFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR.zip
  INSTFILE=$DEPLOYDIR/OpenSCAD-$VERSION-$ARCH_INDICATOR-Installer.exe

  #package
  if [ $OPENSCAD_BUILD_TARGET_ABI = "shared" ]; then
    flprefix=$MXE_SYS_DIR/bin
    echo Copying dlls for shared library build
    echo from $flprefix
    echo to $DEPLOYDIR/$MAKE_TARGET
    flist=
    fl=

    qtlist="PrintSupport Core Gui OpenGL Widgets"
    boostlist="filesystem program_options regex system thread_win32 chrono"
    dlist="icuin icudt icudt54 zlib1 GLEW ../qt5/lib/qscintilla2"
    liblist="stdc++-6 png16-16 pcre16-0 freetype-6 iconv-2 intl-8 bz2 expat-1"
    liblist="$liblist fontconfig-1 harfbuzz-0 opencsg-1 glib-2.0-0"
    liblist="$liblist CGAL_Core CGAL gmpxx-4 gmp-10 pcre-1"
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
      copyfail $flprefix/$dllfile /$DEPLOYDIR/$MAKE_TARGET/
    done
  fi # shared

  echo "Copying main binary .exe, .com, and other stuff"
  echo "from $DEPLOYDIR/$MAKE_TARGET"
  echo "to $DEPLOYDIR/openscad-$VERSION"
  TMPTAR=$DEPLOYDIR/tmpmingw.$OPENSCAD_BUILD_TARGET_TRIPLE.tar
  cd $DEPLOYDIR
  cd $MAKE_TARGET
  tar cvf $TMPTAR --exclude=winconsole.o .
  cd $DEPLOYDIR
  cd ./openscad-$VERSION
  tar xf $TMPTAR
  cd $DEPLOYDIR
  rm -f $TMPTAR

  echo "Creating binary zip package `basename $BINFILE`"
  rm -f $BINFILE
  "$ZIP" $ZIPARGS $BINFILE openscad-$VERSION
  cd $OPENSCADDIR

  echo "Creating installer `basename $INSTFILE`"
  echo "Copying NSIS files to $DEPLOYDIR/openscad-$VERSION"
  cp ./scripts/installer$OPENSCAD_BUILD_TARGET_ARCH.nsi $DEPLOYDIR/openscad-$VERSION/installer_arch.nsi
  cp ./scripts/installer.nsi $DEPLOYDIR/openscad-$VERSION/
  cp ./scripts/mingw-file-association.nsh $DEPLOYDIR/openscad-$VERSION/
  cp ./scripts/x64.nsh $DEPLOYDIR/openscad-$VERSION/
  cp ./scripts/LogicLib.nsh $DEPLOYDIR/openscad-$VERSION/
  cd $DEPLOYDIR/openscad-$VERSION
  NSISDEBUG=-V2
  # NSISDEBUG=    # leave blank for full log
  echo $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
  $MAKENSIS $NSISDEBUG "-DVERSION=$VERSION" installer.nsi
  cp $DEPLOYDIR/openscad-$VERSION/openscad_setup.exe $INSTFILE
  cd $OPENSCADDIR
}

create_archive_linux()
{
  # Do stuff from release-linux.sh
  mkdir openscad-$VERSION/bin
  mkdir -p openscad-$VERSION/lib/openscad
  cp scripts/openscad-linux openscad-$VERSION/bin/openscad
  cp openscad openscad-$VERSION/lib/openscad/
  if [[ $OPENSCAD_BUILD_TARGET_ARCH == x86_64 ]]; then
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
  PACKAGEFILE=openscad-$VERSION.$OPENSCAD_BUILD_TARGET_ARCH.tar.gz
  tar cz openscad-$VERSION > $PACKAGEFILE
  echo
  echo "Binary created:" $PACKAGEFILE
  echo
}

setup_misc_generic()
{
  MAKE_TARGET=
  # for QT4 set QT_SELECT=4
  QT_SELECT=5
  export QT_SELECT
}

setup_misc_mxe()
{
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
  QMAKE="`command -v qmake-qt5`"
  if [ ! -x "$QMAKE" ]; then
    QMAKE=qmake
  fi
  "$QMAKE" VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug openscad.pro
}

qmaker_msys()
{
  cd $DEPLOYDIR
  echo qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug ../openscad.pro
  qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG-=debug ../openscad.pro
  cd $OPENSCADDIR
}

qmaker_mxe()
{
  cd $DEPLOYDIR
  MINGWCONFIG=mingw-cross-env
  if [ $OPENSCAD_BUILD_TARGET_ABI = "shared" ]; then
    MINGWCONFIG=mingw-cross-env-shared
  fi
  qmake VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT CONFIG+="$CONFIG" CONFIG+=$MINGWCONFIG CONFIG-=debug ../openscad.pro
  cd $OPENSCADDIR
}

make_clean_mxe()
{
  cd $DEPLOYDIR
  make clean
  rm -f ./release/*
  rm -f ./debug/*
  cd $OPENSCADDIR
}

make_clean_msys()
{
  make_clean_mxe
}

make_clean_darwin()
{
  make -s clean
  rm -rf OpenSCAD.app
}

make_clean_linux()
{
  make -s clean
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

build_gui_binary_generic()
{
  make -j$NUMCPU $MAKE_TARGET
  if [[ $? != 0 ]]; then
    echo "Error building OpenSCAD. Aborting."
    exit 1
  fi
}

build_fake_gui_binary_mxe()
{
  # fake to speed debug of building process
  cd $DEPLOYDIR
  touch $MAKE_TARGET/openscad.exe
  touch $MAKE_TARGET/openscad.com
  cd $OPENSCADDIR
}

build_fake_gui_binary_msys()
{
  build_fake_gui_binary_mxe
}

build_gui_binary_mxe()
{
  if [ $FAKEMAKE ]; then build_fake_gui_binary_mxe ; return; fi
  # make main openscad.exe
  cd $DEPLOYDIR
  make $MAKE_TARGET -j$NUMCPU
  # make console pipe-able openscad.com - see winconsole.pro for info
  qmake ../winconsole/winconsole.pro
  make
  cd $OPENSCADDIR
}

build_gui_binary_msys()
{
  if [ $FAKEMAKE ]; then build_fake_gui_binary_mxe ; return; fi
  build_gui_binary_mxe
}

OPENSCADDIR=$PWD
if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 1
fi

CONFIG=deploy

if [[ "$OSTYPE" == "darwin"* ]]; then
  OPENSCAD_BUILD_TARGET_OSTYPE=darwin
elif [ ! $SETENV_SAVED_ORIGINAL_PATH ]; then
  echo "please run . ./scripts/setenv.sh first (note the  . )"
fi

if [ "`echo $* | grep fake`" ]; then
  FAKEMAKE=1
fi

if [ "`echo $* | grep snapshot`" ]; then
  CONFIG="$CONFIG snapshot experimental"
  OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
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

echo "Building openscad-$VERSION ($VERSIONDATE) $CONFIG..."
run check_prereq
paralell_note
echo "NUMCPU: " $NUMCPU
run update_mcad
run setup_misc
run qmaker
run make_clean
run touch_parser_lexer
run build_gui_binary
run verify_binary
run setup_directories
if [ -n $EXAMPLESDIR ]; then run copy_examples ; fi
if [ -n $FONTSDIR ]; then run copy_fonts ; fi
if [ -n $COLORSCHEMESDIR ]; then run copy_colorschemes ; fi
if [ -n $LIBRARYDIR ]; then run copy_mcad ; fi
if [ -n $TRANSLATIONDIR ]; then run copy_translations ; fi
run create_archive

