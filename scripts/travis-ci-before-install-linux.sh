#!/bin/bash

DIST="$1"

PACKAGES1="build-essential bison cmake curl flex git-core imagemagick ghostscript"
PACKAGES2="libboost-all-dev libboost-dev libeigen3-dev libzip-dev"
PACKAGES3="libxi-dev libxmu-dev qtbase5-dev qtmultimedia5-dev libqt5opengl5-dev libqt5scintilla2-dev"
PACKAGES4="libcgal-dev libcgal-qt5-dev libglew-dev libgmp3-dev libgmp-dev libmpfr-dev"
PACKAGES5="libdouble-conversion-dev libfontconfig-dev libharfbuzz-dev libopencsg-dev lib3mf-dev"

if [[ "$DIST" == "trusty" ]]; then

    LIB3MF_REPO="http://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_14.04/"

elif [[ "$DIST" == "xenial" ]]; then

    LIB3MF_REPO="http://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_16.04/"

elif [[ "$DIST" == "bionic" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_18.04/"

elif [[ "$DIST" == "focal" ]]; then

    LIB3MF_REPO="https://download.opensuse.org/repositories/home:/t-paul:/lib3mf/xUbuntu_20.04/"

else

    echo "ERROR: unhandled DIST: $DIST"
    exit 1

fi

echo "Selected distribution: $DIST"

wget -qO - http://files.openscad.org/OBS-Repository-Key.pub | sudo apt-key add -
echo yes | sudo add-apt-repository "deb $LIB3MF_REPO ./"
sudo apt-get update -qq
sudo apt-get purge -qq fglrx || true
sudo apt-get install -qq $PACKAGES1 $PACKAGES2 $PACKAGES3 $PACKAGES4 $PACKAGES5

