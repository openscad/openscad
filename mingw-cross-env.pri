# cross compilation unix->win32
# To use static linking, pass CONFIG+=mingw-cross-env to qmake
# To use shared linking, pass CONFIG+=mingw-cross-env-shared to qmake

mingw-cross-env* {
  QMAKE_DEL_FILE = rm -f
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedefs #eigen3
}
