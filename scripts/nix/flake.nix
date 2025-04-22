{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            bison
            flex
            pkg-config
            gettext
            cmake
            python3
            ghostscript
            eigen
            boost
            glew
            opencsg
            cgal
            mpfr
            gmp
            glib
            harfbuzz
            lib3mf
            libzip
            double-conversion
            freetype
            fontconfig
            qscintilla
            cairo
            tbb
            libGLU
            libGL
            wayland
            wayland-protocols
            libxml2
            libsForQt5.full
          ];
        };
      });
}
