echo("include");
include<errors-warnings.scad>;
echo("use");
use<errors-warnings-use.scad>;
moduleWithError();
echo("mainfile");
echo(notDefinedVariable);
assert(false); //prevent geometry evalution