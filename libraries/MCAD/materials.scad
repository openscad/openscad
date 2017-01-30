/*
 * Material colors.
 * 
 * Originally by Hans Häggström, 2010.
 * Dual licenced under Creative Commons Attribution-Share Alike 3.0 and LGPL2 or later
 */

// Material colors
Oak = [0.65, 0.5, 0.4];
Pine = [0.85, 0.7, 0.45];
Birch = [0.9, 0.8, 0.6];
FiberBoard = [0.7, 0.67, 0.6];
BlackPaint = [0.2, 0.2, 0.2];
Iron = [0.36, 0.33, 0.33];
Steel = [0.65, 0.67, 0.72];
Stainless = [0.45, 0.43, 0.5];
Aluminum = [0.77, 0.77, 0.8];
Brass = [0.88, 0.78, 0.5];
Transparent = [1, 1, 1, 0.2];

// Example, uncomment to view
//color_demo();

module color_demo(){
    // Wood
    colorTest(Oak, 0, 0);
    colorTest(Pine, 1, 0);
    colorTest(Birch, 2, 0);

    // Metals
    colorTest(Iron, 0, 1);
    colorTest(Steel, 1, 1);
    colorTest(Stainless, 2, 1);
    colorTest(Aluminum, 3, 1);

    // Mixboards
    colorTest(FiberBoard, 0, 2);

    // Paints
    colorTest(BlackPaint, 0, 3);
}

module colorTest(col, row=0, c=0) {
  color(col) translate([row * 30,c*30,0]) sphere(r=10);
}
