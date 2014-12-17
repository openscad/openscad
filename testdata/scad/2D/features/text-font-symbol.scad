use <../../../ttf/marvosym-3.10/marvosym.ttf>
use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>
use <../../../ttf/amiri-0.106/amiri-regular.ttf>

// FIXME: Needs a freely distributable font that is encoded like Webdings
// FIXME: with Microsoft/System charmap and charcodes at 0xf000. Using
// FIXME: Amiri as placeholder for now...

o = 180;
t = [ "0123", "ABCD", "abcd" ];
f = [ "MarVoSym", /* "Webdings" */ "Amiri", "Liberation Sans:style=Regular" ];

// Validate that windows symbol fonts are handled correctly.
// When following the suggested encoding, the charmap is defined
// to use the charcodes at the 0xf000 private use area of Unicode.
// This is true for Webdings and Wingdings, other fonts ignore
// that, e.g. the MarVoSym has the charcodes in the normal places.
// Liberation Sans is just added as cross check that normal
// Unicode encoding also works.
for (a = [0 : len(f) - 1]) {
	for (b = [0 : len(t) - 1]) {
		translate([o * a - o, 10 + 60 * b]) {
			text(t[b], font = f[a], size = 40, halign = "center");
		}
	}
}
