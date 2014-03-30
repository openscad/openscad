import os,sys

thisfile_abspath=os.path.abspath(__file__)
thisdir_abspath=os.path.abspath(os.path.dirname(thisfile_abspath))

starting_dir=os.path.join(thisdir_abspath,'tests-build')
print 'changing current folder to '+starting_dir
os.chdir(starting_dir)

print 'adding ',starting_dir,'folder to sys.path'
sys.path.append(starting_dir)
build_dir=os.path.join(starting_dir,'tests-build')
print 'adding ',build_dir,'folder to sys.path'
sys.path.append(build_dir)

print 'converting CTestTestfile.cmake by calling mingw_convert_test.py'
import mingw_convert_ctest
mingw_convert_ctest.run()

#cmd = 'start "OpenSCAD Test console" /wait /d c:\\temp cmd.exe'
#cmd = 'start /d "'+starting_dir+'" cmd.exe "OpenSCAD Test Console"'
cmd = 'start /d "'+starting_dir+'" cmd.exe mingwcon.bat'
print 'opening console: running ',cmd
os.system( cmd )

# figure out how to run convert script
# dont use mingw64 in linbuild path?

# run a batch file with greeting

# auto find 'ctest' binary and add to path?
# auto find 'python' binary and add to path?

# info on running ctest
# and link to doc/testing.txt and ctest website
# 
# figure out better windows prompt, can it be set?

