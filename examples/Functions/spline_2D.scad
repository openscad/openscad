// spline_2D:  Basic splining examples.
// Draw a smooth hour glass needing only 3 points to specify a cubic spline
// ...or 4 points for a modified Akima spline
// The slope of the spline at its end points may be specified (as in the examples below)
// or omitted and the algorithm will compute slopes, but only if more control points
// are input.
// Finally, the catmull_rom_spline interpolates through any set of [x,y] control points,
// regardless of whether they describe a single-valued function.

HourGlassControlYs = [20.0, 9, 20.0];
// HourGlass control X left 
x0 = 0;
// X right
x1 = 100;
// slope of the spline at its left edge. 0 is horizontal
slope0Angle = 0; // [-89:89]
// slope at its right edge. 0 is horizontal
slope1Angle = 0; //  [-89:89]

slope0 = tan(slope0Angle);
slope1 = tan(slope1Angle);
numInterpolatedPoints = 40; // sort of like $fn
$fn = $preview ? 8 : 100;
lenyMinus1 = len(HourGlassControlYs)-1;
// list of [x,y] control points. needed here only for rendering to the display
controlPoints = [ for (i = [0:1:lenyMinus1]) [x0 +  i*(x1-x0)/lenyMinus1, HourGlassControlYs[i]]];
// to make a openscad display a nicely filled polygon, close it along the x axis
rawProfile = concat([[x0, 0]], controlPoints, [[x1,0]]);

// the cubic_spline is defined to reproduce its control points, and smoothly interpolates between them.
// Its input is defined to be equal-spaced points on the x axis.
// https://www.boost.org/doc/libs/1_92_0/libs/math/doc/html/math_toolkit/cardinal_cubic_b.html
cs = cubic_spline(x0, x1, HourGlassControlYs,
        numInterpolatedPoints , 
        slope0, slope1);
smoothProfile = concat([[x0,0]], cs, [[x1,0]]); // close the profile to display filled interior

// The modified Akima spline also reproduces its specified control points but:
//   The control points can be anywhere along the x axis
//   The algorithm requires at least 4 control points
// https://www.boost.org/doc/libs/1_92_0/libs/math/doc/html/math_toolkit/makima.html
akimaControls = [[x0,20],[45,9], [55,9],[x1,20]];
akimaRawClosed = concat([[x0,0]], akimaControls, [[x1,0]]); // needed for display only

akima = makima_spline(akimaControls, numInterpolatedPoints, slope0, slope1);
akimaClosed = concat([[x0,0]], akima, [[x1,0]]); // close the profile along the X axis as above

// show your work in openscad's display
polygon(smoothProfile);
mirror([0,1,0])
    polygon(rawProfile);        
 translate([0, 50, 0])
 {
    polygon(akimaClosed);
    mirror([0,1,0])
     polygon(akimaRawClosed);
}

/******************************************************************************
** EXAMPLE: Smoothed version of lookup. Using the wiki example of lookup's table   */
table = [[ -200, 5 ],[ -50, 20 ],	[ -20, 18 ], [ +80, 25 ],	[ +150, 2 ]];
function smoothed_lookup(x, tb) = makima_spline(tb, [x])[0][1]; // same as lookup() but smoothed

for (x = [-100, -50, 0, 100]) 
    echo(str("x=", x, " lookup(x)=", lookup(x,table), " smoothed_lookup(x)=", smoothed_lookup(x,table)));

/***************************************************************************************************
** EXAMPLE of working around single-valued-ness of a spline
** A spline is defined to be single-valued: a given x is defined to result in a single y. So it can't
** be used by itself to draw a closed figure. But multiple splines can be rotated and
** concatentated to make a closed figure. As an example, consider these control points: */

puzzle_piece_controls = [[0,0], [16,0], [18, 5], [14,10], [24,20], [39,12], [32, 5], [34,0], [50, 0], [50,-5], [0,-5]]; 

/* In this example, exclude those points with y <= 0 from smoothing. The control points can
** be split into two segements with single-valued control points:
** those at indices 1 through 4, and those at indicies 4 through 7 for segment2.
** puzzle_piece_controls[4] is their common point at the top of the shape.
*/
begseg1 = 1;   seg1and2 = 4;   endseg2 = 7; righttop = 8;

function rotation_matrix (a) = [[cos(a), -sin(a)], [sin(a), cos(a)]];

splicing_rotation = 60; // angle that works for making both sets of points, 1-4 and 4-7, single-valued.
rotate_left= rotation_matrix(splicing_rotation);
rotate_right= rotation_matrix(-splicing_rotation);

// extract the control points to be splined, while also rotating and shifting them the point at the top middle
segment1_controls = [for (i = [begseg1:1:seg1and2])  rotate_right *(puzzle_piece_controls[i] - puzzle_piece_controls[seg1and2]) ];
segment2_controls = [for (i = [seg1and2:1:endseg2])  rotate_left *(puzzle_piece_controls[i] - puzzle_piece_controls[seg1and2]) ];

smoothingPoints = 30; // sufficient smoothing for this example

// derivatives specified to makima_spline maintain continuity of the 1st derivative after concatenation.

smoothed1raw = makima_spline(segment1_controls, smoothingPoints, -tan(splicing_rotation), -tan(splicing_rotation));
smoothed1 = [for (i=[0:1:len(smoothed1raw)-1]) puzzle_piece_controls[seg1and2] + rotate_left*smoothed1raw[i] ];

smoothed2raw = makima_spline(segment2_controls, smoothingPoints, tan(splicing_rotation), tan(splicing_rotation));
smoothed2 = [for (i=[0:1:len(smoothed2raw)-1]) puzzle_piece_controls[seg1and2] + rotate_right*smoothed2raw[i] ];

// assemble the profile by inserting the smoothed ones along with the originals
smoothed_puzzle_piece = concat(
        [for (i=[0:1:begseg1]) puzzle_piece_controls[i]], // original control points 0 through 1
        smoothed1, smoothed2, 
        [ for(i=[endseg2:1:len(puzzle_piece_controls)-1]) puzzle_piece_controls[i]] // original points 7 to the end
    );

translate([0,-50])
        polygon(puzzle_piece_controls);
        
translate([0,-90])
        polygon(smoothed_puzzle_piece);
        
 /********************************************************
 ** EXAMPLE of catmull_rom_spline.
 ** The feature of catmull_rom is that it can smooth any [x,y]
 ** path, not just single-valued y versus x.
 ** For this example, smooth the same puzzle_piece_controls as above. This example preserves
 ** the straight bottom, but not the flatness of the top.
 ** https://www.boost.org/doc/libs/1_92_0/libs/math/doc/html/math_toolkit/catmull_rom.html
 */
 catmull_rom_points = 200; // sets the fineness of the interpolation
 closed_interpolation = false;  // closed_interpolation set to false stops the interpolation at the final control point.
 // true will smooth from the final control point back to the first.
 topProfile = [for (i = [0:1:righttop]) puzzle_piece_controls[i]];
 catmull_rom_smoothed_top = catmull_rom_spline(topProfile, 
        catmull_rom_points, 
        closed_interpolation, 
       );
 translate([0, -130])
    polygon(concat(catmull_rom_smoothed_top, [puzzle_piece_controls[9], puzzle_piece_controls[10]]));
    
// The catmull_rom doesn't explicitly allow controling the slopes of the endpoints.
// That makes it a bit less straightforward to splice it into another path
// without a discontinuous slope, but here's a trick: Use the skipFlags to omit
// the first and/or last control point from the output
// For the puzzle piece, we'll use begseg1 through endseg2, but add in new first/last
// points to make the tangents line up better at the splice points 
 
catmull_rom_controls = [ for (i=[begseg1:1:endseg2]) puzzle_piece_controls[i]];
// The extra control points lie along the original top profile of the puzzle piece
magicControlLeft = [13,15]; // chosen to flatten the angle at the joint
magicControlRight = [37,15]; // ditto
catmull_rom_extra_controls = concat([magicControlLeft], catmull_rom_controls, [magicControlRight]);
// do the smoothing: 
catmull_smoothed = catmull_rom_spline(catmull_rom_extra_controls, catmull_rom_points, 
        false, // closed is false, ie, its an open curve
        3 // omit both first and last control point intervals from the output
        );
translate([0,-170])
    polygon(
	concat([puzzle_piece_controls[0]], // the smoothing omitted this point in the original profile 
	catmull_smoothed, 
    	[for (i=[8,9,10]) puzzle_piece_controls[i]] // also omitted from smoothing
	));
 
 
