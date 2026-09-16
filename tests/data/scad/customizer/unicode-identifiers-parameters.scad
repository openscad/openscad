// Parameter set keys are compared against identifiers from the AST, which the
// lexer normalises to NFC. The accompanying .json spells both keys in NFD, so
// this only produces the values below if they are normalised on read.
// Requires --enable=unicode-identifiers.

/* [Maße] */

// Höhe des Teils
höhe = 10;      // [1:100]

// Wandstärke
wandstärke = 1; // [0.5:0.5:10]

/* [Hidden] */

echo(höhe, wandstärke);
