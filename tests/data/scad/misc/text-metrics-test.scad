use <../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

// Redact the font version information, ref #6774.
function fontmetrics_redact(fm) = object(fm, font=object(fm.font, version="<redacted>"));

echo("Normal...");

// Force Liberation Sans since it's present on all platforms.
// Exercise normal operation with default and explicit settings.

// textmetrics()
echo(textmetrics("hello", font="Liberation Sans"));
echo(textmetrics("hello", font="Liberation Sans", size=20, direction="rtl",
    language="en", script="latin", halign="right", valign="center", spacing=2));

// fontmetrics()
fm1 = fontmetrics(font="Liberation Sans");
assert(fm1.font.version);   // Assert that we do get a version, though we're going to redact it.
echo(fontmetrics_redact(fm1));
echo(fontmetrics_redact(fontmetrics(font="Liberation Sans", size=20)));

echo("Errors...");

// Exercise type checks on all arguments
echo(textmetrics(text=123, font=true, size=[], direction=0, language=[0:10],
    script=0, halign=0, valign=0, spacing=""));

// Exercise type checks on all arguments, and that "text" isn't allowed as
// an argument even though it's in the common argument processing.
echo(fontmetrics_redact(fontmetrics(text="bad", size=true, font=0)));
