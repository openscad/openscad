module noop() {}

module children_test() {
	children() noop();
}
children_test() noop();

module child_test() {
	child() noop();
}
child_test() noop();

import("../../dxf/circle.dxf") noop();
linear_extrude(height = 100, file = "../../dxf-circle.dxf") noop();
rotate_extrude(height = 100, file = "../../dxf-circle.dxf") noop();

surface("../../image/smiley.png") noop();

text("Hello World!", 26, font = "Liberation Sans:style=Regular") noop();
