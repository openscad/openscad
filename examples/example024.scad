// example024 rendering an image via read_* and polyset().

render_obj=2;

// echo usage
tmp_image=read_image();
tmp_dxf=read_dxf();
tmp_stl=read_stl();
tmp_rgb=read_rgb();

translate([0,40,0]) assign(readImg=read_image("example024.png",true,0.5) ) {
  echo(str("Number of polygons: ",len(readImg)));
  echo(str("  First polygon: ",readImg[0]));
  //numPolys=len(readImg);
  echo(str("  Last polygon: ", readImg[ (len(readImg)-1) ] ) );
  echo(str("  Bounds check: ", readImg[ len(readImg) ] ) );
  rotate([90,0,0]) scale([1,1,4]) polyset(data=readImg);
  translate([0,0,-30]) polyset(data=readImg);
}

translate([0,15,0]) {
  polyset(data=read_stl("example012.stl",4));
}

translate([20,-50,0]) {
  polyset(data=read_dxf("example008.dxf","G"),convexity=2);
}

translate([-50,-50,0]) assign(readImg=read_rgb("example024_10x10.png")) {
  scale([5,5,5]) polyset(data=readImg,convexity=2);
  echo(str("read_rgb: ",readImg));
}

