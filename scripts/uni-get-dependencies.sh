# auto-install dependency packages using the systems package manager.
# after running this, run ./script/check-dependencies.sh. see README.md
#
# this assumes you have sudo installed and running, or are running as root.
#

get_fedora_deps_yum()
{
 yum -y install qt5-qtbase-devel bison flex eigen3-devel harfbuzz-devel \
  fontconfig-devel freetype-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc gcc-c++ pkgconfig \
  opencsg-devel git libXmu-devel curl imagemagick ImageMagick glib2-devel make \
  xorg-x11-server-Xvfb gettext qscintilla-devel qscintilla-qt5-devel \
  mesa-dri-drivers
}

get_fedora_deps_dnf()
{
 dnf -y install qt5-qtbase-devel bison flex eigen3-devel harfbuzz-devel \
  fontconfig-devel freetype-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc gcc-c++ pkgconfig \
  opencsg-devel git libXmu-devel curl ImageMagick glib2-devel make \
  xorg-x11-server-Xvfb gettext qscintilla-devel qscintilla-qt5-devel \
  qt5-qtmultimedia-devel mesa-dri-drivers libzip-devel ccache \
  libxml2-devel libffi-devel redhat-rpm-config
}

get_opensuse_deps()
{
 zypper install mpfr-devel gmp-devel boost-devel \
  glew-devel cmake git bison flex cgal-devel curl \
  glib2-devel gettext freetype-devel harfbuzz-devel  \
  libqscintilla-qt5-devel libqt5-qtbase-devel libQt5OpenGL-devel \
  libqt5-qtmultimedia-devel xvfb-run libzip-devel
 zypper install libeigen3-devel
 if [ $? -ne 0 ]; then
  zypper install libeigen3
 fi
 zypper install ImageMagick
 if [ $? -ne 0 ]; then
  zypper install imagemagick
 fi
 zypper install opencsg-devel
 if [ $? -ne 0 ]; then
  pver=`cat /etc/os-release | grep -i pretty_name | sed s/PRETTY_NAME=//g`
  pver=`echo $pver | sed s/\"//g | sed s/\ /_/g `
  echo attempting to add graphics repository for opencsg...
  set +x
  zypper ar -f http://download.opensuse.org/repositories/graphics/$pver graphics
  zypper install opencsg-devel
  set -x
 fi
}

get_debian_deps()
{
  apt-get -y install \
    build-essential curl libffi-dev libxmu-dev cmake bison flex \
    git-core libboost-all-dev libmpfr-dev libboost-dev libglew-dev \
    libeigen3-dev libcgal-dev libopencsg-dev libgmp3-dev libgmp-dev \
    imagemagick libfreetype6-dev gtk-doc-tools libglib2.0-dev gettext \
    xvfb pkg-config ragel libxi-dev libfontconfig1-dev libzip-dev \
    qtbase5-dev libqt5scintilla2-dev qtmultimedia5-dev libqt5opengl5-dev \
    qt5-qmake libharfbuzz-dev libxml2-dev
}

get_ubuntu_16_deps()
{
  get_debian_deps
  # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=804539
  apt-get -y install libcgal-qt5-dev
}

get_arch_deps()
{
  pacman -S --noconfirm qt5 qscintilla-qt5 cgal gmp mpfr boost \
    opencsg glew eigen glib2 fontconfig freetype2 harfbuzz bison flex make
}

unknown()
{
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager."
}

if [ -e /etc/issue ]; then
 if [ "`grep -i ubuntu.1[4-5] /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i ubuntu.1[6-7] /etc/issue`" ]; then
  get_ubuntu_16_deps
 elif [ "`grep -i ubuntu /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i debian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i raspbian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i linux.mint.1[89] /etc/issue`" ]; then
  get_ubuntu_16_deps
 elif [ "`grep -i suse /etc/issue`" ]; then
  get_opensuse_deps
 elif [ "`grep -i fedora.release.2[2-9] /etc/issue`" ]; then
  get_fedora_deps_dnf
 elif [ "`grep -i fedora.release.[3-9][0-9] /etc/issue`" ]; then
  get_fedora_deps_dnf
 elif [ "`grep -i fedora.release.2[0-1] /etc/issue`" ]; then
  get_fedora_deps_yum
 elif [ "`grep -i fedora /etc/issue`" ]; then
  get_fedora_deps_yum
 elif [ "`grep -i arch /etc/issue`" ]; then
   get_arch_deps
 elif [ -e /etc/fedora-release ]; then
  if [ "`grep -i fedora.release /etc/fedora-release`" ]; then
    get_fedora_deps_dnf
  fi
 else
  unknown
 fi
else
 unknown
fi

