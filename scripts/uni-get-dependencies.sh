# auto-install dependency packages using the systems package manager.
# after running this, run ./script/check-dependencies.sh. see README.md
#
# this assumes you have sudo installed or are running as root.
#

get_fedora_deps()
{
 sudo yum install qt-devel bison flex eigen2-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc pkgconfig git
}

get_freebsd_deps()
{
 pkg_add -r bison boost-libs cmake git bash eigen2 flex gmake gmp mpfr \
  xorg libGLU libXmu libXi xorg-vfbserver glew \
  qt4-corelib qt4-gui qt4-moc qt4-opengl qt4-qmake qt4-rcc qt4-uic \
  opencsg cgal
}

get_netbsd_deps()
{
 sudo pkgin install bison boost cmake git bash eigen flex gmake gmp mpfr \
  qt4 glew cgal opencsg modular-xorg
}

get_opensuse_deps()
{
 sudo zypper install libeigen2-devel mpfr-devel gmp-devel boost-devel \
  libqt4-devel glew-devel cmake git bison flex cgal-devel opencsg-devel
}

get_mageia_deps()
{
 pklist="task-c-devel task-c++-devel"
 pklist="$pklist libqt4-devel libgmp-devel libmpfr-devel"
 pklist="$pklist libboost-devel eigen3-devel libglew-devel"
 pklist="$pklist bison flex"
 pklist="$pklist cmake imagemagick python curl git"
 # cgal + opencsg don't exist
 sudo urpmi ctags
 sudo urpmi $pklist
}

get_debian_deps()
{
 for pkg in build-essential libqt4-dev libqt4-opengl-dev \
  libxmu-dev cmake bison flex git-core libboost-all-dev \
  libXi-dev libmpfr-dev libboost-dev libglew-dev libeigen2-dev \
  libeigen3-dev libcgal-dev libopencsg-dev libgmp3-dev libgmp-dev; do
   sudo apt-get -y install $pkg;
 done
}


unknown()
{
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager."
}

if [ -e /etc/issue ]; then
 if [ "`grep -i ubuntu /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i debian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i suse /etc/issue`" ]; then
  get_opensuse_deps
 elif [ "`grep -i fedora /etc/issue`" ]; then
  get_fedora_deps
 elif [ "`grep -i red.hat /etc/issue`" ]; then
  get_fedora_deps
 elif [ "`grep -i mageia /etc/issue`" ]; then
  get_mageia_deps
 else
  unknown
 fi
elif [ "`uname | grep -i freebsd `" ]; then
 get_freebsd_deps
elif [ "`uname | grep -i netbsd`" ]; then
 get_netbsd_deps
else
 unknown
fi

