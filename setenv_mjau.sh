export OPENSCAD_LIBRARIES=$PWD/../libraries/install
export DYLD_LIBRARY_PATH=$OPENSCAD_LIBRARIES/lib

#export OPENCSGDIR=$PWD/../OpenCSG-1.3.0
#export CGALDIR=$PWD/../install/CGAL-3.6
#export QCODEEDITDIR=$PWD/../qcodeedit-2.2.3/install
#export DYLD_LIBRARY_PATH=$OPENCSGDIR/lib:$QCODEEDITDIR/lib

export QTDIR=`qmake -query "QT_INSTALL_PREFIX"`
export PATH=$QTDIR/bin:$PATH

# ccache:
export PATH=/opt/local/libexec/ccache:$PATH
export CCACHE_BASEDIR=$PWD/..
