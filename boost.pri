boost {

  # Optionally specify location of boost using the
  # BOOSTDIR env. variable
  BOOST_DIR = $$(BOOSTDIR)
  !isEmpty(BOOST_DIR) {
    QMAKE_INCDIR += $$BOOST_DIR
    message("boost location: $$BOOST_DIR")
    win32:QMAKE_LIBDIR += -L$$BOOST_DIR/lib
  }

  win32:!CONFIG(mingw-cross-env) {
    LIBS += -llibboost_thread-vc90-mt-s-1_46_1 -llibboost_program_options-vc90-mt-s-1_46_1
  } 

  CONFIG(mingw-cross-env) {
    DEFINES += BOOST_STATIC
    DEFINES += BOOST_THREAD_USE_LIB
    DEFINES += Boost_USE_STATIC_LIBS
    LIBS += -lboost_thread_win32-mt -lboost_program_options-mt
  } 

  unix* {
    BMT_TEST1 = /usr/lib64/libboost*thread-mt*
    BMT_TEST2 = /usr/lib/libboost*thread-mt*
    BMT_TEST3 = $$BOOST_DIR/lib/libboost*thread-mt*

    exists($$BMT_TEST1)|exists($$BMT_TEST2)|exists($$BMT_TEST3) {
      LIBS += -lboost_thread-mt -lboost_program_options-mt
      BOOST_IS_MT = true
    } 
  }

  unix*|macx {
    isEmpty(BOOST_IS_MT) { 
      LIBS += -lboost_thread -lboost_program_options
    }
  }

}
