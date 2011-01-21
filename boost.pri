boost {
  isEmpty(DEPLOYDIR) {
    # Optionally specify location of boost using the
    # BOOSTDIR env. variable
    BOOST_DIR = $$(BOOSTDIR)
    !isEmpty(BOOST_DIR) {
      INCLUDEPATH += $$BOOST_DIR
      message("boost location: $$BOOST_DIR")
    }
  }
}
