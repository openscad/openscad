macx {
  # Mac needs special care to link against the correct C++ library
  # We attempt to auto-detect it by inspecting Boost
  dirs = $${BOOSTDIR} $${QMAKE_LIBDIR}
  for(dir, dirs) {
    system(otool -L $${dir}/libboost_thread*  | grep libc++ >& /dev/null ) {
      message("Using libc++")
      CONFIG += libc++
    }
    else {
      message("Using libstdc++")
      CONFIG += libstdc++
      c++std {
        # libc++ is a requirement for using C++14 
        warning("Disabling C++14 since libstdc++ dependencies were found")
        CONFIG -= c++std
      }
    }
  }

  libc++ {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
    QMAKE_OBJECTIVE_CFLAGS += -stdlib=libc++
  }
}

c++std {
  QMAKE_CXXFLAGS += -std=c++14
  message("Using C++14")

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
