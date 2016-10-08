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
 echo .... freebsd detected. 
 setenv_common
 if [ "`uname -a |grep -i freebsd.9`" ]; then
   echo freebsd9 is unsupported, please use freebsd 11
 elif [ "`uname -a |grep -i freebsd.10`" ]; then
   echo freebsd10 is unsupported, please use freebsd 11
 else
   QMAKESPEC=freebsd-clang
   CC=clang
   CXX=clang++
 fi
 QTDIR=/usr/local/share/qt4
 PATH=/usr/local/lib/qt4/bin:$PATH
 export PATH
 export QMAKESPEC
 export QTDIR
 export CC
 export CXX
 echo QMAKESPEC $QMAKESPEC
 echo QTDIR $QTDIR
 echo CXX CC $CXX $CC
}

setenv_netbsd()
{
 setenv_common
 if [ "` uname -a |grep NetBSD.6 `" ]; then
   echo sorry, recommend NetBSD 7 or more, you will have to hack 6 yourself
 fi
 # we have to use qt4 here because netbsd7 has no qscintilla for qt5
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

setenv_netbsd_clang()
{
 echo --------------------- this is not yet supported. netbsd 6 lacks
 echo --------------------- certain things needed for clang support
 export CC=clang
 export CXX=clang++
 export QMAKESPEC=./patches/mkspecs/netbsd-clang

 echo CC has been modified: $CC
 echo CXX has been modified: $CXX
 echo QMAKESPEC has been modified: $QMAKESPEC
}

clean_note()
{
 if [ "`command -v qmake-qt4`" ]; then
  QMAKEBIN=qmake-qt4
 else
  QMAKEBIN=qmake
 fi
 echo "Please re-run" $QMAKEBIN "and run 'make clean' if necessary"
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
 if [ "`echo $* | grep clang`" ]; then
  setenv_netbsd_clang
 fi
else
 # guess
 setenv_common
 echo unknown system. guessed env variables. see 'setenv-unibuild.sh'
fi

if [ -e $DEPLOYDIR/include/Qt ]; then
  echo "Qt found under $DEPLOYDIR ... "
  QTDIR=$DEPLOYDIR
  export QTDIR
  echo "QTDIR modified to $DEPLOYDIR"
fi

clean_note

