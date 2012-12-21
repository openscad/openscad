
get_fedora_deps()
{
 echo "Tested on Fedora 17"
 sudo yum install qt-devel bison flex eigen2-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc pkgconfig git
}

get_freebsd_deps()
{
 echo "Tested on FreeBSD 9"
 pkg_add -r bison boost-libs cmake git bash eigen2 flex gmake gmp mpfr
 pkg_add -r xorg libGLU libXmu libXi xorg-vfbserver glew
 pkg_add -r qt4-corelib qt4-gui qt4-moc qt4-opengl qt4-qmake qt4-rcc qt4-uic
 pkg_add -r opencsg cgal
}

get_opensuse_deps()
{
 sudo zypper install libeigen2-devel mpfr-devel gmp-devel boost-devel \
  libqt4-devel glew-devel cmake git bison flex cgal-devel opencsg-devel
}

get_debian_deps()
{
 echo "Tested on Ubuntu 12.04"

 sudo apt-get install build-essential libqt4-dev libqt4-opengl-dev \
  libxmu-dev cmake bison flex libeigen2-dev git-core libboost-all-dev \
  libXi-dev libmpfr-dev libboost-dev libglew1.6-dev \
  libcgal-dev libopencsg-dev

 if [ "`apt-cache search libgmp | grep libgmp3-dev`" ]; then
   sudo apt-get install libgmp3-dev
 else
   sudo apt-get install libgmp-dev
 fi
}


if [ "`cat /etc/issue | grep -i ubuntu`" ]; then
 get_debian_deps
elif [ "`cat /etc/issue | grep -i debian`" ]; then
 get_debian_deps
elif [ "`cat /etc/issue | grep -i opensuse`" ]; then
 get_opensuse_deps
elif [ "`cat /etc/issue | grep -i freebsd`" ]; then
 get_freebsd_deps
elif [ "`cat /etc/issue | grep  -i fedora`" ]; then
 get_fedora_deps
elif [ "`cat /etc/issue | grep  -i redhat`" ]; then
 get_fedora_deps
else
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager."
fi

