importfile = "../../../stl/nonmanifold.stl";
projection() import(importfile);
translate([40,0]) projection(cut=true) import(importfile);
