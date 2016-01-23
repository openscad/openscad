// 
% import("../3D/features/import.stl");
echo(read("../3D/features/import.stl"));

% linear_extrude(0.1) import(file="../../dxf/multiple-layers.dxf",layer="noname");
echo(read("../../dxf/multiple-layers.dxf","noname"));
cube();
