# Runs OpenSCAD on INPUT, exporting to OUTPUT, and fails if OpenSCAD crashed:
# an exit code other than 0 (success) or 1 (a regular error such as a parse
# error), or an uncaught std::filesystem exception in its output.
#
# Usage: cmake -DOPENSCAD=<exe> -DINPUT=<file> -DOUTPUT=<file> -P UncNoCrashTest.cmake

execute_process(
  COMMAND "${OPENSCAD}" -o "${OUTPUT}" "${INPUT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
message("${output}")

if(NOT (result STREQUAL "0" OR result STREQUAL "1") OR output MATCHES "terminate called|filesystem error")
  message(FATAL_ERROR "OpenSCAD crashed on ${INPUT} (exit code ${result})")
endif()
