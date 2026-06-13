#! /usr/bin/env bash
# This script aims to be a one-step stop to compile and build OpenSCAD on macOS.
# It attempts to follow the instructions listed in the README of this project.
# WARNING: This will install openscad using SUDO, so it might not be desired.
# On mac, it will prefer to use Homebrew, then MacPorts, then neither.

set -ex

mac() {
    if [ -x "$(command -v brew)" ]; then
        echo 'Looks like Homebrew is installed. Using Homebrew.' >&2
        ./scripts/macosx-build-homebrew.sh
    elif [ -x "$(command -v port)" ]; then
        echo 'Looks like MacPorts is installed. Using MacPorts.' >&2
        sudo port install opencsg qscintilla boost cgal pkgconfig eigen3 harfbuzz fontconfig
    else
        echo 'Looks like neither Homebrew nor MacPorts is installed. Building dependencies from scratch.' >&2
        ./scripts/macosx-build-homebrew.sh
    fi

    qmake openscad.pro
    make
    sudo make install
}

cd "$(dirname "${BASH_SOURCE[0]}")"/../

# pull MCAD
if ! [ -x "$(command -v git)" ]; then
    echo 'Error: git is not installed.' >&2
    exit 1
fi

# git submodule update --init

unameOut="$(uname -s)"
case "${unameOut}" in
Darwin*) mac ;;
*)
    echo "OS not supported: ${unameOut}"
    exit 1
    ;;
esac
