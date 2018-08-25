#!/usr/bin/env python
import sys,os,re,uuid,subprocess,shutil

#
# create MSI file for installation of OpenSCAD program on Microsoft Windows
# computer operating system. from inside of a linux cross-build environment.
# using Gnome's Wixl and Wixl-Heat which are based on the 'WiX' project.
#
# why:
#
# MSI is a good choice for delivering programs to people using Windows.
# MSI is in general less likely to get hacked and injected with a virus
# versus other methods
#
# MSI is also less likely to get 'false positive' from virus scanners.
# Lastly, MSI is also easier for schools to mass-deploy to their students
# windows machines through Microsoft's special mass-deployment systems.
# Why? Because MSI is not an executable format. It is a database that contains
# "components" such as files. It is like, in the linux world, the difference
# between a dpkg or rpm file and a shell script.
#
# downside:
# MSI files are bigger, slower to install, slower to uninstall. MSI is also
# poorly documented and contains many extremely confusing options (all of which
# we try to avoid here)
#
# overview of this script:
#
# this script assumes you already have cross-built openscad. you have
# openscad.exe and the other files needed. see openscad README.md for
# how to do this.
#
# the script 1. lists files for the windows bundle, 2. combines openscsad.wxs
# and generated filelist.wxs, to 3. create openscad.msi
#
# checklist when woking on this script:
#
# new build:
#
# test in Windows(TM), look at directory where files acre copied/created
# test run of openscad.exe and openscad.com
# test if icons works
# test file association works
# uninstall, make sure directories are deleted
# try installing twice, the uninstall, see what happens.
# try installing two versions, should be OK
#
# Testing your msi file itself
#
# see handy utilities like msiextract, msidump, etc
# see also
# https://docs.microsoft.com/en-us/windows/desktop/Msi/windows-installer-portal
# https://blogs.msdn.microsoft.com/pusu/2009/06/10/what-are-upgrade-product-and-package-codes-used-for/
# https://wiki.gnome.org/msitools
# http://p-nand-q.com/programming/windows/wix/index.html
# https://www.firegiant.com/wix/tutorial/
# https://github.com/ml-workshare/wix-msi-ui-example/blob/master/Product.wxs
# https://wiki.gnome.org/msitools/HowTo/CreateLibraryWxi
# wixl test suite (msitools 'make check-local')
# log while installing: msiexec /i openscad.msi /l*v log.txt
# 64bit vs 32bit:
#  https://docs.microsoft.com/en-us/windows/desktop/msi/about-windows-installer-on-64-bit-operating-systems
#  https://docs.microsoft.com/en-us/windows/desktop/msi/using-64-bit-windows-installer-packages
#  https://stackoverflow.com/questions/16568901/what-exactly-does-the-arch-argument-on-the-candle-command-line-do
#  i.e. the fourth column of the Component table is called Attributes, and
#  on a 64bit MSI it should be set to the value '256'.

def verify_deps():
	score = 0
	for exe in ['msiinfo','wixl-heat','wixl']:
		check = shutil.which(exe)
		if check!=None: score += 1
		print(exe,'\t', check)
	return score>0

def verify_msi(msi_filename):
	if os.path.exists(msi_filename):
		print('created',msi_filename,'size',os.path.getsize(msi_filename),'bytes')
	else:
		print('sorry something went awry. no',msi_filename,'created')
		return False

	cmds = [['msiinfo','export',msi_filename,'File'],
	        ['msiinfo','export',msi_filename,'Directory']]
	lines = []
	for cmd in cmds:
		print('running',' '.join(cmd))
		p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		lines += p.stdout.readlines() + p.stderr.readlines()
	needs = {'openscad_executable':0,'INSTALLDIR':0,'examples':0,'locale':0}
	for line in lines:
		l = line.decode('utf-8').strip().replace('\t',' ')[:79]
		for need in needs.keys():
			if need in l:
				needs[need]=1
				print(l)
	for need in needs.keys():
		if needs[need]:
			print('found need',need,' in .msi')
		else:
			print('need not found in .msi:',need)
			return False
	return True

def verify_path(dir,file):
	if not os.path.exists(os.path.join(dir,file)):
		print('cannot find',os.path.join(dir,file))
		return False
	return True

def guessarch(arch):
	# wixl needs the string 'intel' for 32 bit x86
	# and it needs the string 'x64' for 64 bit amd64
	a32='x32 686 586 486 386 32 x86-32 x86_32 x8632 amd32 intel'.split(' ')
	a64='amd64 amd-64 amd_64 x86-64 x86_64 x8664 64 x64'.split(' ')
	if arch in a32: return 'intel'
	if arch in a64: return 'x64'
	return None

def main(openscad_crossbuild_dir, openscad_src_dir, openscad_version, arch ):
	print('build MSI for openscad')
	print('openscad_crossbuild_dir',openscad_crossbuild_dir)
	print('openscad_src_dir',openscad_src_dir)
	print('openscad_version',openscad_version)
	print('architecture input from command line:', arch)
	wixlarch = guessarch( arch )
	print('wixl special name for input architecture:', wixlarch)

	print('please cross-build before running this script')
	if not verify_path(openscad_src_dir,'README.md'):
		return
	if not verify_path(openscad_crossbuild_dir,'openscad.exe'):
		print('please cross-build openscad.exe before running this script')
		return
	if wixlarch==None:
		print('cannot guess what this architecture is:',arch)
		return

	mainwxs_filename = os.path.join(openscad_src_dir,'scripts','openscad.wxs')
	filelistwxs_filename = 'filelist.wxs'
	msi_filename = 'openscad.msi'

	if 'clean' in ''.join(sys.argv[1:]):
		for f in filelistwxs_filename,msi_filename:
			if os.path.exists(f): os.remove(f)
			print('clean',f)
		return

	filelist=''
	for root,folder,files in os.walk(openscad_crossbuild_dir):
		for file in files:
			fname = os.path.join(root,file)
			filelist += fname + '\n'
	print('generated filelist, # of lines:',len(filelist.split('\n')))

	cmd=['wixl-heat']
	# note that the dir for prefix must end with '/' or else you wind up
	# with paths like source//subdir/file which creates an 'empty' directory
	# between the two //, which crashes wixl during msi creation
	cmd+=['--prefix',openscad_crossbuild_dir+'/']
	# component group is a way to 'communicate' between filelist.wxs
	# and openscad.wxs, with a single line inside openscad.wxs
	cmd+=['--component-group','OPENSCADFILELIST']
	# installdir is where it will 'live' on Windows,
	# which we want under Program Files\OpenSCAD\OpenSCAD-version
	cmd+=['--directory-ref','INSTALLDIR']
	# this "var" is not just any var, its the path 'source'
	# of the files to be copied into the MSI. you can see
	# it in the output filelist.xml, its a prefix to all file paths
	# later on, when we call wixl, we will '--define' this variable
	# so it will know what to do
	cmd+=['--var','var.OPENSCADCROSSBUILDDIR']
	#if arch=='x86-64':
	#	cmd += ['--win64']

	print('calling',' '.join(cmd))
	p2=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	p2.stdin.write( filelist.encode('utf-8') )
	p2.stdin.close()
	xml = p2.stdout.read().decode('utf-8')
	if ')//' in xml:
		print('oops, somehow "//" path ended up in',flname)
		print('this will probably crash wixl.. trying anyways..')
		print('please check notes in this script about build.py')
		print('regarding prefix, wixl-heat, and trailng /')
	fxml = open(filelistwxs_filename,'w+')
	fxml.write(xml)
	fxml.close()
	print('created ',filelistwxs_filename,' size',len(xml))

	cmd=['wixl','--verbose']
	cmd+=['--arch',wixlarch]
	cmd+=['--define','OPENSCADCROSSBUILDDIR='+openscad_crossbuild_dir]
	cmd+=['--define','OPENSCADSRCDIR='+openscad_src_dir]
	cmd+=['--define','OPENSCADVERSION='+openscad_version]
	if arch=='x86-64':
		cmd+=['--define','PROGFILESDIRNAME=ProgramFiles64Folder']
	else:
		cmd+=['--define','PROGFILESDIRNAME=ProgramFilesFolder']
	cmd+=['--output',msi_filename]
	cmd+=[mainwxs_filename,filelistwxs_filename]
	print('calling',' '.join(cmd))
	p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	print(p.stdout.read().decode('utf-8'))
	print(p.stderr.read().decode('utf-8'))

	verify_msi(msi_filename)

if verify_deps():
	args=sys.argv
	if len(args)<5:
		print('xbuilddir, srcdir, version, arch')
		sys.exit(1)
	main(args[1],args[2],args[3],args[4])
	#main('./osbuilddirx','/home/d/src/openscad','2018.08.12','x86-64')
	#main('./osbuilddirx','/home/d/src/openscad','2018.08.12','x86-32')
else:
	print("please install the MSI tools for your platform")
	print("i need msiinfo, wixl, and wixl-heat")
	print("please see https://wiki.gnome.org/msitools")
