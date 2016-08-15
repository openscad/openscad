// First example of parameteric model
//   
//    syntax: 
//        //Description
//        variable=value; //Parameter
//        
//        This type of comment tells the name of group to which parameters below
//        this comment will belong 
//    
//       /*[ group name ]*/ 
//


//Below comment tells the group to which a varaible will belong
/*[ properties of Sign]*/

//The resolution of the curves. Higher values give smoother curves but may increase the model render time.
resolution = 10; //[10, 20, 30, 50, 100]

//The horizontal radius of the outer ellipse of the sign.
radius = 80;//[60 : 200]

//Total height of the sign
height = 2;//[1 : 10]

/*[ Content To be written ] */

//Message to be write 
Message = "Welcome to..."; //["Welcome to...", "Happy Birthday!", "Happy Anniversary", "Congratulations", "Thank You"]

//Name of Person, company etc.
To = "Parametric Designs";

$fn = resolution;

scale([1, 0.5]) difference() {
    cylinder(r = radius, h = 2 * height, center = true);
    translate([0, 0, height])
        cylinder(r = radius - 10, h = height + 1, center = true);
}
linear_extrude(height = height) {
    translate([0, --4]) text(Message, halign = "center");
    translate([0, -16]) text(To, halign = "center");
}

// Written by Amarjeet Singh Kapoor <amarjeet.kapoor1@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.