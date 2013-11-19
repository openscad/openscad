#
# This script contains functions for building various libraries
# used by OpenSCAD.
# It's supposed to be included from the system specific scripts.
#

build_freetype()
{
  version="$1"
  extra_config_flags="$2"

  if [ -e "$DEPLOYDIR/include/freetype2" ]; then
    echo "freetype already installed. not building"
    return
  fi

  echo "Building freetype $version..."
  cd "$BASEDIR"/src
  rm -rf "freetype-$version"
  if [ ! -f "freetype-$version.tar.gz" ]; then
    curl --insecure -LO "http://download.savannah.gnu.org/releases/freetype/freetype-$version.tar.gz"
  fi
  tar xzf "freetype-$version.tar.gz"
  cd "freetype-$version"
  ./configure --prefix="$DEPLOYDIR" $extra_config_flags
  make -j"$NUMCPU"
  make install
}
 
build_libxml2()
{
  version="$1"

  if [ -e $DEPLOYDIR/include/libxml2 ]; then
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
  ./configure --prefix="$DEPLOYDIR" --without-docbook --without-ftp --without-html --without-http --without-legacy --without-python --without-sax1
  make -j$NUMCPU
  make install
}

build_fontconfig()
{
  version=$1

  if [ -e $DEPLOYDIR/include/fontconfig ]; then
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
  ./configure --prefix="$DEPLOYDIR" --enable-libxml2
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
    curl --insecure -LO "http://www.complang.org/ragel/ragel-$version.tar.gz"
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

  if [ -e $DEPLOYDIR/include/harfbuzz ]; then
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
  ./autogen.sh --prefix="$DEPLOYDIR" --with-freetype=yes --with-gobject=no --with-cairo=no --with-icu=no $extra_config_flags
  make -j$NUMCPU
  make install
}
