export OPENSCAD_LIBRARIES=$PWD/../libraries/install
export DYLD_LIBRARY_PATH=$OPENSCAD_LIBRARIES/lib
export DYLD_FRAMEWORK_PATH=$OPENSCAD_LIBRARIES/lib
#export QMAKESPEC=macx-g++

#export OPENCSGDIR=$PWD/../OpenCSG-1.3.0
#export CGALDIR=$PWD/../install/CGAL-3.6
#export DYLD_LIBRARY_PATH=$OPENCSGDIR/lib

# Our own Qt
export PATH=$OPENSCAD_LIBRARIES/bin:$PATH

# ccache:
export PATH=/opt/local/libexec/ccache:$PATH
export CCACHE_BASEDIR=$PWD/..
