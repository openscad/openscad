# Get OPENSCAD_VERSION
# Use the standalone detect_version.sh script (single source of truth)
if ("${OPENSCAD_VERSION}" STREQUAL "")
  execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/detect_version.sh"
    OUTPUT_VARIABLE OPENSCAD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE VERSION_RESULT
  )
  if (VERSION_RESULT EQUAL 0 AND NOT "${OPENSCAD_VERSION}" STREQUAL "")
    message(STATUS "Detected version: ${OPENSCAD_VERSION}")
  else()
    message(FATAL_ERROR "Version detection failed. Please ensure git tags are available or VERSION.txt exists.")
  endif()
endif()

# Get OPENSCAD_COMMIT
# Use the standalone detect_commit.sh script (single source of truth)
if ("${OPENSCAD_COMMIT}" STREQUAL "")
  execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/detect_commit.sh"
    OUTPUT_VARIABLE OPENSCAD_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE COMMIT_RESULT
  )
  if (COMMIT_RESULT EQUAL 0 AND NOT "${OPENSCAD_COMMIT}" STREQUAL "")
    message(STATUS "Detected commit: ${OPENSCAD_COMMIT}")
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
