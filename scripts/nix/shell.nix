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
    qscintilla
    qt6.full
    tbb
    wayland
    wayland-protocols
  ];
}
