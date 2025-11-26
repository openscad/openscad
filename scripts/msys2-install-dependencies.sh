#!/usr/bin/env bash
set -e

QT="$1"

date "+### %Y-%m-%d %T msys2-install-dependencies started"

if [[ -z "${GITHUB_RUN_ID}" ]]; then
    if [ -z $MSYSTEM ]; then
	MSYSTEM_DEFAULT=UCRT64
	# Possible values: (MSYS|UCRT64|CLANG64|CLANGARM64|CLANG32|MINGW64|MINGW32)
	# For explanation of options see: https://www.msys2.org/docs/environments/
	echo "MSYSTEM is unset or blank, defaulting to '${MSYSTEM_DEFAULT}'";
	export MSYSTEM=$MSYSTEM_DEFAULT
    else
	echo "MSYSTEM is set to '$MSYSTEM'";
    fi

    pacman --query --explicit

    date "+### %Y-%m-%d %T install pactoys (for pacboy)"
    pacman --noconfirm --sync --needed pactoys libxml2
    # pacboy is a pacman wrapper for MSYS2 which handles the package prefixes automatically
    #            name:p means MINGW_PACKAGE_PREFIX-only
    #            name:  disables any translation for name
fi

if [[ "$QT" == "qt6" ]]; then
  QT_PACKAGES="qscintilla-qt6:p qt6-5compat:p qt6-multimedia:p qt6-svg:p"
else
  QT_PACKAGES="qscintilla:p qt5-multimedia:p qt5-svg:p"
fi

PACKAGE_LIST=(
    "$QT_PACKAGES"
    "git:"
    "make:"
    "bison:"
    "flex:"
    "toolchain:p"
    "cmake:p"
    "ninja:p"
    "boost:p"
    "catch:p"
    "cgal:p"
    "eigen3:p"
    "glew:p"
    "opencsg:p"
    "lib3mf:p"
    "libzip:p"
    "mimalloc:p"
    "double-conversion:p"
    "cairo:p"
    "ghostscript:p"
    "imagemagick:p"
    "tbb:p"
    "python:p"
    "python-pip:p"
    "python-numpy:p"
    "python-pillow:p"
)

# Expand array to a single space-separated string
PACBOY_PACKAGES="${PACKAGE_LIST[*]}"

if [[ -z "${GITHUB_RUN_ID}" ]]; then
    date "+### %Y-%m-%d %T install remaining packages"
    pacboy --noconfirm --sync --needed $PACBOY_PACKAGES
else
    echo "PACBOY_PACKAGES=${PACBOY_PACKAGES}" >> ${GITHUB_ENV}
fi

date "+### %Y-%m-%d %T msys2-install-dependencies finished"
