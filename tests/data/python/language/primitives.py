# Sample python script to check *most* available python bindings and their
# parameters

fn=20

cube1 = cube([3,2,1],center=True)
cube2 = cube(1.5).rotate([10,20,30])
cube3 = cube([5,1,1],center=True)
sphere1 = sphere(r=2,fn=6)
sphere2 = sphere(d=4,fn=20)
cylinder1 = cylinder(r=2,h=4)
cylinder2 = cylinder(r=1,h=9,center=True)

output([
        cube1.translate([0,1,0]),
        cube2.translate([4,0,0]),
        sphere1.translate([0,5,0]),
        sphere2.translate([4,5,0]),
        ((cylinder1 | cube3) -cylinder2).translate([0,10,0]),
    ])

