import_stl("import.stl");
translate([2,0,0]) import("import.stl");
translate([4,0,0]) import("import_bin.stl");
translate([0,2,0]) import("@CMAKE_SOURCE_DIR@/../testdata/scad/features/import.stl");
