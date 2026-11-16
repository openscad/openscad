// linear_extrude(segments=...) with valid and invalid values.
// Values below 0 are clamped to 0 and non-integers are truncated, like slices=.
// Non-finite and non-number values are ignored with a warning.
for (s = [undef, -1, 0, 2.5, 8, 1/0, -1/0, 0/0, "8", true, [8], [1:3]]) {
  echo(segments = s);
  linear_extrude(height = 1, segments = s) square(1);
}
