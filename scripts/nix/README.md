# Nix Build

This shell definition is intended for quick development. Run `nix-shell` in this directory to install all required dependencies for compiling manually.

The final results will not be portable, but this is a good way to run incremental builds and test locally. __Running install is not recommended.__

Testing instructions:
```
cd scripts/nix
nix-shell
cd ../..

# run compile commands from main README.md
cmake -B build -DEXPERIMENTAL=1
cmake --build build

# launch openscad from build directory
# this must be done in a shell with dependencies installed
./build/openscad
```
