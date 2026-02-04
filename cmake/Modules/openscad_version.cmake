# Get OPENSCAD_VERSION
# Use the standalone detect_version.sh script (single source of truth)
if ("${OPENSCAD_VERSION}" STREQUAL "")
  # Try git describe first (preferred method, works cross-platform)
  find_package(Git QUIET)
  if (GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*.[0-9]*.[0-9]*" --always --dirty
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE GIT_RESULT
    )
    if (GIT_RESULT EQUAL 0 AND NOT "${GIT_VERSION}" STREQUAL "")
      # Remove leading 'v' if present
      string(REGEX REPLACE "^v" "" OPENSCAD_VERSION "${GIT_VERSION}")
      message(STATUS "Detected version from git: ${OPENSCAD_VERSION}")
    endif()
  endif()

  # Fallback to VERSION.txt if git didn't work
  if ("${OPENSCAD_VERSION}" STREQUAL "")
    set(VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION.txt")
    if (EXISTS "${VERSION_FILE}")
      file(READ "${VERSION_FILE}" FILE_VERSION)
      string(STRIP "${FILE_VERSION}" OPENSCAD_VERSION)
      if (NOT "${OPENSCAD_VERSION}" STREQUAL "")
        message(STATUS "Detected version from VERSION.txt: ${OPENSCAD_VERSION}")
      endif()
    endif()
  endif()

  # Final check - fail if no version found
  if ("${OPENSCAD_VERSION}" STREQUAL "")
    message(FATAL_ERROR "Version detection failed. Please ensure git tags are available or VERSION.txt exists.")
  endif()
endif()

# Get OPENSCAD_COMMIT
# Use git to get the commit hash (works cross-platform)
if ("${OPENSCAD_COMMIT}" STREQUAL "")
  if (GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse --short=9 HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE OPENSCAD_COMMIT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE COMMIT_RESULT
    )
    if (COMMIT_RESULT EQUAL 0 AND NOT "${OPENSCAD_COMMIT}" STREQUAL "")
      message(STATUS "Detected commit: ${OPENSCAD_COMMIT}")
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

# If regex didn't match (e.g., git describe returned just a commit hash with no tags),
# fail with an error
if ("${OPENSCAD_SHORTVERSION}" STREQUAL "")
  message(FATAL_ERROR "Version string '${OPENSCAD_VERSION}' doesn't match expected format. Please ensure git tags are fetched or VERSION.txt contains a valid version.")
endif()

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
