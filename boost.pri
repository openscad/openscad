boost {

  # Optionally specify location of boost using the
  # BOOSTDIR env. variable
  BOOST_DIR = $$(BOOSTDIR)
  !isEmpty(BOOST_DIR) {
    QMAKE_INCDIR += $$BOOST_DIR
    message("boost location: $$BOOST_DIR")
    win*: QMAKE_LIBDIR += -L$$BOOST_DIR/lib
  }

  # See https://svn.boost.org/trac/boost/ticket/6219
  macx: DEFINES += __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0

  # MXE cross build
  CONFIG(mingw-cross-env) {
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    BOOST_LINK_FLAGS = -lboost_thread_win32-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -lboost_chrono-mt
  }

  # MSYS2
  isEmpty(BOOST_LINK_FLAGS):win32-g++ {
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    BOOST_LINK_FLAGS = -lboost_thread-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt
  } 

  # check for OPENSCAD_LIBDIR + multithread
  isEmpty(BOOST_LINK_FLAGS) {
    OPENSCAD_LIBDIR = $$(OPENSCAD_LIBRARIES)
    !isEmpty(OPENSCAD_LIBDIR) {
      exists($$OPENSCAD_LIBDIR/lib/libboost*thread-mt*) {
        BOOST_LINK_FLAGS = -lboost_thread-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt
      } else {
        exists($$OPENSCAD_LIBDIR/lib/libboost*thread*) {
          BOOST_LINK_FLAGS = -lboost_thread -lboost_program_options -lboost_filesystem -lboost_system -lboost_regex
        }
      }
    }
  }

  # check for BOOSTDIR + multithread
  isEmpty(BOOST_LINK_FLAGS) {
    BOOST_DIR = $$(BOOSTDIR)
    !isEmpty(BOOST_DIR) {
      exists($$BOOST_DIR/lib/libboost*thread-mt*) {
        BOOST_LINK_FLAGS = -lboost_thread-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt
      } else {
        exists($$BOOST_DIR/lib/libboost*thread*) {
          BOOST_LINK_FLAGS = -lboost_thread -lboost_program_options -lboost_filesystem -lboost_system -lboost_regex
        }
      }
    }
  }

  isEmpty(BOOST_LINK_FLAGS) {
    unix {
      BMT_TEST1 = /usr/lib64/libboost*thread-mt*
      BMT_TEST2 = /usr/lib/libboost*thread-mt*
      BMT_TEST3 = /usr/pkg/lib/libboost*thread-mt* # netbsd
      BMT_TEST4 = /usr/local/lib/libboost*thread-mt* # homebrew
      BMT_TEST5 = /opt/local/lib/libboost*thread-mt* # macports
      exists($$BMT_TEST1)|exists($$BMT_TEST2)|exists($$BMT_TEST3)|exists($$BMT_TEST4)|exists($$BMT_TEST5) {
        BOOST_LINK_FLAGS = -lboost_thread-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt
      }
    }
  }

  isEmpty(BOOST_LINK_FLAGS) {
    unix|macx {
      BOOST_LINK_FLAGS = -lboost_thread -lboost_program_options -lboost_filesystem -lboost_system -lboost_regex
    }
  }

  LIBS += $$BOOST_LINK_FLAGS

}
