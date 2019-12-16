for (x = [0:1]) {
    for (y = [0:1]) {
        translate ([(x*50),0,(y*30)]) {
            difference () {
                cube ([55,35,35]);
                translate ([5,-0.01,5]) {
                    cube ([45,30,25]);
                }
            }
            difference () {
                translate ([27.5,15,7]) {
                    sphere (d=29, $fn=8);
                }
                translate ([0,0,-20]) {
                    cube ([55,35,20]);
                }
            }
        }
    }
}