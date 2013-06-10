color("red")
linear_extrude(height=30)
import("../../svg/layers.svg", layer="first");

color("green")
linear_extrude(height=60)
import("../../svg/layers.svg", layer="second");

color("blue")
linear_extrude(height=90)
import("../../svg/layers.svg", layer="third");

