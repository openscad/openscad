echo("include");
include<errors-warnings.scad>;
echo("mainfile");
echo(notDefinedVariable);
echo("use");
use<errors-warnings-use.scad>;
moduleWithError();