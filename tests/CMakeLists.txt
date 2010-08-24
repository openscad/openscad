cmake_minimum_required(VERSION 2.8)
project(tests)

#
# This functions adds cmd-line tests for those of the FILES which have
# a corresponding TESTNAME-expected.<suffix> file present.
#
function(add_cmdline_test TESTCMD TESTSUFFIX FILES)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  foreach (SCADFILE ${SCAD_FILES})
    get_filename_component(TESTNAME ${SCADFILE} NAME_WE)
    if (EXISTS ${CMAKE_SOURCE_DIR}/regression/${TESTCMD_NAME}/${TESTNAME}-expected.${TESTSUFFIX})
      add_test(${TESTCMD_NAME}_${TESTNAME} ${tests_SOURCE_DIR}/test_cmdline_tool.py -s ${TESTSUFFIX} ${TESTCMD} ${SCADFILE})
    endif()
  endforeach()
endfunction()

enable_testing()

# Find all scad files
file(GLOB SCAD_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/*.scad)

# Add dumptest tests to CTest
add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/dumptest txt ${SCAD_FILES})
# Add csgtexttest tests to CTest
add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/csgtexttest txt ${SCAD_FILES})
# Add cgaltest tests to CTest
add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/cgaltest stl ${SCAD_FILES})
# Add dxfexport tests to CTest
add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/exportdxf dxf ${SCAD_FILES})
