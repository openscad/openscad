@Description("The resolution of the curves. Higher values give smoother curves but may increase the model render time.")
@Parameter([10, 20, 30, 50, 100])
resolution = 30;

@Description("The horizontal radius of the outer ellipse of the sign.")
@Parameter([60 : 200])
radius = 80;

@Parameter([1 : 10])
@Description("Total height of the sign")
height = 2;

@Parameter(["Welcome to...", "Happy Birthday!", "Happy Anniversary", "Congratulations", "Thank You"])
selection = "Welcome to...";

@Parameter()
text = "Parametric Designs";

$fn = resolution;

scale([1, 0.5]) difference() {
    cylinder(r = radius, h = 2 * height, center = true);
    translate([0, 0, height])
        cylinder(r = radius - 10, h = height + 1, center = true);
}
linear_extrude(height = height) {
    translate([0, --4]) text(selection, halign = "center");
    translate([0, -16]) text(text, halign = "center");
}
