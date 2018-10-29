#!/bin/bash

pacman --noconfirm --ask 20 -Sy
pacman --noconfirm --ask 20 -Su

pacman --query --explicit

pacman --noconfirm --ask 20 --force --sync \
	mingw-w64-x86_64-gdb \
	mingw-w64-x86_64-boost \
	mingw-w64-x86_64-cgal \
	mingw-w64-x86_64-eigen3 \
	mingw-w64-x86_64-glew \
	mingw-w64-x86_64-qscintilla \
	mingw-w64-x86_64-opencsg \
	mingw-w64-x86_64-pkg-config \
	mingw-w64-x86_64-cmake \
	bison \
	git

# overwrite minizip which seems to come in via a dependency
pacman --noconfirm --ask 20 --force --sync \
	mingw-w64-x86_64-libzip
