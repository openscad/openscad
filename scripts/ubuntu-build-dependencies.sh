
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

sudo apt-get install build-essential libqt4-dev libqt4-opengl-dev \
 libxmu-dev cmake bison flex libeigen2-dev git-core libboost-all-dev \
 libXi-dev libmpfr-dev libgmp-dev libboost-dev libglew1.6-dev \
 libcgal-dev libopencsg-dev

