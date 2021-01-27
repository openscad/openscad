// transformation matrices should consist of all -1, 0, and 1 values
for(mx=[0,1],my=[0,1],mz=[0,1]) {
  m = [mx,my,mz];
  if (m == [1,1,1]) scale(-1) cube(); // can't mirror all 3 axes with single mirror operation
  else mirror(m) cube();
}
