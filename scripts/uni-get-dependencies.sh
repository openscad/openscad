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
  mesa-dri-drivers
}

get_qomo_deps()
{
 get_fedora_deps
}

get_altlinux_deps()
{
 for i in boost-devel boost-filesystem-devel gcc4.5 gcc4.5-c++ boost-program_options-devel \
  boost-thread-devel boost-system-devel boost-regex-devel eigen3 libmpfr libgmp libgmp_cxx-devel qt4-devel libcgal-devel git-core \
  libglew-devel flex bison curl imagemagick gettext glib2-devel; do apt-get install $i; done
}

get_freebsd_deps()
{
 pkg_add -r bison boost-libs cmake git bash eigen3 flex gmake gmp mpfr \
  xorg libGLU libXmu libXi xorg-vfbserver glew \
  qt4-corelib qt4-gui qt4-moc qt4-opengl qt4-qmake qt4-rcc qt4-uic \
  opencsg cgal curl imagemagick glib2-devel gettext
}

get_netbsd_deps()
{
 pkgin install bison boost cmake git bash eigen3 flex gmake gmp mpfr \
  qt4 glew cgal opencsg python27 curl \
  ImageMagick glib2 gettext
}

get_opensuse_deps()
{
 zypper install libeigen3-devel mpfr-devel gmp-devel boost-devel \
  libqt4-devel glew-devel cmake git bison flex cgal-devel curl \
  glib2-devel gettext freetype-devel harfbuzz-devel libqscintilla-devel \
  xvfb-run imagemagick opencsg-devel
  echo if you are missing opencsg, please add the -graphics- repository
  echo find your version from cat /etc/issue, then replace it below, then run
  echo " zypper ar -f http://download.opensuse.org/repositories/graphics/openSUSE_13.2 graphics"
  echo " zypper install opencsg-devel"
}

get_mageia_deps()
{
 urpmi ctags
 urpmi task-c-devel task-c++-devel libqt4-devel libgmp-devel \
  libmpfr-devel libboost-devel eigen3-devel libglew-devel bison flex \
  cmake imagemagick glib2-devel python curl git x11-server-xvfb gettext
}

get_debian_deps()
{
 apt-get -y install \
  build-essential curl libffi-dev \
  libxmu-dev cmake bison flex git-core libboost-all-dev \
  libXi-dev libmpfr-dev libboost-dev libglew-dev \
  libeigen3-dev libcgal-dev libopencsg-dev libgmp3-dev libgmp-dev \
  imagemagick libfontconfig-dev libfreetype6-dev \
  gtk-doc-tools libglib2.0-dev gettext xvfb pkg-config ragel
}

get_debian_8_deps()
{
  get_debian_deps
  apt-get -y install libharfbuzz-dev qtbase5-dev libqt5scintilla2-dev
}

get_debian_7_deps()
{
  get_debian_deps
  apt-get -y install libqt4-dev libqscintilla2-dev
}

get_ubuntu_14_deps()
{
  get_debian_8_deps
  apt-get -y install qt5-qmake
}

get_msys2_x86_64_deps()
{
 # for Windows(TM), see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Microsoft_Windows
 pacman -Sy
 pacman -S --noconfirm git
 pacman -S --noconfirm make
 pacman -S --noconfirm mingw-w64-x86_64-freetype
 pacman -S --noconfirm mingw-w64-x86_64-fontconfig
 pacman -S --noconfirm mingw-w64-x86_64-harfbuzz
 pacman -S --noconfirm mingw-w64-x86_64-qt-creator
 pacman -S --noconfirm mingw-w64-x86_64-gdb
 pacman -S --noconfirm mingw-w64-x86_64-boost
 pacman -S --noconfirm mingw-w64-x86_64-cgal
 pacman -S --noconfirm mingw-w64-x86_64-eigen3
 pacman -S --noconfirm mingw-w64-x86_64-glew
 pacman -S --noconfirm mingw-w64-x86_64-qscintilla
 pacman -S --noconfirm mingw-w64-x86_64-opencsg
 pacman -S --noconfirm mingw-w64-x86_64-bison
 pacman -S --noconfirm mingw-w64-x86_64-pkg-config
 pacman -S --noconfirm mingw-w64-x86_64-cmake
}

get_msys2_i686_deps()
{
 # for Windows(TM), see http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Microsoft_Windows
 pacman -Sy
 pacman -S --noconfirm git
 pacman -S --noconfirm make
 pacman -S --noconfirm mingw-w64-i686-freetype
 pacman -S --noconfirm mingw-w64-i686-fontconfig
 pacman -S --noconfirm mingw-w64-i686-harfbuzz
 pacman -S --noconfirm mingw-w64-i686-qt-creator
 pacman -S --noconfirm mingw-w64-i686-gdb
 pacman -S --noconfirm mingw-w64-i686-boost
 pacman -S --noconfirm mingw-w64-i686-cgal
 pacman -S --noconfirm mingw-w64-i686-eigen3
 pacman -S --noconfirm mingw-w64-i686-glew
 pacman -S --noconfirm mingw-w64-i686-qscintilla
 pacman -S --noconfirm mingw-w64-i686-opencsg
 pacman -S --noconfirm mingw-w64-i686-bison
 pacman -S --noconfirm mingw-w64-i686-pkg-config
 pacman -S --noconfirm mingw-w64-i686-cmake
}

unknown()
{
 echo "Unknown system type. Please install the dependency packages listed"
 echo "in README.md using your system's package manager"
}

try_using_etc_issue()
{
 if [ ! -e /etc/issue ]; then
  try_result=0
 elif [ "`grep -i ubuntu.1[4-9] /etc/issue`" ]; then
  get_ubuntu_14_deps
 elif [ "`grep -i ubuntu /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i debian.GNU.Linux.7 /etc/issue`" ]; then
  get_debian_7_deps
 elif [ "`grep -i debian /etc/issue`" ]; then
  get_debian_8_deps
 elif [ "`grep -i raspbian /etc/issue`" ]; then
  get_debian_deps
 elif [ "`grep -i mint /etc/issue`" ]; then
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
 else
  try_result=0
 fi
}

try_using_uname()
{
 if [ ! "`command -v uname`" ]; then
  try_result=0
 elif [ "`uname -a | grep -i x86_64.*Msys`" ]; then
  get_msys2_x86_64_deps
 elif [ "`uname -a | grep -i i686.*Msys`" ]; then
  get_msys2_i686_deps
 elif [ "`uname | grep -i freebsd `" ]; then
  get_freebsd_deps
 elif [ "`uname | grep -i netbsd`" ]; then
  get_netbsd_deps
 else
  try_result=0
 fi
}

try_using_rpm()
{
 if [ ! "`command -v rpm`" ]; then
  try_result=0
 elif [ "`rpm -qa | grep altlinux`" ]; then
  get_altlinux_deps
 else
  try_result=0
 fi
}

try_result=1
try_using_etc_issue
if [ $try_result -eq 0 ]; then
 try_using_uname
fi
if [ $try_result -eq 0 ]; then
 try_using_rpm
fi
if [ $try_result -eq 0 ]; then
 unknown
fi

