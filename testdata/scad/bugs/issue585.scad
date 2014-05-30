// Somehow the 2D union/tessellation algorithm doesn't support touching polygons
// Changing translate([-10,-10,0]) to translate([-9.99,-9.99,0]) works
// Fixed after 2014.03
square([10,10]);
translate([-10,-10,0]) square([10,10]);
