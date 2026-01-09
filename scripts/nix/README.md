# Nix Build

This shell definition is intended for quick development. The final results will not be portable, but this is a good way to run incremental builds and test locally.

Run `nix develop` in this directory to enter a shell with all required dependencies:

```bash
cd scripts/nix
nix develop
cd ../..

# run compile commands from main README.md, with QT6 enabled
cmake -B build -DEXPERIMENTAL=1 -DUSE_QT6=on
cmake --build build

# launch openscad from build directory
# this must be done in a shell with dependencies installed
./build/openscad
```


## Graphics errors when running

When running the built application, you may see errors like this:

```
(nix:nix-shell-env)$ ./build/openscad
qt.qpa.wayland: EGL not available
QRhiGles2: Failed to create temporary context
QRhiGles2: Failed to create context
Failed to create QRhi for QBackingStoreRhiSupport
```

This is caused by some kind of graphics driver nonsense, and the solution [is to install and use nixGL](https://github.com/nix-community/nixGL). The following command will install a recent version of nixGL for your current user:

```
$ nix profile install github:guibou/nixGL --impure --override-input nixpkgs nixpkgs/nixos-25.11
```

You can then execute OpenSCAD like this:

```
(nix:nix-shell-env)$ nixGL ./build/openscad
```
