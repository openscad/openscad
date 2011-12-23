//
// Bug description: The intersection results in an empty object.
// Cause: The rotated bounding box is wrongly calculated, yielding a
// box which don't overlap with the bounding box of the second object.
//
intersection() {
  rotate(45) cube(10);
  translate([3,2,0]) cube(10);
}
