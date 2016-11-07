//
// WARNING: this file contains some invalid UTF-8 chars
// at the end of the file. When editing make sure those
// are still unchanged.
//

a1 = "abcdefäöüß";
a2 = "012345!§$%";
u1 = "\u2190\u2191\u2193\u2192\U01F640\U01F0A1\U01F0D1";
u2 = "\U01F603";

echo(text = a1, codepoints = ord(a1));
echo(text = a2, codepoints = ord(a2));

echo(text = u1, codepoints = ord(u1));
echo(text = u2, codepoints = ord(u2));

echo(empty_string = ord(""));
echo(empty_strings = ord("", "", ""));
echo(empty_string_and_undef = ord(undef, ""));

echo(multiple_args = ord("a", "b", "", undef, "123"));

ra1 = chr(ord(a1));
ra2 = chr(ord(a2));
echo(equals = a1 == ra1, len_input = len(a1), len_output = len(ra1));
echo(equals = a2 == ra2, len_input = len(a2), len_output = len(ra2));

ru1 = chr(ord(u1));
ru2 = chr(ord(u2));
echo(equals = u1 == ru1, len_input = len(u1), len_output = len(ru1));
echo(equals = u2 == ru2, len_input = len(u2), len_output = len(ru2));

echo(ord(undef));
echo(ord(undef, undef));
echo(ord(1/0));
echo(ord(-1/0));
echo(ord(0/0));
echo(ord([2:4]));
echo(ord([]));
echo(ord([1, 2, 3]));
echo(ord(["a", "b"]));

// invalid utf-8 string, text €ÄÖÜß as latin15 (bytes: A4 C4 D6 DC DF)
echo(ord("�����"));
