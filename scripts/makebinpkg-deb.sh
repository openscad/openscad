#!/bin/sh

# build a simple .deb package.
# 1. build and install files using 'qmake PREFIX=$DEPLOYDIR;make install'
# 2. run this script
# 3. resulting .deb should be installed with 'sudo gdebi'.
#    resulting .deb file does not contain dependencies, only a list of them

OPENSCADDIR=$1
DEPLOYDIR=$2
OPENSCAD_VERSION=$3

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
ARCH=$DEB_HOST_ARCH
MACHINE_TRIPLE=$DEB_HOST_GNU_TYPE

if [ ! $OPENSCAD_VERSION ]; then
  OPENSCAD_VERSION=`date "+%Y.%m.%d"`
  OPENSCADDIR=`pwd`
  DEPLOYDIR=$OPENSCADDIR/bin/$MACHINE_TRIPLE
  mkdir $DEPLOYDIR
  cd $DEPLOYDIR
fi

BINARYFILE=$DEPLOYDIR/bin/openscad
DPKGNAME=openscad_$OPENSCAD_VERSION-$TARGETOS-$ARCH
DPKGFILE=$DPKGNAME.deb
ESCAPED_VERSION_ID=`echo $VERSION_ID | sed -e s/\\\./_/g -`

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
TOPDIR=$DEPLOYDIR/$DPKGNAME/usr/local
find ./bin -depth -print | cpio -pud $TOPDIR
find ./share -depth -print | cpio -pud $TOPDIR

DEBDIR=$DEPLOYDIR/$DPKGNAME/DEBIAN
CONTROLFILE=$DEBDIR/control
mkdir -p $DEBDIR
cat << EOF > $CONTROLFILE
Package: openscad
Version: $OPENSCAD_VERSION
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

