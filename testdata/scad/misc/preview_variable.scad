echo($preview); // true when doing openCSG preview, false otherwise
if($preview)
    color("red") sphere(50);
else
    cube(50);

