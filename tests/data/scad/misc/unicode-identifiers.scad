größe = 10;
echo(größe);

// Identifiers are normalised to NFC, so the decomposed spelling of "höhe"
// names the same variable as the precomposed one.
// Assignment identifier: NFC: U+0068, U+00F6, U+0068, U+0065; UTF-8: 68 C3 B6 68 65
// Following echo identifier: NFD: U+0068, U+006F, U+0308, U+0068, U+0065; UTF-8: 68 6F CC 88 68 65
höhe = 4;
echo(höhe);

// U+2126 OHM SIGN is canonically equivalent to U+03A9 GREEK CAPITAL LETTER OMEGA,
// so the two spellings are the same identifier as well.
// Assignment identifier: NFC: U+03A9; UTF-8: CE A9
// Following echo identifier (before NFC normalization): Non-NFC: U+2126; UTF-8: E2 84 A6
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
