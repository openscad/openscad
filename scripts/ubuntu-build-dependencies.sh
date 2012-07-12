
too_old()
{
	echo "System version too low. Please try 'old linux' build (see README.md)"
	exit
}

if [ "`cat /etc/issue | grep 'Debian GNU/Linux 6.0'`" ]; then
	too_old
fi
if [ "`cat /etc/issue | grep 'Debian GNU/Linux 5'`" ]; then
	too_old
fi
if [ "`cat /etc/issue | grep 'Ubunutu 10'`" ]; then
	too_old
fi
if [ "`cat /etc/issue | grep 'Ubunutu 9'`" ]; then
	too_old
fi
if [ "`cat /etc/issue | grep 'Ubunutu 8'`" ]; then
	too_old
fi
if [ "`cat /etc/issue | grep 'Ubunutu 7'`" ]; then
	too_old
fi

echo "tested on Ubuntu 12. If this fails try 'old linux' build (see README.md)"


if [ "`dpkg --list | grep -i cgal`" ]; then
	echo "Please make sure you have run apt-get purge on all cgal packages"
  exit
fi

if [ "`dpkg --list | grep -i opencsg`" ]; then
	echo "Please make sure you have run apt-get purge on all opencsg packages"
  exit
fi

sudo apt-get install build-essential libqt4-dev libqt4-opengl-dev \
 libxmu-dev cmake bison flex libeigen2-dev git-core libboost-all-dev \
 libXi-dev libmpfr-dev libgmp-dev libboost-dev libglew1.6-dev

echo "now copy/paste the following to install CGAL and OpenCSG from source:"
echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh cgal-use-sys-libs"
echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg"
