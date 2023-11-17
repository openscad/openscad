// Hull Example
// The hull of a toy sailboat
$fs = .1;
hull(){
    translate([0,30,2.5]) sphere(1); //bow
    translate([0,10,-1]) cube([20,1,8],true); //1st bulkhead
    translate([0,10,-5]) cube([1,1,8],true); //1st keel
    translate([0,-8,-1]) cube([20,1,8],true); //2nd bulkhead
    translate([0,-8,-5]) cube([1,1,8],true); //2nd keel
    translate([0,-30,1]) cube([16,1,4],true); //stern
}
    cylinder(40,1,1); //Mast


// written by Paul Young, 2022
//  paulwhy.2@gmail.com

// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.