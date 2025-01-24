// Approximate rotate extrude using extrude so I can re-use those tests
function scale_func(i,steps,scale) = 
    is_list(scale) 
        ? (len(scale)==2 ? 1 : 1)
        : 1+(scale-1)*((i+0.0001)/steps);

        
module rotate_extrude_using_skin(angle=360,start=0,convexity=0,$fn=0)
{
    //translate((minh/norm(v))*v)
    slices = $fn==0 ? 32: $fn<3 ? 3 : $fn;
    
    skin(convexity=convexity) for (i=[0:min(slices,ceil(slices/16)):slices],union=false)   
    {
        rotate([0,0,start+angle*(i/slices)])
        rotate([90,0,0])
        children();
    }
}


// Empty
rotate_extrude_using_skin();
// No children
rotate_extrude_using_skin() { }
// 3D child
rotate_extrude_using_skin() { cube(); }

// Normal
rotate_extrude_using_skin() translate([20,0,0]) circle(r=10);

// Sweep of polygon with hole
translate([50,-20,0]) {
  difference() { 
    rotate_extrude_using_skin(convexity=4) translate([20,0,0]) difference() {
      circle(r=10); circle(r=8);
    }
    translate([-50,0,0]) cube([100,100,100], center=true);
  }
}

// Alternative, difference between two solid sweeps
translate([50,50,0]) {
  difference() { 
    difference() {
      rotate_extrude_using_skin(convexity=2) translate([20,0,0]) circle(r=10);
      rotate_extrude_using_skin(convexity=2) translate([20,0,0]) circle(r=8);
    }
    translate([-50,0,0]) cube([100,100,100], center=true);
  }
}

// Minimal $fn
translate([0,-60,0]) rotate_extrude_using_skin($fn=1) translate([20,0,0]) circle(r=10,$fn=1);

// Object in negative X
translate([0,60,0]) rotate_extrude_using_skin() translate([-20,0]) square(10);

