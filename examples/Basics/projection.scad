echo(version=version());

%import("projection.stl");

// projection() without the cut = true parameter will project
// the outline of the object onto the X/Y plane. The result is
// a 2D shape.

color("red")
    translate([0, 0, -20])
        linear_extrude(height = 2, center = true)
            difference() {
                square(30, center = true);
                projection()
                    import("projection.stl");
            }

color("green")
    rotate([0, 90, 0])
        translate([0, 0, -20])
            linear_extrude(height = 2, center = true)
                difference() {
                    square(30, center = true);
                    projection()
                        rotate([0, 90, 0])
                            import("projection.stl");
                }

color("cyan")
    rotate([-90, 0, 0])
        translate([0, 0, 20])
            linear_extrude(height = 2, center = true)
                difference() {
                    square(30, center = true);
                    projection()
                        rotate([90, 0, 0])
                            import("projection.stl");
                }

// Including the cut = true uses the outline of the cut at
// the X/Y plane.at Z = 0. This can make internal features
// of the model visible.

color("yellow", 0.5)
    translate([0, 0, 20])
        linear_extrude(height = 2, center = true)
            difference() {
                square(30, center = true);
                projection(cut = true)
                    import("projection.stl");
            }



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
