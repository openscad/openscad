// 'after' is defined in 'include-overwrite-after.scad'
// 'overwriteFlase ' is defined before 'include-overwrite-before.scad'
// this works as intuitively expected
overwriteFalse  = false;
include <include-overwrite-after.scad>;
after = overwriteFalse;
echo(after=after);

//-----

include <include-overwrite-before.scad>;
// 'before' is defined in 'include-overwrite-before.scad'
// 'overwriteTrue ' is not defined in or before 'include-overwrite-before.scad'
// this causes 'before' to be undef
overwriteTrue  = true;
before = overwriteTrue;
echo(before=before);