scintilla {
  HEADERS += src/scintillaeditor.h src/scadlexer.h
  SOURCES += src/scintillaeditor.cpp src/scadlexer.cpp

  DEFINES += USE_SCINTILLA_EDITOR

  QSCILOADED =
  OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)

  !isEmpty(OPENSCAD_LIBDIR) {
    exists($$OPENSCAD_LIBDIR) {
      exists($$OPENSCAD_LIBDIR/lib/x86_64-linux-gnu/qt5/mkspecs/features/qscintilla2.prf) {
        include($$OPENSCAD_LIBDIR/lib/x86_64-linux-gnu/qt5/mkspecs/features/qscintilla2.prf)
        INCLUDEPATH = $$OPENSCAD_LIBDIR/include/qt5 $$INCLUDEPATH
        LIBS = -L$$OPENSCAD_LIBDIR/lib/x86_64-linux-gnu $$LIBS
        QSCILOADED=yes
      }
    }
  }

  # The above looks in too specific paths, use QT_* variables to find where things really should be.
  isEmpty(QSCILOADED) {
      exists($$[QT_INSTALL_DATA]/mkspecs/features/qscintilla2.prf) {
      include($$[QT_INSTALL_DATA]/mkspecs/features/qscintilla2.prf)
      INCLUDEPATH = $$[QT_INSTALL_HEADERS] $$INCLUDEPATH
      LIBS = -L$$[QT_INSTALL_LIBS] $$LIBS
      QSCILOADED=yes
    }
  }

  # The qscintilla2.prf which ships with QScintilla is broken for Mac/Windows
  # debug builds, so we supply our own
  isEmpty(QSCILOADED) {
    win32|macx: {
      include(qscintilla2.prf)
      QSCILOADED=yes
    }
  }

  isEmpty(QSCILOADED) {
    load(qscintilla2) {
      QSCILOADED=yes
      # All good, found installed *.prf file.
    }
  }

  isEmpty(QSCILOADED) {
    # Older scintilla libs (e.g. 2.7.2 on fedora20) do not provide the
    # prf file.
    #
    # In addition Ubuntu (and maybe other distributions) have the Qt5
    # prf file in the wrong location so it's not picked up by qmake
    #
    message("Using local copy of qscintilla2.prf instead.")
    include(qscintilla2.prf)
    QSCILOADED=yes
  }

}
