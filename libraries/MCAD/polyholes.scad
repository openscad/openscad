// Copyright 2011 Nophead (of RepRap fame)
// This file is licensed under the terms of Creative Commons Attribution 3.0 Unported.

// Using this holes should come out approximately right when printed
module polyhole(h, d) {
    n = max(round(2 * d),3);
    rotate([0,0,180])
        cylinder(h = h, r = (d / 2) / cos (180 / n), $fn = n);
}

module test_polyhole(){
difference() {
	cube(size = [100,27,3]);
    union() {
    	for(i = [1:10]) {
            translate([(i * i + i)/2 + 3 * i , 8,-1])
                polyhole(h = 5, d = i);
                
            assign(d = i + 0.5)
                translate([(d * d + d)/2 + 3 * d, 19,-1])
                    polyhole(h = 5, d = d);
    	}
    }
}
}

