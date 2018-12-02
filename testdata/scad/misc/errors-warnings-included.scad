echo("include");
include<errors-warnings.scad>;
include<../3D/features/for-tests.scad>;
echo("mainfile");
echo(notDefinedVariable);
echo("use");
use<errors-warnings-use.scad>;
moduleWithError();
