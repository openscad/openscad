# BASEDIR and DEPLOYDIR must be the same as in linux-build-dependencies.sh
BASEDIR=$HOME/openscad_deps
DEPLOYDIR=$BASEDIR

export PATH=$BASEDIR/bin:$PATH
export LD_LIBRARY_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
export LD_RUN_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
export OPENSCAD_LIBRARIES=$DEPLOYDIR
export GLEWDIR=$DEPLOYDIR

echo BASEDIR: $BASEDIR
echo DEPLOYDIR: $DEPLOYDIR
echo PATH modified
echo LD_LIBRARY_PATH modified
echo LD_RUN_PATH modified
echo OPENSCAD_LIBRARIES modified
echo GLEWDIR modified

