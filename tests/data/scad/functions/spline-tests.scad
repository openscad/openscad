// invoke cubic_spline on simple test cases
yPoints = [100, 25, 100]; // control points
x0 = 10;    // and their corresponding x's
x1 = 110;
numInterpolatedPoints = 5;
le = 0;
re = 0;
// alternate signatures. specify number of output points
s=cubic_spline(yPoints, numInterpolatedPoints, x0, x1, le, re);
echo("counted output:");
echo(s=s);
// ... or specify list of points. One will do
p= [(x1+x0)/2];
s2=cubic_spline(yPoints, p, x0, x1, le, re);
echo("explicit X:");
echo (s2=s2);
// le and re may be omitted
s3 = cubic_spline([0,1,2,3,4,5,6,7,8,9,10],[5.5], 0,10);
echo ("slope0 and slope1 optional:");
echo (s3=s3);
// test that only fourth argument undef
s4 = cubic_spline([1, 10, 20, 10, 1], 5, 0, 100, undef, -1);
echo ("omit only slope0:");
echo (s4=s4);

// invoke makima_spline on simple test cases
controlPoints = [[0,3], [1,2], [2,1], [3,0]];
// alternate signatures. specify number of output points
a=makima_spline(controlPoints, 4);
echo(a=a);

function roundToZero(a, val = 1e-7) = [ for (v = a) 
// for echo comparison, print out any number less than val as actually zero
            [abs(v[0]) < val ? 0 : v[0], abs(v[1]) < val ? 0 : v[1]]];

sqr2 = 0.5 * sqrt(2);
// invoke catmull_rom_spline on an easy set of control points
crControlsRaw = [[-1,0], [-sqr2,sqr2], [0,1], [sqr2,sqr2], [1,0], [sqr2,-sqr2], [0,-1], [-sqr2,-sqr2]];
// offset by 1 to avoid rounding error comparison failures in the test checking
crControls = [ for (i = [0:1:len(crControlsRaw)-1]) [crControlsRaw[i][0]+1, crControlsRaw[i][1]+1]];
echo (crControls=crControls);
crOpen = catmull_rom_spline(crControls, 2*len(crControls)-1);
echo (crOpen=crOpen);
crAlpha = roundToZero(catmull_rom_spline(crControls, 2*len(crControls)-1, false, 1.0));
echo (crAlpha=crAlpha);
crClosed = catmull_rom_spline(crControls, len(crControls)+1, true);
echo (crClosed=crClosed);
crOmitFirst = catmull_rom_spline(crControls, [len(crControls)-1, 1, len(crControls)-1]);
echo (crOmitFirst=crOmitFirst);
crOmitLast = catmull_rom_spline(crControls, [len(crControls)-1, 0, len(crControls)-2]);
echo (crOmitLast=crOmitLast);
crTangents = roundToZero(
        catmull_rom_spline(crControls, len(crControls)+1, true, 0.5, true));
echo (crTangents=crTangents);
crConcats = catmull_rom_spline(crControls, [[2, 1, 2], [2,2,3]]);
echo (crConcats=crConcats);
