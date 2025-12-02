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
# Supports both semantic versioning (X.Y.Z) and old date format (YYYY.MM.DD)
# Format: MAJOR.MINOR[.PATCH][-PRERELEASE][+BUILD]
string(REGEX MATCH "^(([0-9]+)\\.([0-9]+)(\\.([0-9]+))?)(-([^\\.\\+]+))?(\\+(.*))?$" MATCH "${OPENSCAD_VERSION}")

set(OPENSCAD_SHORTVERSION ${CMAKE_MATCH_1})
set(OPENSCAD_MAJOR ${CMAKE_MATCH_2})
set(OPENSCAD_MINOR ${CMAKE_MATCH_3})
set(OPENSCAD_PATCH ${CMAKE_MATCH_5})
set(OPENSCAD_PRERELEASE ${CMAKE_MATCH_7})
set(OPENSCAD_BUILD ${CMAKE_MATCH_9})

# For backward compatibility, also set YEAR/MONTH/DAY
# If using semantic versioning (e.g., 0.6.0), these will be the version components
# If using date format (e.g., 2025.12.02), these will be the date components
set(OPENSCAD_YEAR ${OPENSCAD_MAJOR})
set(OPENSCAD_MONTH ${OPENSCAD_MINOR})
if (DEFINED OPENSCAD_PATCH AND NOT "${OPENSCAD_PATCH}" STREQUAL "")
  set(OPENSCAD_DAY ${OPENSCAD_PATCH})
else()
  # Patch version is optional (e.g., 0.6 instead of 0.6.0)
  set(OPENSCAD_DAY 0)
endif()

set(PROJECT_VERSION ${OPENSCAD_SHORTVERSION})

message(STATUS "OPENSCAD_VERSION: ${OPENSCAD_VERSION}")
message(STATUS "OPENSCAD_SHORTVERSION: ${OPENSCAD_SHORTVERSION}")
message(STATUS "OPENSCAD_MAJOR: ${OPENSCAD_MAJOR}")
message(STATUS "OPENSCAD_MINOR: ${OPENSCAD_MINOR}")
message(STATUS "OPENSCAD_PATCH: ${OPENSCAD_PATCH}")
message(STATUS "OPENSCAD_PRERELEASE: ${OPENSCAD_PRERELEASE}")
message(STATUS "OPENSCAD_BUILD: ${OPENSCAD_BUILD}")
message(STATUS "Backward compat - YEAR: ${OPENSCAD_YEAR}, MONTH: ${OPENSCAD_MONTH}, DAY: ${OPENSCAD_DAY}")
