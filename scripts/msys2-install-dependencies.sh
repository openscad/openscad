#!/bin/bash

date "+### %Y-%m-%d %T msys2-install-dependencies started"

#pacman --noconfirm --ask 20 -Sy
#pacman --noconfirm --ask 20 -Su

pacman --query --explicit

for pkg in mingw-w64-x86_64-toolchain mingw-w64-x86_64-boost mingw-w64-x86_64-cgal mingw-w64-x86_64-eigen3 mingw-w64-x86_64-glew mingw-w64-x86_64-qscintilla mingw-w64-x86_64-opencsg mingw-w64-x86_64-pkg-config mingw-w64-x86_64-libzip mingw-w64-x86_64-cmake mingw-w64-x86_64-mesa mingw-w64-x86_64-double-conversion mingw-w64-x86_64-cairo mingw-w64-x86_64-ghostscript make bison flex git
do
	date "+### %Y-%m-%d %T install ${pkg}"
	pacman --noconfirm --ask 20 --sync --needed ${pkg}
done

date "+### %Y-%m-%d %T msys2-install-dependencies finished"
