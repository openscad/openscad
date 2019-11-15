points=600;
random_angles=rands(0,360,points,42);
 
hull()
  for(i=[0:3:points-1])
    rotate([random_angles[i],random_angles[i+1],random_angles[i+2]]) translate([10,0,0]) cube();
