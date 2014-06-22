#!/bin/bash

cd tests
cmake . 
if [[ $? != 0 ]]; then
  echo "Error configuring test suite"
  exit 1
fi
make -j2
if [[ $? != 0 ]]; then
  echo "Error building test suite"
  exit 1
fi
# Exclude tests known the cause issues on Travis
# opencsgtest_rotate_extrude-tests - Fails on Ubuntu 12.04 using Gallium 0.4 drivers
ctest -j8 -E "opencsgtest_rotate_extrude-tests|opencsgtest_render-tests"
if [[ $? != 0 ]]; then
  echo "Test failure"
  exit 1
fi
