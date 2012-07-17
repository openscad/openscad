boost {

  # Optionally specify location of boost using the
  # BOOSTDIR env. variable
  BOOST_DIR = $$(BOOSTDIR)
  !isEmpty(BOOST_DIR) {
    QMAKE_INCDIR += $$BOOST_DIR
    message("boost location: $$BOOST_DIR")
    win32: QMAKE_LIBDIR += -L$$BOOST_DIR/lib
  }

  CONFIG(mingw-cross-env) {
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    BOOST_LINK_FLAGS = -lboost_thread_win32-mt -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -lboost_chrono-mt
  } 

  isEmpty(BOOST_LINK_FLAGS):win32 {
    BOOST_LINK_FLAGS = -llibboost_thread-vc90-mt-s-1_46_1 -llibboost_program_options-vc90-mt-s-1_46_1 -llibboost_filesystem-vc90-mt-s-1_46_1 -llibboost_system-vc90-mt-s-1_46_1 -llibboost_regex-vc90-mt-s-1_46_1
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
      exists($$BMT_TEST1)|exists($$BMT_TEST2)|exists($$BMT_TEST3) {
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
