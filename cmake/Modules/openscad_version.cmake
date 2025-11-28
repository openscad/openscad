# Get OPENSCAD_VERSION
if ("${OPENSCAD_VERSION}" STREQUAL "")
  if (EXISTS "${CMAKE_SOURCE_DIR}/VERSION.txt")
    file(READ "${CMAKE_SOURCE_DIR}/VERSION.txt" OPENSCAD_VERSION_FILE)
    string(STRIP "${OPENSCAD_VERSION_FILE}" OPENSCAD_VERSION)
    if (NOT "${OPENSCAD_VERSION}" STREQUAL "")
      message(STATUS "Found VERSION.txt file, using VERSION=${OPENSCAD_VERSION}")
    endif()
  endif()
endif()

# Default to today's date
if ("${OPENSCAD_VERSION}" STREQUAL "")
  string(TIMESTAMP OPENSCAD_VERSION "%Y.%m.%d")
  message(STATUS "No VERSION.txt file found, defaulting VERSION=${OPENSCAD_VERSION}")
endif()

# Get OPENSCAD_COMMIT
if ("${OPENSCAD_COMMIT}" STREQUAL "")
  if (EXISTS "${CMAKE_SOURCE_DIR}/COMMIT.txt")
    file(READ "${CMAKE_SOURCE_DIR}/COMMIT.txt" OPENSCAD_COMMIT_FILE)
    string(STRIP "${OPENSCAD_COMMIT_FILE}" OPENSCAD_COMMIT)
    if (NOT "${OPENSCAD_COMMIT}" STREQUAL "")
      message(STATUS "Found COMMIT.txt file, using COMMIT=${OPENSCAD_COMMIT}")
    endif()
  else()
    find_package(Git QUIET)
    if (GIT_FOUND)
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}" rev-parse --is-inside-work-tree
        RESULT_VARIABLE GIT_INSIDE_WORK_TREE
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if ("${GIT_INSIDE_WORK_TREE}" EQUAL 0)
        execute_process(
          COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}" log -1 --pretty=format:%h
          OUTPUT_VARIABLE OPENSCAD_COMMIT
          OUTPUT_STRIP_TRAILING_WHITESPACE
          ERROR_QUIET
        )
        if (NOT "${OPENSCAD_COMMIT}" STREQUAL "")
           message(STATUS "No COMMIT.txt file found, using git commit: ${OPENSCAD_COMMIT}")
        endif()
      endif()
    endif()
  endif()
endif()

# Split OPENSCAD_VERSION into components
# YYYY.MM[.DD][-PPP][.build]
string(REGEX MATCH "^(([0-9]+)\\.([0-9]+)(\\.([0-9]+))?)(-([^\\.]+))?(\\.(.*))?$" MATCH "${OPENSCAD_VERSION}")

set(OPENSCAD_SHORTVERSION ${CMAKE_MATCH_1})
set(OPENSCAD_YEAR ${CMAKE_MATCH_2})
set(OPENSCAD_MONTH ${CMAKE_MATCH_3})
math(EXPR OPENSCAD_MONTH ${OPENSCAD_MONTH}) # get rid of leading zero
set(OPENSCAD_DAY ${CMAKE_MATCH_5})
if (DEFINED OPENSCAD_DAY)
  math(EXPR OPENSCAD_DAY ${OPENSCAD_DAY}) # get rid of leading zero
endif()
set(OPENSCAD_PATCH ${CMAKE_MATCH_7})
set(OPENSCAD_BUILD ${CMAKE_MATCH_9})

set(PROJECT_VERSION ${OPENSCAD_SHORTVERSION})

 message(STATUS "OPENSCAD_SHORTVERSION: ${OPENSCAD_SHORTVERSION}")
 message(STATUS "OPENSCAD_YEAR: ${OPENSCAD_YEAR}")
 message(STATUS "OPENSCAD_MONTH: ${OPENSCAD_MONTH}")
 message(STATUS "OPENSCAD_DAY: ${OPENSCAD_DAY}")
 message(STATUS "OPENSCAD_PATCH: ${OPENSCAD_PATCH}")
 message(STATUS "OPENSCAD_BUILD: ${OPENSCAD_BUILD}")
