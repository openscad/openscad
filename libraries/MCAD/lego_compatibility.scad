// This file is placed under the public domain

// from: http://www.thingiverse.com/thing:9512


// EXAMPLES:
//   standard LEGO 2x1 tile has no pin
//      block(1,2,1/3,reinforcement=false,flat_top=true);
//   standard LEGO 2x1 flat has pin
//      block(1,2,1/3,reinforcement=true);
//   standard LEGO 2x1 brick has pin
//      block(1,2,1,reinforcement=true);
//   standard LEGO 2x1 brick without pin
//      block(1,2,1,reinforcement=false);
//   standard LEGO 2x1x5 brick has no pin and has hollow knobs
//      block(1,2,5,reinforcement=false,hollow_knob=true);


knob_diameter=4.8;		//knobs on top of blocks
knob_height=2;
knob_spacing=8.0;
wall_thickness=1.45;
roof_thickness=1.05;
block_height=9.5;
pin_diameter=3;		//pin for bottom blocks with width or length of 1
post_diameter=6.5;
reinforcing_width=1.5;
axle_spline_width=2.0;
axle_diameter=5;
cylinder_precision=0.5;

/* EXAMPLES:

block(2,1,1/3,axle_hole=false,circular_hole=true,reinforcement=true,hollow_knob=true,flat_top=true);

translate([50,-10,0])
	block(1,2,1/3,axle_hole=false,circular_hole=true,reinforcement=false,hollow_knob=true,flat_top=true);

translate([10,0,0])
	block(2,2,1/3,axle_hole=false,circular_hole=true,reinforcement=true,hollow_knob=true,flat_top=true);
translate([30,0,0])
	block(2,2,1/3,axle_hole=false,circular_hole=true,reinforcement=true,hollow_knob=false,flat_top=false);
translate([50,0,0])
	block(2,2,1/3,axle_hole=false,circular_hole=true,reinforcement=true,hollow_knob=true,flat_top=false);
translate([0,20,0])
	block(3,2,2/3,axle_hole=false,circular_hole=true,reinforcement=true,hollow_knob=true,flat_top=false);
translate([20,20,0])
	block(3,2,1,axle_hole=true,circular_hole=false,reinforcement=true,hollow_knob=false,flat_top=false);
translate([40,20,0])
	block(3,2,1/3,axle_hole=false,circular_hole=false,reinforcement=false,hollow_knob=false,flat_top=false);
translate([0,-10,0])
	block(1,5,1/3,axle_hole=true,circular_hole=false,reinforcement=true,hollow_knob=false,flat_top=false);
translate([0,-20,0])
	block(1,5,1/3,axle_hole=true,circular_hole=false,reinforcement=true,hollow_knob=true,flat_top=false);
translate([0,-30,0])
	block(1,5,1/3,axle_hole=true,circular_hole=false,reinforcement=true,hollow_knob=true,flat_top=true);
//*/

module block(width,length,height,axle_hole=false,reinforcement=false, hollow_knob=false, flat_top=false, circular_hole=false, solid_bottom=true, center=false) {
	overall_length=(length-1)*knob_spacing+knob_diameter+wall_thickness*2;
	overall_width=(width-1)*knob_spacing+knob_diameter+wall_thickness*2;
	center= center==true ? 1 : 0;
	translate(center*[-overall_length/2, -overall_width/2, 0])
	union() {
		difference() {
			union() {
				// body:
				cube([overall_length,overall_width,height*block_height]);
				// knobs:
				if (flat_top != true)
				translate([knob_diameter/2+wall_thickness,knob_diameter/2+wall_thickness,0]) 
					for (ycount=[0:width-1])
						for (xcount=[0:length-1]) {
							translate([xcount*knob_spacing,ycount*knob_spacing,0])
								difference() {
									cylinder(r=knob_diameter/2,h=block_height*height+knob_height,$fs=cylinder_precision);
									if (hollow_knob==true)
										translate([0,0,-roof_thickness])
											cylinder(r=pin_diameter/2,h=block_height*height+knob_height+2*roof_thickness,$fs=cylinder_precision);
								}
					}
			}
			// hollow bottom:
			if (solid_bottom == false)
				translate([wall_thickness,wall_thickness,-roof_thickness]) cube([overall_length-wall_thickness*2,overall_width-wall_thickness*2,block_height*height]);
			// flat_top -> groove around bottom
			if (flat_top == true) {
				translate([-wall_thickness/2,-wall_thickness*2/3,-wall_thickness/2])
					cube([overall_length+wall_thickness,wall_thickness,wall_thickness]);
				translate([-wall_thickness/2,overall_width-wall_thickness/3,-wall_thickness/2])
					cube([overall_length+wall_thickness,wall_thickness,wall_thickness]);
	
				translate([-wall_thickness*2/3,-wall_thickness/2,-wall_thickness/2])
					cube([wall_thickness,overall_width+wall_thickness,wall_thickness]);
				translate([overall_length-wall_thickness/3,0,-wall_thickness/2])
					cube([wall_thickness,overall_width+wall_thickness,wall_thickness]);
			}
			if (axle_hole==true)
				if (width>1 && length>1) for (ycount=[1:width-1])
					for (xcount=[1:length-1])
						translate([xcount*knob_spacing,ycount*knob_spacing,roof_thickness])  axle(height);
			if (circular_hole==true)
				if (width>1 && length>1) for (ycount=[1:width-1])
					for (xcount=[1:length-1])
						translate([xcount*knob_spacing,ycount*knob_spacing,roof_thickness])
							cylinder(r=knob_diameter/2, h=height*block_height+roof_thickness/4,$fs=cylinder_precision);
		}

		if (reinforcement==true && width>1 && length>1)
			difference() {
				for (ycount=[1:width-1])
					for (xcount=[1:length-1])
						translate([xcount*knob_spacing,ycount*knob_spacing,0]) reinforcement(height);
				for (ycount=[1:width-1])
					for (xcount=[1:length-1])
						translate([xcount*knob_spacing,ycount*knob_spacing,-roof_thickness/2]) cylinder(r=knob_diameter/2, h=height*block_height+roof_thickness, $fs=cylinder_precision);
			}
		// posts:
		if (solid_bottom == false)
			if (width>1 && length>1) for (ycount=[1:width-1])
				for (xcount=[1:length-1])
					translate([xcount*knob_spacing,ycount*knob_spacing,0]) post(height);

		if (reinforcement == true && width==1 && length!=1)
			for (xcount=[1:length-1])
				translate([xcount*knob_spacing,overall_width/2,0]) cylinder(r=pin_diameter/2,h=block_height*height,$fs=cylinder_precision);

		if (reinforcement == true && length==1 && width!=1)
			for (ycount=[1:width-1])
				translate([overall_length/2,ycount*knob_spacing,0]) cylinder(r=pin_diameter/2,h=block_height*height,$fs=cylinder_precision);
	}
}

module post(height) {
	difference() {
		cylinder(r=post_diameter/2, h=height*block_height-roof_thickness/2,$fs=cylinder_precision);
		translate([0,0,-roof_thickness/2])
			cylinder(r=knob_diameter/2, h=height*block_height+roof_thickness/4,$fs=cylinder_precision);
	}
}

module reinforcement(height) {
	union() {
		translate([0,0,height*block_height/2]) union() {
			cube([reinforcing_width,knob_spacing+knob_diameter+wall_thickness/2,height*block_height],center=true);
			rotate(v=[0,0,1],a=90) cube([reinforcing_width,knob_spacing+knob_diameter+wall_thickness/2,height*block_height], center=true);
		}
	}
}

module axle(height) {
	translate([0,0,height*block_height/2]) union() {
		cube([axle_diameter,axle_spline_width,height*block_height],center=true);
		cube([axle_spline_width,axle_diameter,height*block_height],center=true);
	}
}
			
