//create a multidimensional vector
//and output it to console.
//This will run into a limit eventually.
//OpenSCAD should not crash.

//As the implementation is recursive and adds multiple dimension per call,
//it should run into stack exhaust trying to create the echo string
//(and not into a stack exhaust due to recursive usermodule call).

rec();

module rec(a=1)
{
  echo(a);
  rec(
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
   [[[[[[[[[[[[
    a
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   ]]]]]]]]]]]]
   );
}
