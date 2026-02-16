$fa = $preview ? 5 : 1;
$fs = $preview ? 1 : 0.2;

width = 100;
height = 100;
paper_width = 80;
thin = 4;
thick = 10;
rounding = 2;
corner_size = 30;

list_icons = false;
selected_icon = undef;

font = "Open Sans:style=Regular";
export_font = "Open Sans:style=Bold";

icons = [
    ["export-stl"],
    ["export-obj"],
    ["export-off"],
    ["export-wrl"],
    ["export-amf"],
    ["export-3mf"],
    ["export-dxf"],
    ["export-svg"],
    ["export-csg"],
    ["export-pdf"],
    ["export-png"],
    ["export-pov"],
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
    ["file"],
    ["folder"],
    ["open"],
    ["save"],
    ["reset-view"],
    ["view-right"],
    ["view-left"],
    ["view-back"],
    ["view-front"],
    ["view-top"],
    ["view-bottom"],
    ["perspective"],
    ["orthogonal"],
    ["axes"],
    ["scalemarkers"],
    ["show-edges"],
    ["crosshairs"],
    ["animate-play"],
    ["animate-disabled"],
    ["animate-pause"],
    ["surface"],
    ["wireframe"],
    ["throwntogether"],
    ["vcr-control-start"],
    ["vcr-control-step-back"],
    ["vcr-control-play"],
    ["vcr-control-pause"],
    ["vcr-control-step-forward"],
    ["vcr-control-end"],
    ["measure-distance"],
    ["measure-angle"],
    ["edit-copy"],
    ["up"],
    ["down"],
    ["add"],
    ["remove"],
    ["parameter"],
    ["loading"],
    ["circle-checkmark"],
    ["circle-error"],
    ["filetype-autodetect"],
    ["filetype-scad"],
    ["filetype-python"],
];

icon(selected_icon) {
    export("STL");
    export("OBJ");
    export("OFF");
    export("WRL");
    export("AMF");
    export("3MF");
    export("DXF");
    export("SVG");
    export("CSG");
    export("PDF");
    export("PNG");
    export("POV");
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
	file();
	folder();
    open();
    save();
    reset_view();
    view_right();
    view_left();
    view_back();
    view_front();
    view_top();
    view_bottom();
    perspective();
    orthogonal();
    axes();
    scalemarkers();
    show_edges();
    crosshairs();
    animate();
    animate_disabled();
    animate_pause();
    surface_();
    wireframe();
    throwntogether();
    vcr_control_start();
    vcr_control_step_back();
    vcr_control_play();
    vcr_control_pause();
    vcr_control_step_forward();
    vcr_control_end();
    measure_dist();
    measure_ang();
	edit_copy();
	up();
	down();
	add();
	remove();
	parameter();
	loading();
	circle_checkmark();
    circle_error();
    filetype_autodetect();
    filetype_scad();
    filetype_python();
}

if (list_icons) {
    for (a = [ 0 : len(icons) - 1 ]) {
        echo(icon = icons[a][0]);
    }
}

module icon(icon) {
    assert(len(icons) == $children, "list of icon names needs to be same length as number of child modules to icon()");
    cnt = $children;
    cols = ceil(pow(cnt, 0.55));
    if (is_undef(icon)) {
        for (i = [0:cnt - 1]) {
            pos = 200 * [i % cols + 1, floor(i / cols) + 1];
            translate(pos) {
                %translate([width / 2, -40]) text(icons[i][0], 20, halign = "center");
                box();
                children(i);
            }
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

module connect_points(cnt, pos) {
    for (idx = [0:cnt - 1]) {
        p = pos(idx);
        hull() {
            translate(p[0])
                children();
            translate(p[1])
                children();
        }
    }
}

module connect_points_dotted(cnt, pos, step = 1) {
    for (idx = [0:cnt - 1]) {
        p = pos(idx);
        v = p[1] - p[0];
        l = norm(v);
        cnt = ceil(l / step);
        s = l / cnt;
        for (a = [0 : cnt])
            translate(p[0] + a * s * v / l)
                children();
    }
}

module box(center = false) {
    module b(o, border) {
        size = width + o;
        render() difference() {
            translate([-border - o / 2, -border - o / 2]) square(size + 2 * border, center = center);
            translate([-o / 2, -o / 2]) square(size, center = center);
        }
    }

    color("black", 0.1) %b(0, rounding);
    color("red", 0.1) %b(16, rounding);
}

module paper() {
    w = rounding + thin;
    offset(rounding)
        translate([w, w])
            square([paper_width - 2 * w, height - 2 * w]);
}

module export_paper() {
    difference() {
        outline(thin) paper();
        translate([-1, height - corner_size + 1]) square([corner_size, corner_size]);
    }
    export_paper_corner();
}

module text_paper() {
	export_paper();
	x = [30, 40, 40, 10, 50, 40];
	for (y = [0:5])
	  translate([15, 11 * y + 17])
		square([x[y], 3]);
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
        translate([10, 0]) export_paper();
        translate([-20, -5]) square([height, 55]);
    }
    resize([75, 40], true)
        text(t, 40, font = export_font);
}

module hourglass() {
    module side() {
        hull() { translate([-1, 14]) circle(d = 1); translate([-7, 3]) circle(d = 1); }
        hull() { translate([-1, 14]) circle(d = 1); translate([-7, 24]) circle(d = 1); }
    }

    side();
    mirror([1, 0]) side();
    translate([0, 2]) square([18, 2], center = true);
    translate([0, 25]) square([18, 2], center = true);
    polygon([[-5, 3.5], [5, 3.5], [0.1, 6], [0.2, 14.5], [4, 21], [-4, 21], [-0.2, 14.5], [-0.1, 6]]);
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

module preview_cube(size = 40) {
    offset(2) {
        children(0);
        rotate(-60) children(0);
        rotate(60) children(0);
        translate([0, size]) rotate(-60) children(0);
        translate([0, size]) rotate(60) children(0);
        translate([-sin(60) * size, cos(60) * size]) children(0);
        translate([sin(60) * size, cos(60) * size]) children(0);
        translate([-sin(60) * size, cos(60) * size + size]) rotate(-60) children(0);
        translate([sin(60) * size, cos(60) * size + size]) rotate(60) children(0);
    }
}

module render_() {
    translate([width / 2, 10]) {
        difference() {
            preview_cube() line();
            polygon([[-15, 36], [15, 36], [9, 18], [25, -10], [-25, -10], [-9, 18]]);
        }
        translate([0, -10]) scale(1.6) hourglass();
    }
}

module preview() {
    translate([width / 2, 10]) {
        difference() {
            preview_cube() line_dotted();
            translate([0, 8]) square([35, 28], center = true);
        }
        h = 14;
        translate([0, 4]) polygon([[-18, h], [-9, h], [0, 0], [-9, -h], [-18, -h], [-9, 0]]);
        translate([18, 4]) polygon([[-18, h], [-9, h], [0, 0], [-9, -h], [-18, -h], [-9, 0]]);
    }
}

module send() {
    translate([width / 2, 10]) {
        difference() {
            preview_cube() line();
            translate([0, 75]) square(40, center = true);
        }
        translate([0, 87]) offset(2) {
            rotate(180) line(30);
            rotate(150) line(15);
            rotate(210) line(15);
        }
    }
}

module zoom(r = 30) {
    children();
    difference() {
        union() {
            circle(r = r);
                rotate(210)
                    offset(thin)
                        translate([-thick/2, 0])
                            square([thick, 61]);
        }
        circle(r = r - thin);
    }
}

module cross() {
    square([thin, 32], center = true);
    square([32, thin], center = true);
}

module zoom_in() {
    r = 30;
    translate([r + thick, height - r - thick])
        zoom(r)
            cross();
}

module zoom_out() {
    r = 30;
    translate([r + thick, height - r - thick])
        zoom(r)
            square([35, thin], center = true);
}

module zoom_all() {
    r = 30;
    translate([r + thick, height - r - thick])
        zoom() {
            w = 75;
            offset(rounding) difference() {
                square(w, center = true);
                square(w - thin, center = true);
                rotate(45) square(85, center = true);
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

module curved_arrow() {
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

module undo() {
    translate([width / 2, height / 3])
        mirror([1, 0, 0])
            curved_arrow();
}

module redo() {
    translate([width / 2, height / 3])
        curved_arrow();
}

module indent_document() {
    for (a = [ 0 : 3 ]) {
        x = abs(a - 1.5) < 1 ? 30 : 0;
        translate([x, 20 * a]) square([80 - x, thick]);
    }
    offset(2) children();
}

module indent() {
    translate([thick, thick])
        indent_document()
            polygon([[2, 50], [17, 35], [2, 20]]);
}

module unindent() {
    translate([thick, thick])
        indent_document()
            polygon([[2, 35], [17, 50], [17, 20]]);
}

module new() {
    translate([10, 0]) {
        export_paper();
        translate([paper_width / 2, 2 * height / 5]) cross();
    }
}

module file() {
    u = height/32;
    translate([10, 0]) {
        export_paper();
    }
    translate([8,6]*u) square([16,1]*u);
    translate([8,11]*u) square([16,1]*u);
    translate([8,16]*u) square([16,1]*u);
    translate([16,21]*u) square([8,1]*u);
    translate([16,24]*u) square([8,1]*u);
}

module folder_backside() {
    u = height/32;
    w = rounding + thin;
	square([24,28]*u-[w,w]);
	translate([5.5,0]*u) square([22,22]*u-[w,w]);
}

module folder_outline() {
    w = rounding + thin;
    difference() {
      translate([rounding, rounding])
		offset(r=rounding)
			folder_backside();
      translate([rounding, rounding])
		offset(r=rounding)
			offset(-w)
				folder_backside();
    }
}

module folder_flap() {
    u = height/32;
	translate([rounding,rounding])
		offset(r=rounding)
			polygon([[3,0]*u, [26,0]*u, [30,15]*u, [7,15]*u]);
}

module folder() {
	folder_outline();
    folder_flap();
}

module open() {
    module small_paper() translate([4.5,5]*u) scale(0.77) export_paper();

    u = height/32;
    w = rounding + thin;
    difference() {
		folder_outline();
		offset(r=rounding) hull() small_paper();
    }
    difference() {
      small_paper();
      offset(r=rounding) folder_flap();
    }
    folder_flap();
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

module reset_view() {
    r = 0.35 * width;
    translate([width / 2, height / 2]) {
        difference() {
            outline(thin) circle(r);
            polygon([[0, 0], [-width, -width / 2], [-width, width / 2]]);
        }
        rotate(60) translate([0, r + thin / 2]) rotate(-10)
            polygon([[-thin, 0], [thick, -thick], [thick, thick]]);
    }
}

module view_axes(eyepos) {
  b = 3;
  L = 33;
  Lo = L/2-1;
  projection() rotate([-65, 0, 0]) rotate([0, 0, -25]) {
    translate([Lo, 0, 0]) cube([L, b, b], center=true);
    translate([-Lo, 0, 0]) cube([L, b, b], center=true);
    translate([0, Lo, 0]) cube([b, L, b], center=true);
    translate([0, -Lo, 0]) cube([b, L, b], center=true);
    translate([0, 0, Lo]) cube([b, b, L], center=true);
    translate([0, 0, -Lo]) cube([b, b, L], center=true);
  }

  translate(eyepos) {
    difference() {
      intersection() {
        translate([0, 20]) circle(30);
        translate([0, -20]) circle(30);
      }
      circle(9);
    }
  }
}

module view_cube_pos() {
    translate([width / 2 - 8, height / 2 + 5])
        children();
}

module view_text(t) {
    translate([width - 2, 0])
        resize([38, 40])
            text(t, 40, font = font, halign="right", spacing = 0.8);
}

module view_right() {
    view_cube_pos()
        view_axes([40, +2]);
    view_text("+X");
}

module view_left() {
    view_cube_pos()
        view_axes([-27, 15]);
    view_text("-X");
}

module view_back() {
    view_cube_pos()
        view_axes([28, 24]);
    view_text("+Y");
}

module view_front() {
    view_cube_pos()
        view_axes([-28, -23]);
    view_text("-Y");
}

module view_top() {
    view_cube_pos()
        view_axes([0, 41]);
    view_text("+Z");
}

module view_bottom() {
    view_cube_pos()
        view_axes([0, -41]);
    view_text("-Z");
}

module perspective() {
    s1 = 25;
    s2 = 5;
    o1 = [30, 30];
    o2 = [90, 90];
    p = [[-1, 1], [1, 1], [1, -1], [-1, -1]];
    connect_points(4, function(i) [o1 + s1 * p[i], o1 + s1 * p[(i + 1) % 4]]) circle(d = thin);
    connect_points(2, function(i) [o2 + s2 * p[i], o2 + s2 * p[(i + 1) % 4]]) circle(d = thin);
    connect_points(3, function(i) [o1 + s1 * p[i], o2 + s2 * p[i]]) circle(d = thin);
}

module orthogonal() {
    o1 = [40, 40];
    o2 = [60, 60];
    p = 30 * [[-1, 1], [1, 1], [1, -1], [-1, -1]];
    connect_points(4, function(i) [o1 + p[i], o1 + p[(i + 1) % 4]]) circle(d = thin);
    connect_points(4, function(i) [o2 + p[i], o2 + p[(i + 1) % 4]]) circle(d = thin);
    connect_points(4, function(i) [o1 + p[i], o2 + p[i]]) circle(d = thin);
}

module axes() {
    p = [ for (r = [10, 40], a = [0, -90, 135]) r * [-sin(a), cos(a) ] ];

    translate([width / 2, height / 2]) {
        connect_points(2, function(i) [p[i], p[i + 3]]) circle(d = thin);
        connect_points_dotted(1, function(i) [p[i + 2], p[i + 5]], 1.2 * thin) circle(d = 0.8 * thin);
        circle(thin);
    }
}

module scalemarkers() {
    module markers() {
        connect_points(3, function(i) let(x = 18 + 10 * i) [[x, 0], [x, 8]]) circle(d = 0.5 * thin);
    }

    axes();
    translate([width - 8, 10]) text("10", 30, font = font, halign="right", spacing = 0.8);
    translate([width / 2, height / 2]) {
        markers();
        rotate(90) mirror([0, 1, 0]) markers();
        rotate(225) mirror([0, 1, 0]) markers();
    }
}

module show_edges() {
    p = [[-35, 25], [35, 25], [35, -25], [-35, -25]];

    translate([width / 2, height / 2]) {
        for (p = p) translate(p) circle(d = thick);
        difference() {
            offset(thin/2) polygon(p);
            offset(-thin/2) polygon(p);
            for (p = p) translate(p) square(3 * thick, center = true);
        }
    }
}

module crosshairs() {
    r1 = 15;
    r2 = 40;

    translate([width / 2, height / 2]) {
        for (a = [20, -20]) {
            hull() {
                translate(r1 * [cos(a), sin(a)]) circle(d = thin);
                translate(r1 * [-cos(-a), sin(-a)]) circle(d = thin);
            }
        }
        for (a = [30, -30]) {
            hull() {
                translate(r2 * [sin(a), cos(a)]) circle(d = thin);
                translate(r2 * [sin(-a), -cos(-a)]) circle(d = thin);
            }
        }
    }
}

module animate() {
    translate([width / 2, height / 2]) {
        outline(thin) circle(d = 0.8 * width);
        circle(d = 0.5 * width, $fn = 3);
    }
}

module animate_disabled() {
    translate([width / 2, height / 2]) {
        outline(thin) circle(d = 0.8 * width);
        circle(d = 0.5 * width, $fn = 3);
        rotate(50) square([thin, 0.8 * width], center = true);
    }
}

module animate_pause() {
    translate([width / 2, height / 2]) {
        outline(thin) circle(d = 0.8 * width);
        for (x = [-thick, thick]) {
            translate([x, 0]) square([thick, 0.4 * width], center = true);
        }
    }
}

module surface_() {
    translate([width / 2, height / 2]) {
        circle(d = 0.8 * width);
    }
}

module wireframe() {
    r = 0.4 * width;
    translate([width / 2, height / 2]) {
        outline(thin) circle(r = r - thin / 2);
        difference() {
            outline(thin) scale([0.3, 1]) circle(r = r - thin / 2);
            translate([0, -height / 2]) square([width, height]);
        }
        for (a = [20, -20]) {
            hull() {
                translate(r * [cos(a), sin(a)]) circle(d = thin);
                translate(r * [-cos(a), sin(a)]) circle(d = thin);
            }
        }
    }
}

module throwntogether() {
    module c() translate([10, 50]) preview_cube(15) line(15);

    translate([width / 2, 10]) {
        difference() {
            preview_cube(35) line(35);
                hull() c();
        }
        c();
    }
}

module vcr_control_start(){
    offset(rounding)
    translate([width/2,height/2])
    rotate([0,180,0]){
        x =  0.5 * width /2;
        translate([-x, 0])
        circle(d = 0.5 * width, $fn = 3);
        circle(d = 0.5 * width, $fn = 3);
       translate([x, 0]) square([thick, 0.4 * width], center = true);
    }
}
module vcr_control_step_back(){
    offset(rounding)
    translate([width/2,height/2])
    rotate([0,180,0]){
        circle(d = 0.5 * width, $fn = 3);
        x =  0.5 * width /2;
       translate([x, 0]) square([thick, 0.4 * width], center = true);
    }
}

module vcr_control_play(){
    offset(rounding)
    translate([width/2,height/2])
    circle(d = 0.5 * width, $fn = 3);
}

module vcr_control_pause(){
    offset(rounding)
    translate([width/2,height/2])
    for (x = [-thick, thick]) {
        translate([x, 0]) square([thick, 0.4 * width], center = true);
    }
}

module vcr_control_step_forward(){
    offset(rounding)
    translate([width/2,height/2]){
        circle(d = 0.5 * width, $fn = 3);
        x =  0.5 * width /2;
       translate([x, 0]) square([thick, 0.4 * width], center = true);
    }
}

module vcr_control_end(){
    offset(rounding)
    translate([width/2,height/2]){
        x =  0.5 * width /2;
        translate([-x, 0])
        circle(d = 0.5 * width, $fn = 3);
        circle(d = 0.5 * width, $fn = 3);
       translate([x, 0]) square([thick, 0.4 * width], center = true);
    }
}

module measure_dist(){
    x =  0.75 * width /2;
    a = width*0.2;
    t = thin*0.1;
    offset(rounding)
    translate([width/2,height/2]){
        for(mirr=[1,0,0]) mirror([mirr,0,0])  {
            translate([x, 0]) square([t, 0.8 * width], center = true);
        polygon([[x-t,-15],[x-a-t,a-15],[x-a-t,-a-15]]);
        }
       translate([0,-15])  square([2*x, thin], center = true);
    }
    translate([25,50])
    resize([40, 40], true)
    text("10", 40, font = export_font);

}

module measure_ang() {
    x =  0.75 * width /2;
    a = width*0.2;
    offset(rounding)
    translate([width/2,height/2]){
        for(mirr=[1,0,0]) mirror([mirr,0,0])  {
            translate([0, -x]) rotate([0,0,45]) square([thin, 0.8 * width/sqrt(2)]);
        }
    }
    translate([width*0.5,width*0.6]) scale(0.7) curved_arrow();
    translate([30,30])
    resize([40, 40], true)
    text("45", 40, font = export_font);
}

module edit_copy() {
	union() {
		difference() {
			translate([10, 30]) scale(0.7) text_paper();
			translate([26, 5]) scale(0.7) paper();
		}
		translate([32, -1]) scale(0.7) text_paper();
	}
}

module simple_arrow() {
	polygon([
		[         0,  height / 3],
		[-width / 5, -height / 3],
		[         0, -height / 4],
		[ width / 5, -height / 3],
	]);
}

module up() {
    translate([width / 2, height / 2])
		simple_arrow();
}

module down() {
    translate([width / 2, height / 2])
		rotate(180)
			simple_arrow();
}

module add() {
    translate([width / 2, height / 2]) {
		square([0.8 * width, thick], center = true);
		square([thick, 0.8 * width], center = true);
	}
}

module remove() {
    translate([width / 2, height / 2]) {
		square([0.8 * width, thick], center = true);
	}
}

module gear(r) {
	difference() {
		union() {
			circle(r);
			for (a = [0:3])
				rotate(45 * a)
					offset(thin) offset(-thin)
						square([2.4 * r, r/2.0], center = true);
		}
		circle(r - 1.5 * thick);
	}
}

module parameter() {
    r = 0.35 * width;
    translate([width / 2, height / 2]) {
		difference() {
			gear(r);
			offset(-thin) gear(r);
		}
	}
}

module loading() {
    n = 12;
    translate([width / 2, height / 2]) {
        for (a = [0:n-2]) {
            rotate(-a * 360 / n)
                translate([width / 3, 0])
                    circle(2 + a / 3 * 2);
        }
	}
}

module circle_checkmark() {
    r = 0.3 * width;
    translate([width / 2, height / 2]) {
        outline(thin) circle(r = r - thin / 2);
        translate([-2, -r / 2]) {
            hull() {
                circle(d = thick);
                translate([35, 35]) circle(d = thick);
            }
            hull() {
                circle(d = thick);
                translate([-15, 15]) circle(d = thick);
            }
        }
    }
}

module circle_error() {
    l = 28;
    r = 0.3 * width;
    translate([width / 2, height / 2]) {
        outline(thin) circle(r = r - thin / 2);
        for (a = [0, 90]) {
            rotate(a) {
                hull() {
                    translate([l, l]) circle(d = thick);
                    translate([-l, -l]) circle(d = thick);
                }
            }
        }
    }
}

// Based on https://openclipart.org/detail/114883/ftwizard-by-anonymous
module filetype_autodetect() {
    l = 28;
    r = 0.4 * width;
    module l(r, p1, p2) {
        hull() {
            translate(p1 * width / 2) circle(r);
            translate(p2 * width / 2) circle(r);
        }
    }
    translate([width / 2, height / 2]) {
        l(6.0, [-0.62, -0.66], [0.186, 0.213]);
        l(3.2, [0.065, 0.480], [-0.127, 0.776]);
        l(3.2, [0.276, 0.504], [0.324, 0.826]);
        l(3.2, [0.434, 0.344], [0.591, 0.439]);
        l(3.2, [0.405, 0.095], [0.758, -0.031]);
    }
}

module filetype_scad() {
    l = 28;
    r = 0.4 * width;

    module x () {
        hull() {
            translate([-width / 4.0, -width / 7.8])
            rotate(-61) scale([1, 0.52]) circle(r = 0.19 * width);
            translate([-width / 2.6, -width / 5.2])
            rotate(-61) scale([1, 0.55]) circle(r = 0.19 * width);
        }
    }

    translate([width / 2, height / 2]) {
        difference() {
            outline(thin) circle(r = r - thin / 2);
            x();
        }
        translate([0, width / 3.60])
        outline(thin) scale([1, 0.50]) circle(r = 0.19 * width);

        translate([width / 4.3, -width / 7.5])
        rotate(61) outline(thin) scale([1, 0.55]) circle(r = 0.19 * width);
        outline(thin) x();
        translate([-width / 2.6, -width / 5.2])
        rotate(-61) outline(thin) scale([1, 0.55]) circle(r = 0.19 * width);
        outline(thin)
        difference() {
            translate([width / 1.63, width / 3.2]) x();
            offset(thin) circle(r = r - thin / 2);
        }
    }
}

module filetype_python() {
    module p() {
        outline(thin) difference() {
            polygon(python_points);
            translate([-13.5, 37.7]) circle(4.5);
        }
    }
    translate([width / 2, height / 2]) {
        p();
        rotate(180) p();
    }
}


// Outline of a single snake, based on the official SVG
// from https://www.python.org/community/logos/
python_points = [
[ -26.8573,-12.8772 ], [ -26.807,-11.7181 ], [ -26.6587,-10.5773 ], [ -26.4167,-9.45905 ], [ -26.0851,-8.36797 ],
[ -25.6679,-7.30849 ], [ -25.1695,-6.28505 ], [ -24.5938,-5.30211 ], [ -23.9452,-4.36414 ], [ -23.2276,-3.47559 ],
[ -22.4453,-2.64092 ], [ -21.6024,-1.86458 ], [ -20.703,-1.15103 ], [ -19.7513,-0.504739 ], [ -18.7515,0.069845 ],
[ -17.7076,0.568262 ], [ -16.6239,0.986054 ], [ -15.5045,1.31876 ], [ -14.3535,1.56193 ], [ -13.175,1.7111 ],
[ -11.9733,1.76182 ], [ 11.8245,1.76182 ], [ 12.8076,1.80175 ], [ 13.7678,1.91945 ], [ 14.7019,2.11176 ],
[ 15.6072,2.37552 ], [ 16.4805,2.70757 ], [ 17.319,3.10475 ], [ 18.1197,3.56392 ], [ 18.8795,4.08191 ],
[ 19.5956,4.65556 ], [ 20.265,5.28172 ], [ 20.8847,5.95724 ], [ 21.4518,6.67894 ], [ 21.9632,7.44368 ],
[ 22.4161,8.24831 ], [ 22.8074,9.08965 ], [ 23.1342,9.96456 ], [ 23.3936,10.8699 ], [ 23.5825,11.8025 ],
[ 23.6981,12.7591 ], [ 23.7372,13.7367 ], [ 23.7372,36.1759 ], [ 23.6969,37.1217 ], [ 23.5782,38.042 ],
[ 23.3845,38.9353 ], [ 23.119,39.7999 ], [ 22.7851,40.6343 ], [ 22.3861,41.4368 ], [ 21.9253,42.2058 ],
[ 21.406,42.9397 ], [ 20.8317,43.6369 ], [ 20.2055,44.2959 ], [ 19.5308,44.9149 ], [ 18.8109,45.4925 ],
[ 18.0492,46.0269 ], [ 17.249,46.5166 ], [ 16.4135,46.9599 ], [ 15.5462,47.3553 ], [ 14.6503,47.7012 ],
[ 13.7292,47.9959 ], [ 12.7861,48.2379 ], [ 11.8245,48.4255 ], [ 11.2094,48.5238 ], [ 10.5923,48.6167 ],
[ 9.97339,48.704 ], [ 9.3529,48.786 ], [ 8.73108,48.8626 ], [ 8.10818,48.9338 ], [ 7.48445,48.9997 ],
[ 6.86013,49.0604 ], [ 6.23547,49.1157 ], [ 5.61071,49.1658 ], [ 4.9861,49.2108 ], [ 4.36189,49.2506 ],
[ 3.73832,49.2852 ], [ 3.11563,49.3147 ], [ 2.49407,49.3392 ], [ 1.87389,49.3587 ], [ 1.25533,49.3731 ],
[ 0.638643,49.3826 ], [ 0.0240675,49.3871 ], [ -0.588148,49.3868 ], [ -1.19769,49.3815 ], [ -1.80427,49.3715 ],
[ -2.40767,49.3567 ], [ -3.00769,49.3371 ], [ -3.6041,49.3129 ], [ -4.1967,49.2841 ], [ -4.78527,49.2507 ],
[ -5.3696,49.2128 ], [ -5.94947,49.1705 ], [ -6.52468,49.1237 ], [ -7.09501,49.0726 ], [ -7.66025,49.0172 ],
[ -8.22018,48.9576 ], [ -8.7746,48.8937 ], [ -9.32329,48.8257 ], [ -9.86603,48.7537 ], [ -10.4026,48.6776 ],
[ -10.9328,48.5974 ], [ -11.4565,48.5134 ], [ -11.9733,48.4255 ], [ -13.4246,48.1466 ], [ -14.7569,47.838 ],
[ -15.975,47.4987 ], [ -17.0838,47.1278 ], [ -18.0881,46.7245 ], [ -18.9927,46.2879 ], [ -19.8025,45.817 ],
[ -20.5221,45.3111 ], [ -21.1565,44.7691 ], [ -21.7105,44.1902 ], [ -22.1889,43.5735 ], [ -22.5965,42.9181 ],
[ -22.9381,42.2232 ], [ -23.2185,41.4878 ], [ -23.4425,40.7111 ], [ -23.615,39.8921 ], [ -23.7408,39.03 ],
[ -23.8247,38.1238 ], [ -23.8715,37.1728 ], [ -23.8861,36.1759 ], [ -23.8861,27.1947 ], [ -0.0605425,27.1947 ],
[ -0.0605425,24.201 ], [ -32.8276,24.201 ], [ -33.8594,24.1702 ], [ -34.8758,24.0779 ], [ -35.8743,23.9245 ],
[ -36.8524,23.7103 ], [ -37.8078,23.4355 ], [ -38.7378,23.1004 ], [ -39.64,22.7055 ], [ -40.5119,22.2508 ],
[ -41.3511,21.7369 ], [ -42.1551,21.1639 ], [ -42.9213,20.5322 ], [ -43.6474,19.8421 ], [ -44.3308,19.0938 ],
[ -44.969,18.2877 ], [ -45.5597,17.4241 ], [ -46.1002,16.5032 ], [ -46.5881,15.5254 ], [ -47.021,14.491 ],
[ -47.3963,13.4003 ], [ -47.7116,12.2536 ], [ -48.024,10.9316 ], [ -48.3049,9.65486 ], [ -48.5539,8.41803 ],
[ -48.771,7.21575 ], [ -48.9558,6.04265 ], [ -49.1081,4.89337 ], [ -49.2278,3.76256 ], [ -49.3146,2.64486 ],
[ -49.3683,1.5349 ], [ -49.3887,0.427333 ], [ -49.3755,-0.68321 ], [ -49.3286,-1.80209 ], [ -49.2477,-2.93465 ],
[ -49.1326,-4.08627 ], [ -48.9831,-5.2623 ], [ -48.7989,-6.46811 ], [ -48.5799,-7.70904 ], [ -48.3258,-8.99046 ],
[ -48.0365,-10.3177 ], [ -47.7116,-11.6962 ], [ -47.4399,-12.7273 ], [ -47.1325,-13.726 ], [ -46.789,-14.6903 ],
[ -46.4091,-15.6177 ], [ -45.9924,-16.5062 ], [ -45.5383,-17.3535 ], [ -45.0467,-18.1574 ], [ -44.517,-18.9156 ],
[ -43.9489,-19.6259 ], [ -43.3419,-20.2861 ], [ -42.6957,-20.894 ], [ -42.0099,-21.4474 ], [ -41.284,-21.944 ],
[ -40.5178,-22.3816 ], [ -39.7107,-22.7579 ], [ -38.8624,-23.0709 ], [ -37.9725,-23.3181 ], [ -37.0405,-23.4975 ],
[ -36.0662,-23.6067 ], [ -35.0491,-23.6437 ], [ -26.8573,-23.6437 ],
];
