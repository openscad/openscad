// example024 rendering an image via read() and polyset().

render_obj=1;

// echo usage
tmp_read=read();

if(render_obj==0 || render_obj==1) translate([0,40,0]) assign(readImg=read("example024.png",true,0.5) ) {
  echo(str("Number of polygons: ",len(readImg)));
  echo(str("  First polygon: ",readImg[0]));
  //numPolys=len(readImg);
  echo(str("  Last polygon: ", readImg[ (len(readImg)-1) ] ) );
  echo(str("  Bounds check: ", readImg[ len(readImg) ] ) );
  rotate([90,0,0]) scale([1,1,4]) polyset(data=readImg);
  translate([0,0,-30]) polyset(data=readImg);
}

if(render_obj==0 || render_obj==2) translate([0,15,0]) {
  polyset(data=read("example012.stl",4));
}

if(render_obj==0 || render_obj==3) translate([20,-50,0]) {
  polyset(data=read("example008.dxf","G"),convexity=2);
}
