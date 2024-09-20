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
    ["save"],
    ["open"],
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
    ["animate"],
    ["animate_disabled"],
    ["animate_pause"],
    ["surface"],
    ["wireframe"],
    ["throwntogether"],
    ["vcr-control-start"],
    ["vcr-control-step-back"],
    ["vcr-control-play"],
    ["vcr-control-pause"],
    ["vcr-control-step-forward"],
    ["vcr-control-end"],
    ["measure-dist"],
    ["measure-ang"],
    ["edit-copy"],
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
    save();
    open();
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
}}
