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
  }
}

c++11 {
  # -std=c++11 is only available in gcc>=4.7
  *g++*: QMAKE_CXXFLAGS += -std=c++0x
  else: QMAKE_CXXFLAGS += -std=c++11
  message("Using C++11")

  *clang*: {
      # 3rd party libraries will probably violate this for a long time
    CXX11_SUPPRESS_WARNINGS += -Wno-inconsistent-missing-override
    # boost/algorithm/string.hpp does this
    CXX11_SUPPRESS_WARNINGS += -Wno-unused-local-typedef
    # CGAL
    CXX11_SUPPRESS_WARNINGS += -Wno-deprecated-register

    QMAKE_CXXFLAGS_WARN_ON += $$CXX11_SUPPRESS_WARNINGS
    QMAKE_OBJECTIVE_CFLAGS_WARN_ON += $$CXX11_SUPPRESS_WARNINGS
  }
}
else {
  *clang* {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-c++11-extensions
  }
}
