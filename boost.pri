boost {
  isEmpty(DEPLOYDIR) {
    # Optionally specify location of boost using the
    # BOOSTDIR env. variable
    BOOST_DIR = $$(BOOSTDIR)
    !isEmpty(BOOST_DIR) {
      INCLUDEPATH += $$BOOST_DIR
      message("boost location: $$BOOST_DIR")
      win32:LIBS += -L$$BOOST_DIR/libs/program_options/build/msvc-10.0/release/link-static/threading-multi
      win32:LIBS += -L$$BOOST_DIR/libs/thread/build/msvc-10.0/release/link-static/threading-multi
    }
  }

  win32 {
     LIBS += -llibboost_thread-vc100-mt-1_45 -llibboost_program_options-vc100-mt-1_45
  } else {
     LIBS += -lboost_thread -lboost_program_options
  }
}
