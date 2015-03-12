// children_indexed.scad - Usage of indexed children()

// children() with a parameter allows access to a specific child
// object with children(0) being the first one. In addition the
// $children variable is automatically set to the number of child
// objects.

color("red")
    translate([-100, -20, 0])
        align_in_grid_and_add_text();

color("yellow")
    translate([-50, -20, 0])
        align_in_grid_and_add_text() {
            cube(5, center = true);
        }

color("cyan")
    translate([0, -20, 0])
        align_in_grid_and_add_text() {
            cube(5, center = true);
            sphere(4);
        }

color("green")
    translate([50, -20, 0])
        align_in_grid_and_add_text() {
            cube(5, center = true);
            sphere(4);
            cylinder(r = 4, h = 5);
        }


module align_in_grid_and_add_text()
{
    if ($children == 0) {
        linear_extrude(height = 1, center = true)
          text("Nothing...", 6, halign = "center");
    } else {
        t = $children == 1 ? "one object" : str($children, " objects ");
        linear_extrude(height = 1, center = true)
          text(t, 6, halign = "center");

        for (y = [0 : $children - 1])
            for (x = [0 : $children - 1])
                translate([15 * (x - ($children - 1) / 2), 20 * y + 40, 0])
                    scale(1 + x / $children)
                        children(y);
    }
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
