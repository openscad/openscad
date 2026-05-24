# Testing

Caution, this file may be outdated.

## Running Regression Tests

### Prerequisites

* Install the prerequisite helper programs on your system: `cmake`, `python3`, `python3-venv`, `python3-pip`.
* There are binary installer packages of these tools available for Mac, Win, Linux, BSD, and other systems.

### Building Test Environment

Test files will be automatically configured and built (but not run) as part of the main
pythonscad build. See `README.md` for how to get a build of the main pythonscad binary working.

### Running Tests

From your build directory:

* `ctest -j8`: Runs tests enabled by default using 8 parallel processes.
* `ctest -R <regex>`: Runs only matching tests.
  * Example: `ctest -R dxf`.
* `ctest -C <configs>`: Adds extended tests belonging to configs.
* Valid configs include:
  * **Default:** Run default tests.
  * **Heavy:** Run more time consuming tests (\> \~10 seconds).
  * **Examples:** Test all examples.
  * **Bugs:** Test known bugs (tests will fail).
  * **All:** Test everything.

## Running GUI Tests

GUI tests verify the user interface behavior. They require a window system to run (even if headless).

To run all GUI tests:

```bash
./pythonscad --run-all-gui-tests
```

On macOS, the executable is named `PythonSCAD` and must be run from inside the app bundle:

```bash
./PythonSCAD.app/Contents/MacOS/PythonSCAD --run-all-gui-tests
```

The GUI tests must be enabled during build with `-DENABLE_GUI_TESTS=ON` (enabled by default).

### Available GUI Tests

The following test suites are currently implemented:

* **TestTabManager**: Tests tab opening, closing, and management.
* **TestMainWindow**: Tests main window properties and behaviors.
* **TestModuleCache**: Tests the caching of modules and file reloading.

### Test Parameters

Currently, the `--run-all-gui-tests` flag runs all available GUI tests. There are no
command-line parameters to run specific GUI tests or filter them at this time.

When running GUI tests, the welcome screen (launching screen) is automatically skipped
to prevent blocking the tests.

## Adding a New Test

1. Create a test file at an appropriate location under `tests/data/`.
2. If the test is non-obvious, create a human readable description as comments in the
   test (or in another file in the same directory in case the file isn't human readable).
3. If a new test app was written, this must be added to `tests/CMakeLists.txt`.
4. Add the tests to the test apps for which you want them to run (in `tests/CMakeLists.txt`).
5. Rebuild the test environment.
6. Run the test with the environment variable `TEST_GENERATE=1`.
   * Example: `TEST_GENERATE=1 ctest -R mytest`.
   * This will generate a `mytest-expected.txt` file which is used for regression testing.
7. Manually verify that the output is correct (`tests/regression/<testapp>/mytest-expected.<suffix>`).
8. Run the test normally and verify that it passes: `ctest -R mytest`.

### Adding a New Example

This is almost the same as adding a new regression test:

1. Create the example under `examples/`.
2. Run the test with the environment variable `TEST_GENERATE=1`.
   * Example: `TEST_GENERATE=1 ctest -C Examples -R exampleNNN`.
   * This will generate a `exampleNNN-expected.txt` file which is used for regression testing.
3. Manually verify that the output is correct (`tests/regression/<testapp>/exampleNNN.<suffix>`).
4. Run the test normally and verify that it passes: `ctest -C Examples -R exampleNNN`.

## Troubleshooting

### Headless Unix Servers

If you are attempting to run the tests on a unix-like system but only have shell-console
access, you may be able to run the tests by using a virtual framebuffer program like Xvnc
or Xvfb.

For example :

```bash
Xvfb :5 -screen 0 800x600x24 &
DISPLAY=:5 ctest
```

or

```bash
xvfb-run ctest
```

Some versions of Xvfb may fail, however.

### Trouble Finding Libraries on Unix

To help CMake find eigen, OpenCSG, CGAL, Boost, and GLEW, you can use environment
variables when invoking cmake.

Examples:

```bash
OPENSCAD_LIBRARIES=$HOME cmake ..
CGALDIR=$HOME/CGAL-3.9 BOOSTDIR=$HOME/boost-1.47.0 cmake ..
```

Valid variables are as follows :

* `BOOSTDIR`, `CGALDIR`, `EIGENDIR`, `GLEWDIR`, `OPENCSGDIR`, `OPENSCAD_LIBRARIES`

When running, this might help find your locally built libraries (assuming you installed into `$HOME`) :

* **Linux:** `export LD_LIBRARY_PATH=$HOME/lib:$HOME/lib64`
* **Mac:** `export DYLD_LIBRARY_PATH=$HOME/lib`

### Location of Logs

* Logs of test runs and a pretty-printed index.html are found in `build/Testing/Temporary`.
* Expected results are found in `tests/regression/*`.
* Actual results are found in `build/tests/output/*`.

### Alternatives to the image\_compare.py Image Comparison Script

If cmake is given the option `-DUSE_IMAGE_COMPARE_PY=OFF` then ImageMagick comparison
and fallback to diffpng are available.

* Note that ImageMagick tests are less sensitive because they are pixel-based with a large
  threshold while `image_compare.py` checks for any 3x3 blocks (with overlap) that have
  non-zero differences of the same sign.
* With `-DUSE_IMAGE_COMPARE_PY=OFF` additional options are available :
  * `-DCOMPARATOR=ncc`: Normalized Cross Comparison which is less accurate but more
    runtime stable on some ImageMagick versions.
  * `-DCOMPARATOR=old`: Lowered reliability test that works on older ImageMagick versions.
    Use this with "morphology not found" in the log.

### Locale Errors

This error is a boost/libstdc++ bug:

```text
terminate called after throwing an instance of 'std::runtime_error' what():
locale::facet::_S_create_c_locale name not valid
```

Fix like so before running :

```bash
export LC_MESSAGES=
```

### I Want to Build Without OpenGL

There is an unsupported way to do this, by defining NULLGL to CMake :

```bash
mkdir nullglbin
cd nullglbin && cmake .. -DNULLGL=1 && make
```

The resulting binary (named `pythonscad` / `PythonSCAD`) will fail most tests, but may
be useful for debugging and outputting 3d-formats like STL on systems without GL. This
option may break in the future and require tweaking to get working again.

### Proprietary GL Driver Issues

There are sporadic reports of problems running on remote machines with proprietary GL
drivers. Try doing a web search for your exact error message to see solutions and
workarounds that others have found.

### Windows + MSVC

The MSVC build was last tested circa 2012. The last time it worked, these were the
necessary commands to run :

```cmd
> Start the 'QT command prompt'
> cd \where\you\installed\pythonscad
> cd tests
> cmake. -DCMAKE_BUILD_TYPE=Release
> sed -i s/\/MD/\/MT/ CMakeCache.txt
> cmake.
> nmake -f Makefile
```

### Other Issues

The OpenSCAD User Manual Wiki has a section on building. Please check there for possible
updates and workarounds: `https://en.wikibooks.org/wiki/OpenSCAD_User_Manual`.

Please report build errors (after double checking the instructions) in the GitHub issue
tracker: `https://github.com/pythonscad/pythonscad/issues`.
