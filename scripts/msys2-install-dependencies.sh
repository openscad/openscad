#!/bin/bash

export MINGW_PACKAGE_PREFIX=ucrt-x86_64

# pacboy is a pacman wrapper for MSYS2 which handles the package prefixes automatically
#            name:p means MINGW_PACKAGE_PREFIX-only
#            name:  disables any translation for name

date "+### %Y-%m-%d %T msys2-install-dependencies started"

for pkg in \
    git: \
    make: \
    bison: \
    flex: \
    toolchain:p \
    cmake:p \
    ninja:p \
    boost:p \
    cgal:p \
    eigen3:p \
    glew:p \
    qscintilla:p \
    opencsg:p \
    libzip:p \
    double-conversion:p \
    cairo:p \
    ghostscript:p \
    imagemagick:p \
    qt5-svg:p \
    qt5-multimedia:p
do
	date "+### %Y-%m-%d %T install ${pkg}"
	pacboy --noconfirm --sync --needed ${pkg}
done

date "+### %Y-%m-%d %T msys2-install-dependencies finished"
