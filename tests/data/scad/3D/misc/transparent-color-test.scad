// Deliberately contains partially transparent geometry, which is the case the plain
// transparent-background export gets wrong. Used by transparent_compositing_pngtest.py.
color([1, 0, 0, 0.5]) cube(20, center = true);
color([0, 0, 1, 1]) sphere(7);
%cube([30, 4, 4], center = true);
