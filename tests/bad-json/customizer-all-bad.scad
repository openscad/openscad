// Using all the parameters in the customizer
//
// parameters are adjusted in the Customizer
//  to define a parameter use comments and a variable
//    //Description
//    variable=value; //Parameter Widget spec
//
// Comments of this form define a group of parameters
//  /*[ group name ]*/
//
/* [Combo Boxes] */
// select a number
Numbers=2; // [0, 1, 2, 3,4]
echo( "Number selected ", Numbers );

// select a string
Strings="foo"; // [foo, bar, baz]
echo( "String selected ", Strings );

num_select_rev_lookup = object(
  [
  ["5","S"],["10","L"], ["20","M"], ["30","XL"]
  ]);
//labeled combo box for numbers
Labeled_Values=10; // [5:S, 10:L, 20:M, 30:XL]
echo( "Selected ",
    num_select_rev_lookup[str(Labeled_Values)],
    " got ",
    Labeled_Values
    );

str_select_rev_lookup = object(
  [
  ["S","Small"],["M","Medium"], ["L","Large"]
  ]);
//labeled combo box for string
Labeled_Strings="S"; // [S:Small, M:Medium, L:Large]

echo( "Selected ",
    str_select_rev_lookup[Labeled_Strings],
    " got ",
    Labeled_Strings
    );

/*[ Sliders ]*/
// slider widget for number
slider =34; // [10:100]
echo( "Slider Selected ", slider );

//step slider for number
stepSlider=2; //[0:5:100]
echo( "Step Selected ", stepSlider );

/* [Checkboxes (booleans)] */

// 2B or Not 2B
Two_Bee = true;
echo( Two_Bee ? true : false, " selected " );

/*[Spinbox Group] */

// spinbox with step size 1
SpinCube = 5;
echo( "Spinner Cube Size ", SpinCube );
translate([0,0,15]) cube(SpinCube);

// spinbox with step size 5
SpinSphere = 5;  //[0:5:100]
PolySides  = 1;  //[1:1:100]
echo( "Spinner Sphere Size ", SpinSphere );
translate([0,6,0]) sphere(SpinSphere, $fn=PolySides);

/* [Textbox Group] */
// Text box for string
TextBox="hello";
echo( "What you said ", TextBox );

/* [Vector Examples Group] */
//Text boxes for up to 4 elements
Vector1=[12]; //[0:2:50]
v1=[Vector1.x, Vector1.x, Vector1.x];
translate(v1) color("green")
  cube(SpinCube);
echo("Vector 1 moves the green cube.");

Vector2=[12,34]; //[-50:2:50]
translate(Vector2)
  color("red")
    sphere(SpinSphere, $fn=5);
echo("Vector 2 translates the Red Sphere only in the X-Y plane");

Vector3=[12,34,46]; //[-50:2:50]
translate(Vector3)
  color("yellow")
    sphere(SpinSphere, $fn=8);
echo("Vector 2 translates the Yellow Sphere 3D space");

Vector4=[.5,.5,.5,.5]; //[0:0.1:1.0]
translate(v1*-1.0)
  color(Vector4)
    sphere(SpinSphere, $fn=PolySides);
echo("Vector 1 also moves the multicolored sphere");

/* [Functions] */
// function literals are only in versions
//  after 2025
// has to be last because if() {} creates a local scope
//  that stops the customizer collecting widgets


/* [Objects] */
// objects are only in versions from 2025
// has to be last because if() {} creates a local scope
//  that stops the customizer collecting widgets

barney_data = [["name","Barney"],["spouse","Betty"]];


if( version_num() >= 2.02503e7 ) {
// set up for selecting a function
  f = function (s) str( "f ",s);
  g = function (s) is_num(s)?
    "g: want string"
    :
    str("<", s, ">")
    ;
  h = function (s) is_string(s)?
    "h: want number"
    :
    s * 12
    ;
  //labeled combo box for functions
  functions=g; // [f():Eff, g():Gee, h():Ach]

  echo( "Function gets string ", fs=functions("test"));
  echo( "Function gets number ", fn=functions(3));
  }



if( version_num() >= 2.02503e7 ) {
// set up for selecting a function
  o = object( name="Fred", spouse="Wilma" );

  barney_data = [["name","Barney"],["spouse","Betty"]];
  p = object(barney_data);

  //labeled combo box for functions
  objects=o; // [o:Oh, p():Pee]

  echo( "Selected Objext ", objects);
  }

echo( "You wont see the next one" );
/* [Hidden] */
debugMode = true;


/*
 2025 by bitbasher (Jeff Hayes)

 To the extent possible under law, the author(s)
  have dedicated all copyright and related and
  neighboring rights to this software to the
  public domain worldwide. This software is
  distributed without anywarranty.

  You should have received a copy of the
  Creative Commons Zero (CC0)
  Public Domain Dedication along with this software.
  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
