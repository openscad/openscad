cmake_minimum_required(VERSION 2.8)
project(tests)

enable_testing()

# Find all scad files
file(GLOB SCAD_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/*.scad)

# Add dumptest tests to CTest
set(DUMPTEST_CMD ${CMAKE_SOURCE_DIR}/../test-code/dumptest)
foreach (SCADFILE ${SCAD_FILES})
  get_filename_component(TEST ${SCADFILE} NAME_WE)
  add_test(${TEST} ${tests_SOURCE_DIR}/test_cmdline_tool.py ${DUMPTEST_CMD} ${SCADFILE})
endforeach()
