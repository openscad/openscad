// BE VERY CAREFUL MODIFYING THIS FILE.  It has downright peculiar line
// endings and editors and other tools are likely to damage it.
// Also, line numbers must match both the internal tests and the
// expected-output file.

// linenumber-NL-1.bin.scad tests runtime errors.
// linenumber-NL-2.bin.scad tests parse errors.
// The two files are identical except that -2 has a syntax error at the
// very end.  The commented-out syntaxXX lines can be used to isolate
// problems.

// Following lines are terminated by LF alone (UNIX standard).

// This comment tests line number tracking in C++ comments.

echo(line16);
// syntax17;

/*
 * This comment tests line number tracking in old-school comments.
 */

echo(line23);
// syntax24;

// The following tests line number tracking in include and use.
include
<
line29
line30
>

echo(line33);
// syntax34;

use
<
line38
line39
>

echo(line42);
// syntax43;

// The following tests line number tracking in strings.
group() { x="
"; }
echo(line48);
// syntax49;


// Following lines are terminated by CRLF (Windows standard).

// This comment tests line number tracking in C++ comments.

echo(line56);
// syntax57;

/*
 * This comment tests line number tracking in old-school comments.
 */

echo(line63);
// syntax64;

// The following tests line number tracking in include and use.
include
<
line69
line70
>

echo(line73);
// syntax74;

use
<
line78
line79
>

echo(line82);
// syntax83;

// The following tests line number tracking in strings.
group() { x="
"; }
echo(line88);
// syntax89;


// Get exotic:  lines terminated by CR alone (Old MacOS, obscure systems).
// This comment tests line number tracking in C++ comments.echo(line96);// syntax97;/* * This comment tests line number tracking in old-school comments. */echo(line103);// syntax104;// The following tests line number tracking in include and use.include<line109line110>echo(line113);// syntax114;use<line118line119>echo(line122);// syntax123;// The following tests line number tracking in strings.group() { x=""; }echo(line128);// syntax129;
