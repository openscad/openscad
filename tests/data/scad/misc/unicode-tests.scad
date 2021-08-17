echo("A");

// \x only allows \x01 - \x7f, everything outside that range is not handled
echo("\x30\x31\x3a\x3A\x80\xFF");

echo("\u0 \U0");

echo("\u00 \U00");

echo("\u000 \U000");

echo("\u0000\u0030\u0031\u003a\u003A \u2698 \u27BE \U0000");

echo("\u00000 \U00000");

echo("\U000000\U000030\U000031\U00003a\U00003A \U01F638 \U01F0A1 ");

echo("\u0000000 \U0000000");

echo("B");
