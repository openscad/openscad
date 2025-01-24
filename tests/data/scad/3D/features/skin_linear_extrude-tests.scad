// Approximate linear extrude using extrude so I can re-use those tests
function scale_func(i,steps,scale) = 
    is_list(scale) 
        ? (len(scale)==2 ? scale[0]+(scale[1]-scale[0])*((i+0.0001)/steps) : 1)
        : 1+(scale-1)*((i+0.0001)/steps);

        
module linear_extrude_using_skin(height=undef,center=false, twist=0, slices=20,scale=1.0, v=[0,0,1])
{
    height= max((height==undef ? norm(v) : height)* ((v[0]>=0 && v[1]>=0 && v[2]>=0)?1:0),0);

    scale=is_list(scale) ? scale : (scale==undef ? 1.0 : max(scale,0.0001));

    minh = center ? -height/2:0;       

    translate((minh/norm(v))*v)
    if (twist!=0 || scale!=1 || slices!=20)
    {        
        skin() for (i=[0:1:slices],union=false)
        {           
        rotate([0,0,-twist*(i/slices)])
        translate((i/slices)*(height/norm(v))*v)
        scale([scale_func(i,slices,scale),scale_func(i,slices,scale),1])
        children();
        }
    }
    else
    {        
        skin()
        {
        translate((height/norm(v))*v)
        children();
        children();
        }    
    }
}

// Empty
linear_extrude_using_skin();
// No children
linear_extrude_using_skin() { }
// 3D child
linear_extrude_using_skin() { cube(); }


linear_extrude_using_skin(height=10) square([10,10]);

translate([19,5,0]) linear_extrude_using_skin(height=10, center=true) difference() {circle(5); circle(3);}
translate([31.5,2.5,0]) linear_extrude_using_skin(height=10, twist=-45) polygon(points = [[-5,-2.5], [5,-2.5], [0,2.5]]);

translate([1,21,0]) linear_extrude_using_skin(height=20, twist=30, slices=2, segments=0) {
    difference() {
        translate([-1,-1]) square([10,10]);
        square([8,8]);
    }
}
translate([19,20,0]) linear_extrude_using_skin(height=20, twist=45, slices=10) square([10,10]);


translate([0,-15,0]) linear_extrude_using_skin(5) square([10,10]);

translate([15,-15,0]) linear_extrude_using_skin(v=[3 ,2 ,5]) square([10, 10]);

translate([30,-15,0]) linear_extrude_using_skin(height=8, v=[3, 2, 5]) square([10,10]);



// scale given as a scalar
translate([-25,-10,0]) linear_extrude_using_skin(height=10, scale=2) square(5, center=true);
// scale given as a 3-dim vector
translate([-15,20,0]) linear_extrude_using_skin(height=20, scale=[4,5,6]) square(10);

// scale is negative
translate([-10,5,0]) linear_extrude_using_skin(height=15, scale=-2) square(10, center=true);

// scale given as undefined
translate([-15,-15,0]) linear_extrude_using_skin(height=10, scale=var_undef) square(10);

// height is negative
translate([0,-25,0]) linear_extrude_using_skin(-1) square(10, center=true);

// vector has negative z coordinate
translate([0,-25,0]) linear_extrude_using_skin(v=[10,10,-5]) square(10, center=true);

