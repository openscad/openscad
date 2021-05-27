echo(str("Can a variable be used when it assigned later? ",later));

echo(str("Is overwriting possible? ", overwritten));

echo(str("Does an include before the assignment take priority? ", before));

echo(str("Does an include after the assignment take priority? ", after));

use     <include-overwrite-use.scad>;
include <include-overwrite-before.scad>;

overwritten=false;
main = true;
before=false;
after=false;
overwritten=true;

include <include-overwrite-after.scad>;

later=true;
