for (x = [0:1]) {
  for (y = [0:1]) {
    translate ([(x*51),0,(y*31)]) {
      difference () {
        cube ([55,35,35]);
        translate ([5,-1,5]) {
          cube ([45,31,25]);
        }
      }
      translate ([27.5,15,7]) {
        difference () {
          sphere (d=29, $fn=8);
          translate([0,0,-20]) {
            cube (32,center=true);
          }
        }
      }
    }
  }
}