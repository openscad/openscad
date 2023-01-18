# CPack-time project CPack configuration file.
# This file is included at cpack time, once per generator
# after CPack has set CPACK_GENERATOR to the actual generator being used.
# It allows per-generator setting of CPACK_* variables at cpack time.

if(CPACK_GENERATOR MATCHES "NSIS")
  set(CPACK_INSTALL_CMAKE_PROJECTS "/home/gsohler/git/openscad/build;OpenSCAD;Unspecified;/")
  set(CPACK_NSIS_COMPONENT_INSTALL 0)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Openscad")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-Installer")
  set(CPACK_NSIS_COMPRESSOR "/SOLID lzma\n\
  SetCompressorDictSize 32")
elseif(CPACK_GENERATOR MATCHES "ARCHIVE")
  set(CPACK_INSTALL_CMAKE_PROJECTS "/home/gsohler/git/openscad/build;OpenSCAD;ALL;/")
endif()
