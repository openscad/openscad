#!/bin/sh -e
#
# This script builds all library dependencies of OpenSCAD for Mac OS X.
# The libraries will be build in 64-bit (and optionally 32-bit mode) mode
# and backwards compatible with 10.5 "Leopard".
# 
# This script must be run from the OpenSCAD source root directory
#
# Usage: macosx-build-dependencies.sh [-6lcd]
#  -6   Build only 64-bit binaries
#  -l   Force use of LLVM compiler
#  -c   Force use of clang compiler
#  -d   Build for deployment (if not specified, e.g. Sparkle won't be built)
#
# Prerequisites:
# - MacPorts: curl, cmake
#
# FIXME:
# o Verbose option
# o Force rebuild vs. only rebuild changes
#

BASEDIR=$PWD/../libraries
OPENSCADDIR=$PWD
SRCDIR=$BASEDIR/src
DEPLOYDIR=$BASEDIR/install
MAC_OSX_VERSION_MIN=10.7
OPTION_32BIT=false
OPTION_LLVM=false
OPTION_CLANG=false
OPTION_GCC=false
OPTION_DEPLOY=false

printUsage()
{
  echo "Usage: $0 [-6lcd]"
  echo
  echo "  -6   Build only 64-bit binaries"
  echo "  -l   Force use of LLVM compiler"
  echo "  -c   Force use of clang compiler"
  echo "  -d   Build for deployment"
}

patch_qt_disable_core_wlan()
{
  version="$1"

  patch -p1 <<END-OF-PATCH
--- qt-everywhere-opensource-src-4.8.5/src/plugins/bearer/bearer.pro.orig	2013-11-01 19:04:29.000000000 +0100
+++ qt-everywhere-opensource-src-4.8.5/src/plugins/bearer/bearer.pro	2013-10-31 21:53:00.000000000 +0100
@@ -12,7 +12,7 @@
 #win32:SUBDIRS += nla
 win32:SUBDIRS += generic
 win32:!wince*:SUBDIRS += nativewifi
-macx:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
+#macx:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
 macx:SUBDIRS += generic
 symbian:SUBDIRS += symbian
 blackberry:SUBDIRS += blackberry
END-OF-PATCH
}

# FIXME: Support gcc/llvm/clang flags. Use -platform <whatever> to make this work? kintel 20130117
build_qt()
{
  version=$1

  if [ -d $DEPLOYDIR/lib/QtCore.framework ]; then
    echo "qt already installed. not building"
    return
  fi

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
     curl -O -L http://download.qt-project.org/official_releases/qt/4.8/4.8.5/qt-everywhere-opensource-src-4.8.5.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-src_corelib_global_qglobal.h.diff
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-libtiff.diff
  patch -p0 < $OPENSCADDIR/patches/qt4/patch-src_plugins_bearer_corewlan_qcorewlanengine.mm.diff
  if $USING_CLANG; then
    # FIX for clang
    sed -i "" -e "s/::TabletProximityRec/TabletProximityRec/g"  src/gui/kernel/qt_cocoa_helpers_mac_p.h
    PLATFORM="-platform unsupported/macx-clang"
  fi
  if $OPTION_32BIT; then
    QT_32BIT="-arch x86"
  fi
  case "$OSX_VERSION" in
    9)
      # libtiff fails in the linker step with Mavericks / XCode 5.0.1
      MACOSX_RELEASE_OPTIONS=-no-libtiff
      # wlan support bails out with lots of compiler errors, disable it for the build
      patch_qt_disable_core_wlan "$version"
      ;;
    *)
      MACOSX_RELEASE_OPTIONS=
      ;;
  esac
  ./configure -prefix $DEPLOYDIR -release $QT_32BIT -arch x86_64 -opensource -confirm-license $PLATFORM -fast -no-qt3support -no-svg -no-phonon -no-audio-backend -no-multimedia -no-javascript-jit -no-script -no-scripttools -no-declarative -no-xmlpatterns -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit $MACOSX_RELEASE_OPTIONS
  make -j"$NUMCPU" install
}

build_qt5()
{
  version=$1

  if [ -d $DEPLOYDIR/lib/QtCore.framework ]; then
    echo "Qt5 already installed. not building"
    return
  fi

  echo "Building Qt" $version "..."
  cd $BASEDIR/src
  v=(${version//./ }) # Split into array
  rm -rf qt-everywhere-opensource-src-$version
  if [ ! -f qt-everywhere-opensource-src-$version.tar.gz ]; then
     curl -O -L http://download.qt-project.org/official_releases/qt/${v[0]}.${v[1]}/$version/single/qt-everywhere-opensource-src-$version.tar.gz
  fi
  tar xzf qt-everywhere-opensource-src-$version.tar.gz
  cd qt-everywhere-opensource-src-$version
  ./configure -prefix $DEPLOYDIR -release -opensource -confirm-license \
		-nomake examples -nomake tests \
		-no-xcb -no-c++11 -no-glib -no-harfbuzz -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc \
		-no-sql-psql -no-sql-sqlite2 -no-sql-tds -no-cups -no-qml-debug \
		-skip activeqt -skip connectivity -skip declarative -skip doc \
		-skip enginio -skip graphicaleffects -skip location -skip multimedia \
		-skip quick1 -skip quickcontrols -skip script -skip sensors -skip serialport \
		-skip svg -skip webkit -skip webkit-examples -skip websockets -skip xmlpatterns
  make -j"$NUMCPU" install
}

build_qscintilla()
{
  version=$1
  echo "Building QScintilla" $version "..."
  cd $BASEDIR/src
  rm -rf QScintilla-gpl-$version
  if [ ! -f QScintilla-gpl-$version.tar.gz ]; then
    curl -LO http://downloads.sourceforge.net/project/pyqt/QScintilla2/QScintilla-$version/QScintilla-gpl-$version.tar.gz
  fi
  tar xzf QScintilla-gpl-$version.tar.gz
  cd QScintilla-gpl-$version/Qt4Qt5
  qmake qscintilla.pro
  make -j6 install
  install_name_tool -id $DEPLOYDIR/lib/libqscintilla2.dylib $DEPLOYDIR/lib/libqscintilla2.dylib
}

# Hack warning: gmplib is built separately in 32-bit and 64-bit mode
# and then merged afterwards. gmplib's header files are dependent on
# the CPU architecture on which configure was run and will be patched accordingly.
build_gmp()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libgmp.dylib ]; then
    echo "gmp already installed. not building"
    return
  fi

  echo "Building gmp" $version "..."
  cd $BASEDIR/src
  rm -rf gmp-$version
  if [ ! -f gmp-$version.tar.bz2 ]; then
    curl -O ftp://ftp.gmplib.org/pub/gmp-$version/gmp-$version.tar.bz2
  fi
  tar xjf gmp-$version.tar.bz2
  cd gmp-$version
  patch -p0 gmp-h.in << EOF
--- gmp-5.1.3/gmp-h.in.old	2013-12-02 20:16:26.000000000 -0800
+++ gmp-5.1.3/gmp-h.in	2013-12-02 20:21:22.000000000 -0800
@@ -27,13 +27,38 @@
 #endif
 
 
-/* Instantiated by configure. */
 #if ! defined (__GMP_WITHIN_CONFIGURE)
+/* For benefit of fat builds on MacOSX, generate a .h file that can
+ * be used with a universal fat library
+ */
+#if defined(__x86_64__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 0
+#define GMP_LIMB_BITS                      64
+#define GMP_NAIL_BITS                      0
+#elif defined(__i386__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 0
+#define GMP_LIMB_BITS                      32
+#define GMP_NAIL_BITS                      0
+#elif defined(__powerpc64__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 1
+#define GMP_LIMB_BITS                      64
+#define GMP_NAIL_BITS                      0
+#elif defined(__ppc__)
+#define __GMP_HAVE_HOST_CPU_FAMILY_power   0
+#define __GMP_HAVE_HOST_CPU_FAMILY_powerpc 1
+#define GMP_LIMB_BITS                      32
+#define GMP_NAIL_BITS                      0
+#else
+/* For other architectures, fall back on values computed by configure */
 #define __GMP_HAVE_HOST_CPU_FAMILY_power   @HAVE_HOST_CPU_FAMILY_power@
 #define __GMP_HAVE_HOST_CPU_FAMILY_powerpc @HAVE_HOST_CPU_FAMILY_powerpc@
 #define GMP_LIMB_BITS                      @GMP_LIMB_BITS@
 #define GMP_NAIL_BITS                      @GMP_NAIL_BITS@
 #endif
+#endif
 #define GMP_NUMB_BITS     (GMP_LIMB_BITS - GMP_NAIL_BITS)
 #define GMP_NUMB_MASK     ((~ __GMP_CAST (mp_limb_t, 0)) >> GMP_NAIL_BITS)
 #define GMP_NUMB_MAX      GMP_NUMB_MASK
EOF

  if $OPTION_32BIT; then
    mkdir build-i386
    cd build-i386
    ../configure --prefix=$DEPLOYDIR/i386 "CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" ABI=32 --enable-cxx
    make install
    cd ..
  fi

  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 "CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" ABI=64 --enable-cxx
  make install

  # merge
  cd $DEPLOYDIR
  mkdir -p lib
  if $OPTION_32BIT; then
    lipo -create i386/lib/libgmp.dylib x86_64/lib/libgmp.dylib -output lib/libgmp.dylib
    lipo -create i386/lib/libgmpxx.dylib x86_64/lib/libgmpxx.dylib -output lib/libgmpxx.dylib
  else
    cp x86_64/lib/libgmp.dylib lib/libgmp.dylib
    cp x86_64/lib/libgmpxx.dylib lib/libgmpxx.dylib
  fi
  install_name_tool -id $DEPLOYDIR/lib/libgmp.dylib lib/libgmp.dylib
  install_name_tool -id $DEPLOYDIR/lib/libgmpxx.dylib lib/libgmpxx.dylib
  install_name_tool -change $DEPLOYDIR/x86_64/lib/libgmp.10.dylib $DEPLOYDIR/lib/libgmp.dylib lib/libgmpxx.dylib
  if $OPTION_32BIT; then
    cp lib/libgmp.dylib i386/lib/
    cp lib/libgmp.dylib x86_64/lib/
    cp lib/libgmpxx.dylib i386/lib/
    cp lib/libgmpxx.dylib x86_64/lib/
  fi
  mkdir -p include
  cp x86_64/include/gmp.h include/
  cp x86_64/include/gmpxx.h include/
}

# As with gmplib, mpfr is built separately in 32-bit and 64-bit mode and then merged
# afterwards.
build_mpfr()
{
  version=$1

  if [ -f $DEPLOYDIR/include/mpfr.h ]; then
    echo "mpfr already installed. not building"
    return
  fi

  echo "Building mpfr" $version "..."
  cd $BASEDIR/src
  rm -rf mpfr-$version
  if [ ! -f mpfr-$version.tar.bz2 ]; then
    curl -O http://www.mpfr.org/mpfr-$version/mpfr-$version.tar.bz2
  fi
  tar xjf mpfr-$version.tar.bz2
  cd mpfr-$version
#  curl -O http://www.mpfr.org/mpfr-$version/allpatches
#  patch -N -Z -p1 < allpatches 
  if $OPTION_32BIT; then
    mkdir build-i386
    cd build-i386
    ../configure --prefix=$DEPLOYDIR/i386 --with-gmp=$DEPLOYDIR/i386 CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch i386"
    make install
    cd ..
  fi

  # 64-bit version
  mkdir build-x86_64
  cd build-x86_64
  ../configure --prefix=$DEPLOYDIR/x86_64 --with-gmp=$DEPLOYDIR/x86_64 CFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64" LDFLAGS="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64"
  make install

  # merge
  cd $DEPLOYDIR
  if $OPTION_32BIT; then
    lipo -create i386/lib/libmpfr.dylib x86_64/lib/libmpfr.dylib -output lib/libmpfr.dylib
  else
    cp x86_64/lib/libmpfr.dylib lib/libmpfr.dylib
  fi
  install_name_tool -id $DEPLOYDIR/lib/libmpfr.dylib lib/libmpfr.dylib
  mkdir -p include
  cp x86_64/include/mpfr.h include/
  cp x86_64/include/mpf2mpfr.h include/
}

build_boost()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libboost_system.dylib ]; then
    echo "boost already installed. not building"
    return
  fi

  bversion=`echo $version | tr "." "_"`
  echo "Building boost" $version "..."
  cd $BASEDIR/src
  rm -rf boost_$bversion
  if [ ! -f boost_$bversion.tar.bz2 ]; then
    curl -LO http://downloads.sourceforge.net/project/boost/boost/$version/boost_$bversion.tar.bz2
  fi
  tar xjf boost_$bversion.tar.bz2
  cd boost_$bversion
  # We only need the thread and program_options libraries
  ./bootstrap.sh --prefix=$DEPLOYDIR --with-libraries=thread,program_options,filesystem,chrono,system,regex
  if $OPTION_32BIT; then
    BOOST_EXTRA_FLAGS="-arch i386"
  fi
  if $USING_LLVM; then
    BOOST_TOOLSET="toolset=darwin-llvm"
    echo "using darwin : llvm : llvm-g++ ;" >> tools/build/user-config.jam 
  elif $USING_CLANG; then
    BOOST_TOOLSET="toolset=clang"
    echo "using clang ;" >> tools/build/user-config.jam 
  fi
  ./b2 -j"$NUMCPU" -d+2 $BOOST_TOOLSET cflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS" linkflags="-mmacosx-version-min=$MAC_OSX_VERSION_MIN -arch x86_64 $BOOST_EXTRA_FLAGS -headerpad_max_install_names" install
  install_name_tool -id $DEPLOYDIR/lib/libboost_thread.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -change libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -change libboost_chrono.dylib $DEPLOYDIR/lib/libboost_chrono.dylib $DEPLOYDIR/lib/libboost_thread.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_program_options.dylib $DEPLOYDIR/lib/libboost_program_options.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_filesystem.dylib $DEPLOYDIR/lib/libboost_filesystem.dylib 
  install_name_tool -change libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_filesystem.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_system.dylib $DEPLOYDIR/lib/libboost_system.dylib 
  install_name_tool -id $DEPLOYDIR/lib/libboost_regex.dylib $DEPLOYDIR/lib/libboost_regex.dylib 


}

build_cgal()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libCGAL.dylib ]; then
    echo "cgal already installed. not building"
    return
  fi

  echo "Building CGAL" $version "..."
  cd $BASEDIR/src
  rm -rf CGAL-$version
  if [ ! -f CGAL-$version.tar.gz ]; then
    # 4.5.1
    curl -O https://gforge.inria.fr/frs/download.php/file/34400/CGAL-$version.tar.gz
    # 4.5 curl -O https://gforge.inria.fr/frs/download.php/file/34149/CGAL-$version.tar.gz
    # 4.4 curl -O https://gforge.inria.fr/frs/download.php/file/33525/CGAL-$version.tar.gz
    # 4.3 curl -O https://gforge.inria.fr/frs/download.php/32994/CGAL-$version.tar.gz
    # 4.2 curl -O https://gforge.inria.fr/frs/download.php/32359/CGAL-$version.tar.gz
    # 4.1 curl -O https://gforge.inria.fr/frs/download.php/31641/CGAL-$version.tar.gz
    # 4.1-beta1 curl -O https://gforge.inria.fr/frs/download.php/31348/CGAL-$version.tar.gz
    # 4.0.2 curl -O https://gforge.inria.fr/frs/download.php/31175/CGAL-$version.tar.gz
    # 4.0 curl -O https://gforge.inria.fr/frs/download.php/30387/CGAL-$version.tar.gz
    # 3.9 curl -O https://gforge.inria.fr/frs/download.php/29125/CGAL-$version.tar.gz
    # 3.8 curl -O https://gforge.inria.fr/frs/download.php/28500/CGAL-$version.tar.gz
    # 3.7 curl -O https://gforge.inria.fr/frs/download.php/27641/CGAL-$version.tar.gz
  fi
  tar xzf CGAL-$version.tar.gz
  cd CGAL-$version
  if $OPTION_32BIT; then
    CGAL_EXTRA_FLAGS=";i386"
  fi
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DGMP_INCLUDE_DIR=$DEPLOYDIR/include -DGMP_LIBRARIES=$DEPLOYDIR/lib/libgmp.dylib -DGMPXX_LIBRARIES=$DEPLOYDIR/lib/libgmpxx.dylib -DGMPXX_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_INCLUDE_DIR=$DEPLOYDIR/include -DMPFR_LIBRARIES=$DEPLOYDIR/lib/libmpfr.dylib -DWITH_CGAL_Qt3=OFF -DWITH_CGAL_Qt4=OFF -DWITH_CGAL_ImageIO=OFF -DBUILD_SHARED_LIBS=TRUE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64$CGAL_EXTRA_FLAGS" -DBOOST_ROOT=$DEPLOYDIR -DBoost_USE_MULTITHREADED=false
  make -j"$NUMCPU" install
  make install
  install_name_tool -id $DEPLOYDIR/lib/libCGAL.dylib $DEPLOYDIR/lib/libCGAL.dylib
  install_name_tool -id $DEPLOYDIR/lib/libCGAL_Core.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
  install_name_tool -change $PWD/lib/libCGAL.9.dylib $DEPLOYDIR/lib/libCGAL.dylib $DEPLOYDIR/lib/libCGAL_Core.dylib
}

build_glew()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libGLEW.dylib ]; then
    echo "glew already installed. not building"
    return
  fi

  echo "Building GLEW" $version "..."
  cd $BASEDIR/src
  rm -rf glew-$version
  if [ ! -f glew-$version.tgz ]; then
    curl -LO http://downloads.sourceforge.net/project/glew/glew/$version/glew-$version.tgz
  fi
  tar xzf glew-$version.tgz
  cd glew-$version
  mkdir -p $DEPLOYDIR/lib/pkgconfig
  if $OPTION_32BIT; then
    GLEW_EXTRA_FLAGS="-arch i386"
  fi
  make GLEW_DEST=$DEPLOYDIR CC=$CC CFLAGS.EXTRA="-no-cpp-precomp -dynamic -fno-common -mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" LDFLAGS.EXTRA="-mmacosx-version-min=$MAC_OSX_VERSION_MIN $GLEW_EXTRA_FLAGS -arch x86_64" STRIP= install
}

build_opencsg()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libopencsg.dylib ]; then
    echo "opencsg already installed. not building"
    return
  fi

  echo "Building OpenCSG" $version "..."
  cd $BASEDIR/src
  rm -rf OpenCSG-$version
  if [ ! -f OpenCSG-$version.tar.gz ]; then
    curl -O http://www.opencsg.org/OpenCSG-$version.tar.gz
  fi
  tar xzf OpenCSG-$version.tar.gz
  cd OpenCSG-$version
  patch -p1 < $OPENSCADDIR/patches/OpenCSG-$version-MacOSX-port.patch
  if $OPTION_32BIT; then
    OPENCSG_EXTRA_FLAGS="x86"
  fi
  qmake -r QMAKE_CXXFLAGS+="-I$DEPLOYDIR/include" QMAKE_LFLAGS+="-L$DEPLOYDIR/lib" CONFIG+="x86_64 $OPENCSG_EXTRA_FLAGS" DESTDIR=$DEPLOYDIR
  make install
}

build_eigen()
{
  version=$1

  if [ -d $DEPLOYDIR/include/eigen3 ]; then
    echo "eigen3 already installed. not building"
    return
  fi

  echo "Building eigen" $version "..."
  cd $BASEDIR/src
  rm -rf eigen-$version

  EIGENDIR="none"
  if [ $version = "3.1.2" ]; then EIGENDIR=eigen-eigen-5097c01bcdc4;
  elif [ $version = "3.1.3" ]; then EIGENDIR=eigen-eigen-2249f9c22fe8;
  elif [ $version = "3.1.4" ]; then EIGENDIR=eigen-eigen-36bf2ceaf8f5;
  elif [ $version = "3.2.0" ]; then EIGENDIR=eigen-eigen-ffa86ffb5570;
  elif [ $version = "3.2.1" ]; then EIGENDIR=eigen-eigen-6b38706d90a9;
  elif [ $version = "3.2.2" ]; then EIGENDIR=eigen-eigen-1306d75b4a21;
  elif [ $version = "3.2.3" ]; then EIGENDIR=eigen-eigen-36fd1ba04c12;
  fi

  if [ $EIGENDIR = "none" ]; then
    echo Unknown eigen version. Please edit script.
    exit 1
  fi
  rm -rf ./$EIGENDIR

  if [ ! -f eigen-$version.tar.bz2 ]; then
    curl -LO http://bitbucket.org/eigen/eigen/get/$version.tar.bz2
    mv $version.tar.bz2 eigen-$version.tar.bz2
  fi
  tar xjf eigen-$version.tar.bz2
  ln -s ./$EIGENDIR eigen-$version
  cd eigen-$version
  mkdir build
  cd build
  if $OPTION_32BIT; then
    EIGEN_EXTRA_FLAGS=";i386"
  fi
  cmake -DCMAKE_INSTALL_PREFIX=$DEPLOYDIR -DEIGEN_BUILD_LIB=ON -DBUILD_SHARED_LIBS=FALSE -DCMAKE_OSX_DEPLOYMENT_TARGET="$MAC_OSX_VERSION_MIN" -DCMAKE_OSX_ARCHITECTURES="x86_64$EIGEN_EXTRA_FLAGS" ..
  make -j"$NUMCPU" install
  make install
}

build_sparkle()
{
  # Let Sparkle use the default compiler
  unset CC
  unset CXX
  github=$1
  version=$2
  echo "Building Sparkle" $version "..."
  cd $BASEDIR/src
  rm -rf Sparkle-$version
  if [ ! -f Sparkle-$version.zip ]; then
      curl -o Sparkle-$version.zip https://nodeload.github.com/$github/Sparkle/zip/$version
  fi
  unzip -q Sparkle-$version.zip
  cd Sparkle-$version
  patch -p1 < $OPENSCADDIR/patches/sparkle.patch
  if $OPTION_32BIT; then
    SPARKLE_EXTRA_FLAGS="-arch i386"
  fi
  xcodebuild clean
  xcodebuild -arch x86_64 $SPARKLE_EXTRA_FLAGS
  rm -rf $DEPLOYDIR/lib/Sparkle.framework
  cp -Rf build/Release/Sparkle.framework $DEPLOYDIR/lib/ 
  install_name_tool -id $DEPLOYDIR/lib/Sparkle.framework/Versions/A/Sparkle $DEPLOYDIR/lib/Sparkle.framework/Sparkle
}

build_freetype()
{
  version="$1"
  extra_config_flags="$2"

  if [ -f $DEPLOYDIR/lib/libfreetype.dylib ]; then
    echo "freetype already installed. not building"
    return
  fi

  echo "Building freetype $version..."
  cd "$BASEDIR"/src
  rm -rf "freetype-$version"
  if [ ! -f "freetype-$version.tar.gz" ]; then
    curl --insecure -LO "http://downloads.sourceforge.net/project/freetype/freetype2/$version/freetype-$version.tar.gz"
  fi
  tar xzf "freetype-$version.tar.gz"
  cd "freetype-$version"
  PKG_CONFIG_LIBDIR="$DEPLOYDOR/lib/pkgconfig" ./configure --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN $extra_config_flags
  make -j"$NUMCPU"
  make install
}
 
build_libxml2()
{
  version="$1"

  if [ -f $DEPLOYDIR/lib/libxml2.dylib ]; then
    echo "libxml2 already installed. not building"
    return
  fi

  echo "Building libxml2 $version..."
  cd "$BASEDIR"/src
  rm -rf "libxml2-$version"
  if [ ! -f "libxml2-$version.tar.gz" ]; then
    curl --insecure -LO "ftp://xmlsoft.org/libxml2/libxml2-$version.tar.gz"
  fi
  tar xzf "libxml2-$version.tar.gz"
  cd "libxml2-$version"
  ./configure --prefix="$DEPLOYDIR" --with-zlib=/usr -without-lzma --without-ftp --without-http --without-python CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  make -j$NUMCPU
  make install
}

build_fontconfig()
{
  version=$1

  if [ -f $DEPLOYDIR/lib/libfontconfig.dylib ]; then
    echo "fontconfig already installed. not building"
    return
  fi

  echo "Building fontconfig $version..."
  cd "$BASEDIR"/src
  rm -rf "fontconfig-$version"
  if [ ! -f "fontconfig-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.freedesktop.org/software/fontconfig/release/fontconfig-$version.tar.gz"
  fi
  tar xzf "fontconfig-$version.tar.gz"
  cd "fontconfig-$version"
  export PKG_CONFIG_PATH="$DEPLOYDIR/lib/pkgconfig"
  ./configure --prefix="$DEPLOYDIR" --enable-libxml2 CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  unset PKG_CONFIG_PATH
  make -j$NUMCPU
  make install
}

build_libffi()
{
  version="$1"

  if [ -f $DEPLOYDIR/lib/libffi.dylib ]; then
    echo "libffi already installed. not building"
    return
  fi

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
}

build_gettext()
{
  version="$1"

  if [ -f $DEPLOYDIR/lib/libgettextlib.dylib ]; then
    echo "gettext already installed. not building"
    return
  fi

  echo "Building gettext $version..."
  cd "$BASEDIR"/src
  rm -rf "gettext-$version"
  if [ ! -f "gettext-$version.tar.xz" ]; then
    curl --insecure -LO "http://ftpmirror.gnu.org/gettext/gettext-$version.tar.gz"
  fi
  tar xzf "gettext-$version.tar.gz"
  cd "gettext-$version"

  ./configure --prefix="$DEPLOYDIR" CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN
  make -j$NUMCPU
  make install
}

build_glib2()
{
  version="$1"

  if [ -f $DEPLOYDIR/lib/libglib-2.0.dylib ]; then
    echo "glib2 already installed. not building"
    return
  fi

  echo "Building glib2 $version..."

  cd "$BASEDIR"/src
  rm -rf "glib-$version"
  maj_min_version="${version%.*}" #Drop micro
  if [ ! -f "glib-$version.tar.xz" ]; then
    curl --insecure -LO "http://ftp.gnome.org/pub/gnome/sources/glib/$maj_min_version/glib-$version.tar.xz"
  fi
  tar xJf "glib-$version.tar.xz"
  cd "glib-$version"

  export PKG_CONFIG_PATH="$DEPLOYDIR/lib/pkgconfig"
  ./configure --disable-gtk-doc --disable-man --prefix="$DEPLOYDIR" CFLAGS="-I$DEPLOYDIR/include -mmacosx-version-min=$MAC_OSX_VERSION_MIN" LDFLAGS="-L$DEPLOYDIR/lib -mmacosx-version-min=$MAC_OSX_VERSION_MIN"
  unset PKG_CONFIG_PATH
  make -j$NUMCPU
  make install
}

build_ragel()
{
  version=$1

  if [ -f $DEPLOYDIR/bin/ragel ]; then
    echo "ragel already installed. not building"
    return
  fi

  echo "Building ragel $version..."
  cd "$BASEDIR"/src
  rm -rf "ragel-$version"
  if [ ! -f "ragel-$version.tar.gz" ]; then
    curl --insecure -LO "http://www.colm.net/files/ragel/ragel-$version.tar.gz"
  fi
  tar xzf "ragel-$version.tar.gz"
  cd "ragel-$version"
  sed -e "s/setiosflags(ios::right)/std::&/g" ragel/javacodegen.cpp > ragel/javacodegen.cpp.new && mv ragel/javacodegen.cpp.new ragel/javacodegen.cpp
  ./configure --prefix="$DEPLOYDIR"
  make -j$NUMCPU
  make install
}

build_harfbuzz()
{
  version=$1
  extra_config_flags="$2"

  if [ -f $DEPLOYDIR/lib/libharfbuzz.dylib ]; then
    echo "harfbuzz already installed. not building"
    return
  fi

  echo "Building harfbuzz $version..."
  cd "$BASEDIR"/src
  rm -rf "harfbuzz-$version"
  if [ ! -f "harfbuzz-$version.tar.gz" ]; then
    curl --insecure -LO "http://cgit.freedesktop.org/harfbuzz/snapshot/harfbuzz-$version.tar.gz"
  fi
  tar xzf "harfbuzz-$version.tar.gz"
  cd "harfbuzz-$version"
  # disable doc directories as they make problems on Mac OS Build
  sed -e "s/SUBDIRS = src util test docs/SUBDIRS = src util test/g" Makefile.am > Makefile.am.bak && mv Makefile.am.bak Makefile.am
  sed -e "s/^docs.*$//" configure.ac > configure.ac.bak && mv configure.ac.bak configure.ac
  PKG_CONFIG_LIBDIR="$DEPLOYDIR/lib/pkgconfig" ./autogen.sh --prefix="$DEPLOYDIR" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no CFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN CXXFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN LDFLAGS=-mmacosx-version-min=$MAC_OSX_VERSION_MIN $extra_config_flags
  make -j$NUMCPU
  make install
}

if [ ! -f $OPENSCADDIR/openscad.pro ]; then
  echo "Must be run from the OpenSCAD source root directory"
  exit 0
fi
OPENSCAD_SCRIPTDIR=$PWD/scripts

while getopts '6lcd' c
do
  case $c in
    6) OPTION_32BIT=false;;
    l) OPTION_LLVM=true;;
    c) OPTION_CLANG=true;;
    d) OPTION_DEPLOY=true;;
  esac
done

OSX_VERSION=`sw_vers -productVersion | cut -d. -f2`
if (( $OSX_VERSION >= 8 )); then
  echo "Detected Mountain Lion (10.8) or later"
elif (( $OSX_VERSION >= 7 )); then
  echo "Detected Lion (10.7)"
else
  echo "Detected Snow Leopard (10.6) or earlier"
fi

USING_LLVM=false
USING_GCC=false
USING_CLANG=false
if $OPTION_LLVM; then
  USING_LLVM=true
elif $OPTION_GCC; then
  USING_GCC=true
elif $OPTION_CLANG; then
  USING_CLANG=true
elif (( $OSX_VERSION >= 7 )); then
  USING_CLANG=true
fi

if $USING_LLVM; then
  echo "Using gcc LLVM compiler"
  export CC=llvm-gcc
  export CXX=llvm-g++
  export QMAKESPEC=macx-llvm
elif $USING_GCC; then
  echo "Using gcc compiler"
  export CC=gcc
  export CXX=g++
  export CPP=cpp
  # Somehow, qmake in Qt-4.8.2 doesn't detect Lion's gcc and falls back into
  # project file mode unless manually given a QMAKESPEC
  export QMAKESPEC=macx-llvm
elif $USING_CLANG; then
  echo "Using clang compiler"
  export CC=clang
  export CXX=clang++
fi

echo "Building for $MAC_OSX_VERSION_MIN or later"

if [ ! $NUMCPU ]; then
  NUMCPU=$(sysctl -n hw.ncpu)
  echo "Setting number of CPUs to $NUMCPU"
fi

if $OPTION_DEPLOY; then
  echo "Building deployment version of libraries"
  OPTION_32BIT=true
else
  OPTION_32BIT=false
fi

if $OPTION_32BIT; then
  echo "Building combined 32/64-bit binaries"
else
  echo "Building 64-bit binaries"
fi

echo "Using basedir:" $BASEDIR
mkdir -p $SRCDIR $DEPLOYDIR
build_qt5 5.4.0
build_qscintilla 2.8.4
# NB! For eigen, also update the path in the function
build_eigen 3.2.3
build_gmp 5.1.3
build_mpfr 3.1.2
build_boost 1.57.0
# NB! For CGAL, also update the actual download URL in the function
build_cgal 4.5.1
build_glew 1.11.0
build_gettext 0.19.4
build_libffi 3.2.1
build_glib2 2.42.1
build_opencsg 1.4.0
build_freetype 2.5.5 --without-png
build_ragel 6.9
build_harfbuzz 0.9.37 "--with-coretext=auto --with-glib=no"
export FREETYPE_CFLAGS="-I$DEPLOYDIR/include -I$DEPLOYDIR/include/freetype2"
export FREETYPE_LIBS="-L$DEPLOYDIR/lib -lfreetype"
build_libxml2 2.9.2
build_fontconfig 2.11.1
if $OPTION_DEPLOY; then
#  build_sparkle andymatuschak 0ed83cf9f2eeb425d4fdd141c01a29d843970c20
  build_sparkle Cocoanetics 1e7dcb1a48b96d1a8c62100b5864bd50211cbae1
fi
