use <modlitexamples_find.scad>

cyl = module(s) { translate([s.x/2,s.y/2,0]) cylinder(d = min(s.x,s.y), h = s.z);};
cube1 = module(s) cube(s);

size = [20,20,40];
mycube = module cube1(size);
mycyl = module cyl(size *2);

function get_info(m) =
let(info_dict = [
  [mycube, ["mycube",size]],
  [mycyl, ["mycyl",size*2]]
])
find(info_dict,m);

module printinfo(m){
   m();
   info = get_info(m);
   echo(name = info[0]);
   echo(size = info[1]);
}
echo ("-----------");
printinfo(mycube);
echo ("-----------");

function get_y(m) = get_info(m)[1][1];

translate([0,get_y(mycube),0]){
  printinfo(mycyl);
}
echo ("-----------");



