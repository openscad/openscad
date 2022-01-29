# CPack-time project CPack configuration file. 
# This file is included at cpack time, once per generator 
# after CPack has set CPACK_GENERATOR to the actual generator being used.
# It allows per-generator setting of CPACK_* variables at cpack time.

if(CPACK_GENERATOR MATCHES "NSIS")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Openscad${SUFFIX_WITH_DASH}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-Installer")
  set(CPACK_NSIS_COMPRESSOR "/SOLID lzma\n\
  SetCompressorDictSize 64")
endif()
