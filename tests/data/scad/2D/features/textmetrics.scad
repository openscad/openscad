//use <../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

s = "OpenSCAD";
valign = "default";
halign = "default";
f = "Liberation Sans";
dir = "ltr";
size = 10;
spacing = 1;
bg = 1;
line = 0.2;

tm = textmetrics(s, size=size, valign=valign, halign=halign, font=f, direction=dir, spacing=spacing);
echo(tm=tm);
height = tm.ascent - tm.descent;

fm = fontmetrics(font=f, size=size);
echo(fm=fm);

module sampletext() {
    text(s, size=size, valign=valign, halign=halign, font=f, direction=dir, spacing=spacing);
}

// This is the bounding box.
difference() {
    translate(tm.position) square(tm.size);
    sampletext();
}
// Translate to the {x,y}_offset to to set the origin to
// the new origin set by halign/valign.
translate(tm.offset) {
    color("black") {
        // Down to descent
        translate([0,tm.descent]) {
            // Left side at origin
            square([line,height]);
            // Descent line
            square([tm.advance.x,line]);
            // Right side of advance
            translate([tm.advance.x,0]) {
                square([line,height]);
            }
        }
        // Baseline
        square([tm.advance.x,line]);
        // Ascent
        translate([0,tm.ascent])
            square([tm.advance.x, line]);
    }

    color("red") {
        // Font ascent
        translate([0,fm.nominal.ascent])
            square([tm.advance.x, line]);
        // Font descent
        translate([0,fm.nominal.descent])
            square([tm.advance.x, line]);
    }
    color("lightgreen") {
        // Font max ascent
        translate([0,fm.max.ascent])
            square([tm.advance.x, line]);
        // Font max descent
        translate([0,fm.max.descent])
            square([tm.advance.x, line]);
    }
    color("blue") {
        translate([0,-fm.interline])
            square([tm.advance.x, line]);
        translate([0,fm.interline])
            square([tm.advance.x, line]);
    }
}
