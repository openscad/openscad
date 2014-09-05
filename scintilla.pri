scintilla {
  HEADERS += src/scintillaeditor.h src/scadlexer.h
  SOURCES += src/scintillaeditor.cpp src/scadlexer.cpp

  DEFINES += USE_SCINTILLA_EDITOR

  # The qscintilla2.prf which ships with QScintilla is broken for Mac/Windows 
  # debug builds, so we supply our own
  win32|macx: include(qscintilla2.prf)
  else: CONFIG += qscintilla2

  # Older scintilla libs (e.g. 2.7.2 on fedora20) do not provide the
  # prf file, so if no lib added at this point, we still fall back to
  # the included qscintilla2.prf
  !contains(LIBS, ".*scintilla.*") {
    include(qscintilla2.prf)
  }
}
