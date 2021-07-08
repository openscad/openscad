use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

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
height = tm.ascent - tm.descent;

fm = fontmetrics(font=f, size=size);

module sampletext() {
    text(s, size=size, valign=valign, halign=halign, font=f, direction=dir, spacing=spacing);
}

// XOR, at least in a sense.
// Include the "unique" parts of each child - that is, those parts
// that do not overlap with any other child.
module xor() {
    for (i = [0:$children-1]) {
        difference() {
            children(i);
            for (j = [0:$children-1]) {
                if (i != j) {
                    children(j);
                }
            }
        }
    }
}

module xline(len) {
    translate([0,-line/2]) square([len, line]);
}
module yline(len) {
    translate([-line/2,0]) square([line, len]);
}

xor() {
    // This is the bounding box.
    translate(tm.position) square(tm.size);
    sampletext();

    // Translate to the {x,y}_offset to to set the origin to
    // the new origin set by halign/valign.
    translate(tm.offset) {
        color("black") {
            // Down to descent
            translate([0,tm.descent]) {
                // Left side at origin
                yline(height);
                // Descent line
                xline(tm.advance.x);
                // Right side of advance
                translate([tm.advance.x,0]) {
                    yline(height);
                }
            }
            // Baseline
            xline(tm.advance.x);
            // Ascent
            translate([0,tm.ascent])
                xline(tm.advance.x);
        }

        color("red") {
            // Font ascent
            translate([0,fm.nominal.ascent])
                xline(tm.advance.x);
            // Font descent
            translate([0,fm.nominal.descent])
                xline(tm.advance.x);
        }
        color("lightgreen") {
            // Font max ascent
            translate([0,fm.max.ascent])
                xline(tm.advance.x);
            // Font max descent
            translate([0,fm.max.descent])
                xline(tm.advance.x);
        }
        color("blue") {
            translate([0,-fm.interline])
                xline(tm.advance.x);
            translate([0,fm.interline])
                xline(tm.advance.x);
        }
    }
}
