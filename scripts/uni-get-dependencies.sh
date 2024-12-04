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
  xorg-x11-server-Xvfb gettext qscintilla-qt5-devel \
  mesa-dri-drivers double-conversion-devel tbb-devel
}

get_fedora_deps_dnf()
{
 dnf -y install qt5-qtbase-devel bison flex eigen3-devel harfbuzz-devel \
  fontconfig-devel freetype-devel \
  boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc gcc-c++ pkgconfig \
  opencsg-devel git libXmu-devel curl ImageMagick glib2-devel make \
  xorg-x11-server-Xvfb gettext qscintilla-qt5-devel \
  mesa-dri-drivers libzip-devel ccache qt5-qtmultimedia-devel qt5-qtsvg-devel \
  double-conversion-devel tbb-devel
 dnf -y install libxml2-devel
 dnf -y install libffi-devel
 dnf -y install redhat-rpm-config
}

get_qomo_deps()
{
 get_fedora_deps
}

get_altlinux_deps()
{
 for i in boost-devel gcc4.5 gcc4.5-c++ boost-program_options-devel \
  boost-thread-devel boost-system-devel boost-regex-devel eigen3 \
  libmpfr libgmp libgmp_cxx-devel qt5-devel libcgal-devel git-core tbb-devel \
  libglew-devel flex bison curl imagemagick gettext glib2-devel; do apt-get install $i; done
}

get_freebsd_deps()
{
 pkg_add -r bison boost-libs cmake git bash eigen3 flex gmake gmp mpfr \
  xorg libGLU libXmu libXi xorg-vfbserver glew \
  qt5-core qt5-gui qt5-buildtools qt5-opengl qt5-qmake \
  opencsg cgal curl imagemagick glib2-devel gettext libdouble-conversion-3.0.0 \
  devel/onetbb
}

get_netbsd_deps()
{
 pkgin install bison boost cmake git bash eigen3 flex gmake gmp mpfr \
  qt5 glew cgal opencsg python27 curl \
  ImageMagick glib2 gettext threadingbuildingblocks
}

get_opensuse_deps()
{
 zypper install mpfr-devel gmp-devel boost-devel \
  glew-devel cmake git bison flex cgal-devel curl \
  glib2-devel gettext freetype-devel harfbuzz-devel  \
  qscintilla-qt5-devel libqt5-qtbase-devel libQt5OpenGL-devel \
  xvfb-run libzip-devel libqt5-qtmultimedia-devel libqt5-qtsvg-devel \
  double-conversion-devel libboost_regex-devel \
  libboost_program_options-devel tbb-devel
 # qscintilla-qt5-devel replaces libqscintilla_qt5-devel
 # but openscad compiles with both
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

get_mageia_deps()
{
 urpmi ctags
 urpmi task-c-devel task-c++-devel libqt5-devel libgmp-devel \
  libmpfr-devel libboost-devel eigen3-devel libglew-devel bison flex \
  cmake imagemagick glib2-devel python curl git x11-server-xvfb gettext \
  double-conversion-devel tbb
}

get_debian_deps()
{
 apt-get update
 apt-get -y install \
  build-essential bison flex git curl cmake ninja-build libffi-dev \
  libboost-program-options-dev libboost-regex-dev libboost-system-dev \
  libmpfr-dev libglew-dev libcairo2-dev libharfbuzz-dev \
  libeigen3-dev libcgal-dev libopencsg-dev libgmp-dev \
  imagemagick libfreetype6-dev libdouble-conversion-dev libxml2-dev \
  gtk-doc-tools libglib2.0-dev gettext xvfb pkg-config ragel libtbb-dev \
  libgl1-mesa-dev libxi-dev libxmu-dev libfontconfig-dev libzip-dev
 get_qt5_deps_debian
}

get_qt5_deps_debian()
{
 apt-get -y install qtbase5-dev libqscintilla2-qt5-dev libqt5opengl5-dev \
  libqt5svg5-dev qtmultimedia5-dev libqt5multimedia5-plugins qt5-qmake
}

get_arch_deps()
{
  pacman -S --noconfirm \
	base-devel gcc bison flex make libzip \
	qt5 qscintilla-qt5 cgal gmp mpfr boost opencsg \
	glew eigen glib2 fontconfig freetype2 harfbuzz \
	double-conversion imagemagick tbb
}

get_solus_deps()
{
  eopkg -y it -c system.devel
  eopkg -y install qt5-base-devel qt5-multimedia-devel qt5-svg-devel qscintilla-devel \
	CGAL-devel gmp-devel mpfr-devel glib2-devel libboost-devel \
	opencsg-devel glew-devel eigen3 \
	fontconfig-devel freetype2-devel harfbuzz-devel libzip-devel \
	double-conversion-devel \
	bison flex intel-tbb-devel
}

unknown()
{
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager."
}

if [ -e /etc/issue ]; then
 if [ "`grep -i ubuntu /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i KDE.neon /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep ID=.solus /etc/os-release`" ]; then
  get_solus_deps
 elif [ "`grep -i debian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i raspbian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i linux.mint /etc/issue`" ]; then
  get_debian_deps
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
 elif [ "`grep -i red.hat /etc/issue`" ]; then
  get_fedora_deps
 elif [ "`grep -i mageia /etc/issue`" ]; then
  get_mageia_deps
 elif [ "`grep -i qomo /etc/issue`" ]; then
  get_qomo_deps
 elif [ "`grep -i arch /etc/issue`" ]; then
   get_arch_deps
 elif [ -e /etc/fedora-release ]; then
  if [ "`grep -i fedora.release /etc/fedora-release`" ]; then
    get_fedora_deps_dnf
  fi
 elif [ "`command -v rpm`" ]; then
  if [ "`rpm -qa | grep altlinux`" ]; then
   get_altlinux_deps
  fi
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
