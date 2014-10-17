scintilla {
  HEADERS += src/scintillaeditor.h src/scadlexer.h
  SOURCES += src/scintillaeditor.cpp src/scadlexer.cpp

  DEFINES += USE_SCINTILLA_EDITOR

  # The qscintilla2.prf which ships with QScintilla is broken for Mac/Windows
  # debug builds, so we supply our own
  win32|macx: {
    include(qscintilla2.prf)
  }
  else: {
    load(qscintilla2) {
      # All good, found installed *.prf file.
    } else {
      # Older scintilla libs (e.g. 2.7.2 on fedora20) do not provide the
      # prf file.
      #
      # In addition Ubuntu (and maybe other distributions) have the Qt5
      # prf file in the wrong location so it's not picked up by qmake
      #
      message("Using local copy of qscintilla2.prf instead.")
      include(qscintilla2.prf)
    }
  }
}
