# Nix Build

This flake is intended for quick development. Run `nix develop` in this directory to install all required dependencies for compiling manually.

The final results will not be portable but this is a good way to run incremental builds and test locally. __Running install is not recommended.__

Testing instructions:
```
cd scripts/nix
nix develop
cd ../..

# run compile commands from README.md

# launch openscad from build directory
# this must be done in a shell with dependencies installed
./build/openscad
```

`flake.lock` is intentionally ignored since this is for quick local tests.
