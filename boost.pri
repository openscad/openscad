boost {

  # Optionally specify location of boost using the
  # BOOSTDIR env. variable
  BOOST_DIR = $$(BOOSTDIR)
  !isEmpty(BOOST_DIR) {
    QMAKE_INCDIR += $$BOOST_DIR
    message("boost location: $$BOOST_DIR")
    win32:QMAKE_LIBDIR += -L$$BOOST_DIR/lib
  }

  CONFIG(mingw-cross-env) {
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    LIBS += -lboost_thread_win32-mt -lboost_program_options-mt
  } else {
    win32 {
      LIBS += -llibboost_thread-vc90-mt-s-1_46_1 -llibboost_program_options-vc90-mt-s-1_46_1
    } else {
      # some platforms have only '-mt' versions. uncomment if needed. 
      # LIBS += -lboost_thread-mt -lboost_program_options-mt
      LIBS += -lboost_thread -lboost_program_options
    }
  }
}
