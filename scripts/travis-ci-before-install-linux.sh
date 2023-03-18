#!/bin/bash

DIST="$1"

PACKAGES1="build-essential bison cmake curl flex git-core imagemagick ghostscript"
PACKAGES2="libboost-all-dev libboost-dev libeigen3-dev libzip-dev"
PACKAGES3="libxi-dev libxmu-dev qtbase5-dev qtmultimedia5-dev libqt5opengl5-dev libqt5svg5-dev libqt5scintilla2-dev"
PACKAGES4="libcgal-dev libglew-dev libgmp3-dev libgmp-dev libmpfr-dev"
PACKAGES5="libdouble-conversion-dev libfontconfig-dev libharfbuzz-dev libopencsg-dev lib3mf-dev libtbb-dev"

if [[ "$DIST" == "xenial" ]]; then

    LIB3MF_REPO="http://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_16.04/"
    LIBCGAL_REPO="http://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_16.04/"

elif [[ "$DIST" == "bionic" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_18.04/"
    LIBCGAL_REPO="https://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_18.04/"

elif [[ "$DIST" == "focal" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_20.04/"
    LIBCGAL_REPO="https://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_20.04/"

elif [[ "$DIST" == "kinetic" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_22.04/"
    LIBCGAL_REPO="https://download.opensuse.org/repositories/home:/t-paul:/cgal/xUbuntu_22.04/"

else

    echo "ERROR: unhandled DIST: $DIST"
    exit 1

fi

echo "Selected distribution: $DIST"

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

sudo apt-get install -qq $PACKAGES1 $PACKAGES2 $PACKAGES3 $PACKAGES4 $PACKAGES5
