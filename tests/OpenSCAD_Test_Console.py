# Openscad Test Console
#
# Script to make it easier to pull up a command-line console or
# running Ctest under Windows(TM)
#
# public domain, by Don Bright <hugh.m.bright@gmail.com>

import os,sys

thisfile_abspath=os.path.abspath(__file__)
thisdir_abspath=os.path.abspath(os.path.dirname(thisfile_abspath))

starting_dir=os.path.join(thisdir_abspath,'tests-build')
print 'changing current folder to '+starting_dir
os.chdir(starting_dir)

print 'adding ',starting_dir,'folder to sys.path'
sys.path.append(starting_dir)

build_dir=starting_dir

print 'converting CTestTestfile.cmake by calling mingw_convert_test.py'
import mingw_convert_ctest
mingw_convert_ctest.run()

print 'searching for ctest.exe'
ctestpath=''
for basedir in 'C:/Program Files','C:/Program Files (x86)':
        if os.path.isdir(basedir):
            pflist = os.listdir(basedir)
            for subdir in pflist:
                if 'cmake' in subdir.lower():
                    abssubdir=os.path.join(basedir,subdir)
                    for root,dirs,files in os.walk(abssubdir):
                        if 'ctest.exe' in files:
                            ctestpath=os.path.join(root,'ctest.exe')

if not os.path.isfile(ctestpath):
        print 'error, cant find ctest.exe'
else:
    ctestdir = os.pathsep + os.path.dirname(ctestpath)
    print 'adding ctest dir to PATH:',ctestdir
    os.environ['PATH'] += ctestdir

#cmd = 'start "OpenSCAD Test console" /wait /d c:\\temp cmd.exe'
#cmd = 'start /d "'+starting_dir+'" cmd.exe "OpenSCAD Test Console"'
conbat=os.path.join(build_dir,'mingwcon.bat')
cmd = 'start /d "'+starting_dir+'" cmd.exe "/k" "'+conbat+'"'
print 'opening console: running ',cmd
os.system( cmd )

# figure out how to run convert script
# dont use mingw64 in linbuild path?
# figure out better windows prompt, can it be set?


