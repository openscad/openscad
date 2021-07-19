use <../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

echo("Normal...");

// Force Liberation Sans since it's present on all platforms.
// Exercise normal operation with default and explicit settings.

// textmetrics()
echo(textmetrics("hello", font="Liberation Sans"));
echo(textmetrics("hello", font="Liberation Sans", size=20, direction="rtl",
    language="en", script="latin", halign="right", valign="center", spacing=2));

// fontmetrics()
echo(fontmetrics(font="Liberation Sans"));
echo(fontmetrics(font="Liberation Sans", size=20));

echo("Errors...");

// Exercise type checks on all arguments
echo(textmetrics(text=123, font=true, size=[], direction=0, language=[0:10],
    script=0, halign=0, valign=0, spacing=""));

// Exercise type checks on all arguments, and that "text" isn't allowed as
// an argument even though it's in the common argument processing.
echo(fontmetrics(text="bad", size=true, font=0));
