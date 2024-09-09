# QScintilla is a port to Qt of Neil Hodgson's Scintilla C++ editor control
# available at http://www.riverbankcomputing.com/software/qscintilla/
#
# The module defines the following variables:
#  QT6QSCINTILLA_FOUND - the system has QScintilla
#  QT6QSCINTILLA_INCLUDE_DIR - where to find qsciscintilla.h
#  QT6QSCINTILLA_INCLUDE_DIRS - qscintilla includes
#  QT6QSCINTILLA_LIBRARY - where to find the QScintilla library
#  QT6QSCINTILLA_LIBRARIES - additional libraries
#  QT6QSCINTILLA_MAJOR_VERSION - major version
#  QT6QSCINTILLA_MINOR_VERSION - minor version
#  QT6QSCINTILLA_PATCH_VERSION - patch version
#  QT6QSCINTILLA_VERSION_STRING - version (ex. 2.6.2)
#  QT6QSCINTILLA_ROOT_DIR - root dir (ex. /usr/local)

#=============================================================================
# Copyright 2010-2013, Julien Schueller
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# The views and conclusions contained in the software and documentation are those
# of the authors and should not be interpreted as representing official policies, 
# either expressed or implied, of the FreeBSD Project.
#=============================================================================


find_path ( QT6QSCINTILLA_INCLUDE_DIR
  NAMES qsciscintilla.h
  HINTS ${Qt6Widgets_INCLUDE_DIRS}
  PATH_SUFFIXES qt6 Qsci
)

set ( QT6QSCINTILLA_INCLUDE_DIRS ${QT6QSCINTILLA_INCLUDE_DIR} )

# version
set ( _VERSION_FILE ${QT6QSCINTILLA_INCLUDE_DIR}/qsciglobal.h )
if ( EXISTS ${_VERSION_FILE} )
  file ( STRINGS ${_VERSION_FILE} _VERSION_LINE REGEX "define[ ]+QSCINTILLA_VERSION_STR" )
  if ( _VERSION_LINE )
    string ( REGEX REPLACE ".*define[ ]+QSCINTILLA_VERSION_STR[ ]+\"(.*)\".*" "\\1" QT6QSCINTILLA_VERSION_STRING "${_VERSION_LINE}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" QT6QSCINTILLA_MAJOR_VERSION "${QT6QSCINTILLA_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" QT6QSCINTILLA_MINOR_VERSION "${QT6QSCINTILLA_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" QT6QSCINTILLA_PATCH_VERSION "${QT6QSCINTILLA_VERSION_STRING}" )
  endif ()
endif ()


# check version
set ( _QT6QSCINTILLA_VERSION_MATCH TRUE )
if ( QScintilla_FIND_VERSION AND QT6QSCINTILLA_VERSION_STRING )
  if ( QScintilla_FIND_VERSION_EXACT )
    if ( NOT QScintilla_FIND_VERSION VERSION_EQUAL QT6QSCINTILLA_VERSION_STRING )
      set ( _QT6QSCINTILLA_VERSION_MATCH FALSE )
    endif ()
  else ()
    if ( QT6QSCINTILLA_VERSION_STRING VERSION_LESS QScintilla_FIND_VERSION )
      set ( _QT6QSCINTILLA_VERSION_MATCH FALSE )
    endif ()
  endif ()
endif ()


find_library ( QT6QSCINTILLA_LIBRARY
  NAMES qt6scintilla2 qscintilla2-qt6 qscintilla2 qscintilla2_qt6
  HINTS ${Qt6Widgets_LIBRARIES}
)

set ( QT6QSCINTILLA_LIBRARIES ${QT6QSCINTILLA_LIBRARY} )

IF( QT6QSCINTILLA_LIBRARY AND QT6QSCINTILLA_INCLUDE_DIR )
        SET( QT6QSCINTILLA_FOUND TRUE )
ENDIF( QT6QSCINTILLA_LIBRARY AND QT6QSCINTILLA_INCLUDE_DIR )

#IF( QT6QSCINTILLA_FOUND )
#                MESSAGE( STATUS "Found QScintilla-Qt6 header files in ${QT6QSCINTILLA_INCLUDE_DIR}")
#                MESSAGE( STATUS "Found QScintilla-Qt6 libraries: ${QT6QSCINTILLA_LIBRARY}")
#ENDIF(QT6QSCINTILLA_FOUND)

# try to guess root dir from include dir
if ( QT6QSCINTILLA_INCLUDE_DIR )
  string ( REGEX REPLACE "(.*)/include.*" "\\1" QT6QSCINTILLA_ROOT_DIR ${QT6QSCINTILLA_INCLUDE_DIR} )
# try to guess root dir from library dir
elseif ( QT6QSCINTILLA_LIBRARY )
  string ( REGEX REPLACE "(.*)/lib[/|32|64].*" "\\1" QT6QSCINTILLA_ROOT_DIR ${QT6QSCINTILLA_LIBRARY} )
endif ()


# handle the QUIETLY and REQUIRED arguments
include ( FindPackageHandleStandardArgs )
if ( CMAKE_VERSION VERSION_LESS 2.8.3 )
  find_package_handle_standard_args( Qt6QScintilla DEFAULT_MSG QT6QSCINTILLA_LIBRARY QT6QSCINTILLA_INCLUDE_DIR _QT6QSCINTILLA_VERSION_MATCH )
else ()
  find_package_handle_standard_args( Qt6QScintilla REQUIRED_VARS QT6QSCINTILLA_LIBRARY QT6QSCINTILLA_INCLUDE_DIR _QT6QSCINTILLA_VERSION_MATCH VERSION_VAR QT6QSCINTILLA_VERSION_STRING )
endif ()

mark_as_advanced (
  QT6QSCINTILLA_LIBRARY
  QT6QSCINTILLA_LIBRARIES
  QT6QSCINTILLA_INCLUDE_DIR
  QT6QSCINTILLA_INCLUDE_DIRS
  QT6QSCINTILLA_MAJOR_VERSION
  QT6QSCINTILLA_MINOR_VERSION
  QT6QSCINTILLA_PATCH_VERSION
  QT6QSCINTILLA_VERSION_STRING
  QT6QSCINTILLA_ROOT_DIR
  )
