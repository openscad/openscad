# Nix Build

This shell definition is intended for quick development. Run `nix-shell` in this directory to install all required dependencies for compiling manually.

The final results will not be portable, but this is a good way to run incremental builds and test locally. __Running install is not recommended.__

Testing instructions:
```
cd scripts/nix
nix-shell
cd ../..

# run compile commands from main README.md, with QT6 enabled
cmake -B build -DEXPERIMENTAL=1 -DUSE_QT6=on
cmake --build build

# launch openscad from build directory
# this must be done in a shell with dependencies installed
./build/openscad
```

For packaging see [nixpgs](https://github.com/NixOS/nixpkgs/blob/master/pkgs/applications/graphics/openscad/default.nix) for the Qt5 release, or [this gist](https://gist.github.com/AaronVerDow/b945a96dbcf35edfc13f543662966534) for a more up to date Qt6 pacakge.
