hole = [[9,-9],[9,9],[-9,9],[9,0],[-9,-9]];

module Shape() {
  difference() {
    square(20,center=true);
    polygon(hole);
  }
}

Shape();

translate([30,0]) projection(cut=false) linear_extrude() Shape();
