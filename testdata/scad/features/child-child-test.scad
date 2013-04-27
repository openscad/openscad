module up() {
  translate([0,0,1]) child(0);
}

module red() {
  color("Red") child(0);
}

up() cylinder(r=5);
translate([5,0,0]) up() up() cylinder(r=5);
translate([10,0,0]) up() up() up() red() cylinder(r=5);
translate([15,0,0]) red() up() up() up() up() cylinder(r=5);
