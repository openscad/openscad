export OPENSCAD_LIBRARIES=$PWD/../libraries/install
export DYLD_LIBRARY_PATH=$OPENSCAD_LIBRARIES/lib
export QMAKESPEC=unsupported/macx-clang

#export OPENCSGDIR=$PWD/../OpenCSG-1.3.0
#export CGALDIR=$PWD/../install/CGAL-3.6
#export DYLD_LIBRARY_PATH=$OPENCSGDIR/lib

# ccache:
export PATH=/opt/local/libexec/ccache:$PATH
export CCACHE_BASEDIR=$PWD/..
