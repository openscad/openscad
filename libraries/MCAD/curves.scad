// Parametric curves, to be used as paths
// Licensed under the MIT license.
// © 2010 by Elmo Mäntynen
use <math.scad>
include <constants.scad>



/* A circular helix of radius a and pitch 2πb is described by the following parametrisation:
x(t) = a*cos(t),
y(t) = a*sin(t),
z(t) = b*t
*/


function b(pitch) = pitch/(TAU);
function t(pitch, z) = z/b(pitch);

function helix_curve(pitch, radius, z) =
    [radius*cos(deg(t(pitch, z))), radius*sin(deg(t(pitch, z))), z];

