#!/usr/bin/env bash
#
# This script creates a package of OpenSCAD for easy installation and 
# deinstallation.
#
# Mac                       .dmg
# Linux gdebi               .deb
# Windows MSYS              .exe/.zip
# Windows cross build MXE   .exe/.zip
#
# The script will create a file called openscad-<versionstring>.<extension>
# under $DEPLOYDIR which is typically openscad/bin/host-triple 
# This means you can build packages for multiple targets within one tree.
#
# Usage: make-pkg.sh [-dryrun]
#  -dryrun  Quickly build a dummy openscad.exe file to test this release script

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

update_mcad()
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
  run ls $DEPLOYDIR/openscad
}

verify_binary_darwin()
{
  run ls $DEPLOYDIR/OpenSCAD.app/Contents/MacOS/OpenSCAD
}

verify_binary_mxe()
{
  run ls $DEPLOYDIR/openscad.com
  run ls $DEPLOYDIR/openscad.exe
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

copy_fonts()
{
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

create_package_darwin()
{
  /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSIONDATE" OpenSCAD.app/Contents/Info.plist
  macdeployqt OpenSCAD.app -dmg -no-strip
  mv OpenSCAD.dmg OpenSCAD-$VERSION.dmg
  hdiutil internet-enable -yes -quiet OpenSCAD-$VERSION.dmg
  echo "Binary created: OpenSCAD-$VERSION.dmg"
}

create_package_msys()
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

create_package_mxe_shared()
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

create_package_mxe()
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

create_package_linux()
{
  cd $OPENSCADDIR
  ./scripts/makepkg-deb.sh
  cd $DEPLOYDIR
}

call_qmake()
{
  QMAKE="`command -v qmake-qt5`"
  if [ ! -x "$QMAKE" ]; then
    QMAKE=qmake
  fi
  QPRO_FILE=$OPENSCADDIR/openscad.pro
  if [ "`echo $* | grep dryrun`" ]; then
    QPRO_FILE=$OPENSCADDIR/scripts/fakescad.pro
  fi
  qmake PREFIX=$DEPLOYDIR VERSION=$VERSION OPENSCAD_COMMIT=$OPENSCAD_COMMIT $QPRO_FILE
}

cleanup_darwin()
{
  make clean
  sed -i.bak s/.Volumes.Macintosh.HD//g Makefile
  rm -rf OpenSCAD.app
}

cleanup_mxe()
{
  make clean
  rm -f ./release/*
  rm -f ./debug/*
  rm -rf $DEPLOYDIR/openscad-$VERSION
  mkdir $DEPLOYDIR/openscad-$VERSION
  #touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.h
  #touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.cpp
  #touch -t 200012121010 $OPENSCADDIR/src/parser_yacc.hpp
  #touch -t 200012121010 $OPENSCADDIR/src/lexer_lex.cpp
}

call_make()
{
  run make
  run make install
}

call_make_mxe()
{
  run call_make
  # make console pipe-able openscad.com - see winconsole.pro for info
  run qmake $OPENSCADDIR/winconsole/winconsole.pro
  run make
}

call_make_msys()
{
  call_make_mxe
}

setup_dirs()
{
  RESOURCES_DIR=$DEPLOYDIR/openscad-$VERSION
}

setup_dirs_darwin()
{
  RESOURCES_DIR=$DEPLOYDIR/OpenSCAD.app/Contents/Resources
}


copy_fonts()
{
  find ./fonts/10-liberation.conf -print -depth | cpio -pud $RESOURCES_DIR/fonts
  find ./fonts/Liberation-2.00.1 -print -depth | cpio -pud $RESOURCES_DIR/fonts
}

copy_fonts_darwin()
{
  copy_fonts
  cp -a ./fonts/05-osx-fonts.conf $RESOURCES_DIR/fonts
  cp -a fonts-osx/* $RESOURCES_DIR/fonts
}

copy_fonts_mxe()
{
  copy_fonts
  mxefonts=$MXEDIR/usr/$MXE_TARGET/etc
  if [ -d $mxefonts ]; then
    pwds=$PWD
    cd $mxefonts
    find ./fonts -print -depth | cpio -pud $RESOURCES_DIR
    cd $pwds
  else
    echo cannot find mxe fonts
    exit 1
  fi
}

copy_resources()
{
  cd $OPENSCADDIR
  find ./examples -print -depth | cpio -pud $RESOURCES_DIR
  find ./color-schemes -print -depth | cpio -pud $RESOURCES_DIR
  find  ./libraries -print -depth | grep -v ".git" | cpio -pud $RESOURCES_DIR
  find  ./locale -print -depth | grep ".mo" | cpio -pud $RESOURCES_DIR
  chmod -R u=rwx,go=r,+X $RESOURCES_DIR/libraries
  chmod -R 644 $RESOURCES_DIR/examples
  cd $DEPLOYDIR
}


if [ ! $OPENSCADDIR ]; then
  echo please run scripts/setenv.sh first
fi
if [ ! $DEPLOYDIR ]; then
  echo please run scripts/setenv.sh first
fi

if [ ! -d $DEPLOYDIR ]; then
  mkdir -p $DEPLOYDIR
fi

OPENSCAD_COMMIT=`git log -1 --pretty=format:"%h"`
VERSION=`date "+%Y.%m.%d"`
export_and_print_vars OPENSCAD_COMMIT VERSION

run update_mcad
run check_prereq

cd $DEPLOYDIR

QT_SELECT=5
ZIP="zip"
ZIPARGS="-r -q"

run cleanup
run call_qmake
run call_make
run setup_dirs
run copy_resources
run copy_fonts
run create_package
