# setup environment variables for building OpenSCAD against custom built
# dependency libraries. works on Linux/BSD.
#
# Please see the 'uni-build-dependencies.sh' file for usage information
#

setenv_common()
{
 if [ ! $BASEDIR ]; then
  if [ -f openscad.pro ]; then
    # if in main openscad dir, put under $HOME
    BASEDIR=$HOME/openscad_deps
  else
    # otherwise, assume its being run 'out of tree'. treat it somewhat like
    # "configure" or "cmake", so you can build dependencies where u wish.
    echo "Warning: Not in OpenSCAD src dir... using current directory as base of build"
    BASEDIR=$PWD/openscad_deps
  fi
 fi
 DEPLOYDIR=$BASEDIR

 export BASEDIR
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

 if [ "`uname -m | grep sparc64`" ]; then
   echo detected sparc64. forcing 32 bit with export ABI=32
   ABI=32
   export ABI
 fi
}

setenv_freebsd()
{
 setenv_common
 QMAKESPEC=freebsd-g++
 QTDIR=/usr/local/share/qt4
}

setenv_netbsd()
{
 setenv_common
 QMAKESPEC=netbsd-g++
 QTDIR=/usr/pkg/qt4
 PATH=/usr/pkg/qt4/bin:$PATH
 LD_LIBRARY_PATH=/usr/pkg/qt4/lib:$LD_LIBRARY_PATH
 LD_LIBRARY_PATH=/usr/X11R7/lib:$LD_LIBRARY_PATH
 LD_LIBRARY_PATH=/usr/pkg/lib:$LD_LIBRARY_PATH

 export QMAKESPEC
 export QTDIR
 export PATH
 export LD_LIBRARY_PATH
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

clean_note()
{
 if [ $QT5_SETUP ]; then
  QMAKEBIN=qmake
 elif [ "`command -v qmake-qt4`" ]; then
  QMAKEBIN=qmake-qt4
 else
  QMAKEBIN=qmake
 fi
 echo "Please re-run" $QMAKEBIN "and run 'make clean' if necessary"
}

setenv_qt5()
{
 QT5_SETUP=true
 if [ ! $QTDIR ]; then
  QTDIR=/opt/qt5
  echo Please set QTDIR before running this qt5 script. Assuming $QTDIR
 fi
 PATH=$QTDIR/bin:$PATH
 LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
 LD_RUN_PATH=$QTDIR/lib:$LD_RUN_PATH
 if [ "`echo $CC | grep clang`" ]; then
  if [ "`uname | grep -i linux`" ]; then
   QMAKESPEC=linux-clang
   echo QMAKESPEC has been modified: $QMAKESPEC
  fi
 fi

 export QTDIR
 export PATH
 export LD_LIBRARY_PATH
 export LD_RUN_PATH
 export QMAKESPEC

 echo QTDIR is set to: $QTDIR
 echo PATH has been modified with $QTDIR/bin
 echo LD_LIBRARY_PATH has been modified with $QTDIR/lib
 echo LD_RUN_PATH has been modified with $QTDIR/lib

 export QT5_SETUP
}

if [ "`uname | grep -i 'linux\|debian'`" ]; then
 setenv_common
 if [ "`echo $* | grep clang`" ]; then
  setenv_linux_clang
 fi
elif [ "`uname | grep -i freebsd`" ]; then
 setenv_freebsd
elif [ "`uname | grep -i netbsd`" ]; then
 setenv_netbsd
else
 # guess
 setenv_common
 echo unknown system. guessed env variables. see 'setenv-unibuild.sh'
fi

if [ "`echo $* | grep qt5`" ]; then
 setenv_qt5
fi

if [ -e $DEPLOYDIR/include/Qt ]; then
  echo "Qt found under $DEPLOYDIR ... "
  QTDIR=$DEPLOYDIR
  export QTDIR
  echo "QTDIR modified to $DEPLOYDIR"
fi

clean_note

