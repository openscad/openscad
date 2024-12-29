# instructions - see ../doc/testing.txt

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 3.13)
if(POLICY CMP0017)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # https://cmake.org/cmake/help/v2.8.8/cmake.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()
cmake_policy(SET CMP0057 NEW) # Interpret IN_LIST
cmake_policy(SET CMP0070 NEW) # Allow relative paths for file(GENERATE ...)

option(USE_IMAGE_COMPARE_PY "Use built-in image_compare.py" ON)

# Use variables for common paths, to reduce line lengths.

# Very commonly used cmake-provided paths
set(CBD ${CMAKE_BINARY_DIR}) # build top level
set(CSD ${CMAKE_SOURCE_DIR}) # project top level
set(CCBD ${CMAKE_CURRENT_BINARY_DIR}) # tests build dir
set(CCSD ${CMAKE_CURRENT_SOURCE_DIR}) # tests source dir
# Project paths
set(LIBRARIES_DIR       "${CSD}/libraries")
set(EXAMPLES_DIR        "${CSD}/examples")
# Test Data paths
set(TEST_DATA_DIR       "${CCSD}/data")
set(TEST_SCAD_DIR       "${CCSD}/data/scad")
set(TEST_CUSTOMIZER_DIR "${CCSD}/data/scad/customizer")
set(TEST_PYTHON_DIR     "${CCSD}/data/python")
# Test runner Python scripts
set(STLEXPORTSANITYTEST_PY "${CCSD}/stlexportsanitytest.py")
set(EXPORT_IMPORT_PNGTEST_PY     "${CCSD}/export_import_pngtest.py")
set(EXPORT_PNGTEST_PY    "${CCSD}/export_pngtest.py")
set(SHOULDFAIL_PY        "${CCSD}/shouldfail.py")
set(TEST_CMDLINE_TOOL_PY "${CCSD}/test_cmdline_tool.py")

######################
# Check Dependencies #
######################

# If ctest is run with no configuration specified, then force "Default"
set_directory_properties(PROPERTIES TEST_INCLUDE_FILES "${CCSD}/EnforceConfig.cmake")

# MCAD
if(NOT EXISTS ${LIBRARIES_DIR}/MCAD/__init__.py)
  message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init --recursive")
endif()
list(APPEND CTEST_ENVIRONMENT "OPENSCADPATH=${LIBRARIES_DIR}")

find_package(Python3 3.4 COMPONENTS Interpreter REQUIRED)
message(STATUS "Found Python at ${Python3_EXECUTABLE}")

# Image comparison - expected test image vs actual generated image

if(USE_IMAGE_COMPARE_PY)
  set(VENV_DIR "${CCBD}/venv")
  message(STATUS "Preparing image_compare.py for test suite image comparison: ${VENV_DIR}")

  # Since msys2 on Windows prefers bin/ over Scripts, we need to look for the actual folder to determine
  # how to utilize the venv
  find_path(VENV_BIN_PATH activate PATHS "${VENV_DIR}/bin" "${VENV_DIR}/Scripts" NO_DEFAULT_PATH NO_CACHE)
  if(WIN32)
    set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python.exe")
  else()
    set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python")
  endif()
  if(EXISTS "${IMAGE_COMPARE_EXE}")
    message(STATUS "venv found, testing libraries")
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" "${CCSD}/image_compare.py" "--status"
      WORKING_DIRECTORY "${CCSD}" ERROR_QUIET RESULT_VARIABLE ret)
    if(ret AND NOT ret EQUAL 0)
      message(STATUS "venv libraries incomplete")
      set(BUILD_VENV TRUE)
    else()
      message(STATUS "venv libraries complete")
      set(BUILD_VENV FALSE)
    endif()
  else()
    message(STATUS "venv not found")
    set(BUILD_VENV TRUE)
  endif()
  if(BUILD_VENV)
    message(STATUS "Setting up testing venv for image comparison")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "-m" "venv" "venv" "--system-site-packages" "--without-pip"
      WORKING_DIRECTORY "${CCBD}")
    # Since msys2 on Windows prefers bin/ over Scripts, we need to look for the actual folder to determine
    # how to utilize the venv
    find_path(VENV_BIN_PATH activate PATHS "${VENV_DIR}/bin" "${VENV_DIR}/Scripts" NO_DEFAULT_PATH NO_CACHE)
    if(WIN32)
      set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python.exe")
    else()
      set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python")
    endif()
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" "-m" "ensurepip"
      WORKING_DIRECTORY "${CCBD}")
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" "-m" "pip" "install" "numpy" "Pillow"
      WORKING_DIRECTORY "${CCBD}")
  endif()
  set(COMPARATOR "--comparator=image_compare")
  if(BUILD_VENV)
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" "${CCSD}/image_compare.py" "--status"
      WORKING_DIRECTORY "${CCSD}" RESULT_VARIABLE ret)
    if(ret AND NOT ret EQUAL 0)
      message(WARNING "Failed to setup the test suite venv for ${IMAGE_COMPARE_EXE}  See doc/testing.txt for dependency information.")
    else()
      message(STATUS "venv setup for ${IMAGE_COMPARE_EXE}")
    endif()
  else()
    message(STATUS "venv already setup for ${IMAGE_COMPARE_EXE}")
  endif()
else()
  # Imagemagick
  find_package(ImageMagick COMPONENTS convert)
  if(ImageMagick_convert_FOUND)
    message(STATUS "ImageMagick convert executable found: " ${ImageMagick_convert_EXECUTABLE})
    set(IMAGE_COMPARE_EXE ${ImageMagick_convert_EXECUTABLE})
    if ("${ImageMagick_VERSION_STRING}" VERSION_LESS "6.5.9.4")
      message(STATUS "ImageMagick version less than 6.5.9.4, cannot use -morphology comparison")
      message(STATUS "ImageMagick Using older image comparison method")
      set(COMPARATOR "--comparator=old")
    endif()

    execute_process(COMMAND ${IMAGE_COMPARE_EXE} --version OUTPUT_VARIABLE IM_OUT)
    if (${IM_OUT} MATCHES "OpenMP")
      # http://www.daniloaz.com/en/617/systems/high-cpu-load-when-converting-images-with-imagemagick
      message(STATUS "ImageMagick: OpenMP bug workaround - setting MAGICK_THREAD_LIMIT=1")
      list(APPEND CTEST_ENVIRONMENT "MAGICK_THREAD_LIMIT=1")
    endif()

    message(STATUS "Comparing magicktest1.png with magicktest2.png")
    set(IM_TEST_FILES "${CCSD}/magicktest1.png" "${CCSD}/magicktest2.png")
    set(COMPARE_ARGS ${IMAGE_COMPARE_EXE} ${IM_TEST_FILES} -alpha On -compose difference -composite -threshold 8% -morphology Erode Square -format %[fx:w*h*mean] info:)
    # compare arguments taken from test_cmdline_tool.py
    message(STATUS "Running ImageMagick compare: ${COMPARE_ARGS}")
    execute_process(COMMAND ${COMPARE_ARGS} RESULT_VARIABLE IM_RESULT OUTPUT_VARIABLE IM_OUT)
    if (IM_RESULT)
      message(STATUS "ImageMagick failed to run")
      message(STATUS "Using alternative image comparison")
      set(DIFFPNG 1)
    elif (${IM_OUT} STREQUAL "0")
       message(STATUS "magicktest1.png and magicktest2.png were incorrectly detected as identical")
       message(STATUS "Using alternative image comparison")
       set(DIFFPNG 1)
    else()
      message(STATUS "ImageMagick OK: Detected pixel difference of ${IM_OUT}")
    endif()
  else()
    message(STATUS "Couldn't find imagemagick 'convert' program")
    set(DIFFPNG 1)
  endif()
  if (${DIFFPNG})
    set(IMAGE_COMPARE_EXE ${CCBD}/diffpng)
    set(COMPARATOR "--comparator=diffpng")
    add_executable(diffpng diffpng.cpp ${CSD}/src/ext/lodepng/lodepng.cpp)
    target_include_directories(diffpng PRIVATE ${CSD}/src/ext/lodepng)
    message(STATUS "using diffpng for image comparison")
  endif()
endif()

find_package(Lib3MF QUIET)
# Disable LIB3MF tests if library was disabled in build
if(NOT LIB3MF_FOUND)
  # check again via pkgconfig
  if(NOT MSVC)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(LIB3MF lib3MF)
    endif()
  endif()
endif()

####################################
# Platform Specific Configurations #
####################################

# Workaround Gallium bugs
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
  message(STATUS "Workaround PPC bug https://bugs.freedesktop.org/show_bug.cgi?id=42540")
  list(APPEND CTEST_ENVIRONMENT "GALLIUM_DRIVER=softpipe")
  list(APPEND CTEST_ENVIRONMENT "DRAW_USE_LLVM=no")
endif()
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
  message(STATUS "Workaround MIPS bug https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=868745")
  list(APPEND CTEST_ENVIRONMENT "GALLIUM_DRIVER=softpipe")
  list(APPEND CTEST_ENVIRONMENT "DRAW_USE_LLVM=no")
endif()

# Determine path for openscad executable
if(EXISTS "$ENV{OPENSCAD_BINARY}")
  set(OPENSCAD_BINPATH "$ENV{OPENSCAD_BINARY}")
elseif(APPLE)
  set(OPENSCAD_BINPATH "${CBD}/OpenSCAD.app/Contents/MacOS/OpenSCAD")
elseif (WIN32 OR MXECROSS)
  set(OPENSCAD_BINPATH "${CBD}/openscad.com")
elseif(EXISTS "${CBD}/bin/openscad")
  set(OPENSCAD_BINPATH "${CBD}/bin/openscad")
else()
  set(OPENSCAD_BINPATH "${CBD}/openscad")
endif()

#if(EXISTS "${OPENSCAD_BINPATH}")
#  message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
#else()
#  message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
#  message(FATAL_ERROR "Please build the OpenSCAD binary and place it here: ${OPENSCAD_BINPATH}")
#endif()
list(APPEND CTEST_ENVIRONMENT "OPENSCAD_BINARY=${OPENSCAD_BINPATH}")
# Argument used for import/export tests
set(OPENSCAD_EXE_ARG "--openscad=${OPENSCAD_BINPATH}")

###############################
# Define Macros and Functions #
###############################

#
# Returns into the FULLNAME variable the global full test name (identifier)
# given a test command and source filename
#
function(get_test_fullname TESTCMD FILENAME FULLNAME)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  get_filename_component(TESTNAME ${FILENAME} NAME_WE)
  string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
  set(${FULLNAME} ${TESTCMD_NAME}_${TESTNAME})
  # Export to parent scope
  set(${FULLNAME} ${${FULLNAME}} PARENT_SCOPE)
endfunction()

#
# Tags the given tests as belonging to the given CONFIG, i.e. will
# only be executed when run using ctest -C <CONFIG>
#
# Usage example: set_test_config(Heavy dumptest_testname previewtest_testname2)
#
function(set_test_config CONFIG)
  cmake_parse_arguments(TESTCFG "" "" "FILES;PREFIXES" ${ARGN})
   # Get fullnames for test files
  if (TESTCFG_PREFIXES)
    foreach(PREFIX ${TESTCFG_PREFIXES})
      foreach(FILE ${TESTCFG_FILES})
        get_test_fullname(${PREFIX} ${FILE} TESTCFG_FULLNAME)
        list(APPEND FULLNAMES ${TESTCFG_FULLNAME})
      endforeach()
    endforeach()
  else()
    list(APPEND FULLNAMES ${TESTCFG_FILES})
  endif()
  # Set config on fullnames
  list(APPEND ${CONFIG}_TEST_CONFIG ${FULLNAMES})
  list(FIND TEST_CONFIGS ${CONFIG} FOUND)
  if (FOUND EQUAL -1)
    list(APPEND TEST_CONFIGS ${CONFIG})
    list(SORT TEST_CONFIGS)
    # Export to parent scope
    set(TEST_CONFIGS ${TEST_CONFIGS} CACHE INTERNAL "")
  endif()
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} CACHE INTERNAL "")
endfunction(set_test_config)

#
# Removes a tag from the given tests
#
# Usage example: remove_test_config(Bugs FILES previewtest_testname2)
#
function(remove_test_config CONFIG)
  cmake_parse_arguments(TESTCFG "" "" "FILES" ${ARGN})
  list(APPEND FULLNAMES ${TESTCFG_FILES})
  # Remove config from fullnames
  list(REMOVE_ITEM ${CONFIG}_TEST_CONFIG ${FULLNAMES})
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} CACHE INTERNAL "")
endfunction(remove_test_config)

#
# Returns a list of test configs
#
function(get_test_config TESTNAME OUTVAR)
  unset(CONFIGS)
  foreach(CONFIG ${TEST_CONFIGS})
    list(FIND ${CONFIG}_TEST_CONFIG ${TESTNAME} IDX)
    if (IDX GREATER -1)
      list(APPEND CONFIGS ${CONFIG})
    endif()
  endforeach()
  set(${OUTVAR} "${CONFIGS}" PARENT_SCOPE)
endfunction()

#
# Check if a test file is a 2D test
#
function(is_2d FULLNAME RESULT)
  list(FIND ALL_2D_FILES ${FULLNAME} IDX)
  if (${IDX} GREATER -1)
    set(${RESULT} 1 PARENT_SCOPE)
  else()
    set(${RESULT} PARENT_SCOPE)
  endif()
endfunction()

#
# This functions adds cmd-line tests given files.
#
# Usage add_cmdline_test(testbasename [EXE <executable>] [ARGS <args to exe>]
#                        [SCRIPT <script>]
#                        [EXPECTEDDIR <shared dir>] SUFFIX <suffix> FILES <test files>
#                        [EXPERIMENTAL])
#
# EXPERIMENTAL: If set, tag all tests as experimental
#
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "OPENSCAD;STDIO;EXPERIMENTAL" "EXE;SCRIPT;SUFFIX;KERNEL;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

  set(EXTRA_OPTIONS "")

  # If sharing results with another test, pass on this to the python script
  if (TESTCMD_EXPECTEDDIR)
    list(APPEND EXTRA_OPTIONS -e ${TESTCMD_EXPECTEDDIR})
  endif()

  if (TESTCMD_KERNEL)
    list(APPEND EXTRA_OPTIONS -k ${TESTCMD_KERNEL})
  endif()

  if (TESTCMD_STDIO)
    list(APPEND EXTRA_OPTIONS --stdin --stdout)
  endif()

  if ((TESTCMD_EXE OR TESTCMD_SCRIPT) AND TESTCMD_OPENSCAD)
    message(FATAL_ERROR "add_cmdline_test() does not allow OPENSCAD flag alongside EXE or SCRIPT values")
  endif()

  # python script implies Python3_EXECUTABLE
  if (TESTCMD_SCRIPT MATCHES \\.[Pp][Yy]$)
    set(TESTCMD_EXE ${Python3_EXECUTABLE})
  endif()
  if (TESTCMD_OPENSCAD)
    set(TESTCMD_EXE ${OPENSCAD_BINPATH})
  endif()

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CCBD}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")

    if (TESTCMD_EXPERIMENTAL)
      set(TEST_IS_EXPERIMENTAL true)
    endif()

    # add global experimental options here
    set(EXPERIMENTAL_OPTION "")

    # # To give manifold some extra test coverage, enable manifold 
    # # for all tests except those marked as skipped.
    # if (ENABLE_MANIFOLD
    #     AND NOT SCADFILE IN_LIST SCADFILES_DIFFERENT_MANIFOLD_EXPECTATIONS
    #     AND NOT SCADFILE IN_LIST SCADFILES_FAILING_WITH_MANIFOLD
    #     AND NOT TEST_FULLNAME IN_LIST TESTS_FAILING_WITH_MANIFOLD
    #     AND NOT TESTCMD_BASENAME MATCHES "^(stlexport|objexport)$"
    #     AND NOT TESTCMD_BASENAME MATCHES "^openscad-viewoptions-.*"
    #     AND NOT TESTCMD_BASENAME MATCHES "^remesh-.*")
    #   set(MANIFOLD_OPTION "--backend=manifold")
    # else()
    set(MANIFOLD_OPTION "")
    # endif()

    # 2D tests should be viewed from the top, not an angle.
    set(CAMERA_OPTION "")
    is_2d(${SCADFILE} IS2D)
    if (IS2D)
      set(CAMERA_OPTION "--camera=0,0,100,0,0,0" "--viewall" "--autocenter" "--projection=ortho")
    endif()

    # Handle configurations
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    if (NOT FOUNDCONFIGS)
      set_test_config(Default FILES ${TEST_FULLNAME})
    endif()
    set_test_config(All FILES ${TEST_FULLNAME})
    list(FIND FOUNDCONFIGS Bugs FOUND)
    if (FOUND EQUAL -1)
      set_test_config(Good FILES ${TEST_FULLNAME})
    endif()
    get_test_config(${TEST_FULLNAME} CONFVAL)

    set(FILENAME_OPTION -f ${FILE_BASENAME})

    # Apply lazy-union to *all* tests for comprehensive testing of this experimental feature.
    # Would need all passing before making lazy-union non-experimental, but that's probably a long way off.
    # Mostly just breaks issues that export non-manifold/intersecting geometry without explicit union.
    #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=lazy-union")

    # Enable vertex-object-renderers-indexing by default for all test if experimental build
    # if (EXPERIMENTAL)
    #   set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-indexing")
    # endif()

    string(JOIN " " DBG_COMMAND_STR
      "add_test(" ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
      COMMAND ${Python3_EXECUTABLE}
      ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
      -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
      ${TESTCMD_EXE} ${TESTCMD_SCRIPT} ${SCADFILE} ${CAMERA_OPTION}
      ${EXPERIMENTAL_OPTION} ${MANIFOLD_OPTION} ${TESTCMD_ARGS} ")"
    )

    # only add test if it is not experimental or if it is and experimental option is enabled
    if (NOT TEST_IS_EXPERIMENTAL OR EXPERIMENTAL)
      # Use cmake option "--log-level DEBUG" during top level config to see this
      message(DEBUG "${DBG_COMMAND_STR}")
      add_test(NAME ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
        COMMAND ${Python3_EXECUTABLE}
        ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
        -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
        ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION}
        ${EXPERIMENTAL_OPTION} ${MANIFOLD_OPTION} ${TESTCMD_ARGS}
      )
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT ${CTEST_ENVIRONMENT})
    else()
      message(DEBUG "Experimental Test not added: ${DBG_COMMAND_STR}")
    endif()
  endforeach()
endfunction()

# Usage add_failing_test(testbasename  RETVAL <expected return value>  SUFFIX <suffix>  FILES <test files>
#                        [EXE <executable>] [SCRIPT <script>] [ARGS <args to exe>])
#
function(add_failing_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "RETVAL;EXE;SCRIPT;SUFFIX" "FILES;ARGS" ${ARGN})

  if ("${TESTCMD_SUFFIX}" STREQUAL "") # Suffix "off" counts as a false value, so check directly for empty string.
    message(FATAL_ERROR "add_failing_test() requires SUFFIX to be set" )
  endif()
  if (NOT TESTCMD_EXE)
    set(TESTCMD_EXE ${Python3_EXECUTABLE})
  endif()
  if (NOT TESTCMD_SCRIPT)
    set(TESTCMD_SCRIPT ${SHOULDFAIL_PY})
  endif()

  set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    # Handle configurations
    unset(FOUNDCONFIGS)
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    if (NOT FOUNDCONFIGS)
      set_test_config(Default FILES ${TEST_FULLNAME})
    endif()
    set_test_config(All FILES ${TEST_FULLNAME})
    unset(FOUNDCONFIGS)
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    set(CONFVAL ${FOUNDCONFIGS})

    # The python script cannot extract the testname when given extra parameters
    if (TESTCMD_ARGS)
      set(FILENAME_OPTION -f ${FILE_BASENAME})
    endif()

    add_test(NAME ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL} COMMAND ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" -s ${TESTCMD_SUFFIX} ${TESTCMD_ARGS})
    set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
  endforeach()
endfunction()

#################################################
# Configure Templated Files and Generated Tests #
#################################################

configure_file(${TEST_SCAD_DIR}/templates/include-tests-template.scad
               ${TEST_SCAD_DIR}/misc/include-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/use-tests-template.scad
               ${TEST_SCAD_DIR}/misc/use-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_stl-tests-template.scad
               ${TEST_SCAD_DIR}/3D/features/import_stl-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_3mf-tests-template.scad
               ${TEST_SCAD_DIR}/3D/features/import_3mf-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_dxf-tests-template.scad
               ${TEST_SCAD_DIR}/2D/features/import_dxf-tests.scad)
configure_file(${TEST_PYTHON_DIR}/gen_issue2342-template.py
               ${TEST_PYTHON_DIR}/gen_issue2342.py)
configure_file(${TEST_PYTHON_DIR}/gen_svg_viewbox_tests-template.py
               ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py)

# Set up custom commands to run before & after Ctest run.
# 1. Start/stop Virtual Framebuffer for linux/bsd. 2. Pretty Print
# Please see the CTestCustom.template file for more info.
message(STATUS "creating CTestCustom.cmake")
set(TEMPLATE_HEADER "Generated by cmake from ${CCSD}/CTestCustom.template")
set(UPLOADARG "")
if ($ENV{OPENSCAD_UPLOAD_TESTS})
  set(UPLOADARG "--upload")
endif()
configure_file(${CCSD}/CTestCustom.template ${CBD}/CTestCustom.cmake @ONLY)

# generate a very large scad file which we would rather not commit to the source tree
# this is for stress-testing the parser
add_custom_target(issue2342 ALL
  COMMAND ${Python3_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_issue2342.py ">${TEST_SCAD_DIR}/issues/issue2342.scad"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating issue2342.scad"
)
add_custom_target(svg_viewbox_tests ALL
  COMMAND ${Python3_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py "${TEST_DATA_DIR}/svg/viewbox" "${TEST_SCAD_DIR}/svg/extruded"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating svg viewbox tests"
)

##################################
# Define Various Test File Lists #
##################################

# Find all scad files
file(GLOB FEATURES_2D_FILES   ${TEST_SCAD_DIR}/2D/features/*.scad)
list(REMOVE_ITEM FEATURES_2D_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad # -> EXPERIMENTAL_TEXTMETRICS_FILES
)
file(GLOB ISSUES_2D_FILES     ${TEST_SCAD_DIR}/2D/issues/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES    ${TEST_SCAD_DIR}/bugs2D/*.scad)
file(GLOB SCAD_DXF_FILES      ${TEST_SCAD_DIR}/dxf/*.scad)
file(GLOB SCAD_PDF_FILES      ${TEST_SCAD_DIR}/pdf/*.scad)
file(GLOB SCAD_SVG_FILES      ${TEST_SCAD_DIR}/svg/svg-spec/*.scad
  ${TEST_SCAD_DIR}/svg/box-w-holes-2d.scad
  ${TEST_SCAD_DIR}/svg/display.scad
  ${TEST_SCAD_DIR}/svg/id-selection-test.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
  ${TEST_SCAD_DIR}/svg/line-cap-line-join.scad
  ${TEST_SCAD_DIR}/svg/simple-center-2d.scad
  ${TEST_SCAD_DIR}/svg/use-transform.scad
  ${TEST_SCAD_DIR}/svg/fill-rule.scad
  ${TEST_SCAD_DIR}/svg/size-percent.scad)
  list(APPEND EXAMPLE_2D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

file(GLOB FEATURES_3D_FILES   ${TEST_SCAD_DIR}/3D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${TEST_SCAD_DIR}/3D/deprecated/*.scad)
file(GLOB ISSUES_3D_FILES     ${TEST_SCAD_DIR}/3D/issues/*.scad)
file(GLOB SCAD_AMF_FILES           ${TEST_SCAD_DIR}/amf/*.scad)
file(GLOB SCAD_NEF3_FILES          ${TEST_SCAD_DIR}/nef3/*.scad)
file(GLOB FUNCTION_FILES           ${TEST_SCAD_DIR}/functions/*.scad)
file(GLOB REDEFINITION_FILES       ${TEST_SCAD_DIR}/redefinition/*.scad)
file(GLOB_RECURSE BUGS_FILES       ${TEST_SCAD_DIR}/bugs/*.scad)
file(GLOB_RECURSE EXAMPLE_FILES    ${EXAMPLES_DIR}/*.scad)
list(REMOVE_ITEM EXAMPLE_FILES
  ${EXAMPLES_DIR}/Basics/roof.scad # -> EXPERIMENTAL_ROOF_FILES
)

list(APPEND EXAMPLE_3D_FILES ${EXAMPLE_FILES})
# Remove 2D files from 3D examples
list(REMOVE_ITEM EXAMPLE_3D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND MISC_FILES
  ${TEST_SCAD_DIR}/misc/arg-permutations.scad
  ${TEST_SCAD_DIR}/misc/escape-test.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/include-overwrite-main.scad
  ${TEST_SCAD_DIR}/misc/include-overwrite-main2.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
  ${TEST_SCAD_DIR}/misc/special-consts.scad
  ${TEST_SCAD_DIR}/misc/variable-overwrite.scad
)

list(APPEND FAILING_FILES
  ${TEST_SCAD_DIR}/issues/issue1890-comment.scad
  ${TEST_SCAD_DIR}/issues/issue1890-include.scad
  ${TEST_SCAD_DIR}/issues/issue1890-string.scad
  ${TEST_SCAD_DIR}/issues/issue1890-use.scad
)

list(APPEND ECHO_FILES ${FUNCTION_FILES} ${MISC_FILES} ${REDEFINITION_FILES}
  ${TEST_SCAD_DIR}/3D/features/for-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate-parameters.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-evaluation-tests.scad
  ${TEST_SCAD_DIR}/misc/echo-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-fail1-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail2-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail3-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail4-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail5-test.scad
  ${TEST_SCAD_DIR}/misc/for-c-style-infinite-loop.scad
  ${TEST_SCAD_DIR}/misc/parser-tests.scad
  ${TEST_SCAD_DIR}/misc/builtin-tests.scad
  ${TEST_SCAD_DIR}/misc/dim-all.scad
  ${TEST_SCAD_DIR}/misc/string-test.scad
  ${TEST_SCAD_DIR}/misc/string-indexing.scad
  ${TEST_SCAD_DIR}/misc/string-unicode.scad
  ${TEST_SCAD_DIR}/misc/chr-tests.scad
  ${TEST_SCAD_DIR}/misc/ord-tests.scad
  ${TEST_SCAD_DIR}/misc/vector-values.scad
  ${TEST_SCAD_DIR}/misc/search-tests.scad
  ${TEST_SCAD_DIR}/misc/search-tests-unicode.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function2.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function3.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-module.scad
  ${TEST_SCAD_DIR}/misc/tail-recursion-tests.scad
  ${TEST_SCAD_DIR}/misc/value-reassignment-tests.scad
  ${TEST_SCAD_DIR}/misc/value-reassignment-tests2.scad
  ${TEST_SCAD_DIR}/misc/variable-scope-tests.scad
  ${TEST_SCAD_DIR}/misc/scope-assignment-tests.scad
  ${TEST_SCAD_DIR}/misc/lookup-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-shortcircuit-tests.scad
  ${TEST_SCAD_DIR}/misc/parent_module-tests.scad
  ${TEST_SCAD_DIR}/misc/children-tests.scad
  ${TEST_SCAD_DIR}/misc/range-tests.scad
  ${TEST_SCAD_DIR}/misc/no-break-space-test.scad
  ${TEST_SCAD_DIR}/misc/unicode-tests.scad
  ${TEST_SCAD_DIR}/misc/utf8-tests.scad
  ${TEST_SCAD_DIR}/misc/nbsp-utf8-test.scad
  ${TEST_SCAD_DIR}/misc/nbsp-latin1-test.scad
  ${TEST_SCAD_DIR}/misc/concat-tests.scad
  ${TEST_SCAD_DIR}/misc/include-recursive-test.scad
  ${TEST_SCAD_DIR}/misc/errors-warnings.scad
  ${TEST_SCAD_DIR}/misc/errors-warnings-included.scad
  ${TEST_SCAD_DIR}/misc/children-warnings-tests.scad
  ${TEST_SCAD_DIR}/misc/isundef-test.scad
  ${TEST_SCAD_DIR}/misc/islist-test.scad
  ${TEST_SCAD_DIR}/misc/isnum-test.scad
  ${TEST_SCAD_DIR}/misc/isbool-test.scad
  ${TEST_SCAD_DIR}/misc/isstring-test.scad
  ${TEST_SCAD_DIR}/misc/operators-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-precedence.scad
  ${TEST_SCAD_DIR}/misc/builtins-calling-vec3vec2.scad
  ${TEST_SCAD_DIR}/misc/leaf-module-warnings.scad
  ${TEST_SCAD_DIR}/issues/issue1472.scad
  ${TEST_SCAD_DIR}/misc/empty-stl.scad
  ${TEST_SCAD_DIR}/issues/issue1516.scad
  ${TEST_SCAD_DIR}/issues/issue1528.scad
  ${TEST_SCAD_DIR}/issues/issue1923.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
  ${TEST_SCAD_DIR}/issues/issue1851-each-fail-on-scalar.scad
  ${TEST_SCAD_DIR}/issues/issue2342.scad
  ${TEST_SCAD_DIR}/issues/issue3118-recur-limit.scad
  ${TEST_SCAD_DIR}/issues/issue3541.scad
  ${TEST_SCAD_DIR}/misc/function-scope.scad
  ${TEST_SCAD_DIR}/misc/root-modifiers.scad
  ${TEST_SCAD_DIR}/misc/root-modifier-for.scad
  ${TEST_DATA_DIR}/use-order-test/use-order-test.scad
  ${TEST_SCAD_DIR}/misc/vector-swizzling.scad
  ${TEST_SCAD_DIR}/misc/linenumber.scad
)

list(APPEND ASTDUMPTEST_FILES ${MISC_FILES}
  ${TEST_SCAD_DIR}/functions/assert-expression-fail1-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail2-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail3-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/echo-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/expression-precedence-tests.scad
  ${TEST_SCAD_DIR}/functions/let-test-single.scad
  ${TEST_SCAD_DIR}/functions/let-tests.scad
  ${TEST_SCAD_DIR}/functions/list-comprehensions.scad
  ${TEST_SCAD_DIR}/functions/exponent-operator-test.scad
  ${TEST_SCAD_DIR}/misc/ifelse-ast-dump.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
)

list(APPEND DUMPTEST_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES})

list(APPEND RENDERTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND RENDERTEST_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES})
list(APPEND RENDERTEST_3D_FILES
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
  ${TEST_SCAD_DIR}/misc/empty-shape-tests.scad
  ${TEST_SCAD_DIR}/misc/null-polygons.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
)

# test importing unparseable files, result will be an empty image
list(APPEND STL_IMPORT_FILES
  ${TEST_SCAD_DIR}/stl/stl-import-invalidvertex.scad
  ${TEST_SCAD_DIR}/stl/stl-import-toomanyvertices.scad
  ${TEST_SCAD_DIR}/stl/stl-import-unparseable.scad
)

list(GET RENDERTEST_2D_FILES 0 1 2 RENDERSTDIOTEST_FILES)
list(APPEND RENDERTEST_FILES ${RENDERTEST_2D_FILES} ${RENDERTEST_3D_FILES} ${BUGS_FILES} ${BUGS_2D_FILES})

list(APPEND PREVIEW_ONLY_FILES
  ${TEST_SCAD_DIR}/3D/features/child-background.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-and-background-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/background-modifier2.scad
)

list(REMOVE_ITEM RENDERTEST_FILES 
  # These tests only makes sense in preview mode
  ${PREVIEW_ONLY_FILES}
  # These tests don't make sense in CGAL mode
  ${TEST_SCAD_DIR}/3D/features/color-names-tests.scad
  ${TEST_SCAD_DIR}/3D/features/hex-colors-tests.scad
)

set(PRUNE_TEST ${TEST_SCAD_DIR}/misc/intersection-prune-test.scad)
list(APPEND PREVIEWTEST_FILES ${STL_IMPORT_FILES} ${RENDERTEST_FILES} ${PRUNE_TEST} ${PREVIEW_ONLY_FILES})
list(APPEND THROWNTOGETHERTEST_FILES ${RENDERTEST_FILES} ${PRUNE_TEST} ${PREVIEW_ONLY_FILES})

list(APPEND COLOR_3D_TEST_FILES
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
)

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES
  ${FILES_2D}
  ${SCAD_DXF_FILES}
  ${SCAD_SVG_FILES}
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)

# lazy-union
list(APPEND LAZYUNION_3D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-objects.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-nested-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-children.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-hull-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-root-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-intersection-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-difference-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-minkowski-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-transform-for.scad
  ${TEST_SCAD_DIR}/3D/features/2d-3d.scad
)
list(APPEND LAZYUNION_2D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-2dobjects.scad
)
list(APPEND LAZYUNION_FILES ${LAZYUNION_2D_FILES} ${LAZYUNION_3D_FILES})

list(APPEND SVG_VIEWBOX_TESTS
  viewbox_300x400_none viewbox_600x200_none
  viewbox_300x400_meet_xMinYMin viewbox_300x400_meet_xMidYMin viewbox_300x400_meet_xMaxYMin
  viewbox_600x200_meet_xMinYMin viewbox_600x200_meet_xMinYMid viewbox_600x200_meet_xMinYMax
  viewbox_600x200_slice_xMinYMin viewbox_600x200_slice_xMidYMin viewbox_600x200_slice_xMaxYMin
  viewbox_600x600_slice_xMinYMin viewbox_600x600_slice_xMinYMid viewbox_600x600_slice_xMinYMax
)

# FIXME(kintel): Not in use anymore, but keep for reference?
list(APPEND SCADFILES_WITH_GREEN_FACES
  ${EXAMPLES_DIR}/Advanced/children.scad
  ${EXAMPLES_DIR}/Basics/CSG-modules.scad
  ${EXAMPLES_DIR}/Basics/CSG.scad
  ${EXAMPLES_DIR}/Basics/logo.scad
  ${EXAMPLES_DIR}/Basics/LetterBlock.scad
  ${EXAMPLES_DIR}/Basics/text_on_cube.scad
  ${EXAMPLES_DIR}/Basics/logo_and_text.scad
  ${EXAMPLES_DIR}/Parametric/sign.scad
  ${EXAMPLES_DIR}/Parametric/candleStand.scad
  ${EXAMPLES_DIR}/Old/example001.scad
  ${EXAMPLES_DIR}/Old/example002.scad
  ${EXAMPLES_DIR}/Old/example003.scad
  ${EXAMPLES_DIR}/Old/example004.scad
  ${EXAMPLES_DIR}/Old/example005.scad
  ${EXAMPLES_DIR}/Old/example006.scad
  ${EXAMPLES_DIR}/Old/example007.scad
  ${EXAMPLES_DIR}/Old/example012.scad
  ${EXAMPLES_DIR}/Old/example016.scad
  ${EXAMPLES_DIR}/Old/example024.scad
  ${TEST_SCAD_DIR}/3D/features/difference-tests.scad
  ${TEST_SCAD_DIR}/3D/features/for-tests.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-difference-test.scad
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1803.scad
  ${TEST_SCAD_DIR}/3D/issues/issue3158.scad
  ${TEST_SCAD_DIR}/3D/issues/issue835.scad
  ${TEST_SCAD_DIR}/3D/issues/issue911.scad
  ${TEST_SCAD_DIR}/3D/issues/issue913.scad
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
  ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
)

list(APPEND SCADFILES_WITH_COLOR
  ${TEST_SCAD_DIR}/3D/features/child-child-test.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests2.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate-parameters.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-names-tests.scad
  ${TEST_SCAD_DIR}/3D/features/hex-colors-tests.scad

  ${EXAMPLES_DIR}/Advanced/assert.scad
  ${EXAMPLES_DIR}/Advanced/animation.scad
  ${EXAMPLES_DIR}/Advanced/GEB.scad
  ${EXAMPLES_DIR}/Advanced/children.scad
  ${EXAMPLES_DIR}/Advanced/children_indexed.scad
  ${EXAMPLES_DIR}/Basics/CSG-modules.scad
  ${EXAMPLES_DIR}/Basics/linear_extrude.scad
  ${EXAMPLES_DIR}/Basics/logo_and_text.scad
  ${EXAMPLES_DIR}/Basics/projection.scad
  ${EXAMPLES_DIR}/Basics/text_on_cube.scad
  ${EXAMPLES_DIR}/Advanced/surface_image.scad
  ${EXAMPLES_DIR}/Functions/functions.scad
  ${EXAMPLES_DIR}/Basics/rotate_extrude.scad
  ${EXAMPLES_DIR}/Old/example017.scad

  ${TEST_SCAD_DIR}/3D/issues/issue5217.scad

  #${TEST_SCAD_DIR}/bugs/issue1000.scad
)

# Render in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND SCADFILES_DIFFERENT_MANIFOLD_RENDER_EXPECTATIONS
  # Manifold backend supports color rendering
  ${SCADFILES_WITH_COLOR}

  # Manifold can repair winding order
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad

  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${EXAMPLES_DIR}/Old/example016.scad

  # Manifold can represent the very think slice
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
)

# Preview in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND SCADFILES_DIFFERENT_MANIFOLD_PREVIEW_EXPECTATIONS
  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad

  # resize() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad

  # TODO(kintel): Run bugs files
  # ${TEST_SCAD_DIR}/bugs/issue1000.scad
  # ${TEST_SCAD_DIR}/bugs/issue802.scad
  # ${TEST_SCAD_DIR}/bugs2D/issue2220.scad
)

list(APPEND SCADFILES_FAILING_WITH_MANIFOLD
  # Add any tests failing just w/ Manifold here.
)

list(APPEND RENDERMANIFOLDTEST_FILES ${RENDERTEST_FILES})
list(REMOVE_ITEM RENDERMANIFOLDTEST_FILES
  ${SCAD_NEF3_FILES}       # Nef3 import not supported in Manifold mode
  ${SCADFILES_DIFFERENT_MANIFOLD_RENDER_EXPECTATIONS}
  ${SCADFILES_FAILING_WITH_MANIFOLD}
)

list(APPEND PREVIEWMANIFOLDTEST_FILES ${PREVIEWTEST_FILES})
list(REMOVE_ITEM PREVIEWMANIFOLDTEST_FILES
  ${SCADFILES_DIFFERENT_MANIFOLD_PREVIEW_EXPECTATIONS}
  ${SCADFILES_FAILING_WITH_MANIFOLD}
)

##############################
# Define test configurations #
##############################
# Must be done BEFORE adding any tests

# Clear test config cache variables
foreach(CONFIG $CACHE{TEST_CONFIGS})
  unset(${CONFIG}_TEST_CONFIG CACHE)
endforeach()

# Heavy tests are tests taking more than 5 seconds on a decent 2023 desktop
set_test_config(Heavy FILES
  csgrendertest_issue267-normalization-crash
  rendertest_issue267-normalization-crash
)

# Bugs
set_test_config(Bugs FILES ${BUGS_FILES} ${BUGS_2D_FILES} PREFIXES
 rendertest rendermanifoldtest csgrendertest previewtest previewmanifoldtest throwntogethertest)
set_test_config(Bugs FILES ${BUGS_FILES} PREFIXES
  offpreviewtest monotonerendertest 
  stlpreviewtest stlpreviewmanifoldtest 
  stlrendertest stlrendermanifoldtest
  stlrenderforcetest stlrendermanifoldforcetest
  binstlrenderforcetest binstlrendermanifoldforcetest
  offrendertest)

# Re-enable "fixed by manifold" bugs for Manifold
set_test_config(Bugs FILES
  # https://github.com/openscad/openscad/issues/3567
  rendertest_linear_extrude-scale-zero-tests
  csgrendertest_linear_extrude-scale-zero-tests

)

# Remove bugs fixed by manifold from the Bugs tag, to guard against regressions
remove_test_config(Bugs FILES
  stlrendermanifoldforcetest_issue584
  rendermanifoldtest_issue591
  rendermanifoldtest_issue791 
  # FIXME: #802
  stlrendermanifoldforcetest_issue945
  stlrendermanifoldforcetest_issue945b
  stlrendermanifoldforcetest_issue945c
  stlrendermanifoldforcetest_issue945d
  stlrendermanifoldforcetest_issue945e
  stlrendermanifoldforcetest_issue945f
  rendermanifoldtest_issue1455
)

# Examples
set_test_config(Examples FILES ${EXAMPLE_FILES} PREFIXES
  rendertest rendermanifoldtest rendermanifoldtest-different 
  previewtest previewmanifoldtest previewmanifoldtest-different previewtestthrowntogethertest 
  csgrendertest monotonerendertest 
  offpreviewtest offrendertest dumptest-examples)
set_test_config(Examples FILES ${EXAMPLE_2D_FILES} PREFIXES dxfrendertest)

#############
# Add tests #
#############

# Types of tests:
# o echotest: Just record console output
# o dumptest: Export .csg
# o rendertest: Export to PNG using --render
# o rendermanifoldtest: Export to PNG using --render with --backend=manifold
# o previewtest: Export to PNG using OpenCSG
# o previewmanifoldtest: Export to PNG in preview mode with --backend=manifold
# o throwntogethertest: Export to PNG using the Throwntogether renderer
# o csgrendertest: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o monotonerendertest: Same as rendertest but with the "Monotone" color scheme
# o stlpreviewtest: Export to STL, Re-import and render to PNG (--render)
# o stlrendertest: Export to STL, Re-import and render to PNG (--render=force)
# o offpreviewtest: Export to OFF, Re-import and render to PNG (--render)
# o offrendertest: Export to STL, Re-import and render to PNG (--render=force)
# o dxfrendertest: Export to DXF, Re-import and render to PNG (--render=force)
#

add_cmdline_test(astdumptest OPENSCAD SUFFIX ast FILES
  ${MISC_FILES}
  ${TEST_SCAD_DIR}/functions/assert-expression-fail1-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail2-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail3-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/echo-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/expression-precedence-tests.scad
  ${TEST_SCAD_DIR}/functions/let-test-single.scad
  ${TEST_SCAD_DIR}/functions/let-tests.scad
  ${TEST_SCAD_DIR}/functions/list-comprehensions.scad
  ${TEST_SCAD_DIR}/functions/exponent-operator-test.scad
  ${TEST_SCAD_DIR}/misc/ifelse-ast-dump.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
)
add_cmdline_test(astdumpstdiotest OPENSCAD SUFFIX ast FILES ${TEST_SCAD_DIR}/misc/allexpressions.scad STDIO EXPECTEDDIR astdumptest ARGS --export-format ast)

add_cmdline_test(csgtermtest      OPENSCAD SUFFIX term FILES
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
)

add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${ECHO_FILES})
# trace-usermodule-parameters is on by default,
# but can generate very long outputs and potentially
# unstable outputs, when combined with recursive tests.
add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/recursion-test-vector.scad ARGS --trace-usermodule-parameters=false)

add_cmdline_test(echostdiotest    OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/echo-tests.scad STDIO EXPECTEDDIR echotest ARGS --export-format echo)
add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/builtin-invalid-range-test.scad ARGS --check-parameter-ranges=on)

# This test is quiet to speed up the test and to have a stable and reproducable output
add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/issues/issue4172-echo-vector-stack-exhaust.scad ARGS --quiet --trace-usermodule-parameters=false)

add_cmdline_test(dumptest           OPENSCAD FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES} SUFFIX csg ARGS)
add_cmdline_test(dumptest-examples  OPENSCAD FILES ${EXAMPLE_FILES} SUFFIX csg ARGS)
# non-ASCII filenames
add_cmdline_test(openscad-nonascii  OPENSCAD FILES ${TEST_SCAD_DIR}/misc/sfære.scad SUFFIX csg)

# Baseline PNG tests
add_cmdline_test(rendertest         OPENSCAD FILES ${RENDERTEST_FILES} SUFFIX png ARGS --render)
add_cmdline_test(previewtest        OPENSCAD FILES ${PREVIEWTEST_FILES} SUFFIX png ARGS)
add_cmdline_test(throwntogethertest OPENSCAD SUFFIX png FILES ${THROWNTOGETHERTEST_FILES} ARGS --preview=throwntogether)

# Other PNG tests
add_cmdline_test(renderstdiotest     OPENSCAD SUFFIX png FILES ${RENDERSTDIOTEST_FILES} STDIO EXPECTEDDIR rendertest ARGS --export-format png --render)
add_cmdline_test(csgrendertest       SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${RENDERTEST_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=csg --render)
if (ENABLE_MANIFOLD)
add_cmdline_test(rendermanifoldtest            OPENSCAD SUFFIX png FILES ${RENDERMANIFOLDTEST_FILES} EXPECTEDDIR rendertest ARGS --render --backend=manifold)
add_cmdline_test(rendermanifoldtest-different  OPENSCAD SUFFIX png FILES ${SCADFILES_DIFFERENT_MANIFOLD_RENDER_EXPECTATIONS} ARGS --render --backend=manifold)
add_cmdline_test(previewmanifoldtest           OPENSCAD SUFFIX png FILES ${PREVIEWMANIFOLDTEST_FILES} EXPECTEDDIR previewtest ARGS --backend=manifold)
add_cmdline_test(previewmanifoldtest-different OPENSCAD SUFFIX png FILES ${SCADFILES_DIFFERENT_MANIFOLD_PREVIEW_EXPECTATIONS} ARGS --backend=manifold)
endif()

set(VIEWBOX_TEST "${TEST_SCAD_DIR}/svg/extruded/viewbox-test.scad")
foreach(TEST ${SVG_VIEWBOX_TESTS})
  add_cmdline_test(svgviewbox-${TEST} OPENSCAD ARGS --imgsize 600,600 "-Dfile=\"${TEST_DATA_DIR}/svg/viewbox/${TEST}.svg\";" SUFFIX png FILES ${VIEWBOX_TEST})
endforeach()

add_cmdline_test(svgimport OPENSCAD ARGS --imgsize 600,600 SUFFIX png FILES
  ${TEST_SCAD_DIR}/svg/extruded/box-w-holes.scad
  ${TEST_SCAD_DIR}/svg/extruded/simple-center.scad
)

#
# Export/import tests
#

list(APPEND EXPORT_STL_TEST_FILES
  ${TEST_SCAD_DIR}/stl/stl-export.scad
)

# FIXME: Reintroduce
list(APPEND COLOR_EXPORT_TEST_FILES
  ${TEST_SCAD_DIR}/misc/color-export.scad
)

list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-export.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_dodecahedron.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_cube.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/3D/features/polyhedron-cube.scad)

list(APPEND EXPORT_3MF_TEST_FILES ${TEST_SCAD_DIR}/3mf/3mf-export.scad)

list(APPEND EXPORT_POV_TEST_FILES ${TEST_SCAD_DIR}/pov/pov-export.scad)

list(APPEND EXPORT_IMPORT_3D_FILES
${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
${TEST_SCAD_DIR}/3D/features/polyhedron-nonplanar-tests.scad
${TEST_SCAD_DIR}/3D/features/rotate_extrude-tests.scad
${TEST_SCAD_DIR}/3D/features/union-coincident-test.scad
${TEST_SCAD_DIR}/3D/issues/fn_bug.scad
${TEST_SCAD_DIR}/3D/issues/issue904.scad
${TEST_SCAD_DIR}/3D/issues/issue1105.scad
${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
${TEST_SCAD_DIR}/3D/issues/issue1215.scad
${TEST_SCAD_DIR}/3D/issues/issue1215b.scad
${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
${TEST_SCAD_DIR}/3D/issues/issue1221.scad
${TEST_SCAD_DIR}/3D/issues/issue1225.scad
${TEST_SCAD_DIR}/3D/issues/issue1258.scad
${TEST_SCAD_DIR}/3D/issues/issue2259.scad
${TEST_SCAD_DIR}/misc/null-polygons.scad
${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
${TEST_SCAD_DIR}/misc/internal-cavity.scad
${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
${TEST_SCAD_DIR}/misc/preview_variable.scad
${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
)

#
# There are some caveats with export and import, so we need to test a few combinations:
# 1. It may be possible to export a non-manifold mesh (e.g. malformed polyhedron) due to 
#    no manifoldness checks at export time. This is by design, e.g. to allow users to 
#    troubleshoot externally.
# 2. It may be possible to import such non-manifolds and preview or render them, but it will
#    likely fail when trying to construct a data structure requiring manifold objects (e.g. --render=force)
#
# This leads to three types of tests:
# 1. <format>rendertest: Manifold object export + render (both with --render=force)
# 2. <format>previewtest: Non-manifold polyhedron export + preview
# 3. <format>rendertest: Complex Manifold polyhedron export + render (both with --render=force)
#    e.g. self-touching polyhedron or malformed but repairable.
#

# Export tests (compare actually exported files)
add_cmdline_test(stlexport              EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render)
add_cmdline_test(stlexport-stdout       EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR stlexport ARGS --enable=predictible-output --render --export-format asciistl)
if (ENABLE_MANIFOLD)
add_cmdline_test(manifold-stlexport     EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} EXPECTEDDIR stlexport ARGS --enable=predictible-output --backend=manifold --render)
endif()

add_cmdline_test(binstlexport           EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render --export-format binstl)
add_cmdline_test(binstlexport-stdout    EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR binstlexport ARGS --enable=predictible-output --render --export-format binstl)

add_cmdline_test(objexport              EXPERIMENTAL OPENSCAD SUFFIX obj FILES ${EXPORT_OBJ_TEST_FILES} ARGS --render --enable=predictible-output)
if (LIB3MF_FOUND)
add_cmdline_test(3mfexport              EXPERIMENTAL OPENSCAD SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES} ARGS --enable=predictible-output)
endif()
add_cmdline_test(povexport-as-is        EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold)
add_cmdline_test(povexport-translate-1  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,0,140)
add_cmdline_test(povexport-translate-2  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=10,0,0,0,0,0,140)
add_cmdline_test(povexport-translate-3  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,10,0,0,0,0,140)
add_cmdline_test(povexport-translate-4  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,10,0,0,0,140)
add_cmdline_test(povexport-rotate-1     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,90,0,0,140)
add_cmdline_test(povexport-rotate-2     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,90,0,140)
add_cmdline_test(povexport-rotate-3     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,90,140)
add_cmdline_test(povexport-distance-1   EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,0,100)
add_cmdline_test(povexport-all          EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=1,2,3,4,5,6,7)

# Trivial Export/Import files: Sanity-checks bidirectional file format import/export
set(SIMPLE_EXPORT_IMPORT_2D_FILES ${TEST_SCAD_DIR}/misc/square10.scad)
set(SIMPLE_EXPORT_IMPORT_3D_FILES ${TEST_SCAD_DIR}/misc/cube10.scad)
set(SIMPLE_EXPORT_IMPORT_NON_MANIFOLD ${TEST_SCAD_DIR}/3D/misc/polyhedron-single-triangle.scad)

set(EXPORT_IMPORT_2D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_2D_TESTS} ${FILES_2D})

set(EXPORT_IMPORT_3D_PREVIEW_FILES ${SIMPLE_EXPORT_IMPORT_NON_MANIFOLD} ${SIMPLE_EXPORT_IMPORT_3D_FILES})
set(EXPORT_IMPORT_3D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_3D_FILES} ${EXPORT_IMPORT_3D_FILES})
list(REMOVE_ITEM EXPORT_IMPORT_3D_RENDER_FILES
  # Non-manifold polyhedrons can never be rendered
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
)
set(EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES ${EXPORT_IMPORT_3D_RENDER_FILES})

# Manifoldness corner cases
set(FILES_MANIFOLD_CORNER_CASES
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-slim.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-vertex.scad
)

# Export-import tests
add_cmdline_test(monotonerendertest OPENSCAD SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} ${SIMPLE_EXPORT_IMPORT_2D_FILES} ARGS --colorscheme=Monotone --render)
add_cmdline_test(stlpreviewtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=STL)
add_cmdline_test(offpreviewtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OFF)
add_cmdline_test(amfpreviewtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=AMF)
add_cmdline_test(objpreviewtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OBJ)

add_cmdline_test(stlrendertest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force)
add_cmdline_test(offrendertest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force)
add_cmdline_test(amfrendertest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=AMF --render=force)
add_cmdline_test(objrendertest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force)

if (ENABLE_MANIFOLD)
add_cmdline_test(offrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${OFFRENDERMANIFOLDTEST_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(offrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TEST_SCAD_DIR}/misc/cube10.scad EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(offrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad EXPECTEDDIR rendermanifoldtest-different ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(offrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)

add_cmdline_test(stlrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)
add_cmdline_test(stlrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR offrendermanifoldtest ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)

set(OFFRENDERMANIFOLDTEST_FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES})
list(REMOVE_ITEM OFFRENDERMANIFOLDTEST_FILES
  ${TEST_SCAD_DIR}/misc/cube10.scad
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
)
add_cmdline_test(amfrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=AMF --render=force --backend=manifold)
add_cmdline_test(objrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
add_cmdline_test(objrendermanifoldtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR offrendermanifoldtest ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
endif(ENABLE_MANIFOLD)

if (LIB3MF_FOUND)
add_cmdline_test(3mfpreviewtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=3MF)
add_cmdline_test(3mfrendertest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force)
add_cmdline_test(3mfrendermanifoldtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR monotonerendertest ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
add_cmdline_test(3mfrendermanifoldtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR offrendermanifoldtest ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
endif()

add_cmdline_test(dxfrendertest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_2D_RENDER_FILES} ${SCAD_DXF_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=DXF --render=force)
add_cmdline_test(svgrendertest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_2D_RENDER_FILES} ${SCAD_SVG_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=SVG --render=force)

# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(stlexportsanitytest  SCRIPT ${STLEXPORTSANITYTEST_PY} SUFFIX txt FILES ${TEST_SCAD_DIR}/misc/normal-nan.scad ARGS ${OPENSCAD_EXE_ARG})

# Export/import color support
add_cmdline_test(offcolorpngtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${COLOR_3D_TEST_FILES} EXPECTEDDIR rendermanifoldtest-different ARGS ${OPENSCAD_EXE_ARG} --format=OFF --backend=manifold --render)

# PDF Export
add_cmdline_test(pdfexporttest SCRIPT ${EXPORT_PNGTEST_PY} SUFFIX png FILES ${SCAD_PDF_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=PDF KERNEL Square:2)

# Expected failing tests
add_failing_test(stlfailedtest         SUFFIX stl  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(offfailedtest         SUFFIX off  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(parsererrors          SUFFIX stl  FILES ${FAILING_FILES} ARGS --retval=1)
# Hardwarning Test
add_failing_test(hardwarnings          SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/errors-warnings.scad ARGS --retval=1 --hardwarnings)

# Verify that test framework is paying attention to alpha channel, issue 1492
#add_cmdline_test(openscad-colorscheme-cornfield-alphafail  ARGS --colorscheme=Cornfield SUFFIX png FILES ${EXAMPLES_DIR}/Basics/logo.scad)

# The "expected image" supplied for this "alphafail" test has the alpha channel for all background pixels cleared (a==0),
# when they should be opaque (a==1) for this colorscheme.
# So if test framework is functioning properly then the image comparison should fail.
# Commented out because the master branch isn't capable of making the expected image yet.
# Also TEST_GENERATE=1 makes an expected image that makes the test fail.
#set_property(TEST openscad-colorscheme-cornfield-alphafail_logo PROPERTY WILL_FAIL TRUE)

#
# Customizer tests
#
set(SET_OF_PARAM_TEST "${TEST_CUSTOMIZER_DIR}/setofparameter.scad")
set(SET_OF_PARAM_JSON "${TEST_CUSTOMIZER_DIR}/setofparameter.json")
add_cmdline_test(customizertest OPENSCAD ARGS SUFFIX ast FILES
  ${TEST_CUSTOMIZER_DIR}/description.scad
  ${TEST_CUSTOMIZER_DIR}/parameter.scad
  ${TEST_CUSTOMIZER_DIR}/allmodulescomment.scad
  ${TEST_CUSTOMIZER_DIR}/allfunctionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/allexpressionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/group.scad
)
add_cmdline_test(customizertest-first          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P firstSet)
add_cmdline_test(customizertest-wrong          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P wrongSetValues)
add_cmdline_test(customizertest-incomplete     OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P thirdSet)
add_cmdline_test(customizertest-imgset         OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P imagine)
add_cmdline_test(customizertest-setNameWithDot OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P Name.dot)

# Variable override (-D arg)
add_cmdline_test(openscad-override         OPENSCAD FILES ${TEST_SCAD_DIR}/misc/override.scad SUFFIX echo ARGS -D a=3$<SEMICOLON>)

#
# Camera tests
#
# Test transcription of --camera options to $vp* for 7-argument --camera.
add_cmdline_test(echotest-cli-view-variables-7         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(dumptest-cli-view-variables-7         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(previewtest-cli-view-variables-7      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)

# Test transcription of --camera options to $vp* for 6-argument --camera.
add_cmdline_test(echotest-cli-view-variables-6         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(dumptest-cli-view-variables-6         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(previewtest-cli-view-variables-6      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)

set(CAMERA_TEST           "${TEST_SCAD_DIR}/3D/misc/camera-tests.scad")
set(CAMERA_TEST_OFFCENTER "${TEST_SCAD_DIR}/3D/misc/camera-tests-offcenter.scad")
set(CAMERA_TEST_VP        "${TEST_SCAD_DIR}/3D/misc/camera-vp.scad")
# Image output parameters
add_cmdline_test(openscad-imgsize          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,100)
add_cmdline_test(openscad-imgstretch       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 500,100)
add_cmdline_test(openscad-imgstretch2      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,500)
# Perspective gimbal cam
set(IMGSIZE "--imgsize=500,500")
add_cmdline_test(openscad-camdist          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,90,0,90,200)
add_cmdline_test(openscad-camrot           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,440,337.5,315,200)
add_cmdline_test(openscad-camtrans         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,200)
add_cmdline_test(openscad-camtrans-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,6000 --viewall)
add_cmdline_test(openscad-camtrans-viewall-offcenter OPENSCAD FILES ${CAMERA_TEST_OFFCENTER} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,30,40,50,10 --viewall --autocenter)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-20,90,0,90,220 --projection=o)
add_cmdline_test(openscad-camortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o)
# Perspective vector cam
add_cmdline_test(openscad-cameye            OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)
add_cmdline_test(openscad-cameye_front      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,-130,0,0,0,0)
add_cmdline_test(openscad-cameye_back       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,130,0,0,0,0)
add_cmdline_test(openscad-cameye_left       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=-130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_right      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_top        OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,130,0,0,0)
add_cmdline_test(openscad-cameye_bottom     OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,-130,0,0,0)
add_cmdline_test(openscad-cameye2           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=160,140,130,0,0,0)
add_cmdline_test(openscad-camcenter         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,60,30,20,10,30)
add_cmdline_test(openscad-camcenter-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=60,40,30,20,10,30 --viewall)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=90,80,75,0,0,0 --projection=o)
add_cmdline_test(openscad-cameyeortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=16,14,13,0,0,0 --viewall --projection=o)

add_cmdline_test(openscad-camvp-variables     OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE})
add_cmdline_test(openscad-camvp-override      OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)

#
# View Options tests
#
set(VIEW_OPTIONS_TEST "${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad")
add_cmdline_test(openscad-viewoptions-axes              OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes)
add_cmdline_test(openscad-viewoptions-axes-scales       OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales)
add_cmdline_test(openscad-viewoptions-edges             OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges)
add_cmdline_test(openscad-viewoptions-axes-scales-edges OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales,edges)
add_cmdline_test(openscad-viewoptions-wireframe         OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view wireframe)
add_cmdline_test(openscad-viewoptions-crosshairs        OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view crosshairs)

#
# Colorscheme tests
#
set(LOGO_EXAMPLE ${EXAMPLES_DIR}/Basics/logo.scad)
set(CSG_EXAMPLE  ${EXAMPLES_DIR}/Basics/CSG.scad)
add_cmdline_test(openscad-colorscheme-cornfield       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Cornfield)
add_cmdline_test(openscad-colorscheme-metallic        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Metallic)
add_cmdline_test(openscad-colorscheme-sunset          OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Sunset)
add_cmdline_test(openscad-colorscheme-starnight       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Starnight)
add_cmdline_test(openscad-colorscheme-monotone        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Monotone)
add_cmdline_test(openscad-colorscheme-clearsky        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=ClearSky)
add_cmdline_test(openscad-colorscheme-metallic-render OPENSCAD FILES ${CSG_EXAMPLE}  SUFFIX png ARGS --colorscheme=Metallic --render)

############################
# Experimental tests       #
############################

#
# --enable=lazyunion tests
#
add_cmdline_test(lazyunion-dump        EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-preview     EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-render      EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(lazyunion-monotonepng EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_3D_FILES} ARGS --colorscheme=Monotone --enable=lazy-union --render )
add_cmdline_test(lazyunion-stlpreviewtest  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS ${OPENSCAD_EXE_ARG} --format=STL --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-offpreviewtest  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS ${OPENSCAD_EXE_ARG} --format=OFF --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-dxfrendertest  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render     ARGS ${OPENSCAD_EXE_ARG} --format=DXF --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-svgrendertest  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render     ARGS ${OPENSCAD_EXE_ARG} --format=SVG --enable=lazy-union --render=force)

#
# --enable=roof tests
#
list(APPEND EXPERIMENTAL_ROOF_FILES ${EXAMPLES_DIR}/Basics/roof.scad)
add_cmdline_test(dumptest-examples   EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof)
add_cmdline_test(rendertest          EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --render --enable=roof)
add_cmdline_test(csgrendertest       EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=roof)
add_cmdline_test(rendermanifoldtest  EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR rendertest ARGS --render --enable=roof --backend=manifold)
add_cmdline_test(previewtest         EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof)
add_cmdline_test(previewmanifoldtest EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof --backend=manifold)
add_cmdline_test(throwntogethertest  EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --preview=throwntogether --enable=roof)

#
# --enable=import-function tests
#
list(APPEND EXPERIMENTAL_IMPORT_FILES
  ${TEST_SCAD_DIR}/json/import-json.scad
  ${TEST_SCAD_DIR}/json/import-json-relative-path.scad
  )
add_cmdline_test(echotest           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_IMPORT_FILES} ARGS --enable=import-function)

#
# --enable=textmetrics tests
#
list(APPEND EXPERIMENTAL_TEXTMETRICS_ECHOTEST_FILES
  ${TEST_SCAD_DIR}/misc/isobject-test.scad
  ${TEST_SCAD_DIR}/misc/text-metrics-test.scad
)
list(APPEND EXPERIMENTAL_TEXTMETRICS_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)
add_cmdline_test(echotest           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_TEXTMETRICS_ECHOTEST_FILES} ARGS --enable=textmetrics)
add_cmdline_test(dumptest           EXPERIMENTAL OPENSCAD SUFFIX csg  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --enable=textmetrics)
add_cmdline_test(previewtest        EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --enable=textmetrics)
add_cmdline_test(throwntogethertest EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --preview=throwntogether --enable=textmetrics)
add_cmdline_test(csgrendertest      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR rendertest ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=textmetrics)
add_cmdline_test(rendertest         EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --render --enable=textmetrics)
add_cmdline_test(dxfrendertest      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=DXF --render=force --enable=textmetrics EXPECTEDDIR rendertest FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})
add_cmdline_test(svgrendertest      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=SVG --render=force --enable=textmetrics EXPECTEDDIR rendertest FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})


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

# Disable tests failing due to https://github.com/openscad/openscad/issues/4632
set_tests_properties(
  relative-output_csg_run
  relative-output_csg_check
  relative-output_ast_run 
  relative-output_ast_check
  PROPERTIES DISABLED TRUE
)

###################################
# Disable Tests with Known Issues #
###################################

set_tests_properties(
  # These don't output anything
  dxfrendertest_text-empty-tests
  dxfrendertest_nothing-decimal-comma-separated
  dxfrendertest_nullspace-2d
  svgrendertest_text-empty-tests
  svgrendertest_nullspace-2d

  # Not useful
  throwntogethertest_internal-cavity
  throwntogethertest_internal-cavity-polyhedron
  throwntogethertest_nullspace-difference

  # z-fighting different on different machines
  throwntogethertest_issue1803
  previewtest_issue1165
  previewtest_issue1215
  throwntogethertest_issue1089
  throwntogethertest_issue1215

  # This test won't render anything meaningful in throwntogether mode
  throwntogethertest_minkowski3-erosion

  # The inf/nan tests fail when exporting CSG and rendering that output again
  # as currently inf/nan is written directly to the CSG file (e.g. r = inf)
  # which is not valid or even misleading in case a variable inf exists.
  # FIXME: define export behavior for inf/nan when exporting CSG files
  # These tests return error code 1.
  # FIXME: We should have a way of running these and verify the return code
  csgrendertest_primitive-inf-tests
  csgrendertest_transform-nan-inf-tests

  # Triggers a floating point accuracy issue causing loaded .csg to
  # render slightly differently
  rendertest_nothing-decimal-comma-separated
  rendertest_import-empty-tests
  rendertest_empty-shape-tests
  csgrendertest_issue1258

  # OpenCSG z fighting https://github.com/openscad/openscad/issues/4595
  openscad-viewoptions-edges_view-options-tests
  openscad-viewoptions-axes-scales-edges_view-options-tests
  previewtest_surface_image
  throwntogethertest_surface_image

  # CGAL sometimes fail to union minkowski convex decompositions, causing visual differences
  # in rendering.
  rendertest_minkowski3-difference-test
  csgrendertest_minkowski3-difference-test
  previewtest_minkowski3-difference-test

  # Transparent objects rendered differently on different machines
  throwntogethertest_offset

  # https://github.com/openscad/openscad/issues/5159
  rendertest_projection
  csgrendertest_projection

  # https://github.com/openscad/openscad/issues/5158
  throwntogethertest_example017
  rendertest_example017 
  previewtest_example017
  
  # https://github.com/openscad/openscad/issues/4398
  csgrendertest_logo_and_text

  # https://github.com/openscad/openscad/issues/909
  amfpreviewtest_polyhedron-single-triangle
  amfrendertest_bad-stl-tardis
  amfrendertest_bad-stl-pcbvicebar
  amfrendertest_bad-stl-wing
  
  # We don't repair polygon winding order in CGAL mode
  # We could perhaps split the test into good+bad polyhedrons
  # and only disable the bad ones.
  stlrendertest_polyhedron-tests
  offrendertest_polyhedron-tests
  amfrendertest_polyhedron-tests
  objrendertest_polyhedron-tests

  # When exported from CGAL, the file cannot be re-imported
  stlrendertest_bad-stl-wing
  offrendertest_bad-stl-wing
  objrendertest_bad-stl-wing

  # https://github.com/openscad/openscad/issues/2301
  rendertest_issue2301
  previewtest_issue2301
  csgrendertest_issue2301

  # https://github.com/openscad/openscad/issues/2090
  rendertest_issue2090
  previewtest_issue2090
  throwntogethertest_issue2090
  csgrendertest_issue2090

  # https://github.com/openscad/openscad/issues/2841
  rendertest_issue2841
  previewtest_issue2841
  csgrendertest_issue2841
  throwntogethertest_issue2841
  rendertest_issue2841b
  previewtest_issue2841b
  throwntogethertest_issue2841b
  csgrendertest_issue2841b
  
  PROPERTIES DISABLED TRUE
)
if (ENABLE_MANIFOLD)
set_tests_properties(
  # z-fighting different on different machines
  previewmanifoldtest_issue1165
  previewmanifoldtest_issue1215

  # Manifold triangle order not stable, causes transparent
  # objects to render differently on different machines
  rendermanifoldtest-different_color-tests
  rendermanifoldtest-different_hex-colors-tests
  rendermanifoldtest-different_projection

  # https://github.com/openscad/openscad/issues/909
  amfrendermanifoldtest_bad-stl-wing

  # https://github.com/openscad/openscad/issues/2841
  rendermanifoldtest_issue2841b
  previewmanifoldtest_issue2841b

  # OpenCSG z fighting https://github.com/openscad/openscad/issues/4595
  previewmanifoldtest_surface_image

  # https://github.com/openscad/openscad/issues/5158
  previewmanifoldtest_example017

  PROPERTIES DISABLED TRUE
)
endif (ENABLE_MANIFOLD)
if (EXPERIMENTAL)
set_tests_properties(
  # https://github.com/openscad/openscad/issues/5220
  rendertest_roof
  csgrendertest_roof

  PROPERTIES DISABLED TRUE
)
endif (EXPERIMENTAL)

if (LIB3MF_FOUND)
  set_tests_properties(
    # We don't repair polygon winding order in CGAL mode
    # We could perhaps split the test into good+bad polyhedrons
    # and only disable the bad ones.
    3mfrendertest_polyhedron-tests

    # When exported from CGAL, the file cannot be re-imported
    3mfrendertest_bad-stl-wing

    # For some reason, only failing on Linux
    3mfrendertest_bad-stl-tardis
    
    PROPERTIES DISABLED TRUE
  )
else()
  set_tests_properties(
    previewtest_import_3mf-tests
    rendertest_import_3mf-tests
    csgrendertest_import_3mf-tests
    throwntogethertest_import_3mf-tests
    PROPERTIES DISABLED TRUE
  )
endif(LIB3MF_FOUND)

list(APPEND NEF3_BROKEN_TESTS
  rendertest_nef3_broken
  previewtest_nef3_broken
  throwntogethertest_nef3_broken
  csgrendertest_nef3_broken
)

# Platform specific test disable
if(APPLE)
  set_tests_properties(
    ${NEF3_BROKEN_TESTS}
    # On macOS, GL_POINTS are sometimes circular even though GL_POINT_SMOOTH is disabled
    openscad-viewoptions-wireframe_view-options-tests
    PROPERTIES DISABLED TRUE
  )
elseif(UNIX)
  set_tests_properties(
    ${NEF3_BROKEN_TESTS}
    PROPERTIES DISABLED TRUE
  )
elseif(WIN32 OR MXECROSS)
  set_tests_properties(
    # Known UTF-8 issue on Windows
    openscad-nonascii_sfære
    # requires `gs` tool
    pdfexporttest_centered
    pdfexporttest_simple-pdf
    PROPERTIES DISABLED TRUE
  )
endif()

##############################################
# Test Installation and Packaging (Win only) #
##############################################

# Package Tests with Windows (.zip archive only)
if(MXECROSS AND EXPERIMENTAL)
  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

  include(CPackComponent)
  cpack_add_component(Tests ARCHIVE_FILE OpenSCAD-Tests-${CPACK_PACKAGE_VERSION})

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION tests
    USE_SOURCE_PERMISSIONS
    COMPONENT Tests
    PATTERN ".git*" EXCLUDE
    PATTERN "OpenSCAD_Test_Console.py" EXCLUDE
    PATTERN "WinReadme.txt" EXCLUDE
    PATTERN "mingw_convert_ctest.py" EXCLUDE
    PATTERN "mingwcon.bat" EXCLUDE
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
    DESTINATION tests-build
    USE_SOURCE_PERMISSIONS
    COMPONENT Tests
    PATTERN "venv" EXCLUDE
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "cmake_install.cmake" EXCLUDE
  )

  # Move files to top project dir for visibility
  install(FILES OpenSCAD_Test_Console.py DESTINATION . COMPONENT Tests)
  install(FILES WinReadme.txt DESTINATION . COMPONENT Tests RENAME "Windows_Test_Readme.txt")
  # Move files to tests-build dir
  install(FILES mingw_convert_ctest.py DESTINATION tests-build COMPONENT Tests)
  install(FILES mingwcon.bat DESTINATION tests-build COMPONENT Tests)

  file(GENERATE OUTPUT mingw_cross_info.py CONTENT
"# created automatically during packaging by cmake from within linux
linux_abs_basedir='${CMAKE_SOURCE_DIR}'
linux_abs_builddir='${CMAKE_BINARY_DIR}'
linux_python='${Python3_EXECUTABLE}'
linux_convert='${IMAGE_COMPARE_EXE}'
win_installdir='OpenSCAD-Tests-${CPACK_PACKAGE_VERSION}'
" NEWLINE_STYLE WIN32)

endif()

####################
# Extra Debug Info #
####################

# Use cmake option "--log-level DEBUG" during top level config to see this
message(DEBUG "Available test configurations: ${TEST_CONFIGS}")
foreach(CONF ${TEST_CONFIGS})
  list(SORT ${CONF}_TEST_CONFIG)
  message(DEBUG "${CONF}: ${${CONF}_TEST_CONFIG}")
endforeach()
