# win32-specific MSVC compiler general settings

win32*msvc* {
  #configure additional directories
  INCLUDEPATH += $$(MPIRDIR)
  INCLUDEPATH += $$(MPFRDIR)

  # don't know where the __WIN32__ is usually set
  DEFINES += _USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS YY_NO_UNISTD_H __WIN32__

  # disable MSVC warnings that are of very low importance
  # disable warning about too long decorated names
  QMAKE_CXXFLAGS += -wd4503
  # CGAL casting int to bool
  QMAKE_CXXFLAGS += -wd4800
  # CGAL's unreferenced formal parameters
  QMAKE_CXXFLAGS += -wd4100
  # lexer uses strdup() & other POSIX stuff
  QMAKE_CXXFLAGS += -D_CRT_NONSTDC_NO_DEPRECATE
  # Treat WChar_t as a builtin type, allows Qt to call boost functions
  QMAKE_CXXFLAGS += /Zc:wchar_t
  # increases the number of sections in .obj file
  QMAKE_CXXFLAGS += /bigobj

}
