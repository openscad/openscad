// Empty
intersection_for();
// No children
intersection_for(i=1) { }

intersection_for(i = [[0, 0, 0],
                      [10, 20, 300],
                      [200, 40, 57],
                      [20, 88, 57]])
  rotate(i) cube([100, 20, 20], center = true);
