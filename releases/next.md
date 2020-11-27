**Language Features**

* Add convexity parameter to resize()
* Allow random seeds to stick between rands() calls

**Program Features**

* New translations: Chinese
* Update French translation
* Add support for line-cap and line-join in SVG import
* Add generic tail-recursion support
* Add --export-format command line option
* Disable HTML rendering in console
* Fix comparison operator for ranges
* Fix search highlight for utf8 text
* Fix search order for imported modules
* Add support for Line/Block copy and move in editor
* Update AppStream release info
* Install start shortcut for all users on Windows
* Install icons with defined sizes (e.g. required by flathub)
* Console messages now only use HTML to highlight errors/warnings
* Switch to C++14 and allow usage of header-only CGAL

**Bug Fixes**

* Don't crash with empty CSG normalizer result (fixes #3085)
* Fix search highlight with multi-byte utf8 characters (fixes #3068)
* Use std::string as storage (fixes #3057)
* Setting minimum size of search label (fixes #2962)
* Use a relative import() path for svg viewbox tests
* Fix Trailing zeroes in output from echo) (fixes #2950)
* Fix interpretation of '&' in title bar of undocked widget
* Fix Recent-files handling with '&' in filename (fixes #2988)
* Have Reindexer return a const ref instead of a pointer into value array
* Set value after min/max so it's not limited to wrong range (fixes #2995)
* Don't crash customizer with saved vector parameters with >4 values
* Avoid undefined behavior for `convexity' parameter to linear_extrude
* Fix reconnect to Xvfb when running tests in parallel
* Fix dangling pointer in "--export-format"
* Fix build with Boost 1.72
* Build fixes for older systems (Ubuntu 16.04, Debian 8)
* Add Work-around for test failures on MIPS
