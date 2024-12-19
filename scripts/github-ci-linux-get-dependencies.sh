#!/usr/bin/env bash

QT="$1"

DIST=$(. /etc/os-release; echo $VERSION_CODENAME)

PACKAGES1="build-essential bison cmake curl flex gettext git imagemagick ghostscript"
PACKAGES2="libboost-program-options-dev libboost-regex-dev libboost-system-dev libeigen3-dev libcrypto++-dev"
PACKAGES3="libxi-dev libxmu-dev qtbase5-dev qtmultimedia5-dev libqt5opengl5-dev libqt5svg5-dev libqscintilla2-qt5-dev"
PACKAGES4="libcairo2-dev libcgal-dev libglew-dev libgmp-dev libmpfr-dev libegl-dev libegl1-mesa-dev libxml2-dev"
PACKAGES5="libdouble-conversion-dev libfontconfig-dev libharfbuzz-dev libopencsg-dev lib3mf-dev libtbb-dev libzip-dev"

if [[ "$DIST" == "focal" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_20.04/"
    LIBCGAL_REPO="https://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_20.04/"

elif [[ "$DIST" == "jammy" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_22.04/"
    LIBCGAL_REPO="https://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_22.04/"

elif [[ "$DIST" == "noble" ]]; then

    if [[ "$QT" == "qt6" ]]; then

      PACKAGES3="libxi-dev libxmu-dev qt6-base-dev qt6-multimedia-dev libqt6core5compat6-dev libqt6svg6-dev libqscintilla2-qt6-dev"

    fi

else

    echo "ERROR: unhandled DIST: $DIST"
    exit 1

fi

echo "Selected distribution: $DIST $QT"

if [[ "$DIST" == "noble" ]]; then

  sudo apt-get update -qq

else

  wget -qO - https://files.openscad.org/OBS-Repository-Key.pub | sudo apt-key add -
  echo yes | sudo add-apt-repository "deb $LIB3MF_REPO ./"
  echo yes | sudo add-apt-repository "deb $LIBCGAL_REPO ./"
  sudo apt-get update -qq
  sudo apt-get purge -qq fglrx || true

  # Workaround for issue installing libzip-dev on github
  # E: Unable to correct problems, you have held broken packages.
  # libzip-dev : Depends: libzip4 (= 1.1.2-1.1) but 1.7.3-1+ubuntu18.04.1+deb.sury.org+2 is to be installed
  sudo apt-cache rdepends libzip4 || true
  sudo apt-get purge -qq libzip4 $(apt-cache rdepends --installed libzip4 | tail -n+3)

fi

sudo apt-get install -qq $PACKAGES1 $PACKAGES2 $PACKAGES3 $PACKAGES4 $PACKAGES5 $PACKAGES6
