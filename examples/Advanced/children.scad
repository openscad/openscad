// children.scad - Usage of children()

// The use of children() allows to write generic modules that
// modify child modules regardless of how the child geometry
// is created.

color("red")
    make_ring_of(radius = 15, count = 6)
        cube(8, center = true);

color("green")
    make_ring_of(radius = 30, count = 12)
        difference() {
            sphere(5);
            cylinder(r = 2, h = 12, center = true);
        }

color("cyan")
    make_ring_of(radius = 50, count = 4)
        something();

module make_ring_of(radius, count)
{
    for (a = [0 : count - 1]) {
        angle = a * 360 / count;
        translate(radius * [sin(angle), -cos(angle), 0])
            rotate([0, 0, angle])
                children();
    }
}

module something()
{
    cube(10, center = true);
    cylinder(r = 2, h = 12, $fn = 40);
    translate([0, 0, 12])
        rotate([90, 0, 0])
            linear_extrude(height = 2, center = true)
                text("SCAD", 8, halign = "center");
    translate([0, 0, 12])
        cube([22, 1.6, 0.4], center = true);
}

echo(version=version());
// Written in 2015 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
