$fn = 60;
width = 70;
height = 100;
thin = 5;
thick = 10;
rounding = 2;
corner_size = 30;

selected_icon = undef;

font = "Open Sans:style=Regular";
export_font = "Open Sans:style=Bold";

icons = [
    ["export-stl"],
    ["export-off"],
    ["export-amf"],
    ["export-3mf"],
    ["export-dxf"],
    ["export-svg"],
    ["export-csg"],
    ["export-pdf"],
    ["export-png"],
    ["preview"],
    ["render"],
    ["send"],
    ["zoom-in"],
    ["zoom-out"],
    ["zoom-all"],
    ["zoom-text-in"],
    ["zoom-text-out"],
    ["undo"],
    ["redo"],
    ["indent"],
    ["unindent"],
    ["new"],
    ["save"],
    ["open"],
];

icon(selected_icon) {
    export("STL");
    export("OFF");
    export("AMF");
    export("3MF");
    export("DXF");
    export("SVG");
    export("CSG");
    export("PDF");
    export("PNG");
    preview();
    render_();
    send();
    zoom_in();
    zoom_out();
    zoom_all();
    zoom_text_in();
    zoom_text_out();
    undo();
    redo();
    indent();
    unindent();
    new();
    save();
    open();
}

for (a = [ 0 : len(icons) - 1 ]) {
    echo(icon = icons[a][0]);
}

module icon(icon) {
    assert(len(icons) == $children, "list of icon names needs to be same length as number of child modules to icon()");
    cnt = $children;
    cols = ceil(sqrt(cnt));
    if (is_undef(icon)) {
        for (i = [0:cnt - 1]) {
            pos = 200 * [i % cols, floor(i / cols)];
            translate(pos) children(i);
        }
    } else {
        i = search([icon], icons)[0];
        echo(i);
        children(i);
    }
}

module outline(w) {
    difference() {
        offset(w) children();
        children();
    }
}

module inset(w) {
    difference() {
        children();
        offset(delta = -w) children();
    }
}

module box(center = true) {
    %square([width, height], center = center);
}

module paper() {
    w = rounding + thin;
    offset(rounding)
        translate([w, w])
            square([width - 2 * w, height - 2 * w]);
}

module export_paper() {
    difference() {
        outline(thin) paper();
        translate([-1, height - corner_size + 1]) square([corner_size, corner_size]);
    }
    export_paper_corner();
}

module export_paper_corner() {
    translate([corner_size, height - corner_size]) {
        rotate(90) {
            hull() {
                intersection() {
                    outline(thin) paper();
                    translate([-1, -1]) square([corner_size + 1, corner_size + 1]);
                }
            }
        }
    }
}

module export(t) {
    difference() {
        export_paper();
        translate([-10, -10]) square([height, 55]);
    }
    translate([width / 2, 0])
        text(t, 35, font = export_font, halign = "center");
}

module hourglass() {
    square([20, 3], center = true);
    translate([-5, 6]) rotate(60) square([15.6, 1], center = true);
    translate([-5, 25 - 6]) rotate(-60) square([15.6, 1], center = true);
    mirror([1, 0]) {
        translate([-5, 6]) rotate(60) square([15.6, 1], center = true);
        translate([-5, 25 - 6]) rotate(-60) square([15.6, 1], center = true);
    }
    polygon([[-5, 2.5], [5, 2.5], [0.1, 5], [0.2, 13.5], [4, 20], [-4, 20], [-0.2, 13.5], [-0.1, 5]]);
    translate([0, 25]) square([20, 3], center = true);
}

module line(l = 40.2) {
    translate([-0.1, -0.1]) square([0.2, l]);
}

module line_dotted() {
    difference() {
        translate([-0.1, -0.1]) square([0.2, 40.2]);
        for (a = [ 0 : 2 ]) {
            translate([-5, 4 + a * 12]) square([10, 8]);
        }
    }
}

module preview_cube() {
    offset(2) {
        children(0);
        rotate(-60) children(0);
        rotate(60) children(0);
        translate([0, 40]) rotate(-60) children(0);
        translate([0, 40]) rotate(60) children(0);
        translate([-sin(60) * 40, cos(60) * 40]) children(0);
        translate([sin(60) * 40, cos(60) * 40]) children(0);
        translate([-sin(60) * 40, cos(60) * 40 + 40]) rotate(-60) children(0);
        translate([sin(60) * 40, cos(60) * 40 + 40]) rotate(60) children(0);
    }
}

module render_() {
    difference() {
        preview_cube() line();
        polygon([[-14, 35], [14, 35], [3, 13], [30, -30], [-30, -30], [-3, 13]]);
    }
    translate([0, -10]) scale(1.6) hourglass();
}

module preview() {
    difference() {
        preview_cube() line_dotted();
        translate([0, 8]) square([35, 28], center = true);
    }
    translate([0, 4]) polygon([[-18, 15], [-9, 15], [0, 0], [-9, -15], [-18, -15], [-9, 0]]);
    translate([18, 4]) polygon([[-18, 15], [-9, 15], [0, 0], [-9, -15], [-18, -15], [-9, 0]]);
}

module send() {
    difference() {
        preview_cube() line();
        translate([0, 75]) square(40, center = true);
    }
    offset(2) {
        translate([0, 90]) rotate(180) line(30);
        translate([0, 90]) rotate(150) line(15);
        translate([0, 90]) rotate(210) line(15);
    }
}

module zoom() {
    children();
    difference() {
        union() {
            circle(r = 40);
            rotate(30)
                translate([0, -50])
                    offset(thin)
                        square([8, 70], center = true);
        }
        circle(r = 40 - thin);
    }
}

module cross() {
    square([thick, 35], center = true);
    square([35, thick], center = true);
}

module zoom_in() {
    zoom() cross();
}

module zoom_out() {
    zoom() {
        square([35, 10], center = true);
    }
}

module zoom_all() {
    zoom() {
        offset(rounding) difference() {
            square(95, center = true);
            square(85, center = true);
            rotate(45) square(95, center = true);
        }
    }
}

module zoom_text(angle, z) {
    text("A", 35, font = font);
    translate([40, 0]) text("A", 70, font = font);
    translate([17, z]) rotate(angle) offset(rounding) circle(14, $fn = 3);
}

module zoom_text_in() {
    zoom_text(90, 65);
}

module zoom_text_out() {
    translate([width, 0]) mirror([1, 0, 0]) zoom_text(-90, 71);
}

module undo() {
    offset(1) {
        difference() {
            circle(r = 40);
            translate([-8, 0]) circle(r = 32);
            translate([-50, -50]) square([100, 54]);
        }
    }
    translate([43, 0])
        polygon([[-35, 0], [0, 0], [0, 35]]);
}

module redo() {
    mirror([1, 0, 0]) undo();
}

module indent_document() {
    for (a = [ 0 : 3 ]) {
        x = abs(a - 1.5) < 1 ? 30 : 0;
        translate([x, 20 * a]) square([80 - x, thick]);
    }
    offset(2) children();
}

module indent() {
    indent_document() polygon([[2, 50], [17, 35], [2, 20]]);
}

module unindent() {
    indent_document() polygon([[2, 35], [17, 50], [17, 20]]);
}

module new() {
    export_paper();
    translate([width / 2, 2 * height / 5]) cross();
}

module open() {
    module small_paper() translate([4.5,5]*u) scale(0.77) export_paper();
    module folder() {
        square([20,22]*u-[w,w]);
        translate([5.5,0]*u) square([20,19]*u-[w,w]);
    }
    module flap() translate([rounding,rounding]) offset(r=rounding) polygon([[3,0]*u, [24,0]*u, [30,12]*u, [9,12]*u]);

    u = height/32;
    w = rounding + thin;
    difference() {
      translate([rounding, rounding]) offset(r=rounding) folder();
      translate([rounding, rounding]) offset(r=rounding) offset(-w) folder();
      offset(r=rounding) hull() small_paper();
    }
    difference() {
      small_paper();
      offset(r=rounding) flap();
    }
    flap();
}

module save() {
	u = height/32;
	difference() {
		square(32*[u,u]);
		translate([4,2]*u) square([24,14]*u);
		translate([8,19]*u) square([16, 23]*u);
		translate([100,100]) rotate(45) square([8,8]*u, center=true);
	}
	translate([18,21]*u) square([4,9]*u);
	translate([6,4]*u) square([20,2]*u);
	translate([6,8]*u) square([20,2]*u);
	translate([6,12]*u) square([20,2]*u);
}
