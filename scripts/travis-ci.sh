#!/bin/bash

qmake
make
cd tests
cmake .
make
ctest
