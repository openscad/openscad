/*
dict is an array of pairs whose first element is the key
and second element is the value to return on match
*/
function find (dict, key, pos = 0) =
   (pos >= len(dict) )
      ? undef
      :(dict[pos][0] == key)
         ? dict[pos][1]
         : find(dict, key , pos +1)
;

// example dictionary
shape_lookup = [
  ["cylinder", module(hgt) cylinder(d= 10,h=hgt,$fn = 20,center = true)],
  ["cube",module(side,side1) cube([side,side1,side], center = true)],
  ["pill", module {
      hull(){
         ts = module (z) {
            translate([0,0,z])sphere(d = 18,$fn = 50);
         };
         ts(10);
         ts(-10);
      }
   } 
  ]
];

cyl = find(shape_lookup,"cylinder");
echo(cyl);
cyl(20);

cube1 = find(shape_lookup,"cube");
echo(cube1);
translate([0,20,0]){cube1(20,10);}

pill = find(shape_lookup,"pill");
echo(pill);
translate([0,40,0]){pill();}

