boost {

  # Optionally specify location of boost using the
  # BOOSTDIR env. variable
  BOOST_DIR = $$(BOOSTDIR)
  !isEmpty(BOOST_DIR) {
    QMAKE_INCDIR += $$BOOST_DIR
    message("boost location: $$BOOST_DIR")
    win32:QMAKE_LIBDIR += -L$$BOOST_DIR/lib
  }

  ORIGINAL_LIBS_VALUE = $$LIBS

  win32 {
    LIBS += -llibboost_thread-vc90-mt-s-1_46_1 -llibboost_program_options-vc90-mt-s-1_46_1
  }

  exists(/usr/lib64/libboost*thread-mt*) {
    LIBS += -lboost_thread-mt -lboost_program_options-mt
    BOOST_IS_MT = true
  } 

  exists(/usr/lib/libboost*thread-mt*) {
    LIBS *= -lboost_thread-mt -lboost_program_options-mt
    BOOST_IS_MT = true
  } 

  isEmpty(BOOST_IS_MT) { 
    unix|macx {
      LIBS += -lboost_thread -lboost_program_options
    }
  }

  CONFIG(mingw-cross-env) {
    LIBS = $$ORIGINAL_LIBS_VALUE # erase, start over
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    LIBS += -lboost_thread_win32-mt -lboost_program_options-mt
  } 

}
