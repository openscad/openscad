#
# CMake build system to download Documentation
#

cmake_minimum_required(VERSION 3.5)

if(MSVC)
    execute_process(
        COMMAND ping files.openscad.org -n 2
        ERROR_QUIET
        RESULT_VARIABLE NO_CONNECTION
    )
else()
    execute_process(
        COMMAND ping files.openscad.org -c 2
        ERROR_QUIET
        RESULT_VARIABLE NO_CONNECTION
    )
endif()

if(NOT NO_CONNECTION EQUAL 0)
    message(WARNING "No internet connection detected, skip downloading User Manual.")

else()
    set(CMAKE_DOCS_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/docs/OpenSCADUserDocs")
    make_directory(${CMAKE_DOCS_DIRECTORY})
    include(FindPythonInterp)
    add_custom_target(
        offline_documentation ALL
        DEPENDS ${CMAKE_DOCS_DIRECTORY}/openscad_docs
    )
    add_custom_command(
        OUTPUT
            ${CMAKE_DOCS_DIRECTORY}/openscad_docs
            ${CMAKE_DOCS_DIRECTORY}/openscad_docs_pdf
        COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/resources/docs/docs_downloader.py"
        COMMENT "Running docs_downloader.py"                                                                                                                       
        WORKING_DIRECTORY ${CMAKE_DOCS_DIRECTORY} DEPENDS ${CMAKE_SOURCE_DIR}/resources/docs/docs_downloader.py
    )
endif()
