#! /usr/bin/env bash

linux() {
  echo Running on linux

  sudo ./scripts/uni-get-dependencies.sh
  ./scripts/check-dependencies.sh

  qmake openscad.pro
  make
  sudo make install
}

mac() {
  echo Running on macOS

  if  [ -x "$(command -v brew)" ]; then
    echo 'Looks like Homebrew is installed. Using Homebrew.' >&2
    ./scripts/macosx-build-homebrew.sh

    qmake openscad.pro
    make
    sudo make install
  fi
}


# pull MCAD
if ! [ -x "$(command -v git)" ]; then
  echo 'Error: git is not installed.' >&2
  exit 1
fi
cd ..
git submodule update --init

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     linux;;
    Darwin*)    mac;;
    *)          echo "Running on UNKNOWN:${unameOut}"; exit 1
esac
