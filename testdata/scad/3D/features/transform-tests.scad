module mycyl() {
  cylinder(r1=10, r2=0, h=20);
}

translate([25,0,0]) scale([1,2,0.5]) mycyl();
translate([20,-30,0]) scale(0.5) mycyl();
translate([0,-20,0]) rotate([90,0,0]) mycyl();
translate([0,-40,0]) rotate([90,0,45]) mycyl();
rotate(v=[-1,0,0], a=45) mycyl();
multmatrix([[1,0,0,-25],
            [0,1,0,0],
            [0,0,1,0],
            [0,0,0,1]]) mycyl();
multmatrix([[1,0.4,0.1,-25],
            [0.4,0.8,0,-25],
            [0.2,0.2,0.5,0],
            [0,0,0,1]]) mycyl();
translate([-25,-40,0])
  multmatrix([[1,0,0,0],
              [0,1,0,0],
              [0,0,1,0],
              [0,0,0,2]]) mycyl();
