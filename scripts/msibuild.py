#!/usr/bin/env python
import sys,os,re,uuid,subprocess,shutil

#
# create MSI file for installation of OpenSCAD program on Microsoft Windows
# computer operating system. from inside of a linux cross-build environment.
# using Gnome's Wixl and Wixl-Heat which are based on the 'WiX' project.
# it requires the 'msitools' package on a linux distribution to run.
#
# overview of this script:
#
# this script assumes you already have cross-built openscad. you have
# openscad.exe and the other files needed. see openscad README.md for
# how to do this.
#
#  0. we have scripts/openscad.wxs which has the info for the main components,
#     openscad.exe and openscad.com, with some icons + such
#  1. we create a list of other files (examples, translations) in to
#     a filelist.wxs. This is done with 'wixl-heat', and excludes files
#     already listed in openscad.wxs
#  2. we set up various command line '-defines' to feed to wixl, which
#     will replace a bunch of strings inside the wxs files, such as the build
#     directory, icon location, 32bit/64bit, etc.
#  3. we run wixl to combine pre-made openscad.wxs with generated filelist.wxs,
#     creating openscad.msi
#  4. we do some basic 'sanity check' on the resulting .msi
#     does it contain at least the .exe and examples?
#
# checklist when woking on this script:
#
# new build:
#
# test in Windows(TM), look at directory where files acre copied/created
# test run of openscad.exe and openscad.com
# test if icons works, in Program Files list, on Desktop Shortcut
# test file association works, and icons for file associations too
# uninstall, make sure directories are deleted
# try installing twice, the uninstall, see what happens.
# try installing two versions, see if both work
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
# What does '64 bit' mean? In the MSI database there is a table, Component
# you can dump with 'msidump' tool. The fourth column of this table is
# the 'attribute'. For '64 bit' this should have a bit set, the 9th bit,
# 0x0100, which typically shows up as the integer '256' in an information dump
# This bit is called the msidbComponentAttributes64bit
# For 32 bit MSI this value is 0, for 64, bit shoud be 1, and as i said,
# since the other bits usually arent set, this shows up as '256' in an MSI dump.
# Signing:
# https://sourceforge.net/projects/osslsigncode/files/osslsigncode/
# https://www.ibm.com/support/knowledgecenter/en/SSWHYP_4.0.0/com.ibm.apimgmt.cmc.doc/task_apionprem_gernerate_self_signed_openSSL.html
# (use -sha256 on ibm link)
# File Association: (.scad file->open automatically)
# https://docs.microsoft.com/en-gb/windows/desktop/shell/fa-file-types

def verify_deps():
	score = 0
	for bin in ['msiinfo','wixl-heat','wixl','osslsigncode']:
		check = shutil.which(bin)
		if check!=None: score += 1
		print(bin,'\t', check)
	return score>0

def sign_msi(msi_filename,pkcs12_file,pkcs12pwd):
	cmd=['osslsigncode','sign']
	cmd+=['-n','"OpenSCAD"']
	cmd+=['-i','http://www.openscad.org']
	cmd+=['-in',msi_filename,'-out','signed'+msi_filename]
	print('calling',' '.join(cmd)+ ' ...')
	cmd+=['-pkcs12',pkcs12_file]
	cmd+=['-pass',pkcs12pwd]

	p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	print(p.stdout.read().decode('utf-8'))
	print(p.stderr.read().decode('utf-8'))

def verify_msi(msi_filename):
	if os.path.exists(msi_filename):
		print('created',msi_filename,' bytes:',os.path.getsize(msi_filename),'bytes')
	else:
		print('sorry something went awry. no',msi_filename,'created')
		return False

	cmds = [['msiinfo','export',msi_filename,'File'],
	        ['msiinfo','export',msi_filename,'Directory'],
	        ['msiinfo','export',msi_filename,'Icon']]
	lines = []
	for cmd in cmds:
		print('running',' '.join(cmd))
		p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		lines += p.stdout.readlines() + p.stderr.readlines()
	needs = {'openscad_executable':0,'INSTALLDIR':0,'examples':0,
		'locale':0,'Icon':0}
	noneeds = {'openscad_setup.exe':0}
	for line in lines:
		l = line.decode('utf-8').strip().replace('\t',' ')[:79]
		for need in needs.keys():
			if need in l:
				needs[need]=1
				#print(l)
		for noneed in noneeds.keys():
			if noneed in l:
				print('dont want',noneed,'but found in ',l)
				return False
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

def make_filelist(openscad_crossbuild_dir,filelistwxs_filename,wixlarch):
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
	# of the files on our *nix build machine that will be
        # copied into the MSI database file. you can see
	# it in the output filelist.xml, its a prefix to all file paths
	# later on, when we call wixl, we will '--define' this variable
	# so it will know what to do
	cmd+=['--var','var.OPENSCADCROSSBUILDDIR']
	# this makes the 'Win64=Yes' so the Component table has the Attribute
	# for 64 bit MSI files set properly to 1 on a 64 bit build.
	if wixlarch=='x64': cmd += ['--win64']
	# .exe and .com already in openscad.wxs, dont want to pack these twice
	cmd+=['--exclude','openscad.exe']
	cmd+=['--exclude','openscad.com']

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
	print('created ',filelistwxs_filename,' bytes:',len(xml))

def main(openscad_crossbuild_dir, openscad_src_dir, openscad_version, arch, pkcs12_file, pkcs12pwd ):
	print('build MSI for openscad')
	print('openscad_crossbuild_dir',openscad_crossbuild_dir)
	print('openscad_src_dir',openscad_src_dir)
	print('openscad_version',openscad_version)
	print('architecture input from command line:', arch)
	wixlarch = guessarch( arch )
	print('wixl special name for input architecture:', wixlarch)
	openscad_version_for_win = openscad_version
	print('openscad_version_for_win',openscad_version_for_win)

	if not verify_path(openscad_src_dir,'README.md'):
		print('cant find openscad source dir, exiting')
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
	icon_filename = openscad_src_dir + '/icons/openscad.ico'
	icon_assoc_filename = openscad_src_dir + '/icons/openscad_doc.ico'
	progfiles_dir = 'ProgramFilesFolder'
	win64var = 'no'
	if wixlarch=='x64':
		progfiles_dir = 'ProgramFiles64Folder'
		win64var = 'yes'

	if 'clean' in ''.join(sys.argv[1:]):
		for f in filelistwxs_filename,msi_filename:
			if os.path.exists(f): os.remove(f)
			print('clean',f)
		return

	make_filelist(openscad_crossbuild_dir,filelistwxs_filename,wixlarch)

	cmd=['wixl','--verbose']
	cmd+=['--arch',wixlarch]
	cmd+=['--define','OPENSCADCROSSBUILDDIR='+openscad_crossbuild_dir]
	cmd+=['--define','OPENSCADSRCDIR='+openscad_src_dir]
	cmd+=['--define','OPENSCADVERSION='+openscad_version]
	cmd+=['--define','OPENSCADVERSIONFORWIN='+openscad_version_for_win]
	cmd+=['--define','PROGFILESDIRNAME='+progfiles_dir]
	cmd+=['--define','OPENSCADICO='+icon_filename]
	cmd+=['--define','OPENSCADDOCICO='+icon_assoc_filename]
	cmd+=['--define','Win64='+win64var]
	cmd+=['--output',msi_filename]
	cmd+=[mainwxs_filename,filelistwxs_filename]
	print('calling',' '.join(cmd))
	p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	print(p.stdout.read().decode('utf-8'))
	print(p.stderr.read().decode('utf-8'))

	if not verify_msi(msi_filename):
		print('build failed')
		return 1
	if pkcs12_file!='': sign_msi(msi_filename,pkcs12_file,pkcs12pwd)
	else: print('skip signing')
	return 0

if verify_deps():
	args=sys.argv
	if len(args)<5:
		print('xbuilddir, srcdir, version, arch')
		sys.exit(1)
	pkcs12_file=pkcs12pwd=''
	if len(args)>=6: pkcs12_file=args[5]
	if len(args)>=6: pkcs12pwd=args[6]
	main(args[1],args[2],args[3],args[4],pkcs12_file,pkcs12pwd)
	#main('./osbuilddirx','/home/d/src/openscad','2018.08.12','x86-64')
	#main('./osbuilddirx','/home/d/src/openscad','2018.08.12','x86-32')
else:
	print("please install the MSI tools for your platform")
	print("i need msiinfo, wixl, and wixl-heat")
	print("please see https://wiki.gnome.org/msitools")
