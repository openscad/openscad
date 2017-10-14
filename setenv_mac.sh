export OPENSCAD_LIBRARIES=$PWD/../libraries/install
export PKG_CONFIG_PATH=$OPENSCAD_LIBRARIES/lib/pkgconfig
export DYLD_LIBRARY_PATH=$OPENSCAD_LIBRARIES/lib
export DYLD_FRAMEWORK_PATH=$OPENSCAD_LIBRARIES/lib

# Our own Qt
export PATH=$OPENSCAD_LIBRARIES/bin:$PATH
unset QMAKESPEC

# ccache:
export PATH=/opt/local/libexec/ccache:$PATH
export CCACHE_BASEDIR=$PWD/..
