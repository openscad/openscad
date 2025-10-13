// Pegboard width, in units of mm, is recommended to be set as a multiple of 20, with a minimum value of 40. (洞洞板宽度，单位为mm，建议设置为20的倍数，最小值为40。)
width =240; // [40:20:760]

// Pegboard height, in units of mm, is recommended to be set as a multiple of 20, with a minimum value of 40. (洞洞板高度，单位为mm，建议设置为20的倍数，最小值为40。)
height = 200; // [40:20:760]

// Pegboard thickness, in units of mm, has a recommended value of between 3 and 5. (洞洞板厚度，单位为mm，推荐值为3到5。)
thickness = 5; // [2:0.2:10]

// Add screw holes at the four corners, and you can adjust the diameter and position of the screw holes in the "Corner Hole Settings". (在四个角上添加螺丝孔，你可以在“螺丝孔设置”里调整螺丝孔的直径和位置。)
add_corner_holes = true;

// Add spacers to position the pegboard away from the wall; you can adjust the diameter and thickness of the spacers in the "Spacer Settings". (添加垫片，用于使洞洞板远离墙面，你可以在“垫片设置”里调整垫片的直径和厚度。)
add_spacers = true;

// Infinite Extension Mode, when enabled, the program will generate half a hole at the edges of the pegboard. (无限扩展模式，启用后程序会在洞洞板边缘生成半个孔。)
infinite_extension_mode = true;

/* [Corner Hole Settings (螺丝孔设置)] */

// Corner holes diameter, in units of mm, has recommended values of 3.4 or 4.4. (螺丝孔直径，单位为mm，推荐值为3.4或4.4。)
corner_holes_diameter = 4.4; // [2.4:0.5:6.4]

// Distance from corner holes to the edge, in units of mm, has a recommended value of between 8 and 12. (螺丝孔到边缘的距离，单位为mm，推荐值为8到12。)
distance_to_edge = 8; // [5:1:15]

/* [Spacer Settings (垫片设置)] */

// Spacer diameter, in units of mm, has a recommended value of between 6.4 and 20. (垫片直径，单位为mm，推荐值为6.4到20。)
spacer_diameter = 14; // [6.4:0.2:20]

// Spacer height, in units of mm, has a recommended value of between 7 and 15. (垫片高度，单位为mm，推荐值为7到15。)
spacer_height = 7; // [0.2:0.2:30]

/* [Infinite Extension Mode Settings (无限扩展模式设置)] */

// Expand Upward (向上扩展)
expand_up = true;

// Expand Downward (向下扩展)
expand_down = false;

// Expand Leftward (向左扩展)
expand_left = true;

// Expand Rightward (向右扩展)
expand_right = false;

// Fewer Screw Mode, when enabled, will generate screw holes at the joints of two pegboards, thereby reducing the number of screw holes required for installation. The extra small discs are used to cover the screw holes.(“更少的螺丝”模式，开启后会在两块洞洞板的接缝处生成螺丝孔，以此来减少安装时所需的螺丝孔数量。额外的小圆片用于遮盖螺丝孔。)
fewer_screw_mode = true;

/* [Other Settings (其他设置)] */

// This variable controls the precision of the model. It is recommended to set it to 50 or lower when running on a web page and to 100 when running locally. A higher number means higher model precision, but it will also result in longer computation times. (这个变量用于控制模型精度。在网页上运行时建议设置为50或更低，在本地运行时建议设置为100。数字越高，模型精度越高，相应的计算时间也会更长。)
$fn = 100; // [20:10:100]

module rounded_rectangle(w, h, r, t) {
    hull() {
        if (infinite_extension_mode){
            r1 = (expand_down || expand_right) ? 1 : r;
            r2 = (expand_down || expand_left) ? 1 : r;
            r3 = (expand_up || expand_left) ? 1 : r;
            r4 = (expand_up || expand_right) ? 1 : r;
            translate([r1, r1, 0]) chamferCylinder(t, r1, r1,  0.4,  0.4);
            translate([w-r2, r2, 0]) chamferCylinder(t, r2, r2,  0.4,  0.4);
            translate([w-r3, h-r3, 0]) chamferCylinder(t, r3, r3,  0.4,  0.4);
            translate([r4, h-r4, 0]) chamferCylinder(t, r4, r4, 0.4, 0.4);
        } else {
            translate([r, r, 0]) chamferCylinder(t, r, r,  0.4,  0.4);
            translate([w-r, r, 0]) chamferCylinder(t, r, r,  0.4,  0.4);
            translate([w-r, h-r, 0]) chamferCylinder(t, r, r,  0.4,  0.4);
            translate([r, h-r, 0]) chamferCylinder(t, r, r,  0.4,  0.4);
        }
    }
}

module hole(w, h, r) {
    chamfer_dimension = 0.8;
    r_chamfer = r + chamfer_dimension;
    
    hull() {
        translate([r, r, 0]) cylinder(r=r, h=thickness);
        translate([r, h-r, 0]) cylinder(r=r, h=thickness);
    }
    hull() {
        translate([r, r, 0]) cylinder(r1=r_chamfer, r2=0, h=r_chamfer); 
        translate([r, h-r, 0]) cylinder(r1=r_chamfer, r2=0, h=r_chamfer);
    }
    hull() {
        translate([r, r, thickness-r_chamfer]) cylinder(r1=0, r2=r_chamfer, h=r_chamfer); 
        translate([r, h-r, thickness-r_chamfer]) cylinder(r1=0, r2=r_chamfer, h=r_chamfer);
    }
}

module corner_holes(offset, diameter) {
    if (fewer_screw_mode) {
        offset_up = expand_up ? 0 : 40; 
        offset_down = expand_down ? 0 : 40; 
        offset_left = expand_left ? 0 : 40; 
        offset_right = expand_right ? 0 : 40; 
        offset_height = height % 40 != 0 ? 20 : 0;  
        offset_width = width % 40 != 0 ? 20 : 0; 
        corners = [
            [offset_right, offset_down],
            [width-offset_left-offset_width, offset_down],
            [width-offset_left-offset_width, height-offset_up-offset_height],
            [offset_right, height-offset_up-offset_height]
        ];

        for (pos = corners) {
            translate(pos) cylinder(r=diameter/2, h=thickness);
            translate(pos) cylinder(r=diameter, h=2);
            translate([0, 0, 2]) translate(pos) cylinder(r1=diameter, r2=0, h=diameter);
        }
    } else {
        corners = [
            [offset, offset],
            [width - offset, offset],
            [width - offset, height - offset],
            [offset, height - offset]
        ];

        for (pos = corners) {
            translate(pos) cylinder(r=diameter/2, h=thickness);
            translate(pos) cylinder(r1=diameter, r2=0, h=diameter);
        }
    }
}

module spacers(offset, hole_diameter)  {
    if (fewer_screw_mode) {
        offset_up = expand_up ? 0 : 40; 
        offset_down = expand_down ? 0 : 40; 
        offset_left = expand_left ? 0 : 40; 
        offset_right = expand_right ? 0 : 40; 
        offset_height = height % 40 != 0 ? 20 : 0;  
        offset_width = width % 40 != 0 ? 20 : 0; 
        corners = [
            [offset_right, offset_down, thickness-0.4],
            [width-offset_left-offset_width, offset_down, thickness-0.4],
            [width-offset_left-offset_width, height-offset_up-offset_height, thickness-0.4],
            [offset_right, height-offset_up-offset_height, thickness-0.4]
        ];
        difference() {
            for (pos = corners) {
                difference() {
                    translate(pos) cylinder(r=spacer_diameter/2, h=spacer_height + 0.4);
                    translate(pos) cylinder(r=hole_diameter/2, h=spacer_height + 0.4);
                }
            }
            translate([-spacer_diameter, 0]) cube(size=[spacer_diameter, height, thickness+ spacer_height+1]);
            translate([width, 0]) cube(size=[spacer_diameter, height, thickness+spacer_height+1]);
            translate([-spacer_diameter, -spacer_diameter]) cube(size=[width+(2*spacer_diameter), spacer_diameter, thickness+spacer_height+1]);
            translate([-spacer_diameter, height]) cube(size=[width+(2*spacer_diameter), spacer_diameter, thickness+spacer_height+1]);
        }
    } else {
        corners = [
            [offset, offset, thickness],
            [width - offset, offset, thickness],
            [width - offset, height - offset, thickness],
            [offset, height - offset, thickness]
        ];
        
        for (pos = corners) {
            difference() {
                translate(pos) cylinder(r=spacer_diameter/2, h=spacer_height);
                translate(pos) cylinder(r=hole_diameter/2, h=spacer_height);
            }
        }
    }
}

module screw_caps()  {
    for (i = [0 : 3]) {
        difference() {
            translate([(i*((corner_holes_diameter*2)+5))+corner_holes_diameter, -corner_holes_diameter-5]) cylinder(r=corner_holes_diameter-0.2, h=1.8);
            translate([(i*((corner_holes_diameter*2)+5))+corner_holes_diameter, -5.4]) cylinder(r=1, h=2);
        }
    }
}

difference() {
    rounded_rectangle(width, height, 8, thickness);
    
    spacing = 20;
    spacing_x2 = spacing * 2;
    w = 5;
    h = 15;
    w_half = w / 2;
    h_half = h / 2;
    r = 2.5;

    if (fewer_screw_mode) {
        for (j = [spacing : spacing : height-spacing-h_half+10]) {
            offset = (j / spacing) % 2 == 0 ? 0 : spacing; 
            for (i = [spacing+offset : spacing_x2 : width-spacing-(w_half)+5]) {
                translate([i-(w_half), j-h_half, 0]) hole(w, h, r);
            } 
        }
    } else {
        for (j = [spacing : spacing : height-spacing-h_half+10]) {
            offset = (j / spacing) % 2 == 0 ? spacing : 0; 
            for (i = [spacing+offset : spacing_x2 : width-spacing-(w_half)+5]) {
                translate([i-(w_half), j-h_half, 0]) hole(w, h, r);
            } 
        }
    }
    
    
    if (infinite_extension_mode) {
        if (expand_up) {
            if (fewer_screw_mode) {
                offset = (height / spacing) % 2 == 0 ? 0 : spacing; 
                for (i = [spacing+offset : spacing_x2 : width-spacing-(w_half)+10]) {
                    translate([i-(w_half), height-h_half, 0]) hole(w, h, r);
                }
            } else {
                offset = (height / spacing) % 2 == 0 ? spacing : 0;
                for (i = [spacing+offset : spacing_x2 : width-spacing-(w_half)+5]) {
                    translate([i-(w_half), height-h_half, 0]) hole(w, h, r);
                }
            }
        }
        
        if (expand_down) { 
            if (fewer_screw_mode) {
                for (i = [spacing : spacing_x2 : width-spacing-(w_half)+50]) {
                    translate([i-(w_half), 0-h_half, 0]) hole(w, h, r);
                }
            } else {
                for (i = [spacing + 20 : spacing_x2 : width-spacing-(w_half)+5]) {
                    translate([i-(w_half), 0-h_half, 0]) hole(w, h, r);
                }
            }
            
        }
        
        if (expand_left) {
            if (fewer_screw_mode) {
                offset_up = expand_up ? 40 : 0;
                for (i = [spacing : spacing : height-spacing-h_half+10+offset_up]) {
                    if ((width / spacing) % 2 == 0) {
                        if ((i / spacing) % 2 != 0) translate([width-(w_half), i-h_half, 0]) hole(w, h, r);
                    } else {
                        if ((i / spacing) % 2 == 0) translate([width-(w_half), i-h_half, 0]) hole(w, h, r);
                    }  
                }
            } else {
                for (i = [spacing : spacing : height-spacing-h_half+10]) {
                    if ((width / spacing) % 2 == 0) {
                        if ((i / spacing) % 2 == 0) translate([width-(w_half), i-h_half, 0]) hole(w, h, r);
                    } else {
                        if ((i / spacing) % 2 != 0) translate([width-(w_half), i-h_half, 0]) hole(w, h, r);
                    }  
                }
            }
        }
        
        if (expand_right) {
            if (fewer_screw_mode) {
                offset_up = expand_up ? 40 : 0;
                for (i = [spacing : spacing : height-spacing-h_half+10+offset_up]) {
                    if ((i / spacing) % 2 != 0) translate([0-(w_half), i-h_half, 0]) hole(w, h, r);
                }
            } else {
                for (i = [spacing : spacing : height-spacing-h_half+10]) {
                    if ((i / spacing) % 2 == 0) translate([0-(w_half), i-h_half, 0]) hole(w, h, r);
                }
            }
            
        }
    }
    
    if (add_corner_holes) corner_holes(distance_to_edge, corner_holes_diameter);
}
if (add_corner_holes && add_spacers) spacers(distance_to_edge, corner_holes_diameter);
if (fewer_screw_mode) screw_caps();

// The following code is from https://github.com/SebiTimeWaster/Chamfers-for-OpenSCAD
module chamferCylinder(h, r, r2 = undef, ch = 1, ch2 = undef, a = 0, q = -1.0, height = undef, radius = undef, radius2 = undef, chamferHeight = undef, chamferHeight2 = undef, angle = undef, quality = undef) {
    // keep backwards compatibility
    h   = (height == undef) ? h : height;
    r   = (radius == undef) ? r : radius;
    r2  = (radius2 == undef) ? r2 : radius2;
    ch  = (chamferHeight == undef) ? ch : chamferHeight;
    ch2 = (chamferHeight2 == undef) ? ch2 : chamferHeight2;
    a   = (angle == undef) ? a : angle;
    q   = (quality == undef) ? q : quality;

    height         = h;
    radius         = r;
    radius2        = (r2 == undef) ? r : r2;
    chamferHeight  = ch;
    chamferHeight2 = (ch2 == undef) ? ch : ch2;
    angle          = a;
    quality        = q;

    module cc() {
        upperOverLength = (chamferHeight2 >= 0) ? 0 : 0.01;
        lowerOverLength = (chamferHeight >= 0) ? 0 : 0.01;
        cSegs = circleSegments(max(radius, radius2), quality);

        if(chamferHeight >= 0 || chamferHeight2 >= 0) {
            hull() {
                if(chamferHeight2 > 0) {
                    translate([0, 0, height - abs(chamferHeight2)]) cylinder(abs(chamferHeight2), r1 = radius2, r2 = radius2 - chamferHeight2, $fn = cSegs);
                }
                translate([0, 0, abs(chamferHeight)]) cylinder(height - abs(chamferHeight2) - abs(chamferHeight), r1 = radius, r2 = radius2, $fn = cSegs);
                if(chamferHeight > 0) {
                    cylinder(abs(chamferHeight), r1 = radius - chamferHeight, r2 = radius, $fn = cSegs);
                }
            }
        }

        if(chamferHeight < 0 || chamferHeight2 < 0) {
            if(chamferHeight2 < 0) {
                translate([0, 0, height - abs(chamferHeight2)]) cylinder(abs(chamferHeight2), r1 = radius2, r2 = radius2 - chamferHeight2, $fn = cSegs);
            }
            translate([0, 0, abs(chamferHeight) - lowerOverLength]) cylinder(height - abs(chamferHeight2) - abs(chamferHeight) + lowerOverLength + upperOverLength, r1 = radius, r2 = radius2, $fn = cSegs);
            if(chamferHeight < 0) {
                cylinder(abs(chamferHeight), r1 = radius - chamferHeight, r2 = radius, $fn = cSegs);
            }
        }
    }
    module box(brim = abs(min(chamferHeight2, 0)) + 1) {
        translate([-radius - brim, 0, -brim]) cube([radius * 2 + brim * 2, radius + brim, height + brim * 2]);
    }
    module hcc() {
        intersection() {
            cc();
            box();
        }
    }
    if(angle <= 0 || angle >= 360) cc();
    else {
        if(angle > 180) hcc();
        difference() {
            if(angle <= 180) hcc();
            else rotate([0, 0, 180]) hcc();
            rotate([0, 0, angle]) box(abs(min(chamferHeight2, 0)) + radius);
        }
    }
}

function circleSegments(r, q = -1.0) = (q >= 3 ? q : ((r * PI * 4 + 40) * ((q >= 0.0) ? q : 1.0)));