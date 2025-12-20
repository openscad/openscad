{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    bison
    cmake
    flex
    pkg-config
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    boost
    cairo
    cgal
    double-conversion
    eigen
    fontconfig
    freetype
    gettext
    ghostscript
    glib
    gmp
    gsettings-desktop-schemas
    harfbuzz
    lib3mf
    libGL
    libGLU
    libxml2
    libzip
    manifold
    mpfr
    opencsg
    python3
    python3Packages.numpy
    tbb
    wayland
    wayland-protocols

    # QT5 
    # libsForQt5.full
    # qscintilla

    # QT6
    qt6.qt5compat
    qt6.qtbase
    qt6.qtmultimedia
    qt6.qtsvg
    qt6.qttools
    qt6.qtwayland
    qt6Packages.qscintilla

    # used by the automated tests
    imagemagick

    # used by scripts/beautify.sh to clean up code
    clang-tools
  ];

  # avoid segfault when showing a file dialog or color picker
  # this is usually handled by GTK wrappers during package build
  GSETTINGS_SCHEMA_DIR = "${pkgs.gtk3}/share/gsettings-schemas/${pkgs.gtk3.name}/glib-2.0/schemas";
}
