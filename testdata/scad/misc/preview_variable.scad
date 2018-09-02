echo($preview=$preview); // true when doing openCSG preview, false otherwise

if($preview)
    color("red") sphere(50);
else
    cube(50);
    
translate([100, 0]) render() {
    if($preview)
        color("red") sphere(50);
    else
        cube(50);
}

