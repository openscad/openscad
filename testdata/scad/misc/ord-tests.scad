//
// WARNING: this file contains some invalid UTF-8 chars
// at the end of the file. When editing make sure those
// are still unchanged.
//

echo(ord("a"));
echo(ord("abc"));
echo(ord());
echo(ord(""));
echo(ord(undef));
echo(ord(1/0));
echo(ord(-1/0));
echo(ord(0/0));
echo(ord(3.1416));
echo(ord([]));
echo(ord([1, 2, 3]));
echo(ord(["a", "b"]));
echo(ord([1 : 5]));
echo(ord("foo", "bar"));
echo([for (c = "test") ord(c)]);
echo(chr([for (c = "test") ord(c)]));

a1 = "abcdefäöüß";
a2 = "012345!§$%";
u1 = "\u2190\u2191\u2193\u2192\U01F640\U01F0A1\U01F0D1";
u2 = "\U01F603";

echo(text = a1, codepoints = [for (c = a1) ord(c)]);
echo(text = a2, codepoints = [for (c = a2) ord(c)]);

echo(text = u1, codepoints = [for (c = u1) ord(c)]);
echo(text = u2, codepoints = [for (c = u2) ord(c)]);

ra1 = chr([for (c = a1) ord(c)]);
ra2 = chr([for (c = a2) ord(c)]);
echo(equals = a1 == ra1, len_input = len(a1), len_output = len(ra1));
echo(equals = a2 == ra2, len_input = len(a2), len_output = len(ra2));

ru1 = chr([for (c = u1) ord(c)]);
ru2 = chr([for (c = u2) ord(c)]);
echo(equals = u1 == ru1, len_input = len(u1), len_output = len(ru1));
echo(equals = u2 == ru2, len_input = len(u2), len_output = len(ru2));

// invalid utf-8 string, text €ÄÖÜß as latin15 (bytes: A4 C4 D6 DC DF)
echo([for (c = "�����") ord(c)]);
