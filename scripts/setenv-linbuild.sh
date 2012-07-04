# setup env variables for building OpenSCAD against custom built
# dependency libraries from linux-build-dependencies.sh

# run this file with 'source setenv-linbuild.sh'

# BASEDIR and DEPLOYDIR must be the same as in linux-build-dependencies.sh
BASEDIR=$HOME/openscad_deps
DEPLOYDIR=$BASEDIR

PATH=$BASEDIR/bin:$PATH
LD_LIBRARY_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
LD_RUN_PATH=$DEPLOYDIR/lib:$DEPLOYDIR/lib64
OPENSCAD_LIBRARIES=$DEPLOYDIR
GLEWDIR=$DEPLOYDIR

echo PATH modified
echo LD_LIBRARY_PATH modified
echo LD_RUN_PATH modified
echo OPENSCAD_LIBRARIES modified
echo GLEWDIR modified

