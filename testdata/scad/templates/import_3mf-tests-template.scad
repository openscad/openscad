translate([4,0,0]) import("import.3mf");
translate([0,4,0]) import("@CMAKE_SOURCE_DIR@/../testdata/scad/3D/features/import.3mf");

translate([4,4,0]) {
    difference() {
        import("not-found.3mf");
        cube([1,1,4], center=true);
    }
}
