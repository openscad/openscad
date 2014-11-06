linear_extrude(height=1) import_dxf("../../dxf/null-polygons.dxf");
translate([0,20,0]) linear_extrude("../../dxf/null-polygons.dxf", height=1);
