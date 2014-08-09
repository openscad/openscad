// Six-Sided Spindle Die
// By Laura Bailey August 8, 2014
// CC BY-SA license


echo(version=version());


module spindle_die() {
  hull() {
    cylinder($fn=6, r1=25, r2=1, h=25);
    rotate([180]) translate([0,0,40]) cylinder($fn=6, r1=25, r2=1, h=25);
    }
  }


module die_numbers() {
  // one
  rotate([90,0,0]) translate([0,-20,20]) sphere(5);

  // two
  rotate([90,0,0]) translate([5,-30,-20]) sphere(4);
  rotate([90,0,0]) translate([-5,-10,-20]) sphere(4);

  // three
  rotate([90,0,60]) translate([-5,-10,20]) sphere(4);
  rotate([90,0,60]) translate([0,-20,20]) sphere(4);
  rotate([90,0,60]) translate([5,-30,20]) sphere(4);

  // four
  rotate([90,0,60]) translate([-5,-10,-20]) sphere(4);
  rotate([90,0,60]) translate([5,-30,-20]) sphere(4);
  rotate([90,0,60]) translate([5,-10,-20]) sphere(4);
  rotate([90,0,60]) translate([-5,-30,-20]) sphere(4);

  // five
  rotate([90,0,120]) translate([-5,-10,20]) sphere(4);
  rotate([90,0,120]) translate([5,-30,20]) sphere(4);
  rotate([90,0,120]) translate([5,-10,20]) sphere(4);
  rotate([90,0,120]) translate([-5,-30,20]) sphere(4);
  rotate([90,0,120]) translate([0,-20,20]) sphere(4);

  // six
  rotate([90,0,120]) translate([-5,-10,-20]) sphere(4);
  rotate([90,0,120]) translate([5,-30,-20]) sphere(4);
  rotate([90,0,120]) translate([5,-10,-20]) sphere(4);
  rotate([90,0,120]) translate([-5,-30,-20]) sphere(4);
  rotate([90,0,120]) translate([5,-20,-20]) sphere(4);
  rotate([90,0,120]) translate([-5,-20,-20]) sphere(4);
  }


difference() {
  spindle_die();
  die_numbers();
  }