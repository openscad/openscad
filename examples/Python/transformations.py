from openscad import *

c=cube([1,2,2]) + [10,0,1] # 10 to the right, 1 up

s=sphere(r=3) * [1,1,0.1]  # squeeze it to 10% in z

cyl = cylinder(r=1,h=5)

show( [ #several objects at once  
       s.rotx(45) , # rot on the x axis
       c + [4,0,1], # 10 to the right, 1 up
       cyl.translate([0,10,0]).color("green") 
       ])



