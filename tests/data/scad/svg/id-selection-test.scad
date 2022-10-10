// single element
translate([  0, 400]) import("../../svg/display.svg", id="rect11");
// selection overrides invisibility of single element
translate([200, 400]) import("../../svg/display.svg", id="rect13");
// undef is the default and selects everything
translate([400, 400]) import("../../svg/display.svg", id=undef);
// group (internal invisibilities respected)
translate([  0, 200]) import("../../svg/display.svg", id="group1");
// selection overrides invisibility of group
translate([200, 200]) import("../../svg/display.svg", id="group3");
// unknown id selects nothing
translate([400, 200]) import("../../svg/display.svg", id="");
translate([400, 200]) import("../../svg/display.svg", id="doesnotexist");
// id from defs selects all instances
translate([  0,   0]) import("../../svg/display.svg", id="tworects");
// id from defs overrides invisibility
translate([200,   0]) import("../../svg/display.svg", id="hiddenrect");
// use id selects only that single use and overrides invisibility
translate([400,   0]) import("../../svg/display.svg", id="use2");
// full file for reference
translate([  0, -200]) import("../../svg/display.svg");
