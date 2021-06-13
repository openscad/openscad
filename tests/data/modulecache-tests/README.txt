Some work is needed to include these into the automated test suite.
For now, run them manually according to these instructions:

Compile OpenSCAD in debug mode and always run OpenSCAD from the cmd-line using the argument --debug=FileModule,ModuleCache.
This will give console output related to module caching, e.g.:
    FileModule: /path/to/used.scad: 0x103612f70

Test1: Basic cache
------

o Turn off Design->Automatic Reload and Compile
o Open use.scad
o Compile twice (F5) - check that module reference is the same and that ModuleCache only says "Compiling library: /path/to/used.scad" once.
o touch used.scad
o Compile (F5) - check that ModuleCache says "Recompiling cached library: /path/to/used.scad".

Test2: MCAD
------

o Open use-mcad.scad
o Compile (F5)
o Check that you get a rounded box

Test3: Overload USEd module
------

o Open moduleoverload.scad
o Compile (F5)
o Verify that you get a sphere rather than a cylinder

Test4: Recursive USE
------

o Open recursivemain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test5: Circular USE
------

o Open circularmain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test6: Dependency tracking of common file USEd by multiple modules
------

o Open multiplemain.scad
o Compile (F5) - verify that you get a sphere and a cube of approximately the same size
o Edit multipleB.scad:
  - cube(1.5*F(), center=true);
  + cube(2.5*F(), center=true);
o Reload and Compile (F4) - verify that the cube got larger
o Edit multiplecommon.scad:
  - function F() = 20;
  + function F() = 10;
o Reload and Compile (F4) - verify that the sphere and the cube both got smaller

Test7: Dependency tracking of file included from module
------

o Open includefrommodule.scad
o Compile (F5) - Verify that you get a circular disc
o Edit radius.scad: Change RADIUS
o Compile (F5) - Verify that the disc changed size

Test8: Circular include
------

o Open circularincludemain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test9: Missing include file appears
------
o rm missing.scad
o Open includemissing.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open include file 'missing.scad'.
o echo "module missing() { sphere(10); }" >  missing.scad
o Reload and Compile (F4) - verify that the sphere appeared
o rm missing.scad
o Reload and Compile (F4) - verify that the sphere is still there
o echo "module missing() { sphere(20); }" >  missing.scad
o Reload and Compile (F4) - verify that the sphere increased in size

Test10: Missing include file in subpath appears
------
o rm subdir/missingsub.scad
o Open includemissingsub.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open include file 'subdir/missingsub.scad'.
o echo "module missingsub() { sphere(10); }" >  subdir/missingsub.scad
o Reload and Compile (F4) - verify that the sphere appeared
o rm subdir/missingsub.scad
o Reload and Compile (F4) - verify that the sphere is still there
o echo "module missingsub() { sphere(20); }" >  subdir/missingsub.scad
o Reload and Compile (F4) - verify that the sphere increased in size

Test11: Missing library file appears
-------
o rm missing.scad
o Open usemissing.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open library file 'missing.scad'.
o echo "module missing() { sphere(10); }" >  missing.scad
o Reload and Compile (F4) - verify that the sphere appeared
o rm missing.scad
o Reload and Compile (F4) - verify that the sphere is still there
o echo "module missing() { sphere(20); }" >  missing.scad
o Reload and Compile (F4) - verify that the sphere increased in size

Test12: Automatic reload of cascading changes
-------

o ./cascade.sh
o Open cascadetest.scad
o Turn on Automatic Reload and Compile
o Verify that the 4 objects render correctly
o rm cascadetest.scad
o Verify that no rerendering was triggered (the 4 objects are still there)
o rm cascade*.scad
o Verify that no rerendering was triggered (the 4 objects are still there)
o ./cascade2.sh
o Verify that everything reloads at once without flickering

Test 13: Correct handling of compile errors in auto-reloaded modules
--------
o Turn on Automatic Reload and Compile
o Open mainusingerror.scad
o Verify that you get:
  - ERROR: Parser error in file ".../error.scad", line 3: syntax error
  - Execution aborted
o Verify that the above doesn't repeat

Test 14: Dependency tracking of underlying dependencies
--------
o Turn on Automatic Reload and Compile
o Open mainsubsub.scad
o Verify that you see a red cylinder
o edit subdir/subsub.scad: Change color
o Verify that color changes

Test 15: Dependency tracking with two open files
--------
o Turn on Automatic Reload and Compile
o Open these 3 files: main-use-include.scad used.scad included.scad
o Verify that you see 1) A green cube and sphere 2) A green sphere 3) a green cube
o In an external editor, edit used.scad: Change color
o Verify that color of the sphere changed also in main-use-include
o In an external editor, edit included.scad: Change color
o Verify that color of the cube changed also in main-use-include

Test 16: Correct auto-reload of errors in includes from main
--------
o Turn on Automatic Reload and Compile
o Open mainincludingerror.scad
o Verify that you get:
  - ERROR: Parser error in file ".../error.scad", line 3: syntax error
  - Execution aborted
o edit error.scad: fix the syntax error (add semicolon)
o Verify that you now see a cube

Test 17: Correct auto-reload of errors in includes from sub modules
--------
o Turn on Automatic Reload and Compile
o Open mainuseincludingerror.scad
o Verify that you get:
  - ERROR: Parser error in file ".../error.scad", line 3: syntax error
  - Execution aborted
o edit error.scad: fix the syntax error (add semicolon)
o Verify that you now see a cube
