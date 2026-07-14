#!/usr/bin/env bash
# Install native build dependencies for macOS wheel builds.
# Runs as cibuildwheel [tool.cibuildwheel.macos] before-all.
set -euo pipefail

echo "=== install-deps-macos.sh: installing pip wheel build deps ==="
PROJECT_ROOT="${GITHUB_WORKSPACE:-$(pwd)}"

# Homebrew is preinstalled on GitHub-hosted macOS runners.
./scripts/get-dependencies.py --yes --profile pythonscad-pip

if ! pkg-config --exists lib3mf; then
    echo "lib3mf unavailable from Homebrew; building lib3mf from source"
    LIB3MF_VERSION=2.4.1
    LIB3MF_SRC="/tmp/lib3mf-${LIB3MF_VERSION}"
    BREW_PREFIX="$(brew --prefix)"
    rm -rf "$LIB3MF_SRC"
    curl -L "https://github.com/3MFConsortium/lib3mf/archive/v${LIB3MF_VERSION}.tar.gz" \
        -o "/tmp/lib3mf-${LIB3MF_VERSION}.tar.gz"
    tar -C /tmp -xzf "/tmp/lib3mf-${LIB3MF_VERSION}.tar.gz"
    cd "$LIB3MF_SRC"
    patch -p1 < "$PROJECT_ROOT/patches/lib3mf-macos.patch"
    cmake -S . -B build \
        -DLIB3MF_TESTS=OFF \
        -DUSE_INCLUDED_ZLIB=OFF \
        -DUSE_INCLUDED_LIBZIP=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BREW_PREFIX" \
        -DCMAKE_INSTALL_LIBDIR=lib
    cmake --build build -j"$(sysctl -n hw.ncpu)"
    cmake --install build
    mkdir -p "$BREW_PREFIX/lib/pkgconfig"
    if ! compgen -G "$BREW_PREFIX/lib/lib3mf*.dylib" >/dev/null; then
        echo "lib3mf install did not produce dylibs under $BREW_PREFIX/lib" >&2
        exit 1
    fi
    if [[ -f "$LIB3MF_SRC/lib/pkgconfig/lib3mf.pc" ]]; then
        cp "$LIB3MF_SRC/lib/pkgconfig/lib3mf.pc" "$BREW_PREFIX/lib/pkgconfig/lib3mf.pc"
    elif [[ ! -f "$BREW_PREFIX/lib/pkgconfig/lib3mf.pc" ]]; then
        echo "lib3mf install did not provide lib3mf.pc" >&2
        exit 1
    fi
    sed -i.bak "s|^prefix=.*|prefix=$BREW_PREFIX|" "$BREW_PREFIX/lib/pkgconfig/lib3mf.pc"
    cd "$PROJECT_ROOT"
fi

echo "=== install-deps-macos.sh: done ==="
