// Opting out restricts identifiers, while Unicode comments and strings remain
// valid and retain their original spelling, including decomposed characters.
size = 10;
$wall = 2;
function area(length, width) = length * width;
module box(edge) {
  echo(edge);
}
echo(size, $wall, area(2, 3));
box(7);
echo("größe", "höhe", "Ω", "µm");
