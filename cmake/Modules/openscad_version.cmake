if ("${OPENSCAD_VERSION}" STREQUAL "")
  string(TIMESTAMP OPENSCAD_VERSION "%Y.%m.%d")
endif()
string(REPLACE "-" ";" SPLITVERSION ${OPENSCAD_VERSION})
list(GET SPLITVERSION 0 OPENSCAD_SHORTVERSION)
string(REGEX MATCHALL "^[0-9]+|[0-9]+|[0-9]+$" VERSIONLIST "${OPENSCAD_SHORTVERSION}")
list(GET VERSIONLIST 0 OPENSCAD_YEAR)
list(GET VERSIONLIST 1 OPENSCAD_MONTH)
math(EXPR OPENSCAD_MONTH ${OPENSCAD_MONTH}) # get rid of leading zero
list(LENGTH VERSIONLIST VERSIONLEN)
if (${VERSIONLEN} EQUAL 3)
  list(GET VERSIONLIST 2 OPENSCAD_DAY)
  math(EXPR OPENSCAD_DAY ${OPENSCAD_DAY}) # get rid of leading zero
endif()

# cmake project() command takes a <version> argument composed of non-negative integer components,
# i.e. <major>[.<minor>[.<patch>[.<tweak>]]]
if (DEFINED OPENSCAD_DAY)
  set(PROJECT_VERSION "${OPENSCAD_YEAR}.${OPENSCAD_MONTH}.${OPENSCAD_DAY}")
else()
  set(PROJECT_VERSION "${OPENSCAD_YEAR}.${OPENSCAD_MONTH}")
endif()
