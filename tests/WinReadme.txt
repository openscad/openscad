Thank you for downloading the OpenSCAD test suite for Windows(TM). 

The test suite provides a basic set of regression tests to determine if 
OpenSCAD runs as expected from release to release, and from platform to 
platform. To use this test suite you must also have the following 
systems installed on your machine:

   Python 3      https://www.python.org
   CMake         https://www.cmake.org
   ImageMagick   https://www.imagemagick.org

To run the test suite, first click on the "OpenSCAD_Test_Console.py" file to
run it. It should open a cmd.exe console in the tests-build folder. Type

   ctest

and the machine should run a basic test suite. An html file summarizing 
the results will be produced at the end of the test run. The file can be 
opened with a web browser, and shared with others as part of the 
debugging and testing process.

Thanks for helping test OpenSCAD. 

See doc/testing.txt in the OpenSCAD source code for more details.

Known bugs:

-These scripts will not find ImageMagick or CMake if they are not 
installed on the C: drive under Program Files*. As a workaround, you can 
edit the OpenSCAD_Test_Console.py file and the .py files in the 
tests-build directory.

-'Diff' text-tests may not run properly on Windows(TM). 

-Detection of ctest, python, and imagemagick doesn't always work properly

