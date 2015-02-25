echo(version=version());

module LetterBlock(letter, size=30) {
    difference() {
        translate([0,0,size/4])
          cube([size,size,size/2], center=true);
        translate([0,0,size/6])
            linear_extrude(height=size, convexity=3)
                text(letter, 
                     size=size*22/30,
                     font="Tahoma",
                     halign="center",
                     valign="center");
    }
}

LetterBlock("M");

// Written by Marius Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
