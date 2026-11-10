**Highlights**

* New geometry engine: Manifold, now the default backend - rendering (F6) is now orders of magnitude faster #5833
* Color support in F6 rendering, and color import/export for OFF and 3MF
* Significantly improved external file import, with built-in mesh repair

**Breaking Changes**

Existing designs may produce errors or different results:

* surface() now preserves the full precision of 16-bit PNG heightmaps instead of truncating them to 8 bits #6950
* The removals listed under Removed below

**New Warnings**

These don't change the result, but stop rendering if warnings are made fatal (--hardwarnings or "Stop on first warning"):

* A malformed escape sequence in a string #6099
* offset() with both r= and delta= specified #6348
* Passing children to modules that don't accept them #3759

**Language Features**

* Hex constants using 0x prefix #4833
* Bitwise and shift operators: ~, |, &, <<, >> #4833
* New modules:
  * fill() #4348
* New functions:
  * textmetrics() #3684
  * fontmetrics() #3684
* text(), textmetrics(), fontmetrics()
  * Added em= to set the size of the em-square #6656
* linear_extrude()
  * Added v= and segments= parameters #3770 #5080
  * h= is an alias for height= #5572
* rotate_extrude()
  * Added start= to specify start angle #5553
  * a= is an alias for angle= #5572
  * When specifying an angle, rotation starts at angle=0 instead of angle=180 #5553
* import()
  * Added center= parameter #5671
  * Improved SVG import
    * Support $fn, $fs, $fa #3994
    * Added id= and layer= parameters #4042 #4378
* Non-ASCII identifiers following Unicode Standard Annex 31 #3736 #6968 #7101

* Bugfixes
  * Relative includes could resolve to the wrong path #569
  * $ variables as module parameters were evaluated wrongly #1968
  * dxf_dim() and dxf_cross() sometimes didn't locate file correctly #2768
  * positional parameter silently overrides previous named parameter #3672
  * modifiers not working properly with children() #1529
  * Can't pass special vars to children #2104
  * Allow trailing comma in let() or lists #2882

**Program Features**

* General
  * Hardwarnings are better behaved with fewer false positives #3660 #3743
  * Better behavior for deep recursions #3671 #3730

* Geometry
  * 2D precision issues #3794 
  * Round up number of fragments in rotate_extrude #4540

* Misc
  * Added WASM build #4128
  * Memory leaks #3174 addressed by adding garbage collection #3765
  * macOS binaries are now signed and notarized #5420

* File I/O
  * Add support for defs/use in svg files. #3848
  * Export customizer .param file. #3864
  * dxf export as polylines #3869
  * Better support for Relative Paths in CSG output #128
  * SVG import: Respect "display: none" #4041
  * SVG scaling wrong with width/height given as percentage #4096
  * VRML export #3883
  * OBJ import and export #4197 
  * PovRay export, with color support #5288
  * SVG export options for fill and stroke #6057
  * PDF export options for fill and stroke #5863
  * Security improvements to SVG import #7180

* GUI
  * Edit call-tips fixes #3744
  * Editor improvements #3749
  * Auto-complete improvements #3901
  * Turkish translation #3992
  * Add option to configure 2D/3D toolbar actions. #4007
  * upgrade syntax coloring #4026
  * Add action to open folder of the current tab. #4038
  * adding support for gradient backgrounds #4167
  * improved animation UI #4237 #4245
  * Added viewport control widget #4243
  * Axis rendering improved #3875 #5247
  * File->Save a Copy #4392
  * Georgian translation  #4519
  * PDF export dialog #4537
  * 3MF export dialog #5577
  * Rendering performance improvement through use of VBOs
  * Adding interactive measurement abilities to OpenSCAD #4813
  * Qt6 now supported
  * Can send rendered geometry to a local slicer instance #5299
  * Add support for multiple 3D print services #5357
  * Cleaned up Window menu and added Ctrl-J Jump To #5630
  * Color list window #6488
  * Autocomplete for user-defined variables #6846
  * "Close all but this tab" action #6659
  * Nord color schemes #7033
  * Indicator in the GUI when the CGAL backend is active #7115
  * Translations: Swedish #5978, Brazilian Portuguese #5813, Galician #7121
  * Remote print services can be enabled or disabled #5600
  * Customizer
    * Major rework #3755
    * Better behavior when multiple tabs open
    * Removed support for editing large vectors (usability issue)
    * Negative numbers now behave correctly #3773
    * Sliders work better #3778
  * Preferences
    * Console font configurable #3623
    * Make customizer font and size selectable. #4946
    * add auto clear console option to preferences #4228
    * Add Option to Swap Mouse Buttons on 3DView #4241
    * Configurable mouse behaviour for 3D navigation #5874
    * Configurable application font #6064


* Cmd-line
  * --enable-all #3874
  * --summary all|camera|cache|time|geometry|bounding-box #3877
  * --summary-file filename|-
  * Linux: Can render files to PNG without X11/GLX
  * Rename --render=cgal to --render=force to force-convert to the current backend-specific geometry #4822
  * Set $vpr, $vpt, $vpd, $vpf when run from command line. #4863
  * Add --backend setting / flag to enable manifold in prod builds #5235
  * --animate-sharding=<shard>/<num_shards> #5473 #7203
  * Add -O export option parameter #5577
  * --reset-window-settings to reset window placement and fonts #6065


**Removed:**

No longer supported. Most of these were deprecated in earlier releases:

* Imported files (e.g. via import() or surface()) are no longer looked up in the document root as a fallback; paths are resolved relative to the file that references them #6997
* Ranges on the form [begin:end] with begin value greater than the end value now yield an empty range #5750
* AMF import/export #6988
* Support for OpenGL < 2.1
* dxf_linear_extrude(), dxf_rotate_extrude(), import_dxf(), import_stl() and import_off() #5559
* The -s and -x cmd-line options #5733
* The child() and assign() modules #6428
* The triangles= parameter to polyhedron() #6825

**Deprecations:**

* Identifiers (variable, function and module names) starting with a digit are deprecated (but still allowed for the time being, except when conflicting with hex constants) #4833

**Misc:**

Stats:
* ~1,300 pull requests merged
