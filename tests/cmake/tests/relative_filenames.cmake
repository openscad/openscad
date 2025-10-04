############################
# Relative filenames tests #
############################
# These tests writing outputs to a relative filename, and verifies that the
# resulting file is written to the correct location.

function(add_output_file_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "FILE;FORMAT" "" ${ARGN})

  add_test(NAME "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_run" CONFIGURATIONS Default COMMAND ${OPENSCAD_BINPATH} ${TESTCMD_FILE} -o ${TESTCMD_BASENAME}.${TESTCMD_FORMAT})
  add_test(NAME "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_check" CONFIGURATIONS Default COMMAND ${CMAKE_COMMAND} -E cat ${TESTCMD_BASENAME}.${TESTCMD_FORMAT})
  set_tests_properties("${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_check" PROPERTIES DEPENDS "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_run")
endfunction()

add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT stl)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT off)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT wrl)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT amf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT 3mf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT csg)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT dxf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT svg)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT pdf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT png)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT echo)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT ast)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT term)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT nef3)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT nefdbg)
