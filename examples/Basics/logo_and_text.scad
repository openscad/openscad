// logo_and_text.scad - Example for use<> and text()

use <logo.scad> // Imports the Logo() module from logo.scad into this namespace

// Set the initial viewport parameters
$vpr = [90, 0, 0];
$vpt = [300, 0, 80];
$vpd = 1600;

logosize = 120;

translate([110, 0, 80]) {
  translate([0, 0, 30]) rotate([25, 25, -40]) Logo(logosize);
  translate([100, 0, 40])  green() t("Open", s = 42, spacing = 1.05);
  translate([247, 0, 40])  corn()  t("SCAD" , s = 42, spacing = 0.9);
  translate([100, 0, 0])   black() t("The Programmers");
  translate([160, 0, -30]) black() t("Solid 3D CAD Modeller");
}

// Helper to create 3D text with correct font and orientation
module t(t, s = 18, style = ":style=Bold", spacing = 1) {
  rotate([90, 0, 0])
    linear_extrude(height = 1)
      text(t, size = s,
           spacing=spacing,
           font = str("Liberation Sans", style),
           $fn = 16);
}

// Color helpers
module green() color([157/255,203/255,81/255]) children();
module corn() color([249/255,210/255,44/255]) children();
module black() color([0, 0, 0]) children();

echo(version=version());
// Written in 2014 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
