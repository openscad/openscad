# Image comparison - expected test image vs actual generated image
# FUTURE: Python 3.15:  -Xutf8=1 will become unnecessary
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
  set(IMAGE_COMPARE_OPTS "-Xutf8=1")
  if(EXISTS "${IMAGE_COMPARE_EXE}")
    message(STATUS "venv found, testing libraries")
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" ${IMAGE_COMPARE_OPTS} "${CCSD}/image_compare.py" "--status"
      WORKING_DIRECTORY "${CCSD}" ERROR_QUIET RESULT_VARIABLE ret)
    if(ret AND NOT ret EQUAL 0)
      message(STATUS "venv libraries incomplete")
      set(BUILD_VENV TRUE)
    else()
      message(STATUS "venv libraries complete")
      set(BUILD_VENV FALSE)
    endif()
  else()
    set(BUILD_VENV TRUE)
  endif()
  if(BUILD_VENV)
    message(STATUS "Setting up testing venv for image comparison")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "-m" "venv" "venv" "--system-site-packages" "--without-pip"
      WORKING_DIRECTORY "${CCBD}"
      OUTPUT_QUIET
      ERROR_QUIET)
    # Since msys2 on Windows prefers bin/ over Scripts, we need to look for the actual folder to determine
    # how to utilize the venv
    find_path(VENV_BIN_PATH activate PATHS "${VENV_DIR}/bin" "${VENV_DIR}/Scripts" NO_DEFAULT_PATH NO_CACHE)
    if(WIN32)
      set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python.exe")
    else()
      set(IMAGE_COMPARE_EXE "${VENV_BIN_PATH}/python")
    endif()
    set(IMAGE_COMPARE_OPTS "-Xutf8=1")
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" ${IMAGE_COMPARE_OPTS} "-m" "ensurepip"
      WORKING_DIRECTORY "${CCBD}"
      OUTPUT_QUIET
      ERROR_QUIET)
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" ${IMAGE_COMPARE_OPTS} "-m" "pip" "install" "numpy" "Pillow"
      WORKING_DIRECTORY "${CCBD}"
      OUTPUT_QUIET
      ERROR_QUIET)
  endif()
  set(COMPARATOR "--comparator=image_compare")
  if(BUILD_VENV)
    execute_process(
      COMMAND "${IMAGE_COMPARE_EXE}" ${IMAGE_COMPARE_OPTS} "${CCSD}/image_compare.py" "--status"
      WORKING_DIRECTORY "${CCSD}" RESULT_VARIABLE ret)
    if(ret AND NOT ret EQUAL 0)
      message(WARNING "Failed to setup the test suite venv for ${IMAGE_COMPARE_EXE}  See doc/testing.md for dependency information.")
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
