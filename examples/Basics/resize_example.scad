// Example demonstrating the resize() function
// This example shows how to resize a sphere to specific dimensions

// Create a sphere with radius 10mm
sphere(r = 10);

// Resize the sphere to 30mm in X, 60mm in Y, and 10mm in Z
// The original sphere is 20mm in each dimension (diameter = 2*radius)
// After resize, it will be exactly 30x60x10mm
resize(newsize = [30, 60, 10])
    sphere(r = 10);

// You can also resize other objects
// Create a cube of 20x20x20mm
cube(size = 20, center = true);

// Resize the cube to 50x30x40mm
translate([60, 0, 0])
    resize(newsize = [50, 30, 40])
        cube(size = 20, center = true);

// Example with a cylinder
// Cylinder with diameter 20mm and height 30mm
cylinder(d = 20, h = 30, center = true);

// Resize it to 40mm diameter and 50mm height
translate([120, 0, 0])
    resize(newsize = [40, 40, 50])
        cylinder(d = 20, h = 30, center = true);
