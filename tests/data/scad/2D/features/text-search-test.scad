// fonts test

use <MCAD/fonts.scad>

thisFont=8bit_polyfont();
thisText="OpenSCAD Rocks!";
// Find one letter matches from 2nd column (index 1)
theseIndicies=search(thisText,thisFont[2],1,1);
// Letter spacing, x direction.
x_shift=thisFont[0][0];
y_shift=thisFont[0][1];
echo(theseIndicies);
// Simple polygon usage.
for(i=[0:len(theseIndicies)-1]) translate([i*x_shift-len(theseIndicies)*x_shift/2,0]) {
  polygon(points=thisFont[2][theseIndicies[i]][6][0],paths=thisFont[2][theseIndicies[i]][6][1]);
}

theseIndicies2=search("ABC",thisFont[2],1,1);
// outline_2d() example
for(i=[0:len(theseIndicies2)-1]) translate([i*x_shift-len(theseIndicies2)*x_shift,-y_shift]) {
  outline_2d(outline=true,points=thisFont[2][theseIndicies2[i]][6][0],paths=thisFont[2][theseIndicies2[i]][6][1],width=0.25);
}

theseIndicies3=search("123",thisFont[2],1,1);
// bold_2d() outline_2d(false) example
for(i=[0:len(theseIndicies3)-1]) translate([i*x_shift,-2*y_shift]) {
  bold_2d(bold=true,width=0.25,resolution=8)
    outline_2d(false,thisFont[2][theseIndicies3[i]][6][0],thisFont[2][theseIndicies3[i]][6][1]);
}