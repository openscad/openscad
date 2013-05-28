Some work is needed to include these into the automated test suite.
For now, run them manually according to these instructions:

Compile OpenSCAD in debug mode. This will give console output related to module caching, e.g.:
/path/to/used.scad: 0x103612f70
Module cache size: 1 modules

Test1: Basic cache
------

o Open use.scad
o Compile twice (F5) - check that module reference is the same

Test2: Dependency tracking of USE
------

o Open use.scad
o Compile (F5)
o touch used.scad
o Compile (F5) - check that the module reference changed

Test3: MCAD
------

o Open use-mcad.scad
o Compile (F5)
o Check that you get a rounded box

Test4: USE Non-existing file
------

o Open usenonexsistingfile.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open 'use' file 'nofile.scad'.

Test5: Overload USEd module
------

o Open moduleoverload.scad
o Compile (F5)
o Verify that you get a sphere rather than a cylinder

Test6: Recursive USE
------

o Open recursivemain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test7: Circular USE
------

o Open circularmain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test8: Dependency tracking of common file USEd by multiple modules 
------

o Open multiplemain.scad
o Compile (F5) - verify that you get a sphere and a cube of approximately the same size
o Edit multipleB.scad:
  - cube(1.5*F(), center=true);
  + cube(2.5*F(), center=true);
o Reload and Compile (F4) - verify that the cube got larger

Test9: Dependency tracking of file included from module
------

o Open includefrommodule.scad
o Compile (F5) - Verify that you get a circular disc
o Edit radius.scad: Change RADIUS
o Compile (F5) - Verify that the disc changed size

Test10: Circular include
------

o Open circularincludemain.scad
o Compile (F5)
o Verify that OpenSCAD won't hang or crash

Test11: Missing include file appears
------
o rm missing.scad
o Open includemissing.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open include file 'missing.scad'.
o echo "module missing() { sphere(10); }" >  missing.scad
o rm missing.scad
o Reload and Compile (F4) - verify that the sphere is gone

Test12: Missing include file in subpath appears
------
o rm subdir/missingsub.scad
o Open includemissingsub.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open include file 'subdir/missingsub.scad'.
o echo "module missingsub() { sphere(10); }" >  subdir/missingsub.scad
o rm subdir/missingsub.scad
o Reload and Compile (F4) - verify that the sphere is gone

Test13: Missing library file appears
-------
o rm missing.scad
o Open usemissing.scad
o Compile (F5)
o Verify that you get: WARNING: Can't open 'use' file 'missing.scad'.
o echo "module missing() { sphere(10); }" >  missing.scad
o rm missing.scad
o Compile (F5) - verify that the sphere is gone

Test14: Automatic reload of cascading changes
-------

o rm cascade-*.scad
o Open cascadetest.scad
o Turn on Automatic Reload and Compile
o Verify that the 4 objects render correctly
o ./cascade.sh
o Verify that everything reloads at once without flickering
