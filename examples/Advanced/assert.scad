echo(version=version());

function size(x) = assert(x % 2 == 0, "Size must be an even number") x;

module ring(r = 10, cnt = 3, s = 6) {
    assert(r >= 10, "Parameter r must be >= 10");
    assert(cnt >= 3 && cnt <= 20, "Parameter cnt must be between 3 and 20 (inclusive");
    for (a = [0 : cnt - 1]) {
        rotate(a * 360 / cnt) translate([r, 0, 0]) cube(size(s), center = true);
    }
}

// ring(5, 5, 4); // trigger assertion for parameter r

// ring(10, 2, 4); // trigger assertion for parameter cnt

// ring(10, 3, 5); // trigger assertion in function size()

color("red") ring(10, 3, 4);
color("green") ring(25, 9, 6);
color("blue") ring(40, 20, 8);

// Written in 2018 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
