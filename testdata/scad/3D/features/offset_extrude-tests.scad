$fn=40;
size = 15;
off = size / 8;

offset_types(size*3, size/2)
angular();

offset_types(size*3, -size/2)
angular();

translate([0,size * 9]) {
    offset_types(size*3, size/2)
    curved();

    offset_types(size*3, -size/2)
    curved();
}


module offset_types(x, h) {
  for(i = [-1,0,1]) { // negative and positive offsets
    translate([i*x, size *-3])
      offset_extrude(h, r = i * off)
        children();
    translate([i*x, size * 0])
      offset_extrude(h, delta = i * off, chamfer = true)
        children();
    translate([i*x, size * 3])
      offset_extrude(h, delta = i * off, chamfer = false)
        children();
  }
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