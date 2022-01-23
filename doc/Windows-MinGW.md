# Compiling OpenSCAD under Windows

If you are attempting to compile OpenSCAD under Windows using MSVC, an MSVC build on Windows, please see this site:
(https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_on_Windows).

To cross-build, first make sure that you have all necessary dependencies 
of the MXE project ( listed at https://mxe.cc/#requirements ). Don't install
MXE itself, the scripts below will do that for you under `$HOME/openscad_deps/mxe`

Then get your development tools installed to get GCC. Then after you've 
cloned this git repository, start a new clean bash shell and run the 
script that sets up the environment variables.

    source ./scripts/setenv-mingw-xbuild.sh 64

Then run the script to download & compile all the prerequisite libraries above:

    ./scripts/mingw-x-build-dependencies.sh 64

Note that this process can take several hours, and tens of gigabytes of 
disk space, as it uses the [https://mxe.cc](https://mxe.cc) system to cross-build many
libraries. After it is complete, build OpenSCAD and package it to an 
installer:

    ./scripts/release-common.sh mingw64

For a 32-bit Windows cross-build, replace 64 with 32 in the above instructions. 

### Compilation

First, run `mkdir build && cd build && cmake ..` to generate a Makefile.

Then run `make`. Finally, on Linux you might run `make install` as root.

If you had problems compiling from source, raise a new issue in the
[issue tracker on the github page](https://github.com/openscad/openscad/issues).

This site and it's subpages can also be helpful:
https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Building_OpenSCAD_from_Sources
