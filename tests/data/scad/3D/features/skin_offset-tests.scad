skin(segments=20)
{
    offset(r=2)
    square(10,center=true);
    translate([0,0,50])
    square(10,center=true);
}

module square_manual(sz)
{
    difference()
    {
        square(sz,center=true);
        square(sz-2,center=true);
    }
}

translate([30,0,0])
skin(segments=20)
{
    square_manual(10);
    translate([0,0,50])
    square_manual(20);
}

module square_offset(sz)
{
    difference()
    {
        square(sz,center=true);
        offset(-2)
        square(sz,center=true);
    }
}

translate([60,0,0])
skin(segments=20, interpolate = false)
{
    square_offset(10);
    translate([0,0,50])
    square_offset(20);
}


translate([90,0,0])
skin(segments=20)
{
    square(10);
    translate([0,0,50])
    circle(10);
}

module circle_offset(sz)
{
    difference()
    {
        circle(sz);
        offset(-2)
        circle(sz);
    }
}

translate([120,0,0])
skin(segments=20)
{
    square_offset(10);
    translate([0,0,50])
    circle_offset(10);
}
