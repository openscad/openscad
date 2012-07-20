# build dependencies and/or openscad on linux with the clang compiler

export CC=clang
export CXX=clang++
export QMAKESPEC=unsupported/linux-clang

echo CC has been modified: $CC
echo CXX has been modified: $CXX
echo QMAKESPEC has been modified: $QMAKESPEC

. ./scripts/setenv-linbuild.sh

