// from https://github.com/openscad/openscad/issues/6597#issuecomment-3836789009

// combo box for number
Numbers=2; // [0, 1, 2, 3]

// combo box for string
Strings="foo"; // [foo, bar, baz]

//labeled combo box for numbers
Labeled_values=10; // [10:S, 20:M, 30:L]

//labeled combo box for string
Labeled_value="S"; // [S:Small, M:Medium, L:Large]

// slider widget for number with max. value
sliderWithMax =34; // [50]

// slider widget for number in range
sliderWithRange =34; // [10:100]

//step slider for number
stepSlider=2; //[0:5:100]

// slider widget for number in range
sliderCentered =0; // [-10:0.1:10]

//description
Variable = true;

// spinbox with step size 1
Spinbox= 5;

// spinbox with step size 0.5
SpinboxWithStep= 5.5; // .5

// Text box for string
String="hello";

// Text box for string with length 8
StringWithLength="length"; //8

//Spin box box for vector with less than or equal to 4 elements
Vector2=[12,34];
Vector3=[12,34,45];
Vector4=[12,34,45,23];

VectorRange3=[12,34,46]; //[1:2:50]
VectorRange4=[12,34,45,23]; //[1:50]
