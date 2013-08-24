export OPENSCAD_LIBRARIES=$PWD/../libraries/install
export DYLD_LIBRARY_PATH=$OPENSCAD_LIBRARIES/lib
export DYLD_FRAMEWORK_PATH=$OPENSCAD_LIBRARIES/lib
export QMAKESPEC=macx-g++

# Our own Qt
export PATH=$OPENSCAD_LIBRARIES/bin:$PATH

# ccache:
export PATH=/opt/local/libexec/ccache:$PATH
export CCACHE_BASEDIR=$PWD/..


# Find the BOOSTDIR if it was installed by port, fink or brew
if [ -z $BOOSTDIR ]
then
    boostlib=`find {/opt/local,/sw,/usr/local}/lib -name libboost\* 2> /dev/null | head -1`
    if [ -z $boostlib ]
    then
        echo 'WARNING: cannot find boost libraries in any of the normal places.'
        echo '$BOOSTDIR may need to be set manually.'
    else
        boostlibdir=`dirname $boostlib`
        export BOOSTDIR=`dirname $boostlibdir`
    fi
fi
