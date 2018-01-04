#!/usr/bin/env python

#
# Mingw_x_testfile - convert paths in CTestTestfile.cmake so they work 
# under Windows(TM). 
#



# Usage:
# 
# In Windows(TM) open a console, cd to the test suite as built by
# openscad/scripts/release-common.sh tests, then run this script.
#
# C:
# cd C:\temp\OpenSCAD-Tests-2012.03.40
# C:\python27\python.exe mingw_convert_ctest.py
#   This will overwrite CTestTestfile.cmake and CTestCustom.cmake.
#   If all goes well, 'ctest' can then be run. 
#
#
# C:\python27\python.exe mingw_convert_ctest.py --debug
#   This will provide extra debug info
#
# C:\python27\python.exe mingw_convert_ctest.py --undo
#   This will restore original CTest files from backup


#
# This is a primitive version - it assumes the locations for several files,
# including ImageMagick, Python, and several other programs.
#

# Note that under 'cmake', Windows(TM) paths use / instead of \ as a 
# folder separator.

# mingw_cross_info.py is created by scripts/release-common.sh during build 
# of the regression test package. it contains info regarding paths in
# CTestTestfiles.cmake that we need to modify
import mingw_cross_info
import sys,os,string
from _winreg import *
_debug=False
_undo=False
def debug(*args):
    global _debug
    if _debug:
        print 'mingw_x_testfile:',
        for arg in args: print arg,
        print

thisfile_abspath=os.path.abspath(__file__)

linbase=mingw_cross_info.linux_abs_basedir
winbase=os.path.dirname(os.path.dirname(thisfile_abspath))

linbuild=mingw_cross_info.linux_abs_builddir
winbuild=winbase+'/'+mingw_cross_info.bindir

lintct=linbase+'/tests/test_cmdline_tool.py'
wintct=winbase+'/tests/test_cmdline_tool.py'

linpy=mingw_cross_info.linux_python #'/usr/bin/python'
# FIXME - find python
winpy='c:/python27/python.exe'

linosng=linbuild+'/openscad_nogui.exe'
winosng=winbuild+'/openscad_nogui.exe'

linconv=mingw_cross_info.linux_convert #'/usr/bin/convert'
# Find imagemagick's convert.exe
list64=[]
list32=[]
imbase=''
winconv=''
try:
    registry = ConnectRegistry(None,HKEY_LOCAL_MACHINE)
    regkey = OpenKey(registry, r"SOFTWARE\ImageMagick\Current")
    imbinpath = QueryValueEx(regkey, 'BinPath')[0]
except Exception as e:
    print "Can't find imagemagick using registry...",
    print str(type(e))+str(e)
    pass

print 'Searching for ImageMagick in Program folders'
for basedir in 'C:/Program Files','C:/Program Files (x86)':
    if os.path.isdir(basedir):
        pflist=os.listdir(basedir)
        for subdir in pflist:
            if 'ImageMagick' in subdir:
                imbase = basedir+'/'+subdir
                winconv = imbase+'/convert.exe'
                if os.path.isfile(winconv):
                    break
    if winconv != '': break
if winconv=='':
    print 'error, cant find convert.exe'

linoslib='OPENSCADPATH='+linbase+'/tests/../libraries'
winoslib='OPENSCADPATH='+winbase+'/tests/../libraries'

lintestdata=linbase+'/tests/../testdata'
wintestdata=winbase+'/tests/../testdata'

linexamples=linbase+'/tests/../examples'
winexamples=winbase+'/tests/../examples'

if '--debug' in string.join(sys.argv): _debug=True
if '--undo' in string.join(sys.argv): _undo=True

if True:
    print thisfile_abspath
    print 'linbase',linbase
    print 'winbase',winbase
    print 'linbuild',linbuild
    print 'winbuild',winbuild
    print 'lintct',lintct
    print 'wintct',wintct
    print 'linpy',linpy
    print 'winpy',winpy
    print 'linosng',linosng
    print 'winosng',winosng
    print 'linconv',linconv
    print 'winconv',winconv
    print 'linoslib',linoslib
    print 'winoslib',winoslib
    print 'lintestdata',lintestdata
    print 'wintestdata',wintestdata
    print 'linexamples',linexamples
    print 'winexamples',winexamples

def processfile(infilename):
    backup_filename = infilename+'.backup'
    if _undo:
        open(infilename,'wb').write(open(backup_filename,'rb').read())
        print 'restored ',infilename,'from backup'
        return
    open(backup_filename,'wb').write(open(infilename,'rb').read())
    debug ('wrote backup of ',infilename,' to ',backup_filename)

    outfilename = infilename.replace('.cmake','.win.cmake')
    fin=open(infilename,'rb')
    lines=fin.readlines()
    fout=open(outfilename,'wb')
    fout.write('#'+os.linesep)
    fout.write('# modified by mingw_x_testfile.py'+os.linesep)
    fout.write('#'+os.linesep)

    debug('inputname',infilename)
    debug('outputname',outfilename)

    for line in lines:
        debug('input:',line)

        # special for CTestCustom.template + ctest bugs w arguments
        line=line.replace('--builddir='+linbuild,'')

        line=line.replace(linbuild,winbuild)
        line=line.replace(lintct,wintct)
        line=line.replace(linpy,winpy)
        line=line.replace(linosng,winosng)
        line=line.replace(linconv,winconv)
        line=line.replace(linoslib,winoslib)
        line=line.replace(lintestdata,wintestdata)
        line=line.replace(linexamples,winexamples)

        line=line.replace(linbase,winbase)

        line=line.replace('\\"','__ESCAPE_WIN_QUOTE_MECHANISM__')
        line=line.replace('\\','/')
        line=line.replace('__ESCAPE_WIN_QUOTE_MECHANISM__','\\"')

        debug('output:',line)

        fout.write(line)

    print 'backed up',infilename, 'to', backup_filename
    print 'processed',infilename, 'to', outfilename
    fin.close()
    fout.close()
    open(infilename,'wb').write(open(outfilename,'rb').read())
    print 'new version of',infilename,'written'

def run():
    processfile('CTestTestfile.cmake')
    processfile('CTestCustom.cmake')

if __name__=='__main__':
    run()

