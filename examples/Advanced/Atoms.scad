//Condensed matter
//All measurements are in Ångstrom

//OpenSCAD
$fn = 40;  //sections per circle
echo(version=version());

//Liquid nitrogen (N2)
AtomicRadiusN2 = 0.65;
BondPairN2 = 1.197; 
ColorN2 = "Silver";
echo(pow(BondPairN2,1/3));

//Silicon (Si)
AtomicRadiusSi = 1.1;
LatticeCellSizeSi = 5.4309;
fccOffset = 0.25;
ColorSi = "Wheat";
echo(fccOffset * LatticeCellSizeSi);
echo(LatticeCellSizeSi / 8);

//Carbon (graphite) [c]
AtomicRadiusC = 0.7;
LatticeCellSizeC = 3.65;
cellLenA = 2.464;
cellLenB = cellLenA;
cellLenC = 6.711;
cellAngleA = 90;
cellAngleB = cellAngleA;
cellAngleC = 120;
LayerSeperationC = 3.364;
ColorC = "Grey";
echo(cellLenA);
echo(cellLenB);
echo(cellLenC);

module bond(p1 = [0,0,0], p2 = [1,1,1], ar1 = 1.0, ar2 = 2.0, c="yellow")
{
    cyR = min(ar1,ar2) / 5.0;
    dX = p1.x - p2.x;
    dY = p1.y - p2.y;
    dZ = p1.z - p2.z;
    dist = norm([dX, dY, dZ]);

    cyC = [p1.x-dX/2, p1.y-dY/2, p1.z-dZ/2];

    beta = acos(dZ/dist); // inclination angle
    gamma = atan2(dY,dX); // azimuthal angle
    rot = [0, beta, gamma];
    
    translate(v=cyC) 
    rotate(a = rot) 
    color(c) 
    cylinder(h=dist, r=cyR, center=true);
};

module bondPair(d=0.0, ar=1.0, c="Black") {
    axD = pow(d,1/3);
    p1 = [+axD,-axD,-axD];
    p2 = [-axD,+axD,+axD];
    translate(p1) {
            color(c) sphere(r=ar);
    };
    translate(p2) {
            color(c) sphere(r=ar);
    }
    bond(p1, p2, ar, ar);
};

module hexagonalClosePacked(dst = [1.0, 1.0, 1.0], ar=1.0, c="Black") {
    //center
    p1 = [0, 0, 0];
    color(c) sphere(r=ar);

    //Three 120 degrees around center
    baseAg = 30;
    ag = [baseAg, baseAg + 120, baseAg + 240];
    points =[ [cos(ag.x)*dst.x,sin(ag.x)*dst.x,0], [cos(ag.y)*dst.y,sin(ag.y)*dst.y,0],[cos(ag.z)*dst.z,sin(ag.z)*dst.z,0] ];
    for (p2 = points)        
         {
            translate(p2)
             color(c) 
             sphere(r=ar);
            bond(p1 = p1, p2 = p2, ar1 = ar, ar2 = ar);
        }
};


module fccDiamond(ar=1.0, unitCell=2.0, fccOffset=0.25, c="Blue") {
    //lattice 8 vertices
    huc = unitCell / 2.0;
    od = fccOffset * unitCell;
        
    //interstitial
    interstitial = [
        [+od, +od, +od],
        [+od, -od, -od],
        [-od, +od, -od],
        [-od, -od, +od]
    ];
    //corners
    corners = [
        [+huc, +huc, +huc],
        [+huc, -huc, -huc],
        [-huc, +huc, -huc],
        [-huc, -huc, +huc]
    ];
    //face centered
    fcc = [
        [+huc, 0, 0],
        [-huc, 0, 0],
        [0, +huc, 0],
        [0, -huc, 0],
        [0, 0, +huc],
        [0, 0, -huc]
    ];

    for(p = corners)
        translate(p)
        color(c)
        sphere(r=ar);
    
    for (p = fcc)
        translate(p)
        color(c)
        sphere(r=ar);
        
    for (p = interstitial)
    {
        translate(p)
        color(c)
        sphere(r=ar);
    }
    
    bonds = [
        [ interstitial[0], corners[0]],
        [ interstitial[0], fcc[0]],
        [ interstitial[0], fcc[2]],
        [ interstitial[0], fcc[4]],
        [ interstitial[1], corners[1]],
        [ interstitial[1], fcc[0]],
        [ interstitial[1], fcc[3]],
        [ interstitial[1], fcc[5]],
        [ interstitial[2], corners[2]],
        [ interstitial[2], fcc[1]],
        [ interstitial[2], fcc[2]],
        [ interstitial[2], fcc[5]],
        [ interstitial[3], corners[3]],
        [ interstitial[3], fcc[1]],
        [ interstitial[3], fcc[3]],
        [ interstitial[3], fcc[4]]
    ];
    for(b = bonds)
        bond(b.x, b.y, ar, ar);
        
};

module SiCell(x = 1.0, y = 1.0, z = 1.0) {
    //Si cell
    translate([+LatticeCellSizeSi * x, +LatticeCellSizeSi * y, +LatticeCellSizeSi * z]) fccDiamond(AtomicRadiusSi, LatticeCellSizeSi, fccOffset, ColorSi);
};

module SiN2Cell(x = 1.0, y = 1.0, z = 1.0) {
    
    n2Offset = LatticeCellSizeSi / 8;
    //N2 Pair
    translate([+LatticeCellSizeSi * x - n2Offset, +LatticeCellSizeSi * y + n2Offset, +LatticeCellSizeSi * z + n2Offset]) bondPair(BondPairN2, AtomicRadiusN2, ColorN2);

    //Si cell
    SiCell(x, y, z);
};

module GraphiteCell(xyz = [1.0, 1.0, 1.0]) {
    //Graphite cell
    loc = [
    (cellLenA * xyz.x * cos(30) * 2),
    ((cellLenB * sin(30)) + cellLenC) * xyz.y,
    xyz.z];
    translate(loc) hexagonalClosePacked([cellLenA, cellLenB, cellLenC], AtomicRadiusC, ColorC);
};

//graphite layer
siOffset = 3 * LatticeCellSizeSi / 8;
rotate([0, 0, 45])
translate([0, -siOffset, 0])
for (x = [-5:1:5])
    for (y = [-2:1:3])
    {
        offset = (y % 2 == 0) ? 0.0 : 0.5;
        GraphiteCell([x + offset, y, LayerSeperationC * 0.5 + LatticeCellSizeSi * 1.5]);
    }

//top layer has N2
xyPlane = [-3,-2,-1,0,+1,+2,+3];
for (x = xyPlane)
    for (y = xyPlane)
        SiN2Cell( x, y, +1);

//below is just the silcon waffer
for (x = xyPlane)
    for (y = xyPlane)
        for (z =[-1,0])
            SiCell( x, y, z);
