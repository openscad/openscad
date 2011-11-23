fn_setting = 41; // does not work
//fn_setting = 40; // works

// basic box sizes
box_width = 720;
box_depth = 450;
box_height = 90;
box_bevel = 35;

union ()
{
	translate ([(box_width / 2) - box_bevel, -((box_depth / 2) - box_bevel), 0])
	cylinder (h = box_height- box_bevel, r = box_bevel, center = false, $fn = fn_setting);

	translate ([0, -((box_depth / 2) - box_bevel), box_height- box_bevel])
	rotate ([0, 90, 0])
	cylinder (h = box_width - (2 * box_bevel), r = box_bevel, center = true, $fn = fn_setting);

	translate ([(box_width / 2) - box_bevel, 0, box_height- box_bevel])
	rotate ([90, 90, 0])
	cylinder (h = box_depth - (2 * box_bevel), r = box_bevel, center = true, $fn = fn_setting);
}
