freetype {
  QMAKE_CXXFLAGS += `pkg-config --cflags freetype2 harfbuzz fontconfig`
  LIBS += `pkg-config --libs freetype2 harfbuzz fontconfig`
}

