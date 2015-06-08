#!/bin/bash

qmake CONFIG+=experimental CONFIG+=nogui
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
# Exclude tests known the cause issues on Travis
# opencsgtest_rotate_extrude-tests - Fails on Ubuntu 12.04 using Gallium 0.4 drivers
# opencsgtest_text-font-direction-tests - Fails due to old freetype (issue #899)
# throwntogethertest_issue964 - Fails due to non-planar quad being tessellated slightly different
# opencsgtest_issue1165 - z buffer tearing
ctest -j8 -E "\
opencsgtest_rotate_extrude-tests|\
opencsgtest_render-tests|\
opencsgtest_rotate_extrude-hole|\
opencsgtest_internal-cavity|\
opencsgtest_internal-cavity-polyhedron|\
opencsgtest_minkowski3-erosion|\
opencsgtest_issue835|\
opencsgtest_issue911|\
opencsgtest_issue913|\
opencsgtest_issue1215|\
opencsgtest_issue1105d|\
dxfpngtest_text-font-direction-tests|\
cgalpngtest_text-font-direction-tests|\
opencsgtest_text-font-direction-tests|\
csgpngtest_text-font-direction-tests|\
throwntogethertest_text-font-direction-tests|\
throwntogethertest_issue964|\
opencsgtest_issue1165\
"
if [[ $? != 0 ]]; then
  echo "Test failure"
  exit 1
fi
