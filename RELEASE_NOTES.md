
# OpenSCAD 2021.01

**Language Features**

* New
  * Add function literals
  * Add exponent function (e.g. 2^3)
  * Add convexity parameter to resize()
  * Add support for generic tail recursion
  * Add $vpf for viewport field of view
  * Add support for line-cap and line-join in SVG import
  * Add support stroke-linejoin in SVG import
  * Add warnings for ranges with begin < end
  * Add warnings for matrix structure problems

* Changed
  * Allow random seeds to stick between rands() calls
  * Make comparison operators on mixed types return undef
  * Track reason for values turning undef

* Fixed
  * Fix Range precision
  * Fix empty geometry handling
  * Fix search order for imported modules
  * Improve internal handling of data values
  * Improve performance of array concatenation
  * Improve float accuracy of mirror matrix calculation
  * Improve polygon and polyhedron warnings/errors

**Program Features**

* GUI
  * Add PDF export (single page only)
  * Add jump to source code from 3D preview
  * Add Error-Log window for tracking errors and warnings
  * Add window menu to allow keyboard access to all docked windows
  * Enable default binary stl output, and ascii option
  * Show message in console when caches are flushed
  * Disable HTML rendering in console
  * Enable color scheme for CGAL 2D render
  * Simplified OpenCSG edge shader, with added smoothing
  * New and updated translations: Chinese, Armenian, French, Russian, Polish, German

* Editor
  * Edit multiple files in tabs
  * Switch tabs via Ctrl+Tab / Ctrl+Shift+Tab
  * Modify numbers via mouse scroll (when holding ALT)
  * Add auto-complete & call-tips for built-ins
  * Add configurable code snippets
  * Add (line) bookmarks
  * Add jump to error
  * Add support for line/block copy and move
  * Add convenience context menu to tab header
  * Open include<>/use<> files via inline link
  * Fix search highlight for utf8 text
  * Fix display of matches in search field
  * Add Find/Find Next/Find Previous to editor context menu

* Command line
  * Support reading scripts from STDIN
  * Support export to STDOUT
  * Add multi-export on command line
  * Add --animate command line option
  * Add --export-format command line option
  * Allow view-port variables in command-line mode
  * Allow export to .ast, .csg, .echo and .term in preview mode
  * Print statistics in cmdline mode as in GUI

* General
  * Change CGAL Union to join least complex geometries first
  * Install start shortcut for all users on Windows
  * Install icons with defined sizes (e.g. required by flathub)
  * Switch to C++14 and allow usage of header-only CGAL
  * Add support for lib3MF v2.0 API
  * Update AppStream release info

**Bug Fixes**
* Raise error when export folder does not exist (fixes #432)
* Fixes for linear_extrude with scale and/or twist
* Avoid undefined behavior for `convexity' parameter to linear_extrude
* Fix echo() formating error (fixes #2950)
* Fix search order for use<>
* Fix large text (fixes #3262)
* Fix initial editor focus (fixes #3471)
* Fix save error handling (fixes #3582)
* Don't show autocomplete in strings (fixes #3563)
* Don't crash with empty CSG normalizer result (fixes #3085)
* Don't crash if 2D hull fails
* Only enable 30ms joystick timer when needed
* Fix 3MF export to produce correct winding order for internal cavities (fixes #3133)
* Fix parent module handling (fixes #3264)
* Fix search highlight with multi-byte utf8 characters (fixes #3068)
* Make editor visible for new file and examples in launching screen (fixes #3401)
* Handle SCN_URIDROPPED introduced in QScintilla 2.11 (fixes #3536)
* Fix Background Modifier (fixes #2978)
* Use std::string as storage (fixes #3057)
* Setting minimum size of search label (fixes #2962)
* Use a relative import() path for svg viewbox tests
* Fix interpretation of '&' in title bar of undocked widget
* Fix Recent-files handling with '&' in filename (fixes #2988)
* Handle empty first geometry in 2D Minkowski (fixes #3527)
* Fix HiDPI non-integer scaling issue (fixes #3614)
* Fix template action shortcut to not clash with Copy&Paste (fixes #3558)
* Abort Nef conversion early on invalid input, to avoid segfault (fixes #2847)
* Have Reindexer return a const ref instead of a pointer into value array
* Set value after min/max so it's not limited to wrong range (fixes #2995)
* Set keyboard shortcut for "reset translation" as Ctrl+Shift+0 (fixes #3488)
* Ensure experimental features are disabled in release mode (fixes #3517)
* Fix handling of file links with spaces or comma characters (fixes #3609)
* Fix customizer presets (fixes #3522)
* Don't crash customizer with saved vector parameters with >4 values
* Fix crash if non-integer in [max] customizer comment
* Fix warning when using deprecated parameter triangles with polyhedron
* Fix reconnect to Xvfb when running tests in parallel
* Fix dangling pointer in "--export-format"
* Fix root modifier warning
* Fix export format name handling
* Fix handling of editor margins
* Fix icon size and location
* Fix build with Boost 1.72
* Build fixes for older systems (Ubuntu 16.04, Debian 8)
* Add work-around for test failures on MIPS

# OpenSCAD 2019.05

**Language Features:**

* New modules
    * assert() - stop script evaluation on failed constraints
    * let() - scoped assignment
* New functions
    * ord() - convert from character to Unicode code point
    * echo()
    * assert() - stop script evaluation on failed constraints
    * Type testing functions: is_undef(), is_list(), is_num(), is_bool(), is_string()
* New special variable: $preview variable which is set to true in preview mode
* List comprehension updates
   * Added if/else condition
   * Added each keyword
   * Added C-style for loop
   * Now allows looping over characters of a string
* rotate_extrude(): Added angle parameter to 
* import() now supports SVG, 3MF and AMF
* color() now supports hex color codes
* Removed glide() and subdiv() which were never implemented

**Program Features:**

* Customizer: Allow parametrizing design parameters with GUI customization
* Support for using 3D-Mouse / Joystick / Gamepad input devices for controlling the 3D view
* 3D Printing support: Purchase from a print service partner or print to Octoprint
* New export file formats: SVG, 3MF, AMF
* Quick-edit and preview of values using Alt-Arrows (Shift-Alt-Arrows on macOS)
* Added --view cmd-line parameter
* Play sound notification on render complete
* Line numbers and filenames are now shown for many errors and warnings
* Hardwarning preference: Stop on first warning
* Hardwarning and assert now shows a stack trace
* New warnings
    * Module call parameterns don't match module declaration
    * Argument value of of range
    * Duplicate passed argument
    * Children passed to module not accepting children
    * Reference to inknown $special_variables
    * Duplicate assignment
* New translations: Ukrainian, Polish
# OpenSCAD 2015.03

## 2015.03-3

**OS X only: Auto-update vulnerability fix**

## 2015.03-2

**Bugfixes**
* \#1483 - Fix Z-fighting in Ortho view
* \#1479 - No check for infinite $fn
* \#1472 - "nan" in list comprehension causes crash
* \#452 - rands() fails when the seed is a floating point number
* \#1407 - Recursive module crash
* \#1425 - Animate Filename Generation - Duplicate/Missing Filenames
* \#1420 - expression-evaluation-tests fails on arm64
* \#1410 - Crash when a polygon contains NaN
* \#1378 - Linear extrude plus infinite twist causes crash instead of just an error
* \#1358 - Add more detailed installer information including version number
* \#1356 - Crash when multiplying matrices with undefined elements
* \#1350 - Saving file when HD is full ends up in data loss
* \#1342 - Syntax Highlighting Does Not Work on Linux Mint 17.1
* \#1337 - Simple detection of script and direction based on given text
* \#1325 - Crash when polygons with > 3 indices turn out to be degenerate
* \#1329 - version() returned ```[0,0,0]```

## 2015.03-1

**Bugfixes**
* \#1203 - Linux: Missing icons on Xfce
* \#1258 - Occasional crash when exporting STL
* \#1260 - Minimal window width too large
* \#1264 - Replace All sometimes caused a hang
* \#1274 - Fixed some preview bugs on Intel GPUs (OpenCSG 1.4.0)
* \#1276 - Module recursion sometimes caused a crash
* \#1277 - Automatic reload sometimes messed up camera position
* \#1284 - Animation flicker eliminated
* \#1294 - Support reproducible builds
* \#1317 - Normals vectors in STL were sometimes 0 0 0

## 2015.03

**Language Features:**
* Added text() module for 2D text
* Added offset() module for 2D offsets
* Added list comprehensions and let()
* Added concat() function
* Added chr() function
* surface() can now take PNG images as input
* min() and max() can now take a vector argument
* 2D minkowski can now handle polygons with holes
* Variables can now be assigned in local blocks without using assign()

**Program Features:**
* Added Toolbar icons
* New code editor based on QScintilla
* Added Splash screen
* Added SVG export
* Added AMF export
* Added --viewall and --autocenter cmd-line parameters
* GUI is now translated into German, Czech, Spanish, French and Russian
* MDI (Multiple Document Interface) is now available on all platforms
* Color schemes for viewer and editor can be user-edited using JSON files
* GUI components are now dockable
* Added Tickmarks on axes

**Bugfixes/improvements:**
* Performance improvement: 2D (clipper), preview, hull, minkowski, surface
* Performance improvement: Reduce duplicate evaluation of identical expressions
* Better recursion behavior
* STL export and import is now more robust
* Internal cavities are better supported
* New examples
* Windows cmd-line behaves better
* Better mirror() and scale() behavior when using negative factors

**Deprecations:**
* polyhedron() now takes a faces= argument rather than triangles=
* assign() is no longer needed. Local variables can be created in any scope
# OpenSCAD 2014.03

**Language Features:**
* Added diameter argument: circle(d), cylinder(d, d1, d2) and sphere(d)
* Added parent_module() and $parent_modules
* Added children() as a replacement for child()
* Unicode strings (using UTF-8) are now correctly handled
* Ranges can have a negative step value
* Added norm() and cross() functions

**Program Features:**
* Cmd-line: --info parameter prints system/library info
* Cmd-line: --csglimit parameter to change CSG rendering limit
* Cmd-line: Better handling of cmd-line arguments under Windows
* GUI: Added Reset View
* GUI: Added Search&Replace in editor
* GUI: Syntax highlighting now has a dark background theme
* GUI: We now create a backup file before rendering to allow for recovery if OpenSCAD crashes/freezes
* GUI: Accessibility features enabled (e.g. screenreading)

**Bugfixes/improvements:**
* Reading empty STL files sometimes caused a crash
* OPENSCADPATH now uses semicolon as path separator under Windows
* polyhedron() is now much more robust handling almost planar polygons
* Automatic reloads of large designs are more robust
* Boolean logic in if() statements are now correctly short-circuited
* rands() with zero range caused an infinite loop
* resize(, auto=true) didn't work when shrinking objects
* The $children variable sometimes misbehaved due to dynamic scoping
* The --camera cmd-line option behaved differently then the corresponding GUI function
* PNG export now doesn't leak transparency settings into the target image
* Improved performance of 3D hull() operations
* Some editor misbehaviors were fixed
* Stability fixes of CGAL-related crashes
* Windows cmd-line can now handle spaces in filenames
* Default CSG rendering limit is now 100K elements
* Fixed a crash reading DXF files using comma as decimal separator
* Fixed a crash running the cmd-line without a HOME env. variable
* Intersecting something with nothing now correctly results in an empty object

**Deprecations:**
* child() is no longer supported. Use children() instead.
* polyhedron(triangles=[...]): Use polyhedron(faces=[...]) instead.

**Misc:**
* Test framework now shares more code with the GUI app
* Test report can now be automatically uploaded to dinkypage.com
* Better compatibility with BSD systems
* Qt5 support

# OpenSCAD 2013.06

**Language Features:**
* linear_extrude now takes a scale parameter:
  linear_extrude(height=a, slices=b, twist=c, scale=[x,y])
* Recursive use of modules is now supported (including cascading child() operations):
  https://github.com/openscad/openscad/blob/master/examples/example024.scad
* Parameter list values can now depend on earlier values, e.g. for (i=[0:2], j=[0:i]) ..
* value assignments in parameters can now depend on already declared parameters
* Added resize() module: 
  http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#resize

**Program Features:**
* Added basic syntax highlighting in the editor
* There is now a built-in library path in user-space:
  http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Libraries#Library_Locations
* Commandline output to PNG, with various camera and rendering settings.        
  Run openscad -h to see usage info or see the OpenSCAD wiki user manual.
* Attempting to open dxf, off or stl files in the GUI will now create an import statement.
* The preview operator (%) will now preserve any manually set color
* The highlight operator (#) will now color the object in transparent red
* Mac: Added document icon
* Mac: Added auto-update check
* Windows: Better cmd-line support using the openscad.com executable

**Bugfixes:**
* Importing files is now always relative to the importing script, also for libraries
* We didn't always print a warning when CSG normalization created too many elements
* Binary STLs can now be read on big endian architectures
* Some binary STLs couldn't be read
* Fixed some issues related to ARM builds
* CGAL triangulation more lenient- enables partial rendering of 'bad' DXF data
* The Automatic Reload feature is now more robust
* If a file couldn't be saved it no longer fails silently
* Fixed a number of crashes related to CGAL and OpenCSG rendering or complex models
* The lookup() function had bad boundary condition behavior
* The surface() module failed when the .dat file lacked a trailing newline
* The hull() module could crash if any of the children were empty objects
* Some problems using unicode filenames have been fixed

**Misc:**
* Build scripts have been further improved
* Regression test now creates single monolithic .html file for easier uploading
* Regression test auto-starts & stops Xvfb / Xvnc if on headless unix machine
* The backend is finally independent of Qt
* Windows: We now have a 64-bit version

**Known Bugs:**
* Linux: command-line png rendering on Gallium is flaky. 
  Workaround: use CGAL --render or hardware rendering.

# OpenSCAD 2013.01

**Features:**
* Snappier GUI while performing CGAL computations (computations running in separate thread)
* The size of the misc. caches can now be adjusted from Preferences
* The limit for when to disable OpenCSG can now be adjusted from Preferences
* Added Dot product operator: vec * vec
* Added Matrix multiplication operator: vec * mat, mat * mat
* Added search() function
* Dependencies are now tracked - any changes in uses/included files will be detected and cause a recompile
* The OPENSCADPATH environment variable is now implemented will have precedence when searching for libraries
* .csg files can now be opened from the GUI
* linear_extrude() will now assume that the first parameter means 'height' if it's a number

**Bugfixes:**
* use'ing an non-existing file sometimes crashed under Windows
* Better font handling: Ensure a monospace font is chosen as default
* Division by zero caused hang in some cases (e.g. sin(1/0))
* Larger minkowski operations sometimes caused a crash after a CGAL assert was thrown
* Fixed crashes in shared_ptr.hpp (or similar places) due bugs in cache management and CSG normalization
* scale() with a scale factor of zero could cause a crash
* Fixed a number of issues related to use/include
* Providing an unknown parameter on the cmd-line caused a crash
* cmd-line overrides using -D now also work for USEd modules
* Modifier characters can now be used in front of if statements
* rotate() with a vector argument with less that 3 elements used uninitialized variables, ending up being non-deterministic.
* .csg files will now have relative filenames whenever possible
* Don't just ignore geometric nodes having zero volume/area - when doing difference/intersection, they tend to turn negative objects into positive ones.
* Always use utf-8 file encoding, also under Windows
* A lot of build script fixes
* Some other crash bugs fixes

**Deprecations:**
* The old include syntax "<filename.scad>" without the include keyword is no
  longer supported and will cause a syntax error.

# OpenSCAD 2011.12

**Features:**
* The MCAD library is now bundled with OpenSCAD
* Added len() function. Takes one vector or string parameter and returns its length.
* The index operator [] now works on strings
* The version() function will return the OpenSCAD version as a vector, e.g. [2011, 09]
* The version_num() function will return the OpenSCAD version as a number, e.g. 20110923
* hull() Now supports 3D objects
* hull() with 2D object can now use for loops and boolean operations as children
* New import() statement reads the correct file format based on the filename extension
  (.stl, .dxf and .off is supported)
* The color() statement now supports an alpha parameter, e.g. color(c=[1,0,0], alpha=0.4)
* The color() statement now supports specifying colors as strings, e.g. color("Red")
* The color() statement now overrides colors specified further down in the tree
* if()/else() and the ternary operator can now take any value type as parameter. false, 0, empty string and empty vector or illegal value type will evaluate as false, everything else as true.
* Strings can now be lexographically compared using the <, <=, >, >= operators
* Added PI constant.
* Number literals in scientific notation are now accepted by the parser
* Added import and export of the OFF file format
* Now uses standard shortcuts for save, reload and quit on Linux and Windows. F2/F3 will still work but is deprecated.

**Bugfixes:**
* Complex CSG models sometimes took extremely long time to normalize before OpenCSG preview
* square() crashed if any of the dimensions were zero
* Flush Caches didn't flush cached USE'd modules
* STL export should be a bit more robust
* Dropping a file into the editor under Windows didn't work (double C:/C:/ problem)
* On some platforms it was possible to insertion rich text in the editor, causing confusion.
* Less crashes due to CGAL assertions
* OpenCSG should now work on systems with OpenGL 1.x, given that the right extensions are available
* include now searches librarydir
* The $fs parameter yielded only half the number of segments it should have
* surface(center=true) is now correctly centered in the XY plane

**Deprecations:**
* dxf_linear_extrude() and dxf_rotate_extrude() are now deprecated.
  Use linear_extrude() and rotate_extrude() instead.
* The file, layer, origin and scale parameters to linear_extrude() and rotate_extrude()
  are now deprecated. Use an import() child instead.
* import_dxf(), import_stl() and import_off() are now deprecated. Use import() instead.
* When exporting geometry from the cmd-line, use the universal -o option. It will export to the correct file format based on the given suffix (dxf, stl, off). The -x and -s parameters are still working but deprecated.
* F2 and F3 for Save and Reload is now deprecated

# OpenSCAD 2011.06

* Added "Export as Image" menu.

**Bugfixes:**
* Cylinder tesselation broke existing models which are using cylinders
  for e.g. captured nut slots and are dependent on the orientation not
  changing.
* DXF output couldn't be imported into e.g. AutoCAD and Solidworks after updating
  to using the AutoCAD 2000 (AC1015) format. Reverted to the old entity-only output,
  causing LWPOLYLINES to not exported allowed anymore.

# OpenSCAD 2011.04

* Added hull() for convex hulls (2D object only)
* minkowski() now supports 2D objects
* Added functions: rands(), sign()
* Now supports escaping of the following characters in strings: \n, \t, \r, \\, \"
* Support nested includes
* Improved parsing of numbers
* DXF: output LWPOLYLINE instead of just LINE entities
* Bugfixes: More robust DXF export, setting $fs/$fa to 0 caused a crash
* Some bugs fixed, maybe some new bugs added

# OpenSCAD 2010.05

* Added functions and statements
  * Added abs() function
  * Added exp(x), log(b, x), log(x) and ln(x) functions
  * Added minkowski() statement for 3d minkowski sums
* Added 'include <filename>' and 'use <filename>' statements
  * Old implicit '<filename>' include statement is now obsolete
* Some bugs fixed, maybe some new bugs added

# OpenSCAD 2010.02

* Added functions and statements
  * Added sqrt() function
  * Added round(), ceil() and floor() functions
  * Added lookup() function for linear interpolation in value list
  * Added projection(cut = true/false) statement
  * Added child() statement for accessing child nodes of module instances
  * Added mirror() statement
* Improved DXF import code (more entities and some bugs fixed)
* Added feature for dumping animation as PNG files
* Added a preferences dialog
* Now using CGAL's delaunay tesselator
* Now using eigen2 for linear algebra
* Reorganisation of the source tree
* Some bugs fixed, maybe some new bugs added

# OpenSCAD 2010.01

* Added functions and statements
  * Added intersection_for()
  * Added str function
  * Added min and max function
  * Added color() statement
* Added 2D Subsystem
  * New primitives: circle(), square() and polygon()
  * 2D->3D path: linear_extrude() and rotate_extrude()
  * Import of DXF to 2d subsystem: import_dxf()
  * Export of 2D data as DXF files
* Some bugs fixed, maybe some new bugs added
