// Test various imports of files with non-ASCII names.
// Note that the resulting image is actually of no interest.
// However, any interesting errors in these imports happen
// during geometry processing, so we can't just be an
// echo, csg, or ast test - we have to actually do the
// geometry processing.
import("../../dxf/☠-2D.dxf");
import("../../stl/☠-3D.stl");
import("../../off/☠-3D.off");
import("../../obj/☠-3D.obj");

// These fail because Windows fopen() can't handle UTF-8 paths.
surface("../../image/☠.dat");
surface("../../image/☠-2D.png");

// This (or another test) should test include<>, use<*.scad>, and use<*.ttf>.
// They all fail in various ways.
