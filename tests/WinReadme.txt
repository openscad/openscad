Thank you for downloading the OpenSCAD test suite for Windows(TM). 

The test suite provides a basic set of regression tests to determine if 
OpenSCAD runs as expected from release to release, and from platform to 
platform. To use this test suite you must also have the following 
systems installed on your machine:

   Python 2      http://www.python.org  (Note: Python 3 will not work)
   CMake         http://www.cmake.org
   ImageMagick   http://www.imagemagick.org

To run the test suite, first click on the "CTest_Cross_Console.py" file to
run it. It should open a cmd.exe console in the tests-build folder. Type

   ctest

and the machine should run a basic test suite. An html file summarizing 
the results will be produced at the end of the test run. The file can be 
opened with a web browser, and shared with others as part of the 
debugging and testing process.

Thanks for helping test OpenSCAD. 

Known bugs:

-CMake 2.8.x for Windows does not properly operate from folders with 
Unicode in the pathname. The workaround is to move the Tests to a folder 
that has no Unicode in any of the parent folder names, such as creating 
a folder called 'c:\temp'.

-The script will only find ImageMagick and Cmake if they are 
installed under standard locations (C:\Program Files) or if their 
executables are in the PATH environment variable. As a workaround, you 
can edit the OpenSCAD_Test_Console.py file to set their location.

-Text-tests that produce different output than expected will throw a 
failure to find the 'diff' command under windows.

See Also:

    ./doc/testing.txt
    http://github.com/openscad/openscad/issues
