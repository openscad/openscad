#!/bin/bash

PACKAGES1="build-essential bison cmake curl flex git-core imagemagick ghostscript"
PACKAGES2="libboost-all-dev libboost-dev libeigen3-dev libzip-dev libcrypto++-dev"
PACKAGES3="libxi-dev libxmu-dev qt6-base-dev qt6-multimedia-dev libqt6core5compat6-dev libqt6svg6-dev libqscintilla2-qt6-dev"
PACKAGES4="libcgal-dev libglew-dev libgmp3-dev libgmp-dev libmpfr-dev libegl-dev libegl1-mesa-dev"
PACKAGES5="libdouble-conversion-dev libfontconfig-dev libharfbuzz-dev libopencsg-dev lib3mf-dev libtbb-dev"
PACKAGES6="libthrust-dev libglm-dev"

sudo apt-get install -qq $PACKAGES1 $PACKAGES2 $PACKAGES3 $PACKAGES4 $PACKAGES5 $PACKAGES6
