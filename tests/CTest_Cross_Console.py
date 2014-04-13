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
#
# Note - On Windows(TM) there are problems with unicode filenames unless
# you use u as a prefix to the filename strings.

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

debugfile=None
def debug(*args):
	global debugfile
	if debugfile==None:
		debugfile=open(u'debuglog.txt',u'w')
	if u'--debug' in string.join(sys.argv):
		s = u''
		for arg in args: s += unicode(arg) + u' '
		debugfile.write((s+u'\n').encode('utf-8'))
		print s.encode('utf-8')
		sys.stdout.flush()

def debug2(*args):
	if u'--debug2' in string.join(sys.argv): debug(args)

def uprint(*args):
	s=''
	for arg in args: s += unicode(arg) + u' '
	print s.encode('utf-8')
	sys.stdout.flush()

def startup():
	debug(u'debugging is on. startup environment:')
	debug(u'sys.executable:',sys.executable)
	debug(u'sys.version:',sys.version.replace(u'\n',u' '))
	debug(u'sys.argv:',sys.argv)
	debug(u'sys.path:',sys.path)
	debug(u'sys.platform:',sys.platform)
	debug(u'sys.meta_path:',sys.meta_path)
	debug(u'abspath(os.curdir):',os.path.abspath(unicode(os.curdir)))
	debug(u'sys.stdout.encoding:',sys.stdout.encoding)
	debug(u'sys.getdefaultencoding:',sys.getdefaultencoding())
	debug(u'sys.getfilesystemencoding:',sys.getfilesystemencoding())
	if os.environ.has_key(u'PYTHONPATH'):
		debug(u'PYTHONPATH:',os.environ[u'PYTHONPATH'])
	else:
		debug(u'PYTHONPATH: ')
	# sys.path modification allows 'import ctest_cross_info' to work properly
	sys.path = [os.path.join(os.path.abspath(os.curdir),u'testbin')]+sys.path
	debug(u'sys.path after',sys.path)

	
class CTestPaths:
	def __init__(self,name):
		self.name=name
		self.abs_cmake_srcdir=self.abs_cmake_bindir=u'unset'
		self.bindir=self.openscad_exec=self.python_exec=u'unset'
		self.convert_exec=self.ctest_exec=u'unset'
	def dump(self):
		s=u''
		s+=u'CTestPaths dump:\nname:'+self.name+u'\n'
		s+=u'abs_cmake_srcdir:'+self.abs_cmake_srcdir+u'\n'
		s+=u'abs_cmake_bindir:'+self.abs_cmake_bindir+u'\n'
		s+=u'bindir:'+self.bindir+u'\n'
		s+=u'openscad_exec:'+self.openscad_exec+u'\n'
		s+=u'python_exec:'+self.python_exec+u'\n'
		s+=u'convert_exec:'+self.convert_exec+u'\n'
		s+=u'ctest_exec:'+self.ctest_exec+u'\n'
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
	uprint(u'cant find ',progname,u'. exiting in ')
	for i in range(0,7):
		time.sleep(1)
		print 7-i,u'.',
		sys.stdout.flush()
	sys.exit()

def windows__find_im_in_registry():
	debug(u'searching for imagemagick convert.exe using registry')
	imbinpath = u''
	try:
		registry = ConnectRegistry(None,HKEY_LOCAL_MACHINE)
		regkey = OpenKey(registry, r"SOFTWARE\ImageMagick\Current")
		tmp = QueryValueEx(regkey,u'BinPath')[0]
		imbinpath = tmp
	except Exception as e:
		debug( "Can't find imagemagick using registry...")
		debug( u'Exception was:',str(type(e))+str(e))
	convert_exec = os.path.join(imbinpath,u'convert.exe')
	if os.path.isfile(convert_exec): return convert_exec
	return None

def windows__find_in_PATH(progname):
	if not os.environ.has_key['PATH']: return None
	for path in os.environ['PATH'].split(os.pathsep):
		if os.path.isfile(os.path.join(path,progname)):
			return os.path.join(path,progname)
	return None

def windows__find_im_in_pfiles():
	uprint(u'Searching for ImageMagick convert.exe in Program folders')
	convert_exec = None
	tmp = None
	for basedir in u'C:/Program Files',u'C:/Program Files (x86)':
		if os.path.isdir(basedir):
			pflist=os.listdir(basedir)
			for subdir in pflist:
				debug2(subdir)
				if u'ImageMagick' in subdir:
					imbase = basedir+u'/'+subdir
					tmp = imbase+u'/convert.exe'
					if os.path.isfile(tmp):
						convert_exec = unicode(tmp)
						break
		if convert_exec != u'' or convert_exec != None: break
	return convert_exec

def windows__find_ctest_in_pfiles():
	uprint( u'Searching for ctest.exe in Program folders')
	ctestpath = None
	for basedir in u'C:/Program Files',u'C:/Program Files (x86)':
		if os.path.isdir(basedir):
			pflist = os.listdir(basedir)
			for subdir in pflist:
				if u'cmake' in subdir.lower():
					abssubdir=os.path.join(basedir,subdir)
					for root,dirs,files in os.walk(abssubdir):
						if u'ctest.exe' in files:
							ctestpath=os.path.join(root,u'ctest.exe')
	return unicode(ctestpath)

def windows__fillpaths(paths):
	thisfile_abspath=unicode(os.path.abspath(unicode(__file__)))
	thisfile_absdir=unicode(os.path.dirname(thisfile_abspath))
	paths.abs_cmake_srcdir = os.path.join(thisfile_absdir,u'tests')
	paths.abs_cmake_bindir = os.path.join(thisfile_absdir,u'testbin')
	paths.bindir = u''
	paths.tct = os.path.join(thisfile_abspath,u'tests')
	paths.tct = os.path.join(paths.tct,u'test_cmdline_tool.py')
	debug(paths.dump())

	paths.python_exec=sys.executable
	if paths.python_exec==u'' or paths.python_exec==None:
		findfail(u'python2')

	paths.convert_exec=windows__find_im_in_registry()
	if paths.convert_exec==None:
		paths.convert_exec=windows__find_im_in_pfiles()
	if paths.convert_exec==None:
		paths.convert_exec=windows__find_in_PATH(u'convert.exe')
	if paths.convert_exec==None:
		findfail(u'imagemagick convert')

	paths.openscad_exec=os.path.join(paths.abs_cmake_bindir,u'openscad_nogui.exe')
	if not os.path.isfile(paths.openscad_exec):
		paths.openscad_exec=windows__find_in_PATH(u'openscad_nogui.exe')
		if paths.openscad_exec==None:
			findfail(paths.openscad_exec)

	paths.ctest_exec=windows__find_ctest_in_pfiles()
	if paths.ctest_exec==None:
		paths.ctest_exec=windows__find_in_PATH(u'ctest.exe')
	if paths.ctest_exec==None:
		findfail(u'ctest.exe')
	paths.normalize()

def get_template_scad_list(basedir):
	newscads=[]
	for r,ds,fs in os.walk('testdata'):
		if 'templates' in r:
			for f in fs:
				debug('testdata tempalte file:',f)
				fnew=f.replace('-template','')
				newscads += [fnew]
	print 'newscads',newscads
	scadlist=[]
	for scadname in newscads:
		for r,ds,fs in os.walk('testdata'):
			if scadname in fs:
				debug('testdata template new:',scadname)
				scadlist += [ os.path.join(r,scadname) ]
	debug('scadlist result',scadlist)
	return scadlist

def process_scadfile(infilename,buildpaths,testpaths):
	infilename=unicode(infilename)
	if not os.path.exists(infilename): findfail(infilename)
	bp=buildpaths
	tp=testpaths
	backup_filename = infilename+u'.backup'
	outfilename = infilename.replace(u'.scad',u'.new.scad')
	fin=open(infilename,u'rb')
	lines=fin.readlines()
	fout=open(outfilename,u'wb')
	fout.write(u'//'+os.linesep)
	fout.write(u'// modified by '+__file__+os.linesep)
	fout.write(u'//'+os.linesep)

	debug2(u'inputname',infilename)
	debug2(u'outputname',outfilename)

	for line in lines:
		debug2(u'input:',line)
		line=line.decode(u'utf-8')
		if bp.abs_cmake_srcdir in line:
			line=line.replace(bp.abs_cmake_srcdir,tp.abs_cmake_srcdir)
			if 'win' in sys.platform:
				line=line.replace('\\','/')
		debug2(u'output:',line)
		line=line.encode(u'utf-8')
		fout.write(line)

	debug2( u'backed up ',infilename, u'to', backup_filename )
	debug2( u'processed ',infilename, u'to', outfilename )
	fin.close()
	fout.close()
	open(infilename,u'wb').write(open(outfilename,u'rb').read())
	uprint(infilename,u'modified \n(old version saved to:',backup_filename,u')')

def process_ctestfile(infilename,buildpaths,testpaths):
	infilename=unicode(infilename)
	if not os.path.exists(infilename): findfail(infilename)
	bp=buildpaths
	tp=testpaths
	backup_filename = infilename+u'.backup'
	if u'--undo' in string.join(sys.argv):
		open(infilename,u'wb').write(open(backup_filename,u'rb').read())
		uprint(u'restored ',infilename,u'from backup')
		return
	open(backup_filename,u'wb').write(open(infilename,u'rb').read())
	debug2 (u'wrote backup of ',infilename,u' to ',backup_filename)

	outfilename = infilename.replace(u'.cmake',u'.new.cmake')
	fin=open(infilename,u'rb')
	lines=fin.readlines()
	fout=open(outfilename,u'wb')
	fout.write(u'#'+os.linesep)
	fout.write(u'# modified by '+__file__+os.linesep)
	fout.write(u'#'+os.linesep)

	debug2(u'inputname',infilename)
	debug2(u'outputname',outfilename)

	for line in lines:
		debug2(u'input:',line)
		line=line.decode(u'utf-8') # CTest in theory uses utf-8
		# special for CTestCustom.template + ctest bugs w arguments
		line=line.replace(u'--builddir='+tp.abs_cmake_bindir,u'')
		
		line=line.replace(bp.abs_cmake_srcdir,tp.abs_cmake_srcdir)
		line=line.replace(bp.abs_cmake_bindir,tp.abs_cmake_bindir)
		line=line.replace(bp.bindir,tp.bindir)
		line=line.replace(bp.openscad_exec,tp.openscad_exec)
		line=line.replace(bp.python_exec,tp.python_exec)
		line=line.replace(bp.convert_exec,tp.convert_exec)
		line=line.replace(bp.ctest_exec,tp.ctest_exec)

		if 'win' in sys.platform:
			line=line.replace(u'\\"',u'__ESCAPE_WIN_QUOTE_MECHANISM__')
			line=line.replace(u'\\',u'/')
			line=line.replace(u'__ESCAPE_WIN_QUOTE_MECHANISM__',u'\\"')

		debug2(u'output:',line)
		line=line.encode(u'utf-8') # Ctest, in theory, uses utf-8
		fout.write(line)

	debug2( u'backed up ',infilename, u'to', backup_filename )
	debug2( u'processed ',infilename, u'to', outfilename )
	fin.close()
	fout.close()
	open(infilename,u'wb').write(open(outfilename,u'rb').read())
	uprint(infilename,u'modified \n(old version saved to:',backup_filename,u')')

def windows__open_console(bindir):
	starting_dir=unicode(bindir)
	#cmd = u'start "OpenSCAD Test console" /wait /d c:\\temp cmd.exe'
	#cmd = u'start /d "'+starting_dir+u'" cmd.exe "OpenSCAD Test Console"'
	conbat=os.path.join(bindir,u'mingwcon.bat')
	# dont use full path.. windows(TM) unicode filenames will not work
	# properly inside os.system(). the workaround is to use os.chdir()
	# (which does work OK with windows(TM) unicode filenames)
	# and then open the batch file with its ascii name, not its full path.
	#cmd = u'start /d "'+starting_dir+u'" cmd.exe "/k" "'+conbat+u'"'
	debug('changing directory to',starting_dir)
	os.chdir(starting_dir)
	conbat=u'mingwcon.bat'
	if not os.path.isfile(conbat): findfail(conbat)
	cmd = u'start cmd.exe "/k" "'+conbat+u'"'
	uprint(u'opening console: running ',cmd)
	uprint(os.path.isdir(starting_dir))
	os.system( cmd.encode('mbcs') )

	# figure out how to run convert script
	# dont use mingw64 in linbuild path?
	# figure out better windows prompt, can it be set?

def open_console(testpaths):
	uprint( u'opening console' )
	ctestdir = os.path.dirname( testpaths.ctest_exec )
	uprint( u'ctest dir:',ctestdir )
	if ctestdir != u'' or ctestdir==None:
		uprint( u'adding ctest dir to PATH' )
		os.environ[u'PATH'] = ctestdir + os.pathsep + os.environ[u'PATH'] 
	if u'win' in sys.platform:
		windows__open_console(testpaths.abs_cmake_bindir)


#sys.argv+=[u'--debug2']
startup()
buildpaths = CTestPaths(u'buildpaths')
testpaths = CTestPaths(u'testpaths')
execfile(os.path.join(u'testbin',u'ctest_cross_info.py'))
#import ctest_cross_info
#ctest_cross_info.set_buildpaths( buildpaths )
set_buildpaths( buildpaths )
if u'win' in sys.platform:
	from _winreg import *
	windows__fillpaths( testpaths )
debug(buildpaths.dump()+u'\n'+testpaths.dump()+u'\n')

ctestfile1 = os.path.join(testpaths.abs_cmake_bindir,u'CTestTestfile.cmake')
ctestfile2 = os.path.join(testpaths.abs_cmake_bindir,u'CTestCustom.cmake')
process_ctestfile(ctestfile1,buildpaths,testpaths)
process_ctestfile(ctestfile2,buildpaths,testpaths)
scads = get_template_scad_list(os.path.join(testpaths.abs_cmake_srcdir,u'testdata'))
for scadfile in scads:
	process_scadfile(scadfile,buildpaths,testpaths)

open_console( testpaths )
