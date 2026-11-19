# Runs OpenSCAD on INPUT, exporting to OUTPUT, and fails if OpenSCAD crashed:
# an exit code other than 0 (success) or 1 (a regular error such as a parse
# error), or an uncaught std::filesystem exception in its output.
#
# If EXPECTED is given, OUTPUT must also match that file, apart from the
# directory part of the file names in messages ("in file <dir>/x.scad" is
# compared as "in file x.scad"), which depends on how INPUT was given.
#
# Usage: cmake -DOPENSCAD=<exe> -DINPUT=<file> -DOUTPUT=<file> [-DEXPECTED=<file>]
#              -P UncPathTest.cmake

if(NOT EXISTS "${OPENSCAD}")
  message(FATAL_ERROR "OpenSCAD not found: ${OPENSCAD}")
endif()

execute_process(
  COMMAND "${OPENSCAD}" -o "${OUTPUT}" "${INPUT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
message("${output}")

if(NOT (result STREQUAL "0" OR result STREQUAL "1") OR output MATCHES "terminate called|filesystem error")
  message(FATAL_ERROR "OpenSCAD crashed on ${INPUT} (exit code ${result})")
endif()

if(DEFINED EXPECTED)
  function(read_normalized file var)
    file(READ "${file}" text)
    string(REPLACE "\r" "" text "${text}")
    # Keep only the file name: the directory differs between a UNC path and a local one
    string(REGEX REPLACE "in file [^,\n]*[/\\]([^/\\,\n]+)," "in file \\1," text "${text}")
    set(${var} "${text}" PARENT_SCOPE)
  endfunction()
  read_normalized("${OUTPUT}" actual)
  read_normalized("${EXPECTED}" expected)
  if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Output for ${INPUT} differs from ${EXPECTED}.\n"
                        "Expected:\n${expected}\nActual:\n${actual}")
  endif()
endif()
