// Parameter set keys are compared against identifiers from the AST, which the
// lexer normalises to NFC. The accompanying .json spells both keys in NFD, so
// this only produces the values below if they are normalised on read.
// Set names are also compared in NFC. The .json includes an NFD "Größ" and
// an NFC "Höhe"; tests select each using the other spelling.

/* [Maße] */

// Following identifier: NFC: U+0068, U+00F6, U+0068, U+0065; UTF-8: 68 C3 B6 68 65
// Höhe des Teils
höhe = 10;      // [1:100]

// Following identifier: NFC: U+0077, U+0061, U+006E, U+0064, U+0073, U+0074, U+00E4, U+0072, U+006B, U+0065; UTF-8: 77 61 6E 64 73 74 C3 A4 72 6B 65
// Wandstärke
wandstärke = 1; // [0.5:0.5:10]

/* [Hidden] */

echo(höhe, wandstärke);
