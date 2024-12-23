**Highlights**

* New geometry engine: Manifold - F6 rendering is now orders of magnitude faster
* Significantly improved external file import, with built-in mesh repair
* Color support in F6 rendering, and color import/export for OFF and 3MF

**Language Features**

* New module: textmetrics() #3684
* New module: fill() #4348
* Added v= and segments= parameters to linear_extrude() #3770 #5080
* Improved SVG import
  * Support $fn, $fs, $fa #3994
  * TODO: Just for SVG, how is this enforced? Added id= and layer= parameters to SVG import() #4042 #4378
* JSON import() #3891
* Vector swizzle feature #4010

* TODO:
  * Inspect #3745 more closely

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

* GUI
  * Double-click to center now works better #3661
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
  * Rendering performance improvement through use of VBOs
  * Adding interactive measurement abilities to OpenSCAD #4813
  * Qt6 now supported
  * Can send rendered geometry to a local slicer instance #5299
  * Add support for multiple 3D print services #5357
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


* Cmd-line
  * --enable-all #3874
  * --summary all|camera|cache|time|geometry|bounding-box #3877
  * --summary-file filename|-
  * Linux: Can render files to PNG without X11/GLX
  * Rename --render=cgal to --render=force to force-convert to the current backend-specific geometry #4822
  * Set $vpr, $vpt, $vpd, $vpf when run from command line. #4863
  * Add --backend setting / flag to enable manifold in prod builds #5235
  * --animate_sharding=<shard>/<num_shards> #5473

* Bugfixes (TODO: Should we list bug ticket rather than PR?)
  * Relative includes could resolve to the wrong path #569
  * $ variables as module parameters were evaluated wrongly #1968
  * dxf_dim() and dxf_cross() sometimes didn't locate file correctly #2768
  * positional parameter silently overrides previous named parameter #3672
  * Passing children to modules that don't accept them now issues a warning #3759
  * modifiers not working properly with children() #1529
  * Can't pass special vars to children #2104
  * Allow trailing comma in let() or lists #2882


**Deprecations:**
* OpenGL >= 2.1 is now required 
* AMF import/export is deprecated

**Misc:**

Stats:
* ~800 pull requests merged

