#!/bin/bash

qmake && make
if [[ $? != 0 ]]; then
  echo "Error building OpenSCAD executable"
  exit 1
fi
cd tests
cmake . 
if [[ $? != 0 ]]; then
  echo "Error configuring test suite"
  exit 1
fi
make
if [[ $? != 0 ]]; then
  echo "Error building test suite"
  exit 1
fi
ctest
if [[ $? != 0 ]]; then
  echo "Test failure"
  exit 1
fi
