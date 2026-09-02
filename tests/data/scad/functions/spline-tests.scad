// invoke cubic_spline on simple test cases
yPoints = [100, 25, 100]; // control points
x0 = 10;    // and their corresponding x's
x1 = 110;
numInterpolatedPoints = 5;
le = 0;
re = 0;
// alternate signatures. specify number of output points
s=cubic_spline(x0, x1, yPoints, numInterpolatedPoints, le, re);
echo("counted output:");
echo(s=s);
// ... or specify list of points. One will do
p= [(x1+x0)/2];
s2=cubic_spline(x0, x1, yPoints, p, le, re);
echo("explicit X:");
echo (s2=s2);
// le and re may be omitted
s3 = cubic_spline(0,10,[0,1,2,3,4,5,6,7,8,9,10],[5.5]);
echo ("slope0 and slope1 optional:");
echo (s3=s3);
// test that only fourth argument undef
s4 = cubic_spline(0, 100, [1, 10, 20, 10, 1], 5, undef, -1);
echo ("omit only slope0:");
echo (s4=s4);

// invoke makima_spline on simple test cases
controlPoints = [[0,3], [1,2], [2,1], [3,0]];
// alternate signatures. specify number of output points
a=makima_spline(controlPoints, 4);
echo(a=a);

sqr2 = 0.5 * sqrt(2);
// invoke catmull_rom_spline on an easy set of control points
crControlsRaw = [[-1,0], [-sqr2,sqr2], [0,1], [sqr2,sqr2], [1,0], [sqr2,-sqr2], [0,-1], [-sqr2,-sqr2]];
// offset by 1 to avoid rounding error comparison failures
crControls = [ for (i = [0:1:len(crControlsRaw)-1]) [crControlsRaw[i][0]+1, crControlsRaw[i][1]+1]];
crOpen = catmull_rom_spline(crControls, len(crControls));
echo (crOpen=crOpen);
crClosed = catmull_rom_spline(crControls, len(crControls)+1, true);
echo (crClosed=crClosed);
crOmitFirst = catmull_rom_spline(crControls, len(crControls)-1, 0, 1);
echo (crOmitFirst=crOmitFirst);
crOmitLast = catmull_rom_spline(crControls, len(crControls)-1, 0, 2);
echo (crOmitLast=crOmitLast);
