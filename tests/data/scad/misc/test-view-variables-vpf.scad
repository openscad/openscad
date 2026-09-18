// Set $vpf from the script, to check what happens when --fov is also given.
// The command line wins; the script's value is reported as ignored.

$vpf = 5;

echo($vpf=$vpf);
