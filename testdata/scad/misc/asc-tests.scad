//
// WARNING: this file contains some invalid UTF-8 chars
// at the end of the file. When editing make sure those
// are still unchanged.
//

a1 = "abcdefäöüß";
a2 = "012345!§$%";
u1 = "\u2190\u2191\u2193\u2192\U01F640\U01F0A1\U01F0D1";
u2 = "\U01F603";

echo(text = a1, codepoints = asc(a1));
echo(text = a2, codepoints = asc(a2));

echo(text = u1, codepoints = asc(u1));
echo(text = u2, codepoints = asc(u2));

echo(multiple_args = asc("a", "b", "", undef, "123"));

ra1 = chr(asc(a1));
ra2 = chr(asc(a2));
echo(equals = a1 == ra1, len_input = len(a1), len_output = len(ra1));
echo(equals = a2 == ra2, len_input = len(a2), len_output = len(ra2));


ru1 = chr(asc(u1));
ru2 = chr(asc(u2));
echo(equals = u1 == ru1, len_input = len(u1), len_output = len(ru1));
echo(equals = u2 == ru2, len_input = len(u2), len_output = len(ru2));

echo(asc(undef));
echo(asc(1/0));
echo(asc(-1/0));
echo(asc(0/0));
echo(asc([2:4]));
echo(asc([]));
echo(asc([1, 2, 3]));
echo(asc(["a", "b"]));

// invalid utf-8 string, text €ÄÖÜß as latin15 (bytes: A4 C4 D6 DC DF)
echo(asc("�����"));
