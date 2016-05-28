Manual tests for pull request #1637, which fixes issue #214.

There are 4 'good' tests, which should render with no errors,
and 4 'bad' tests, which should fail with an error message.
The tests should be run both using the CLI, and using the GUI.

TODO: These tests should be automated.

1. Relative pathnames

The code that handles relative pathnames in import() statements has
been refactored, so test it. (Imported files are found relative to the directory
containing the script that performs the import, which may be different from
the directory containing the "root" script file, as in the case where A.scad
uses B.scad, which in turn imports an STL file.

Part of the pathname resolution code is different between the command line
case and the interactive GUI case, and both versions of this code were changed,
so it's necessary to test both cases.

2. Syntax errors in the presence of `include`

The actual bug fix is that, when a syntax error is encountered, we report
the file name and the correct line number. Previously,
* No file name was reported, which creates a problem if the syntax error was
  actually in an included file.
* The reported line number could be incorrect if the script uses `include`.

3. Run the tests

To run the tests using the CLI,
use this shell script:
```
scad=<pathname of project root>/openscad
for f in [gb]*.scad; do echo $f; $scad -o ,.stl $f; done
```

And here's the output:
```
bad-include-synerr.scad
ERROR: Parser error in file "/res/doug/src/openscad/testdata/manual/issue214/syntax-error-in-line-2.scad", line 2: syntax error

Can't parse file 'bad-include-synerr.scad'!

bad-includeX.scad
WARNING: Can't open import file '"cube.stl"'.
Current top level object is empty.
bad-synerr.scad
ERROR: Parser error in file "/res/doug/src/openscad/testdata/manual/issue214/bad-synerr.scad", line 3: syntax error

Can't parse file 'bad-synerr.scad'!

bad-useX2.scad
DEPRECATED: Imported file (cube2.stl) found in document root instead of relative to the importing module. This behavior is deprecated
good-include2.scad
good-includeX2.scad
good-use2.scad
good-useX.scad
```

I'm testing a bunch of combinations of `include` and `use` to make sure that
pathname interpretation still works as before. Note that `include` and `use`
use different algorithms for looking up relative pathnames in an `import`
statement, as reflected in these tests. The behaviour of `include` looks like
a bug. But, the behaviour is unchanged from before this pull request.

I have two test cases for syntax errors:
* `bad-include-synerr.scad` tests a syntax error in an included file:
  the reported filename is the name of the included file,
  and the line number is correct.
* `bad-synerr.scad` tests a syntax error in the root script, after including
  a file that doesn't have a syntax error. The improvement is that the reported
  line number is correct.

These tests should also be run in the GUI, since different code is involved.
Use a similar shell script:
```
scad=<pathname of project root>/openscad
for f in [gb]*.scad; do echo $f; $scad $f; done
```
This script will open each window one at a time; verify the results then close
the window, and the next window will pop up.
