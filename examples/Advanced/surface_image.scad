echo(version=version());

// surface() can import images, the pixel values are converted
// to grayscale and converted to values between 0 and 100.
// The example takes 3 cuts from the height map and displays
// those as 3 stacked layers.

for (a = [1, 2, 3])
    color([a/6 + 0.5, 0, 0])
       linear_extrude(height = 2 * a, convexity = 10)
            projection(cut = true)
                translate([0, 0, -30 * a])
                    surface("surface_image.png", center = true);



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
