module zigzag() {
    skin() {
        square(1);
        translate([.5,0,1]) square(1);
        translate([-.5,0,2]) square(1);
        translate([0,0,3]) square(1);
}
}

module horn() {
    skin() for (i=[1:8:740] ,union=false)
    {
        $fn = 20;
        rotate(i, [1,0,0])
            translate([2*i/260+i*i*.000001,1+i/360,0])
                circle(r=.5+.5*i/360);
    }
}

module arch() {
    skin()
    for (x=[-2:.15:2.01],union=false)
    {
            $fn = 36;
            z = x*x;
            mirror([0,0,1])
            translate([x,0,z])
                rotate(acos(x/2.1),[0,1,0])
                union()
                {
                    translate([0,-2]) square([1,.5]);
                    translate([0,-2]) square([.5,4]);
                    translate([0,2])square([1,.5]);
                }
    }
}


zigzag();

translate([3,0,0])
horn();

translate([15,0,0])
arch();

