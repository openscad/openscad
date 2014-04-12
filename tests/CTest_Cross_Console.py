# CTest Cross Console, by Don Bright http://github.com/donbright 
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are 
# met:
#
#  Redistributions of source code must retain the above copyright notice, this
#  list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright 
#  notice, this list of conditions and the following disclaimer in the 
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the organization nor the names of its
#  contributors may be used to endorse or promote products derived from
#  this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# This script attempts to provide a 'portable' way to run CTest tests on 
# machines other than the original build machine, for both 
# cross-compiled and non-cross-compiled situations. The running of this 
# script will, in theory, modify CTestTestfile.cmake and open a command 
# prompt so that typing 'ctest' will run the test suite properly.
#
# Usage
#
# python CTest_Cross_Console.py                # basic run
# python CTest_Cross_Console.py --debug        # provide extra debug info
# python CTest_Cross_Console.py --undo         # undo changes to CTestTestfile


# Internal operation
#
# Stage 1
# Load a set of 'original paths' set by CMake during the creation of the
# CTestTestfile.cmake. (using something like 'execfile')
#
# Stage 2
# Attempt to find various programs on the test system, such as python,
# ImageMagick, and ctest
#
# Stage 3
# Combine the results of stage 1 and 2, to replace all build-machine 
# paths in CTestTestfile.cmake with test-machine paths. Note that paths 
# in CMake are always separated by '/' even on Windows(TM)
#
# Stage 4
# Open a simple command-prompt on the test machine, but with the environment
# variables & current directory chosen so that simply typing 'ctest'
# will successfully run the test script. 


# Note - This script is designed to work with the OpenSCAD project 
# tests and their cross-compilation environment. It will not 
# automatically work with any given ctest system.
#
# Tested Machine combinations: (* indicates success)
#
# Build machine      Test-run machine
# -------------      ----------------
# Linux cross MXE    Windows*
# Linux cross MXE    WINE in Linux*
# Linux              Linux x86
# OSX                OSX
# Windows            Windows 
#

import os,sys,string,time

_undo=False
_debug=False
debugfile=None
def debug(*args):
	global _debug, debugfile
	if debugfile==None:
		debugfile=open('debuglog.txt','w')
	if _debug:
		s = ''
		for arg in args: s += str(arg) + ' '
		debugfile.write(s+'\n')
		print s
		sys.stdout.flush()

def debug2(*args):
	if '--debug2' in string.join(sys.argv): debug(args)

def debug_startup():
	print 'debugging is on. startup environment:'
	print 'sys.executable:',sys.executable
	print 'sys.version:',sys.version.replace('\n',' ')
	print 'sys.argv:',sys.argv
	print 'sys.path:',sys.path
	print 'sys.platform:',sys.platform
	print 'sys.meta_path:',sys.meta_path
	print 'abs os.curdir:',os.path.abspath(os.curdir)
	print 'sys.getdefaultencoding:',sys.getdefaultencoding()
	print 'sys.getfilesystemencoding:',sys.getfilesystemencoding()
	if os.environ.has_key('PYTHONPATH'):
		print 'PYTHONPATH:',os.environ['PYTHONPATH']
	else:
		print 'PYTHONPATH: '
	
class CTestPaths:
        def __init__(self,name):
		self.name=name
		self.abs_cmake_srcdir=self.abs_cmake_bindir='unset'
		self.bindir=self.openscad_exec=self.python_exec='unset'
		self.convert_exec=self.ctest_exec='unset'
        def dump(self):
                s=''
		s+='CTestPaths dump:\nname:'+self.name+'\n'
                s+='abs_cmake_srcdir:'+self.abs_cmake_srcdir+'\n'
                s+='abs_cmake_bindir:'+self.abs_cmake_bindir+'\n'
                s+='bindir:'+self.bindir+'\n'
                s+='openscad_exec:'+self.openscad_exec+'\n'
                s+='python_exec:'+self.python_exec+'\n'
                s+='convert_exec:'+self.convert_exec+'\n'
                s+='ctest_exec:'+self.ctest_exec+'\n'
                return s
	def normalize(self):
                self.abs_cmake_srcdir=os.path.realpath(self.abs_cmake_srcdir)
                self.abs_cmake_bindir=os.path.realpath(self.abs_cmake_bindir)
                self.bindir=os.path.realpath(self.bindir)
                self.openscad_exec=os.path.realpath(self.openscad_exec)
                self.python_exec=os.path.realpath(self.python_exec)
                self.convert_exec=os.path.realpath(self.convert_exec)
                self.ctest_exec=os.path.realpath(self.ctest_exec)
		
def findfail(progname):
	print 'cant find '+progname+'. exiting in ',
	for i in range(0,7):
		time.sleep(1)
		print 7-i,'.',
		sys.stdout.flush()
	sys.exit()

def windows__find_im_in_registry():
	imbinpath = None
	try:
		registry = ConnectRegistry(None,HKEY_LOCAL_MACHINE)
		regkey = OpenKey(registry, r"SOFTWARE\ImageMagick\Current")
		tmp = QueryValueEx(regkey,'BinPath')[0]
		imbinpath = tmp
	except Exception as e:
		print "Can't find imagemagick using registry...",
		print str(type(e))+str(e)
	return imbinpath

def windows__find_im_in_pfiles():
	print 'Searching for ImageMagick convert.exe in Program folders'
	winconv = None
	tmp = None
	for basedir in 'C:/Program Files','C:/Program Files (x86)':
		if os.path.isdir(basedir):
			pflist=os.listdir(basedir)
			for subdir in pflist:
				if 'ImageMagick' in subdir:
					imbase = basedir+'/'+subdir
					tmp = imbase+'/convert.exe'
					if os.path.isfile(tmp):
						winconv = tmp
						break
		if winconv != '': break
	return winconv	

def windows__find_ctest_in_pfiles():
	print 'Searching for ImageMagick convert.exe in Program folders'
	ctestpath = None
	for basedir in 'C:/Program Files','C:/Program Files (x86)':
		if os.path.isdir(basedir):
			pflist = os.listdir(basedir)
			for subdir in pflist:
				if 'cmake' in subdir.lower():
					abssubdir=os.path.join(basedir,subdir)
					for root,dirs,files in os.walk(abssubdir):
						if 'ctest.exe' in files:
							ctestpath=os.path.join(root,'ctest.exe')
	return ctestpath

def windows__fillpaths(paths):
	thisfile_abspath=os.path.abspath(__file__)
	thisfile_absdir=os.path.dirname(thisfile_abspath)
	paths.abs_cmake_srcdir = os.path.join(thisfile_absdir,'tests')
	paths.abs_cmake_bindir = os.path.join(thisfile_absdir,'testbin')
	paths.bindir = ''
	paths.tct = os.path.join(thisfile_abspath,'tests')
	paths.tct = os.path.join(paths.tct,'test_cmdline_tool.py')
	debug(paths.dump())
	paths.convert_exec=windows__find_im_in_registry()
	if paths.convert_exec==None:
		paths.convert_exec=windows__find_im_in_pfiles()
	if paths.convert_exec==None:
		findfail('imagemagick convert')
	paths.python_exec=sys.executable
	if paths.python_exec=='' or paths.python_exec==None:
		findfail('python2')
	paths.openscad_exec=os.path.join(paths.abs_cmake_bindir,'openscad_nogui.exe')
	if not os.path.isfile(paths.openscad_exec):
		findfail(paths.openscad_exec)
	paths.ctest_exec=windows__find_ctest_in_pfiles()
	if paths.ctest_exec==None:
		findfail('ctest.exe')
	paths.normalize()
	
def process_ctestfile(infilename,buildpaths,testpaths):
	if not os.path.exists(infilename): findfail(infilename)
	bp=buildpaths
	tp=testpaths
	backup_filename = infilename+'.backup'
	if _undo:
		open(infilename,'wb').write(open(backup_filename,'rb').read())
		print 'restored ',infilename,'from backup'
		return
	open(backup_filename,'wb').write(open(infilename,'rb').read())
	debug2 ('wrote backup of ',infilename,' to ',backup_filename)

	outfilename = infilename.replace('.cmake','.new.cmake')
	fin=open(infilename,'rb')
	lines=fin.readlines()
	fout=open(outfilename,'wb')
	fout.write('#'+os.linesep)
	fout.write('# modified by '+__file__+os.linesep)
	fout.write('#'+os.linesep)

	debug2('inputname',infilename)
	debug2('outputname',outfilename)

	for line in lines:
		debug2('input:',line)

		# special for CTestCustom.template + ctest bugs w arguments
		line=line.replace('--builddir='+tp.abs_cmake_bindir,'')
		
		line=line.replace(bp.abs_cmake_srcdir,tp.abs_cmake_srcdir)
		line=line.replace(bp.abs_cmake_bindir,tp.abs_cmake_bindir)
		line=line.replace(bp.bindir,tp.bindir)
		line=line.replace(bp.openscad_exec,tp.openscad_exec)
		line=line.replace(bp.python_exec,tp.python_exec)
		line=line.replace(bp.convert_exec,tp.convert_exec)
		line=line.replace(bp.ctest_exec,tp.ctest_exec)

		line=line.replace('\\"','__ESCAPE_WIN_QUOTE_MECHANISM__')
		line=line.replace('\\','/')
		line=line.replace('__ESCAPE_WIN_QUOTE_MECHANISM__','\\"')

		debug2('output:',line)

		fout.write(line)

	debug2( 'backed up ',infilename, 'to', backup_filename )
	debug2( 'processed ',infilename, 'to', outfilename )
	fin.close()
	fout.close()
	open(infilename,'wb').write(open(outfilename,'rb').read())
	print infilename,'modified (old version saved to:',backup_filename,')'

def windows__open_console(bindir):
	starting_dir=bindir
	#cmd = 'start "OpenSCAD Test console" /wait /d c:\\temp cmd.exe'
	#cmd = 'start /d "'+starting_dir+'" cmd.exe "OpenSCAD Test Console"'
	conbat=os.path.join(bindir,'mingwcon.bat')
	if not os.path.isfile(conbat): findfail(conbat)
	cmd = 'start /d "'+starting_dir+'" cmd.exe "/k" "'+conbat+'"'
	print 'opening console: running ',cmd
	os.system( cmd )

	# figure out how to run convert script
	# dont use mingw64 in linbuild path?
	# figure out better windows prompt, can it be set?

def open_console(testpaths):
	print 'opening console'
	ctestdir = os.path.dirname( testpaths.ctest_exec )
	print 'ctest dir:',ctestdir
	if ctestdir != '' or ctestdir==None:
		print 'adding ctest dir to PATH'
		os.environ['PATH'] = ctestdir + os.pathsep + os.environ['PATH'] 
	if 'win' in sys.platform:
		windows__open_console(testpaths.abs_cmake_bindir)


if '--debug' in string.join(sys.argv):
	debug_startup()
	_debug=True
if '--undo' in string.join(sys.argv): _undo=True
buildpaths = CTestPaths('buildpaths')
testpaths = CTestPaths('testpaths')
sys.path = [os.path.join(os.path.abspath(os.curdir),'testbin')]+sys.path
debug('sys.path after',sys.path)
import ctest_cross_info
ctest_cross_info.set_buildpaths( buildpaths )
if 'win' in sys.platform:
	from _winreg import *
	windows__fillpaths( testpaths )
debug(buildpaths.dump()+'\n'+testpaths.dump()+'\n')
ctestfile1=os.path.join(testpaths.abs_cmake_bindir,'CTestTestfile.cmake')
ctestfile2=os.path.join(testpaths.abs_cmake_bindir,'CTestCustom.cmake')
process_ctestfile(ctestfile1,buildpaths,testpaths)
process_ctestfile(ctestfile2,buildpaths,testpaths)
open_console( testpaths )
