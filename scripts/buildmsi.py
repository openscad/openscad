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
#
# see also
# https://docs.microsoft.com/en-us/windows/desktop/Msi/windows-installer-portal
# https://blogs.msdn.microsoft.com/pusu/2009/06/10/what-are-upgrade-product-and-package-codes-used-for/
# https://wiki.gnome.org/msitools
# http://p-nand-q.com/programming/windows/wix/index.html
# https://www.firegiant.com/wix/tutorial/
# https://github.com/ml-workshare/wix-msi-ui-example/blob/master/Product.wxs
# https://wiki.gnome.org/msitools/HowTo/CreateLibraryWxi
# wixl test suite (msitools 'make check-local')

def verify_deps():
	score = 0
	for exe in ['msiinfo','wixl-heat','wixl']:
		check = shutil.which(exe)
		if check!=None: score += 1
		print(exe,'\t', check)
	return score>0

def verify_msi(msi_filename):
	if os.path.exists(msi_filename):
		print('created',msi_filename,'size',os.path.getsize(msi_filename))
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
	sanity = [0,0]
	for line in lines:
		l = line.decode('utf-8').strip().replace('\t',' ')[:79]
		if 'openscad_executable' in l:
			sanity[0]=1
			print(l)
		if 'INSTALLDIR' in l:
			sanity[1]=1
			print(l)
	if sanity[0]+sanity[1]<2:
		print('sorry something went awry.',msi_filename,'appears')
		print('to be missing openscad.exe and/or a proper INSTALLDIR')
		return False
	return True

def verify_path(dir,file):
	if not os.path.exists(os.path.join(dir,file)):
		print('cannot find',os.path.join(dir,file))
		return False
	return True

def main(openscad_crossbuild_dir, openscad_src_dir, openscad_version, arch ):
	print('build MSI for openscad')
	print('openscad_crossbuild_dir',openscad_crossbuild_dir)
	print('openscad_src_dir',openscad_src_dir)
	print('openscad_version',openscad_version)
	print('architecture', arch)

	wixarchcodes = {'x86-32':'intel','x86-64':'x64'}

	print('please cross-build before running this script')
	if not verify_path(openscad_src_dir,'README.md'):
		return
	if not verify_path(openscad_crossbuild_dir,'openscad.exe'):
		print('please cross-build openscad.exe before running this script')
		return
	if not arch in wixarchcodes.keys():
		print('cannot find arch',arch,'in ',wixarchcodes)
		return

	mainwxs_filename = 'openscad.wxs'
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
	# note that prefix must end with '/' or else you wind up with
	# paths like source//subdir/file which creates an 'empty' directory
	# between the two //, which crashes wixl during msi creation
	cmd+=['--prefix','./openscad32/']
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
	cmd+=['--arch',wixarchcodes[arch]]
	cmd+=['--define','OPENSCADCROSSBUILDDIR='+openscad_crossbuild_dir]
	cmd+=['--define','OPENSCADsrc_dir='+openscad_src_dir]
	cmd+=['--define','OPENSCADVERSION='+openscad_version]
	cmd+=['--output',msi_filename]
	cmd+=[mainwxs_filename,filelistwxs_filename]
	print('calling',' '.join(cmd))
	p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	print(p.stdout.read().decode('utf-8'))
	print(p.stderr.read().decode('utf-8'))

	verify_msi()

if verify_deps():
	main('./openscad32','/home/don/src/openscad','2018.08.12','x86-32')
	#main('./openscad64','/home/don/src/openscad','2018.08.12','x86-64')
else:
	print("please install the MSI tools for your platform")
	print("i need msiinfo, wixl, and wixl-heat")
	print("please see https://wiki.gnome.org/msitools")
