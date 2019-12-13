$fn=40;
size = 15;
off = 0;

offset_types(size*3, size/2)
angular();

offset_types(size*3, -size/2)
angular();

translate([size * 3, 0]) {
    offset_types(size*3, size/2)
    curved();

    offset_types(size*3, -size/2)
    curved();
}


module offset_types(x, h) {
    translate([x, size *-3])
      offset_extrude(h, r = off)
        children();
    translate([x, size * 0])
      offset_extrude(h, delta = off, chamfer = true)
        children();
    translate([x, size * 3])
      offset_extrude(h, delta = off, chamfer = false)
        children();
}

module curved() {
  difference() {
    circle(size / 2);
    translate([0, -size / 2]) circle(size / 2);
  }
}

module angular() {
  difference() {
    square(size, center = true);
    rotate(-135) square(size);
  }
}