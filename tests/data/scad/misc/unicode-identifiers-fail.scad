// U+00B5 MICRO SIGN is excluded from the identifier profile by the general
// security profile of UTS #39: it is confusable with U+03BC GREEK SMALL LETTER
// MU, and NFC does not fold the two into one identifier.
// See https://github.com/openscad/openscad/issues/737

µm = 0.001;
echo(µm);
