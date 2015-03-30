font = "Liberation Sans";
// Nicer, but not generally installed:
// font = "Bank Gothic";

module G() offset(0.3) text("G", size=10, halign="center", valign="center", font = font);
module E() offset(0.3) text("E", size=10, halign="center", valign="center", font = font);
module B() offset(0.5) text("B", size=10, halign="center", valign="center", font = font);

$fn=64;

module GEB() {
intersection() {
    linear_extrude(height = 20, convexity = 3, center=true) B();
    
    rotate([90, 0, 0])
      linear_extrude(height = 20, convexity = 3, center=true) E();
    
    rotate([90, 0, 90])
      linear_extrude(height = 20, convexity = 3, center=true) G();
  }
}

color("Ivory") GEB();

color("MediumOrchid") 
  translate([0,0,-20])
    linear_extrude(1) 
      difference() {
        square(40, center=true);
        projection() GEB();
      }

color("DarkMagenta")
  rotate([90,0,0]) 
    translate([0,0,-20])
      linear_extrude(1) 
        difference() {
          translate([0,0.5]) square([40,39], center=true);
          projection() rotate([-90,0,0]) GEB();
        }

color("MediumSlateBlue")
  rotate([90,0,90]) 
    translate([0,0,-20])
      linear_extrude(1)
        difference() {
          translate([-0.5,0.5]) square([39,39], center=true);
          projection() rotate([0,-90,-90]) GEB();
        }

echo(version=version());
// Written in 2015 by Marius Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
