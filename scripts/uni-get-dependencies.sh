
get_fedora_deps()
{
 echo "Tested on Fedora 17. Please see README.md for info on older systems."
 sudo yum install qt-devel bison flex eigen2-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc pkgconfig git
}

get_freebsd_deps()
{
 echo "Tested on FreeBSD 9. Please see README.md for info on older systems."

 if [ "`pkg_info | grep -i cgal `" ]; then
  echo Stopping. Please remove any CGAL packages you have installed and restart
  exit
 fi

 if [ "`pkg_info | grep -i opencsg`" ]; then
  echo Stopping. Please remove any OpenCSG packages you have installed and restart
  exit
 fi

 pkg_add -r bison boost-libs cmake git bash eigen2 flex gmake gmp mpfr
 pkg_add -r xorg libGLU libXmu libXi xorg-vfbserver glew
 pkg_add -r qt4-corelib qt4-gui qt4-moc qt4-opengl qt4-qmake qt4-rcc qt4-uic

 #echo "BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh cgal-use-sys-libs"
 #echo "BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg"
}


debian_too_old()
{
	echo "System version too low. Please try 'old linux' build (see README.md)"
	exit
}

get_debian_deps()
{
 if [ "`cat /etc/issue | grep 'Debian GNU/Linux 6.0'`" ]; then
  debian_too_old
 fi
 if [ "`cat /etc/issue | grep 'Debian GNU/Linux 5'`" ]; then
  debian_too_old
 fi
 if [ "`cat /etc/issue | grep 'Ubunutu 10'`" ]; then
  debian_too_old
 fi
 if [ "`cat /etc/issue | grep 'Ubunutu 9'`" ]; then
  debian_too_old
 fi
 if [ "`cat /etc/issue | grep 'Ubunutu 8'`" ]; then
  debian_too_old
 fi
 if [ "`cat /etc/issue | grep 'Ubunutu 7'`" ]; then
  debian_too_old
 fi
 echo "tested on Ubuntu 12. If this fails try 'old linux' build (see README.md)"

 sudo apt-get install build-essential libqt4-dev libqt4-opengl-dev \
  libxmu-dev cmake bison flex libeigen2-dev git-core libboost-all-dev \
  libXi-dev libmpfr-dev libgmp-dev libboost-dev libglew1.6-dev \
  libcgal-dev libopencsg-dev
}


get_opensuse_deps()
{
 echo "tested on OpenSUSE 12. If this fails try 'old linux' build (see README.md)"

 sudo zypper install libeigen2-devel mpfr-devel gmp-devel boost-devel \
  libqt4-devel glew-devel cmake git

 # sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg
}




if [ "`cat /etc/issue | grep -i ubuntu`" ]; then
 get_debian_deps
elif [ "`cat /etc/issue | grep -i debian`" ]; then
 get_ubuntu_deps
elif [ "`cat /etc/issue | grep -i opensuse`" ]; then
 get_opensuse_deps
elif [ "`cat /etc/issue | grep -i freebsd`" ]; then
 get_freebsd_deps
elif [ "`cat /etc/issue | grep  -i fedora`" ]; then
 get_fedora_deps
else
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager."
fi

