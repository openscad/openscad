#!/bin/sh

# build a simple .deb package.
# this does not conform to debian or ubuntu rules and is not suitable
# for putting into a repository. however it is good for simple installation
# on your local system for experimentation.

if [ ! -e ./openscad.pro ]; then
	echo please run from openscad root source dir.
	exit
fi

if [ ! -e /etc/os-release ]; then
	echo sorry this requires an OS with a file /etc/os-release
	exit
fi

if [ ! "`command -v dpkg`" ]; then
	echo this requires dpkg
	exit
fi

# set variables

OSINFO=`cat /etc/os-release`
eval $OSINFO
DEBINFO=`dpkg-architecture`
eval $DEBINFO
TARGETOS=`echo $ID'_'$VERSION_ID`
ARCH=$DEB_TARGET_ARCH
MACHINE_TRIPLE=$DEB_TARGET_GNU_TYPE

OSCADVERSION=`date "+%Y.%m.%d"`
OPENSCADDIR=`pwd`
DEPLOYDIR=$OPENSCADDIR/bin/$MACHINE_TRIPLE
BINARYFILE=$DEPLOYDIR/openscad
DPKGNAME=openscad_$OSCADVERSION-$TARGETOS-$ARCH
DPKGFILE=$DPKGNAME.deb
ESCAPED_VERSION_ID=`echo $VERSION_ID | sed -e s/\\\./_/g -`

mkdir -p $DEPLOYDIR
cd $DEPLOYDIR
qmake ../..
if [ "`echo $* | grep dry`" ]; then
  cp `which ls` openscad
else
  make
fi

if [ ! $? -eq 0 ]; then
  echo make failed. exiting makedpkg
  exit
fi

# find dependency package list using ldd and dpkg -S

DEPLIBS=`ldd $BINARYFILE | awk ' { print $3 } ';`
PKGLIST=
for dlib in `echo $DEPLIBS`; do
	if [ -e $dlib ]; then
		RAWPKGNAME=`dpkg -S $dlib`
		if [ ! $? -eq 0 ]; then
			echo 'unknown package for dependency '$dlib
			echo 'please use system dependencies when building deb'
			exit
                fi
		PKGNAME=`echo $RAWPKGNAME | sed -e s/":.*"//g - `

		echo -n 'dpkg -S:' `basename $dlib`' is from'
		echo ' '$PKGNAME
		if [ ! "`echo $PKGLIST | grep $PKGNAME`" ]; then
			PKGLIST=$PKGLIST' '$PKGNAME
		else
			echo ' already added '$PKGNAME
		fi
	fi
done
echo 'dependency package list: '$PKGLIST

# make package

cd $DEPLOYDIR
BINDIR=$DEPLOYDIR/$DPKGNAME/usr/local/bin
DEBDIR=$DEPLOYDIR/$DPKGNAME/DEBIAN
CONTROLFILE=$DEBDIR/control
mkdir -p $BINDIR
mkdir -p $DEBDIR

cp -a ./openscad $BINDIR/

cat << EOF > $CONTROLFILE
Package: openscad
Version: $OSCADVERSION
Section: graphics
Priority: optional
Architecture: $ARCH
Maintainer: $USER ($USER@$HOSTNAME)
Description: OpenSCAD
 The programmer's solid modeller
Depends: debhelper,
EOF

sed -i '/^\s*$/d' $CONTROLFILE
for dep in $PKGLIST; do
  echo " "$dep"," >> $CONTROLFILE
done
echo " debhelper" >> $CONTROLFILE

cat $CONTROLFILE

dpkg-deb -v -D --build $DPKGNAME
if [ ! $? -eq 0 ]; then
  echo dpkg-deb failed. exiting makedpkg
  exit
fi

echo in directory $DEPLOYDIR
echo to install, please run:
echo '   'sudo gdebi $DPKGFILE

