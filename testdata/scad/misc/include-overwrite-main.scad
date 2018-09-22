echo(str("Can a variable be used when it assigned later? ",later))

echo(str("Is overwritting possible? ", overwritten));

echo(str("Does an include before the assigment take priority? ", before));

echo(str("Does an include after the assigment take priority? ", after));

include <include-overwrite-before.scad>;

overwritten=false;
main = true;
before=false;
after=false;
overwritten=true;

include <include-overwrite-after.scad>;

later=true;
