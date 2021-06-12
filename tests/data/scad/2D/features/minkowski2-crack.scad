/*
  The inner minkowski creates a polygon with a tiny crack in it, due to our
  coincident vertex collapse not being topology-aware.
  The difference and outer minkowski was added to visualize this.
*/
$fn = 64;
minkowski() {
    difference() {
        square(40,center=true);
        minkowski() { circle(2); circle(4); }
    }
    square(1);
}
