module triangle(o_len, a_len, depth, center=false)
{
    centroid = center ? [-a_len/3, -o_len/3, -depth/2] : [0, 0, 0];
    translate(centroid) linear_extrude(height=depth)
    {
        polygon(points=[[0,0],[a_len,0],[0,o_len]], paths=[[0,1,2]]);
    }
}

module a_triangle(tan_angle, a_len, depth, center=false)
{
    triangle(tan(tan_angle) * a_len, a_len, depth, center);
}

module test_triangle() { triangle(5, 5, 5); }
module test_a_triangle() { a_triangle(45, 5, 5); }
module test_triangles()
{
    for (i = [1:10])
    {
        translate([i*7, -30, i*7])
        {
            triangle(i*5, sqrt(i*5+pow(i,2)), 5);
        }
    }

    for (i = [1:85/5])
    {
        translate([i*7, 22, i*7])
        {
            a_triangle(i*5, 10, 5);
        }
    }
}
