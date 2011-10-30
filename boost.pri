boost {
  isEmpty(DEPLOYDIR) {
    # Optionally specify location of boost using the
    # BOOSTDIR env. variable
    BOOST_DIR = $$(BOOSTDIR)
    !isEmpty(BOOST_DIR) {
      INCLUDEPATH += $$BOOST_DIR
      message("boost location: $$BOOST_DIR")
      win32:LIBS += -L$$BOOST_DIR/lib
    }
  }

  CONFIG(mingw-cross-env) {
    LIBS += -lboost_thread_win32-mt -lboost_program_options-mt
  }
  else {
    win32 {
      LIBS += -llibboost_thread-vc90-mt-s-1_46_1 -llibboost_program_options-vc90-mt-s-1_46_1
    } else {
      LIBS += -lboost_thread -lboost_program_options
    }
  }
}
