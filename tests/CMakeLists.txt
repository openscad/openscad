# instructions - see ../doc/testing.txt

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 3.0)
if(POLICY CMP0017)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()
cmake_policy(SET CMP0057 NEW) # Interpret IN_LIST
cmake_policy(SET CMP0070 NEW) # Allow relative paths for file(GENERATE ...)

enable_testing()

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
set(CGALSTLSANITYTEST_PY "${CCSD}/cgalstlsanitytest.py")
set(EX_IM_PNGTEST_PY     "${CCSD}/export_import_pngtest.py")
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

find_package(PythonInterp 3.4 REQUIRED)

# Image comparison - expected test image vs actual generated image

if(USE_IMAGE_COMPARE_PY)
  set(VENV_DIR "${CCBD}/venv")
  message(STATUS "Preparing image_compare.py for test suite image comparison")
  if(WIN32)
    set(IMAGE_COMPARE_EXE "${VENV_DIR}/Scripts/python.exe")
  else()
    set(IMAGE_COMPARE_EXE "${VENV_DIR}/bin/python")
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
    find_package(Python3 COMPONENTS Interpreter)
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "-m" "venv" "venv" "--without-pip"
      WORKING_DIRECTORY "${CCBD}")
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
set(OPENSCAD_ARG "--openscad=${OPENSCAD_BINPATH}")

###############################
# Define Macros and Functions #
###############################

# Tags tests as experimental. This will add all the --enable=<feature>
# options for the tagged tests.
macro(experimental_tests)
  foreach (TESTNAME ${ARGN})
#    message("Marking as experimental ${TESTNAME}")
    list(APPEND EXPERIMENTAL_TESTS ${TESTNAME})
  endforeach()
endmacro()

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
# Usage example: set_test_config(Heavy dumptest_testname opencsgtest_testname2)
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
endfunction()

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
#                        [EXPECTEDDIR <shared dir>] SUFFIX <suffix> FILES <test files>)
#
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "OPENSCAD;STDIO" "EXE;SCRIPT;SUFFIX;KERNEL;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

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

  # python script implies PYTHON_EXECUTABLE
  if (TESTCMD_SCRIPT MATCHES \\.[Pp][Yy]$)
    set(TESTCMD_EXE ${PYTHON_EXECUTABLE})
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
    list(FIND EXPERIMENTAL_TESTS ${TEST_FULLNAME} EXPERIMENTAL_TEST)

    set(EXPERIMENTAL_OPTION "")

    if (NOT (${EXPERIMENTAL_TEST} EQUAL -1))
      # add global experimental options here
      set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=roof" "--enable=lazy-union" "--enable=textmetrics" "--enable=import-function")
    endif()

    # Enable manifold for all tests except those marked as skipped
    if (ENABLE_MANIFOLD
        AND NOT SCADFILE IN_LIST SCADFILES_WITH_DIFFERENT_MANIFOLD_EXPECTATIONS
        AND NOT SCADFILE IN_LIST SCADFILES_FAILING_WITH_MANIFOLD
        AND NOT TEST_FULLNAME IN_LIST TESTS_FAILING_WITH_MANIFOLD
        AND NOT TESTCMD_BASENAME MATCHES "^(stlexport|objexport)$"
        AND NOT TESTCMD_BASENAME MATCHES "^openscad-viewoptions-.*"
        AND NOT TESTCMD_BASENAME MATCHES "^fastcsg-.*"
        AND NOT TESTCMD_BASENAME MATCHES "^remesh-.*")
      set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=manifold")
    endif()
    
    # Enable fast-csg for all tests but those that have different expectations or fail for other reasons.
    if (NOT SCADFILE IN_LIST SCADFILES_WITH_DIFFERENT_FAST_CSG_EXPECTATIONS
        AND NOT SCADFILE IN_LIST SCADFILES_FAILING_WITH_FAST_CSG
        AND NOT TEST_FULLNAME IN_LIST TESTS_FAILING_WITH_FAST_CSG)
      set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=fast-csg")
      if (SCADFILE IN_LIST FAST_CSG_SAFER_NEEDED)
        set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=fast-csg-safer")
      endif()
    endif()

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

    # Enable vertex-object-renderers by default for all test if experimental build and openscad executable
    if (EXPERIMENTAL AND TESTCMD_OPENSCAD)
      set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers")
      #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-prealloc")
      #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-direct")
      #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-indexing")
    endif()

    string(JOIN " " DBG_COMMAND_STR
      "add_test(" ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
      COMMAND ${PYTHON_EXECUTABLE}
      ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
      -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
      ${TESTCMD_EXE} ${TESTCMD_SCRIPT} ${SCADFILE} ${CAMERA_OPTION}
      ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS} ")"
    )
    # Use cmake option "--log-level DEBUG" during top level config to see this
    message(DEBUG "${DBG_COMMAND_STR}")

    # only add test if it is not experimental or if it is and experimental option is enabled
    if (${EXPERIMENTAL_TEST} EQUAL -1 OR EXPERIMENTAL)
      add_test(NAME ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
        COMMAND ${PYTHON_EXECUTABLE}
        ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
        -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
        ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION}
        ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS}
      )
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT ${CTEST_ENVIRONMENT})
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
    set(TESTCMD_EXE ${PYTHON_EXECUTABLE})
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
configure_file(${CCSD}/CTestCustom.template ${CCBD}/CTestCustom.cmake @ONLY)

# generate a very large scad file which we would rather not commit to the source tree
# this is for stress-testing the parser
add_custom_target(issue2342 ALL
  COMMAND ${PYTHON_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_issue2342.py ">${TEST_SCAD_DIR}/issues/issue2342.scad"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating issue2342.scad"
)
add_custom_target(svg_viewbox_tests ALL
  COMMAND ${PYTHON_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py "${TEST_DATA_DIR}/svg/viewbox" "${TEST_SCAD_DIR}/svg/extruded"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating svg viewbox tests"
)

##################################
# Define Various Test File Lists #
##################################

# Find all scad files
file(GLOB DEPRECATED_3D_FILES ${TEST_SCAD_DIR}/3D/deprecated/*.scad)
file(GLOB FEATURES_2D_FILES   ${TEST_SCAD_DIR}/2D/features/*.scad)
file(GLOB FEATURES_3D_FILES   ${TEST_SCAD_DIR}/3D/features/*.scad)
file(GLOB ISSUES_2D_FILES     ${TEST_SCAD_DIR}/2D/issues/*.scad)
file(GLOB ISSUES_3D_FILES     ${TEST_SCAD_DIR}/3D/issues/*.scad)
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
  ${TEST_SCAD_DIR}/svg/size-percent.scad)
file(GLOB SCAD_AMF_FILES           ${TEST_SCAD_DIR}/amf/*.scad)
file(GLOB SCAD_NEF3_FILES          ${TEST_SCAD_DIR}/nef3/*.scad)
file(GLOB FUNCTION_FILES           ${TEST_SCAD_DIR}/functions/*.scad)
file(GLOB REDEFINITION_FILES       ${TEST_SCAD_DIR}/redefinition/*.scad)
file(GLOB_RECURSE BUGS_FILES       ${TEST_SCAD_DIR}/bugs/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES    ${TEST_SCAD_DIR}/bugs2D/*.scad)
file(GLOB_RECURSE EXAMPLE_3D_FILES ${EXAMPLES_DIR}/*.scad)

list(REMOVE_ITEM EXAMPLE_3D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND EXAMPLE_2D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND EXAMPLE_FILES ${EXAMPLE_3D_FILES} ${EXAMPLE_2D_FILES})

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
  ${TEST_SCAD_DIR}/misc/isobject-test.scad
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
  ${TEST_SCAD_DIR}/misc/text-metrics-test.scad
  ${TEST_SCAD_DIR}/json/import-json.scad
  ${TEST_SCAD_DIR}/json/import-json-relative-path.scad
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

list(APPEND CGALPNGTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES})
list(APPEND CGALPNGTEST_3D_FILES
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

list(GET CGALPNGTEST_2D_FILES 0 1 2 CGALPNGSTDIOTEST_FILES)
list(APPEND CGALPNGTEST_FILES ${CGALPNGTEST_2D_FILES} ${CGALPNGTEST_3D_FILES})
set(PRUNE_TEST ${TEST_SCAD_DIR}/misc/intersection-prune-test.scad)
list(APPEND OPENCSGTEST_FILES ${STL_IMPORT_FILES} ${CGALPNGTEST_FILES} ${BUGS_FILES} ${BUGS_2D_FILES} ${PRUNE_TEST})
list(APPEND THROWNTOGETHERTEST_FILES ${CGALPNGTEST_FILES} ${PRUNE_TEST})

list(APPEND CGALSTLSANITYTEST_FILES ${TEST_SCAD_DIR}/misc/normal-nan.scad)

list(APPEND EXPORT_STL_TEST_FILES ${TEST_SCAD_DIR}/stl/stl-export.scad)

list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-export.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export.scad)

list(APPEND EXPORT_3MF_TEST_FILES ${TEST_SCAD_DIR}/3mf/3mf-export.scad)

list(APPEND EXPORT3D_CGALCGAL_TEST_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-nonplanar-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-tests.scad
  ${TEST_SCAD_DIR}/3D/features/union-coincident-test.scad
  ${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
  ${TEST_SCAD_DIR}/misc/null-polygons.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1221.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1225.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
  ${TEST_SCAD_DIR}/3D/issues/fn_bug.scad
)

list(APPEND EXPORT3D_CGAL_TEST_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1258.scad
  ${TEST_SCAD_DIR}/3D/issues/issue2259.scad
)

list(APPEND EXPORT3D_TEST_FILES
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
)

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES ${FILES_2D} ${SCAD_DXF_FILES} ${SCAD_SVG_FILES})

# We know that we cannot import weakly manifold files into CGAL, so to make tests easier
# to manage, don't try. Once we improve import, we can reenable this
# Known good manifold files -> EXPORT3D_CGALCGAL_TEST_FILES
# Known weak manifold files -> EXPORT3D_CGAL_TEST_FILES
# Known non-manifold files -> EXPORT3D_TEST_FILES
list(APPEND EXPORT3D_CGALCGAL_TEST_FILES ${BUGS_FILES})
#list(REMOVE_ITEM EXPORT3D_CGALCGAL_TEST_FILES
#)
#list(APPEND EXPORT3D_CGAL_TEST_FILES
#)

list(APPEND CGALPNGTEST_FILES ${BUGS_FILES} ${BUGS_2D_FILES})

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

list(APPEND FASTCSG_LAZYUNION_FILES ${LAZYUNION_3D_FILES}
  ${TEST_SCAD_DIR}/experimental/fastcsg-lazyunion-issue4109-1.scad
  ${TEST_SCAD_DIR}/experimental/fastcsg-lazyunion-issue4109-2.scad
  ${TEST_SCAD_DIR}/experimental/fastcsg-lazyunion-issue4109-3.scad
  ${TEST_SCAD_DIR}/experimental/fastcsg-lazyunion-issue4109-4.scad
)

list(APPEND SVG_VIEWBOX_TESTS
  viewbox_300x400_none viewbox_600x200_none
  viewbox_300x400_meet_xMinYMin viewbox_300x400_meet_xMidYMin viewbox_300x400_meet_xMaxYMin
  viewbox_600x200_meet_xMinYMin viewbox_600x200_meet_xMinYMid viewbox_600x200_meet_xMinYMax
  viewbox_600x200_slice_xMinYMin viewbox_600x200_slice_xMidYMin viewbox_600x200_slice_xMaxYMin
  viewbox_600x600_slice_xMinYMin viewbox_600x600_slice_xMinYMid viewbox_600x600_slice_xMinYMax
)

# Specific tests for which fast-csg cannot be enabled yet.
list(APPEND TESTS_FAILING_WITH_FAST_CSG
  # Different order of vertices
  3mfexport_3mf-export
  # Test script argument passing issue
  cgalstlsanitytest_normal-nan
)

# Tests for which fast-csg works but produces different expectation files.
# These tests are run twice (w/ and without fast-csg).
list(APPEND SCADFILES_FAILING_WITH_FAST_CSG
  # Corefinement leaves some polyhedra in a state that we can't build Nefs from
  ${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
  # Resize issue!
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad
  # Needs investigation:
  ${TEST_SCAD_DIR}/3D/issues/issue1246.scad
)
list(APPEND FAST_CSG_SAFER_NEEDED
  ${TEST_SCAD_DIR}/3D/features/edge-cases.scad
)

list(APPEND SCADFILES_WITH_GREEN_FACES
  ${EXAMPLES_DIR}/Basics/CSG-modules.scad
  ${EXAMPLES_DIR}/Basics/CSG.scad
  ${EXAMPLES_DIR}/Basics/logo.scad
  ${EXAMPLES_DIR}/Old/example001.scad
  ${EXAMPLES_DIR}/Old/example002.scad
  ${EXAMPLES_DIR}/Old/example003.scad
  ${EXAMPLES_DIR}/Old/example004.scad
  ${EXAMPLES_DIR}/Old/example005.scad
  ${EXAMPLES_DIR}/Old/example006.scad
  ${EXAMPLES_DIR}/Old/example012.scad
  ${EXAMPLES_DIR}/Old/example016.scad
  ${EXAMPLES_DIR}/Old/example024.scad
  ${TEST_SCAD_DIR}/3D/features/difference-tests.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
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
  ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
)

# Tests for which fast-csg works but produces different expectation files.
# These tests are run twice (w/ and without fast-csg).
list(APPEND SCADFILES_WITH_DIFFERENT_FAST_CSG_EXPECTATIONS
  # No more green faces from differences.
  ${SCADFILES_WITH_GREEN_FACES}

  # This one no longer crashes, yay!
  ${TEST_SCAD_DIR}/bugs/issue1455.scad
  # These are less broken than before:
  ${TEST_SCAD_DIR}/bugs/issue791.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1138.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # Less broken and no green faces
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad

  # Misc
  ${TEST_SCAD_DIR}/2D/features/highlight-modifier-2d.scad
)

list(APPEND SCADFILES_WITH_DIFFERENT_MANIFOLD_EXPECTATIONS
  # No more green faces from differences.
  ${SCADFILES_WITH_GREEN_FACES}

  # Different vertex order
  ${TEST_SCAD_DIR}/3mf/3mf-export.scad
)

list(APPEND SCADFILES_FAILING_WITH_MANIFOLD
  # Crashes with Input mesh not closed! / NoManifold
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # Resize issues!
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad

  # Misc
  ${TEST_SCAD_DIR}/misc/normal-nan.scad
)

list(APPEND TESTS_FAILING_WITH_MANIFOLD
  # Need to flip faces if transform determinant is < 0
  cgalstlcgalpngtest_mirror-tests
  cgalbinstlcgalpngtest_mirror-tests
)

###################################
# Add experimental features tests #
###################################

experimental_tests(dumptest-examples_roof)
experimental_tests(cgalpngtest_roof)
experimental_tests(csgpngtest_roof)
experimental_tests(opencsgtest_roof)
experimental_tests(throwntogethertest_roof)

experimental_tests(echotest_allexpressions)
experimental_tests(astdumptest_allexpressions)
experimental_tests(echotest_function-literal-tests)
experimental_tests(echotest_function-literal-compare)
experimental_tests(echotest_isobject-test)
experimental_tests(echotest_text-metrics-test)
experimental_tests(dumptest_text-metrics)
experimental_tests(cgalpngtest_text-metrics)
experimental_tests(opencsgtest_text-metrics)
experimental_tests(csgpngtest_text-metrics)
experimental_tests(throwntogethertest_text-metrics)
experimental_tests(dxfpngtest_text-metrics)
experimental_tests(svgpngtest_text-metrics)
experimental_tests(echotest_import-json)
experimental_tests(echotest_import-json-relative-path)

experimental_tests(lazyunion-dump_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-dump_lazyunion-toplevel-objects)
experimental_tests(lazyunion-dump_lazyunion-toplevel-for)
experimental_tests(lazyunion-dump_lazyunion-nested-for)
experimental_tests(lazyunion-dump_lazyunion-children)
experimental_tests(lazyunion-dump_lazyunion-hull-for)
experimental_tests(lazyunion-dump_lazyunion-root-for)
experimental_tests(lazyunion-dump_lazyunion-intersection-for)
experimental_tests(lazyunion-dump_lazyunion-difference-for)
experimental_tests(lazyunion-dump_lazyunion-minkowski-for)
experimental_tests(lazyunion-dump_lazyunion-transform-for)
experimental_tests(lazyunion-dump_2d-3d)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-objects)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-for)
experimental_tests(lazyunion-opencsg_lazyunion-nested-for)
experimental_tests(lazyunion-opencsg_lazyunion-children)
experimental_tests(lazyunion-opencsg_lazyunion-hull-for)
experimental_tests(lazyunion-opencsg_lazyunion-root-for)
experimental_tests(lazyunion-opencsg_lazyunion-intersection-for)
experimental_tests(lazyunion-opencsg_lazyunion-difference-for)
experimental_tests(lazyunion-opencsg_lazyunion-minkowski-for)
experimental_tests(lazyunion-opencsg_lazyunion-transform-for)
experimental_tests(lazyunion-opencsg_2d-3d)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-objects)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-for)
experimental_tests(lazyunion-cgalpng_lazyunion-nested-for)
experimental_tests(lazyunion-cgalpng_lazyunion-children)
experimental_tests(lazyunion-cgalpng_lazyunion-hull-for)
experimental_tests(lazyunion-cgalpng_lazyunion-root-for)
experimental_tests(lazyunion-cgalpng_lazyunion-intersection-for)
experimental_tests(lazyunion-cgalpng_lazyunion-difference-for)
experimental_tests(lazyunion-cgalpng_lazyunion-minkowski-for)
experimental_tests(lazyunion-cgalpng_lazyunion-transform-for)
experimental_tests(lazyunion-cgalpng_2d-3d)
experimental_tests(lazyunion-monotonepng_lazyunion-toplevel-objects)
experimental_tests(lazyunion-monotonepng_lazyunion-toplevel-for)
experimental_tests(lazyunion-monotonepng_lazyunion-nested-for)
experimental_tests(lazyunion-monotonepng_lazyunion-children)
experimental_tests(lazyunion-monotonepng_lazyunion-hull-for)
experimental_tests(lazyunion-monotonepng_lazyunion-root-for)
experimental_tests(lazyunion-monotonepng_lazyunion-intersection-for)
experimental_tests(lazyunion-monotonepng_lazyunion-difference-for)
experimental_tests(lazyunion-monotonepng_lazyunion-minkowski-for)
experimental_tests(lazyunion-monotonepng_lazyunion-transform-for)
experimental_tests(lazyunion-monotonepng_2d-3d)
experimental_tests(lazyunion-stlpngtest_lazyunion-toplevel-objects)
experimental_tests(lazyunion-stlpngtest_lazyunion-toplevel-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-nested-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-children)
experimental_tests(lazyunion-stlpngtest_lazyunion-hull-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-root-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-intersection-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-difference-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-minkowski-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-transform-for)
experimental_tests(lazyunion-stlpngtest_2d-3d)
experimental_tests(lazyunion-offpngtest_lazyunion-toplevel-objects)
experimental_tests(lazyunion-offpngtest_lazyunion-toplevel-for)
experimental_tests(lazyunion-offpngtest_lazyunion-nested-for)
experimental_tests(lazyunion-offpngtest_lazyunion-children)
experimental_tests(lazyunion-offpngtest_lazyunion-hull-for)
experimental_tests(lazyunion-offpngtest_lazyunion-root-for)
experimental_tests(lazyunion-offpngtest_lazyunion-intersection-for)
experimental_tests(lazyunion-offpngtest_lazyunion-difference-for)
experimental_tests(lazyunion-offpngtest_lazyunion-minkowski-for)
experimental_tests(lazyunion-offpngtest_lazyunion-transform-for)
experimental_tests(lazyunion-offpngtest_2d-3d)
experimental_tests(lazyunion-dxfpngtest_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-svgpngtest_lazyunion-toplevel-2dobjects)

##############################
# Define test configurations #
##############################
# Must be done BEFORE adding any tests

# Clear test config cache variables
foreach(CONFIG $CACHE{TEST_CONFIGS})
  unset(${CONFIG}_TEST_CONFIG CACHE)
endforeach()

# Heavy tests are tests taking more than 10 seconds on a development computer
set_test_config(Heavy FILES
  cgalbinstlcgalpngtest_rotate_extrude-tests
  cgalpngtest_camera-tests
  cgalpngtest_for-nested-tests
  cgalpngtest_fractal
  cgalpngtest_issue267-normalization-crash
  cgalpngtest_iteration
  cgalpngtest_linear_extrude-scale-zero-tests
  cgalpngtest_minkowski3-erosion
  cgalpngtest_projection-extrude-tests
  cgalpngtest_resize-tests
  cgalpngtest_rotate_extrude-angle
  cgalpngtest_rotate_extrude-tests
  cgalpngtest_sphere-tests
  cgalpngtest_surface-tests
  cgalstlcgalpngtest_rotate_extrude-tests
  csgpngtest_camera-tests
  csgpngtest_for-nested-tests
  csgpngtest_fractal
  csgpngtest_issue267-normalization-crash
  csgpngtest_iteration
  csgpngtest_linear_extrude-scale-zero-tests
  csgpngtest_minkowski3-erosion
  csgpngtest_resize-tests
  csgpngtest_rotate_extrude-angle
  csgpngtest_rotate_extrude-tests
  csgpngtest_sphere-tests
  csgpngtest_surface-tests
  monotonepngtest_rotate_extrude-tests
  offpngtest_demo_cut
  offpngtest_difference
  offpngtest_fence
  offpngtest_rounded_box
  offpngtest_search
  offpngtest_surface
  offpngtest_translation
  opencsgtest_issue267-normalization-crash
  opencsgtest_minkowski3-erosion
  openscad-colorscheme-metallic-render_CSG
  stlpngtest_demo_cut
  stlpngtest_difference
  stlpngtest_fence
  stlpngtest_search
  stlpngtest_surface
  stlpngtest_rounded_box
  stlpngtest_translation
  fastcsg-cgalpng_minkowski3-erosion
)

# Bugs
set_test_config(Bugs FILES ${BUGS_FILES} ${BUGS_2D_FILES} PREFIXES opencsgtest cgalpngtest csgpngtest)
set_test_config(Bugs FILES ${BUGS_FILES} PREFIXES offpngtest monotonepngtest stlpngtest stlcgalpngtest cgalstlcgalpngtest cgalbinstlcgalpngtest offcgalpngtest)
set_test_config(Bugs FILES
  offcgalpngtest_polyhedron-tests
  offpngtest_nonmanifold-polyhedron
  offpngtest_bad-stl-wing
  cgalpngtest_escape-test.scad
)

# Examples
set_test_config(Examples FILES ${EXAMPLE_FILES} PREFIXES cgalpngtest opencsgtest throwntogethertest csgpngtest monotonepngtest stlpngtest stlcgalpngtest cgalstlcgalpngtest cgalbinstlcgalpngtest offpngtest offcgalpngtest)
set_test_config(Examples FILES ${EXAMPLE_2D_FILES} PREFIXES dxfpngtest)

#############
# Add tests #
#############

# Types of tests:
# o echotest: Just record console output
# o dumptest: Export .csg
# o cgalpngtest: Export to PNG using --render
# o opencsgtest: Export to PNG using OpenCSG
# o throwntogethertest: Export to PNG using the Throwntogether renderer
# o csgpngtest: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o monotonepngtest: Same as cgalpngtest but with the "Monotone" color scheme
# o stlpngtest: Export to STL, Re-import and render to PNG (--render)
# o stlcgalpngtest: Export to STL, Re-import and render to PNG (--render=cgal)
# o offpngtest: Export to OFF, Re-import and render to PNG (--render)
# o offcgalpngtest: Export to STL, Re-import and render to PNG (--render=cgal)
# o dxfpngtest: Export to DXF, Re-import and render to PNG (--render=cgal)
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
add_cmdline_test(cgalpngtest        OPENSCAD FILES ${CGALPNGTEST_FILES} SUFFIX png ARGS --render)
add_cmdline_test(cgalpngstdiotest   OPENSCAD FILES ${CGALPNGSTDIOTEST_FILES} SUFFIX png STDIO EXPECTEDDIR cgalpngtest ARGS --export-format png --render)
add_cmdline_test(opencsgtest        OPENSCAD FILES ${OPENCSGTEST_FILES} SUFFIX png ARGS)
add_cmdline_test(throwntogethertest OPENSCAD FILES ${THROWNTOGETHERTEST_FILES} ARGS --preview=throwntogether SUFFIX png)
add_cmdline_test(csgpngtest         SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${CGALPNGTEST_FILES} EXPECTEDDIR cgalpngtest ARGS ${OPENSCAD_ARG} --format=csg --render)
# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest  SCRIPT ${CGALSTLSANITYTEST_PY} SUFFIX txt FILES ${CGALSTLSANITYTEST_FILES} ARGS ${OPENSCAD_BINPATH})

set(VIEWBOX_TEST "${TEST_SCAD_DIR}/svg/extruded/viewbox-test.scad")
foreach(TEST ${SVG_VIEWBOX_TESTS})
  add_cmdline_test(svgviewbox-${TEST} OPENSCAD ARGS --imgsize 600,600 "-Dfile=\"${TEST_DATA_DIR}/svg/viewbox/${TEST}.svg\";" SUFFIX png FILES ${VIEWBOX_TEST})
endforeach()

add_cmdline_test(svgimport OPENSCAD ARGS --imgsize 600,600 SUFFIX png FILES
  ${TEST_SCAD_DIR}/svg/extruded/box-w-holes.scad
  ${TEST_SCAD_DIR}/svg/extruded/simple-center.scad
)

add_cmdline_test(lazyunion-dump        OPENSCAD SUFFIX csg FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-opencsg     OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-cgalpng     OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(lazyunion-monotonepng OPENSCAD SUFFIX png FILES ${LAZYUNION_3D_FILES} ARGS --colorscheme=Monotone --enable=lazy-union --render )
add_cmdline_test(lazyunion-stlpngtest  SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS ${OPENSCAD_ARG} --format=STL --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-offpngtest  SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS ${OPENSCAD_ARG} --format=OFF --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-dxfpngtest  SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-cgalpng     ARGS ${OPENSCAD_ARG} --format=DXF --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-svgpngtest  SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-cgalpng     ARGS ${OPENSCAD_ARG} --format=SVG --enable=lazy-union --render=cgal)

add_cmdline_test(manifold-cgalpng             OPENSCAD SUFFIX png FILES ${SCADFILES_WITH_DIFFERENT_MANIFOLD_EXPECTATIONS} ARGS --enable=manifold --render)
add_cmdline_test(fastcsg-cgalpng              OPENSCAD SUFFIX png FILES ${SCADFILES_WITH_DIFFERENT_FAST_CSG_EXPECTATIONS} ARGS --enable=fast-csg --render)
add_cmdline_test(fastcsg-lazyunion-cgalpng    OPENSCAD SUFFIX png FILES ${FASTCSG_LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(fastcsg-lazyunion-amfpngtest SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${FASTCSG_LAZYUNION_FILES} EXPECTEDDIR fastcsg-lazyunion-monotonepngtest ARGS --enable=lazy-union ${OPENSCAD_ARG} --format=AMF)
add_cmdline_test(fastcsg-lazyunion-3mfpngtest SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${FASTCSG_LAZYUNION_FILES} EXPECTEDDIR fastcsg-lazyunion-monotonepngtest ARGS --enable=lazy-union ${OPENSCAD_ARG} --format=3MF)

# Disabled due to https://github.com/openscad/openscad/issues/4470
set_tests_properties(
  fastcsg-cgalpng_highlight-modifier2
  PROPERTIES DISABLED TRUE
)

list(APPEND FASTCSG_REMESH_FILES
  ${TEST_SCAD_DIR}/experimental/fastcsg-remesh-cubes.scad
  ${TEST_SCAD_DIR}/experimental/fastcsg-remesh-cube-1.scad
  ${TEST_SCAD_DIR}/experimental/fastcsg-remesh-cube-2.scad
)

add_cmdline_test(remesh-cgalpng OPENSCAD SUFFIX png FILES ${FASTCSG_REMESH_FILES} ARGS --enable=fast-csg --render)
add_cmdline_test(remesh-stl     OPENSCAD SUFFIX stl FILES ${FASTCSG_REMESH_FILES} ARGS --enable=predictible-output --enable=fast-csg --render)

# Trivial Export/Import files
# This sanity-checks bidirectional file format import/export
set(EXP_IMP_2D_TEST ${TEST_SCAD_DIR}/misc/square10.scad)
set(EXP_IMP_3D_TEST ${TEST_SCAD_DIR}/misc/cube10.scad)
add_cmdline_test(monotonepngtest OPENSCAD SUFFIX png FILES ${EXP_IMP_2D_TEST} ${EXP_IMP_3D_TEST} ARGS --colorscheme=Monotone --render)
add_cmdline_test(stlpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_3D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=STL)
add_cmdline_test(offpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_3D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=OFF)
add_cmdline_test(amfpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_3D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=AMF)
add_cmdline_test(3mfpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_3D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=3MF)
add_cmdline_test(dxfpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_2D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=DXF --render=cgal)
add_cmdline_test(svgpngtest    SCRIPT ${EX_IM_PNGTEST_PY} SUFFIX png FILES ${EXP_IMP_2D_TEST} EXPECTEDDIR monotonepngtest ARGS ${OPENSCAD_ARG} --format=SVG --render=cgal)
add_cmdline_test(pdfexporttest SCRIPT ${EXPORT_PNGTEST_PY} SUFFIX png FILES ${SCAD_PDF_FILES} EXPECTEDDIR pdfexporttest ARGS ${OPENSCAD_ARG} --format=PDF KERNEL Square:2)

# Corner-case Export/Import tests
add_cmdline_test(monotonepngtest OPENSCAD SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES} ${EXPORT3D_CGALCGAL_TEST_FILES} ARGS --colorscheme=Monotone --render)
add_cmdline_test(stlexport             OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render)
add_cmdline_test(manifold-stlexport    OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --enable=manifold --render)
add_cmdline_test(objexport             OPENSCAD SUFFIX obj FILES ${EXPORT_OBJ_TEST_FILES} ARGS --render)
add_cmdline_test(3mfexport             OPENSCAD ARGS SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES})

# stlpngtest: direct STL output, preview rendering
add_cmdline_test(stlpngtest            SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=STL EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
# cgalstlpngtest: CGAL STL output, normal rendering
add_cmdline_test(stlcgalpngtest        SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=STL --require-manifold --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
# cgalstlcgalpngtest: CGAL STL output, CGAL rendering
add_cmdline_test(cgalstlcgalpngtest    SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=ASCIISTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})

# cgalbinstlcgalpngtest: CGAL binary STL output, CGAL rendering
add_cmdline_test(cgalbinstlcgalpngtest SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=BINSTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})
add_cmdline_test(offpngtest            SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=OFF --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(offcgalpngtest        SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=OFF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
add_cmdline_test(dxfpngtest            SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=DXF --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_DXF_FILES})
add_cmdline_test(svgpngtest            SCRIPT ${EX_IM_PNGTEST_PY} ARGS ${OPENSCAD_ARG} --format=SVG --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_SVG_FILES})

# Failing tests
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

# Customizer tests
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

# non-ASCII filenames
add_cmdline_test(openscad-nonascii             OPENSCAD FILES ${TEST_SCAD_DIR}/misc/sfære.scad SUFFIX csg)

# Variable override (-D arg)
add_cmdline_test(openscad-override         OPENSCAD FILES ${TEST_SCAD_DIR}/misc/override.scad SUFFIX echo ARGS -D a=3$<SEMICOLON>)

# Camera tests
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

# View Options tests
set(VIEW_OPTIONS_TEST "${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad")
add_cmdline_test(openscad-viewoptions-axes              OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes)
add_cmdline_test(openscad-viewoptions-axes-scales       OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales)
add_cmdline_test(openscad-viewoptions-edges             OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges)
add_cmdline_test(openscad-viewoptions-axes-scales-edges OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales,edges)
add_cmdline_test(openscad-viewoptions-wireframe         OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view wireframe)
add_cmdline_test(openscad-viewoptions-crosshairs        OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view crosshairs)

# Colorscheme tests
set(LOGO_EXAMPLE ${EXAMPLES_DIR}/Basics/logo.scad)
set(CSG_EXAMPLE  ${EXAMPLES_DIR}/Basics/CSG.scad)
add_cmdline_test(openscad-colorscheme-cornfield       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Cornfield)
add_cmdline_test(openscad-colorscheme-metallic        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Metallic)
add_cmdline_test(openscad-colorscheme-sunset          OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Sunset)
add_cmdline_test(openscad-colorscheme-starnight       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Starnight)
add_cmdline_test(openscad-colorscheme-monotone        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Monotone)
add_cmdline_test(openscad-colorscheme-clearsky        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=ClearSky)
add_cmdline_test(openscad-colorscheme-metallic-render OPENSCAD FILES ${CSG_EXAMPLE}  SUFFIX png ARGS --colorscheme=Metallic --render)

###################################
# Disable Tests with Known Issues #
###################################

set_tests_properties(
  # These don't output anything
  dxfpngtest_text-empty-tests
  dxfpngtest_nothing-decimal-comma-separated
  dxfpngtest_nullspace-2d
  svgpngtest_text-empty-tests
  svgpngtest_nullspace-2d

  # Not useful
  throwntogethertest_internal-cavity
  throwntogethertest_internal-cavity-polyhedron
  throwntogethertest_nullspace-difference

  # z-fighting different on different machines
  throwntogethertest_issue1803
  opencsgtest_issue1165
  opencsgtest_issue1215
  throwntogethertest_issue1089
  throwntogethertest_issue1215

  # FIXME: This test illustrates a weakness in child() combined with modifiers.
  # Reenable it when this is improved
  opencsgtest_child-background

  # These tests only makes sense in OpenCSG mode
  cgalpngtest_child-background
  cgalpngtest_highlight-and-background-modifier
  cgalpngtest_highlight-modifier2
  cgalpngtest_background-modifier2
  cgalpngtest_testcolornames
  csgpngtest_child-background
  csgpngtest_highlight-and-background-modifier
  csgpngtest_highlight-modifier2
  csgpngtest_background-modifier2
  csgpngtest_testcolornames
  throwntogethertest_testcolornames

  # This test won't render anything meaningful in throwntogether mode
  throwntogethertest_minkowski3-erosion

  # The inf/nan tests fail when exporting CSG and rendering that output again
  # as currently inf/nan is written directly to the CSG file (e.g. r = inf)
  # which is not valid or even misleading in case a variable inf exists.
  # FIXME: define export behavior for inf/nan when exporting CSG files
  # These tests return error code 1.
  # FIXME: We should have a way of running these and verify the return code
  csgpngtest_primitive-inf-tests
  csgpngtest_transform-nan-inf-tests

  # Triggers a floating point accuracy issue causing loaded .csg to
  # render slightly differently
  cgalpngtest_nothing-decimal-comma-separated
  cgalpngtest_import-empty-tests
  cgalpngtest_empty-shape-tests
  csgpngtest_issue1258
  PROPERTIES DISABLED TRUE
)

if (NOT LIB3MF_FOUND)
  set_tests_properties(
    opencsgtest_import_3mf-tests
    cgalpngtest_import_3mf-tests
    csgpngtest_import_3mf-tests
    throwntogethertest_import_3mf-tests
    3mfpngtest_cube10
    3mfexport_3mf-export
    fastcsg-lazyunion-3mfpngtest_lazyunion-toplevel-objects
    fastcsg-lazyunion-3mfpngtest_lazyunion-toplevel-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-nested-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-children
    fastcsg-lazyunion-3mfpngtest_lazyunion-hull-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-root-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-intersection-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-difference-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-minkowski-for
    fastcsg-lazyunion-3mfpngtest_lazyunion-transform-for
    fastcsg-lazyunion-3mfpngtest_2d-3d
    fastcsg-lazyunion-3mfpngtest_fastcsg-lazyunion-issue4109-1
    fastcsg-lazyunion-3mfpngtest_fastcsg-lazyunion-issue4109-2
    fastcsg-lazyunion-3mfpngtest_fastcsg-lazyunion-issue4109-3
    fastcsg-lazyunion-3mfpngtest_fastcsg-lazyunion-issue4109-4
    PROPERTIES DISABLED TRUE
  )
endif()

list(APPEND NEF3_BROKEN_TESTS
  cgalpngtest_nef3_broken
  opencsgtest_nef3_broken
  throwntogethertest_nef3_broken
  csgpngtest_nef3_broken
)

# Platform specific test disable
if(APPLE)
  set_tests_properties(
    ${NEF3_BROKEN_TESTS}
    # Z issues specific to edge shader: https://github.com/openscad/openscad/issues/4595
    openscad-viewoptions-edges_view-options-tests
    openscad-viewoptions-axes-scales-edges_view-options-tests
    # issue3158 test failing on macos, see: https://github.com/openscad/openscad/issues/4509
    opencsgtest_issue3158
    PROPERTIES DISABLED TRUE
  )
elseif(UNIX)
  set_tests_properties(
    ${NEF3_BROKEN_TESTS}
    PROPERTIES DISABLED TRUE
  )
elseif(WIN32 OR MXECROSS)
  set_tests_properties(
    # Z issues specific to edge shader: https://github.com/openscad/openscad/issues/4595
    openscad-viewoptions-edges_view-options-tests
    openscad-viewoptions-axes-scales-edges_view-options-tests
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
linux_python='${PYTHON_EXECUTABLE}'
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
