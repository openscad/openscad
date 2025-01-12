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
# python.exe mingw_convert_ctest.py
#   This will overwrite CTestTestfile.cmake and CTestCustom.cmake.
#   If all goes well, 'ctest' can then be run. 
#
# python.exe mingw_convert_ctest.py --debug
#   This will provide extra debug info
#
# python.exe mingw_convert_ctest.py --undo
#   This will restore original CTest files from backup

# Note that under 'cmake', Windows(TM) paths use / instead of \ as a 
# folder separator.

# mingw_cross_info.py is created by scripts/release-common.sh during build 
# of the regression test package. it contains info regarding paths in
# CTestTestfiles.cmake that we need to modify
import mingw_cross_info
import sys,os,string,re
from winreg import *

_debug=False
_undo=False
if '--debug' in sys.argv: _debug=True
if '--undo' in sys.argv: _undo=True

def debug(*args):
    global _debug
    if _debug:
        print('mingw_x_testfile:', end=" ")
        for arg in args: print(arg, end=" ")
        print()

thisfile_abspath=os.path.abspath(__file__)

linbase=mingw_cross_info.linux_abs_basedir
winbase=os.path.dirname(os.path.dirname(thisfile_abspath))

linbuild=mingw_cross_info.linux_abs_builddir
winbuild=winbase

lintct=linbase+'/tests/test_cmdline_tool.py'
wintct=winbase+'/tests/test_cmdline_tool.py'

linpy=mingw_cross_info.linux_python #'/usr/bin/python3'
winpy=sys.executable

executable = "openscad.com"
linosng=linbuild + '/' + executable
winosng=winbuild + '\\' + executable

linconv=mingw_cross_info.linux_convert #'/usr/bin/convert'

linoslib='OPENSCADPATH='+linbase+'/libraries'
winoslib='OPENSCADPATH='+winbase+'/libraries'

lintestdeploy=linbuild+'/tests'
wintestdeploy=winbuild+'/tests-build'

lintestdata=linbase+'/tests/data'
wintestdata=winbase+'/tests/data'

linexamples=linbase+'/examples'
winexamples=winbase+'/examples'

def find_imagemagick():
    # Find imagemagick's convert.exe
    imbase=''
    winconv=''
    try:
        regkey = OpenKey(HKEY_LOCAL_MACHINE, r'SOFTWARE\ImageMagick\Current')
        imbase = QueryValueEx(regkey, 'BinPath')[0]
        print('Found ImageMagick path: ' + imbase)
    except Exception:
        # There are two possible registry views depending on 32bit or 64bit programs
        # If python and imagemagick architecture don't match then we need to check the other one
        import platform
        bitness = platform.architecture()[0]
        if bitness == '32bit':
            arch_flag = KEY_WOW64_64KEY
        elif bitness == '64bit':
            arch_flag = KEY_WOW64_32KEY

        try:
            regkey = OpenKey(HKEY_LOCAL_MACHINE, r'SOFTWARE\ImageMagick\Current', access=KEY_READ | arch_flag)
            imbase = QueryValueEx(regkey, 'BinPath')[0]
            print('Found ImageMagick path: ' + imbase)
        except Exception as e:
            print("Can't find imagemagick using registry... " + str(type(e))+str(e) )
            pass

    if imbase != '':
        winconv = find_imagemagick_bin(imbase)
        if winconv != '': return winconv 

    if winconv == '':
        print('Searching for ImageMagick in Program folders')
        for basedir in 'C:/Program Files','C:/Program Files (x86)':
            if os.path.isdir(basedir):
                pflist=os.listdir(basedir)
                for subdir in pflist:
                    if 'ImageMagick' in subdir:
                        imbase = basedir + '/' + subdir
                        winconv = find_imagemagick_bin(imbase)
                        if winconv != '': return winconv
    return ''

def find_imagemagick_bin(path):
    if os.path.isfile(path + '/convert.exe'):
        return path + '/convert.exe'
    elif os.path.isfile(path + '/magick.exe'):
        return path + '/magick.exe'
    else:
        return ''

def processfile(infilename, winconv):
    backup_filename = infilename+'.backup'
    # Don't backup with modified file if run twice
    if os.path.isfile(backup_filename):
        open(infilename,'wb').write(open(backup_filename,'rb').read())
    else:
        open(backup_filename,'wb').write(open(infilename,'rb').read())
    if _undo:
        return
    
    debug ('wrote backup of ',infilename,' to ',backup_filename)

    outfilename = infilename.replace('.cmake','.win.cmake')
    fin=open(infilename)
    lines=fin.readlines()
    fout=open(outfilename,'w')
    fout.write('#'+os.linesep)
    fout.write('# modified by mingw_convert_ctest.py'+os.linesep)
    fout.write('#'+os.linesep)

    debug('inputname',infilename)
    debug('outputname',outfilename)

    winbase2 = winbase.replace('\\','/')

    for line in lines:
        debug('input:',line)

        # special for CTestCustom.template + ctest bugs w arguments
        line=re.sub('--builddir=[^ "]*', '', line)

        line=line.replace(linosng,winosng)
        line=line.replace(lintestdeploy,wintestdeploy) # this should fix path of diffpng.  must come before linbuild replacement
        line=line.replace(linbuild,winbuild)
        line=line.replace(lintct,wintct)
        line=line.replace(linpy,winpy)
        #line=line.replace(linconv,winconv)
        line=line.replace(linoslib,winoslib)
        line=line.replace(lintestdata,wintestdata)
        line=line.replace(linexamples,winexamples)

        line=line.replace(linbase+'/',winbase2+'/')

        line=line.replace('\\"','__ESCAPE_WIN_QUOTE_MECHANISM__')
        line=line.replace('\\','/')
        line=line.replace('__ESCAPE_WIN_QUOTE_MECHANISM__','\\"')

        debug('output:',line)

        fout.write(line)

    print('backed up ' + infilename + ' to ' + backup_filename)
    print('processed ' + infilename + ' to ' + outfilename)
    fin.close()
    fout.close()
    open(infilename,'wb').write(open(outfilename,'rb').read())
    print('new version of ' + infilename + ' written')


def process_templates():
    cmakebase = winbase.replace('\\','/')+'/tests'
    templatepath = winbase + r'\tests\data\scad\templates'
    # iterate over scad cmake templates
    for filename in os.listdir(templatepath):
        if filename.endswith("-template.scad"):
            scadname = filename.replace("-template","")
            # search for path of template output
            scadpath = find_single_file(scadname, winbase + r'\tests\data\scad')
            if scadpath is not None:
                print('Ovewriting ' + scadpath + ' based on ' + filename + ' using path ' + cmakebase)
                fout = open(scadpath, 'w')
                fin = open(templatepath + '\\' + filename)
                for line in fin.readlines():
                    line = line.replace("@CMAKE_CURRENT_SOURCE_DIR@", cmakebase)
                    fout.write(line)
                fin.close()
                fout.close()
            else:
                print(scadname + " not found")

def find_single_file(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            return os.path.join(root, name)

def run():
    winconv = find_imagemagick()
    if winconv=='':
        print('error, cannot find convert.exe')

    if True:
        print(thisfile_abspath)
        print('linbase ' + linbase)
        print('winbase ' + winbase)
        print('lintestdeploy ' + lintestdeploy)
        print('wintestdeploy ' + wintestdeploy)
        print('linbuild ' + linbuild)
        print('winbuild ' + winbuild)
        print('lintct ' + lintct)
        print('wintct ' + wintct)
        print('linpy ' + linpy)
        print('winpy ' + winpy)
        print('linosng ' + linosng)
        print('winosng ' + winosng)
        #print('linconv ' + linconv) # use diffpng, included in archive, as opposed to ImageMagick
        #print('winconv ' + winconv)
        print('linoslib ' + linoslib)
        print('winoslib ' + winoslib)
        print('lintestdata ' + lintestdata)
        print('wintestdata ' + wintestdata)
        print('linexamples ' + linexamples)
        print('winexamples ' + winexamples)

    processfile('CTestTestfile.cmake', winconv)
    processfile('CTestCustom.cmake', winconv)

    process_templates()


if __name__=='__main__':
    run()
