# Try to find CryptoPP include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# CRYPTOPP_INCLUDE_DIRS - containing the Crypto++ headers
# CRYPTOPP_LIBRARIES - containing the Crypto++ library

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_CRYPTOPP cryptopp>=5.6.0) 

find_path(CRYPTOPP_INCLUDE_DIRS NAMES cryptopp/sha.h
  HINTS ${PC_CRYPTOPP_INCLUDE_DIRS} ${PC_CRYPTOPP_INCLUDEDIR}
)

find_library(CRYPTOPP_LIBRARIES NAMES cryptopp
  HINTS ${PC_CRYPTOPP_LIBRARY_DIRS} ${PC_CRYPTOPP_LIBDIR}
)

set(CRYPTOPP_VERSION ${PC_CRYPTOPP_VERSION})
set(CryptoPP_LIBRARIES ${PC_CRYPTOPP_LIBRARIES})
set(CryptoPP_INCLUDE_DIRS ${PC_CRYPTOPP_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CryptoPP DEFAULT_MSG CRYPTOPP_INCLUDE_DIRS CryptoPP_LIBRARIES)
