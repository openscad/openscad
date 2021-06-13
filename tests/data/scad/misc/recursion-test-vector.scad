// Test for recursion crashing when vectors are parameters to a module
// See github issue1407

rec();

module rec(a=1)
{
  rec([a,10,10]);
}
