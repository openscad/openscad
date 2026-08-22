// Identifiers may contain non-ASCII letters. Requires --enable=unicode-identifiers.
// See https://github.com/openscad/openscad/issues/3736

größe = 10;
echo(größe);

// Identifiers are normalised to NFC, so the decomposed spelling of "höhe"
// (h, o, U+0308, h, e) names the same variable as the precomposed one.
höhe = 4;
echo(höhe);

// U+2126 OHM SIGN is canonically equivalent to U+03A9 GREEK CAPITAL LETTER OMEGA,
// so the two spellings are the same identifier as well.
Ω = 30;
echo(Ω);

π = 3;
echo(π);

function fläche(länge, breite) = länge * breite;
echo(fläche(2, 3));

module kästchen(kantenlänge) {
  echo(kantenlänge);
}
kästchen(7);

// Special variables keep their dollar sign, which is not part of XID_Start and
// is added to the profile explicitly.
$wandstärke = 50;
echo($wandstärke);

// Scripts other than Latin work the same way.
寸法 = 40;
echo(寸法);

// U+1E9E LATIN CAPITAL LETTER SHARP S is a letter and therefore allowed.
FUẞDICKE = 2;
echo(FUẞDICKE);
