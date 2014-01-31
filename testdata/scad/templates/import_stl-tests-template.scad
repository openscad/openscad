import_stl("import.stl");
translate([2,0,0]) import("import.stl");
translate([4,0,0]) import("import_bin.stl");
// Test binary STLs which happen to start with the string "solid"
translate([0,4,0]) import("import_bin_solid.stl");
translate([0,2,0]) import("@CMAKE_SOURCE_DIR@/../testdata/scad/features/import.stl");
translate([2,2,1]) import("../../stl/import_distant.stl",center=true);
import("../../stl/import_empty.stl");

translate([2,2,0]) {
    difference() {
        import("not-found.stl");
        cube([1,1,4], center=true);
    }
}
