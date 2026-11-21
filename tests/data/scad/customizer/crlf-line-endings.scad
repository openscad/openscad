// Customizer annotations in a file with CRLF line endings, see #7173.
// Keep the CRLF line endings, see .gitattributes.

/* [Drop down boxes] */

// combo box for string
side = "Left"; // [Left, Right]

// labeled combo box for numbers
size = 20; // [10:S, 20:M, 30:L]

/* [Sliders] */

// slider widget for number in range
width = 20; // [10:100]

// step slider for number
step = 5; // [0:5:100]

/* [Other] */

// spinbox with step size 0.5
spinbox = 5.5; // .5

// text box for string with length 8
label = "length"; // 8

cube(width);
