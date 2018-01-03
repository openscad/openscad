# Coding Style

The OpenSCAD coding style is encoded in `.uncrustify.cfg`.
Code to be committed can be beautified by installing `uncrustify` and running `scripts/beautify.sh`.

Alternatively, run `scripts/beautify.sh --all` to beautify all the source code.

Note: Uncrustify is in heavy development and tends to introduce breaking changes from time to time.
OpenSCAD has been tested against uncrustify commit a05edf605a5b1ea69ac36918de563d4acf7f31fb (Dec 24 2017).

Coding style highlights:

* Use 2 spaces for indentation
* Use C++11 functionality where applicable. Please read Scott Meyer's Effective Modern C++ for a good primer on modern C++ style and features: http://shop.oreilly.com/product/0636920033707.do

# Regression Tests

See `testing.txt`

# How to add new function/module

* Implement
* Add test
* Modules: Add example
* Document:
   * wikibooks
   * cheatsheet
   * Modules: tooltips (editor.cc)
   * External editor modes
   * Add to RELEASE_NOTES.md
