{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = with pkgs; [
    bison
    boost
    cairo
    cgal
    cmake
    double-conversion
    eigen
    flex
    fontconfig
    freetype
    gettext
    ghostscript
    glib
    gmp
    harfbuzz
    lib3mf
    libGL
    libGLU
    libxml2
    libzip
    manifold
    mpfr
    opencsg
    pkg-config
    python3
    python3Packages.numpy
    tbb
    wayland
    wayland-protocols

    # QT5 
    # libsForQt5.full
    # qscintilla

    #QT6
    qt6.full
    qt6.qt5compat
    qt6.qtbase
    qt6.qtmultimedia
    qt6.qttools
    qt6.wrapQtAppsHook
    qt6Packages.qscintilla
  ];
}
