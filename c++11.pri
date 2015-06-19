macx {
  # Mac needs special care to link against the correct C++ library
  # We attempt to auto-detect it by inspecting Boost
  dirs = $${BOOSTDIR} $${QMAKE_LIBDIR}
  for(dir, dirs) {
    system(grep -q __112basic_string $${dir}/libboost_thread* >& /dev/null) {
      message("Using libc++11")
      CONFIG += libc++
    }
    else {
      message("Using libstdc++")
      CONFIG += libstdc++
      c++11 {
        # libc++ is a requirement for using C++11 
        warning("Disabling C++11 since libstdc++ dependencies were found")
        CONFIG -= c++11
      }
    }
  }

  libc++ {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
    QMAKE_OBJECTIVE_CFLAGS += -stdlib=libc++
    # libc++ on requires Mac OS X 10.7+
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
  }
}

c++11 {
  QMAKE_CXXFLAGS += -std=c++11
  message("Using C++11")
}
else {
  *clang* {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-c++11-extensions
  }
}
