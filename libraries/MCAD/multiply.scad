/*
 * Multiplication along certain curves
 *
 * Copyright by Elmo Mäntynen, 2012.
 * Licenced under LGPL2 or later
 */

include <units.scad>

use <utilities.scad>

// TODO check that the axis parameter works as intended
// Duplicate everything $no of times around an $axis, for $angle/360 rounds
module spin(no, angle=360, axis=Z){
    for (i = [1:no]){
        rotate(normalized_axis(axis)*angle*no/i) union(){
            for (i = [0 : $children-1]) child(i);
        }
    }
}

//Doesn't work currently
module duplicate(axis=Z) spin(no=2, axis=axis) child(0);

module linear_multiply(no, separation, axis=Z){
    for (i = [0:no-1]){
        translate(i*separation*axis) child(0);
    }
}
