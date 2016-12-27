#!/bin/bash

set -x

qmake CONFIG+=experimental CONFIG+=nogui CONFIG+=debug
make

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

if [[ "$DIST" == "trusty" ]]; then
    PARALLEL=-j1
else
    PARALLEL=-j8
fi

# Exclude tests known the cause issues on Travis
# opencsgtest_rotate_extrude-tests - Fails on Ubuntu 12.04 using Gallium 0.4 drivers
# *_text-font-direction-tests - Fails due to old freetype (issue #899)
# throwntogethertest_issue964 - Fails due to non-planar quad being tessellated slightly different
# opencsgtest_issue1165 - z buffer tearing

# Fails on Apple's software renderer:
# opencsgtest_issue1258
# throwntogethertest_issue1089
# throwntogethertest_issue1215
ctest $PARALLEL -R transform-nan-inf
if [[ $? != 0 ]]; then
  echo "Test failure"
  exit 1
fi
