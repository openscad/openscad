# setup environment variables for building OpenSCAD against custom built
# dependency libraries.
#
# run with 'source ./scripts/setenv-unibuild.sh'
#
# run it every time you re-login and want to build or run openscad
# against custom libraries installed into BASEDIR.
#
# used in conjuction with uni-build-dependencies.sh

setenv_common()
{
 if [ ! $BASEDIR ]; then
  BASEDIR=$HOME/openscad_deps
 fi
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

 if [ "`command -v qmake-qt4`" ]; then
 	echo "Please re-run qmake-qt4 and run 'make clean' if necessary"
 else
 	echo "Please re-run qmake and run 'make clean' if necessary"
 fi
}

setenv_freebsd()
{
 setenv_common
 QMAKESPEC=freebsd-g++
 QTDIR=/usr/local/share/qt4
}

setenv_linux_clang()
{
 export CC=clang
 export CXX=clang++
 export QMAKESPEC=unsupported/linux-clang

 echo CC has been modified: $CC
 echo CXX has been modified: $CXX
 echo QMAKESPEC has been modified: $QMAKESPEC
}

if [ "`uname | grep -i 'linux\|debian'`" ]; then
 setenv_common
 if [ "`echo $* | grep clang`" ]; then
  setenv_linux_clang
 fi
elif [ "`uname | grep -i freebsd`" ]; then
 setenv_freebsd
fi
