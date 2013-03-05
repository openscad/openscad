#!/usr/bin/python

# test_upload.py copyright 2013 Don Bright <hugh.m.bright@gmail.com>
# released under Zlib-style license:
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose, 
# including commercial applications, and to alter it and redistribute it 
# freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must 
# not claim that you wrote the original software. If you use this 
# software in a product, an acknowledgment in the product documentation 
# would be appreciated but is not required.
#  2. Altered source versions must be plainly marked as such, and must 
# not be misrepresented as being the original software. 3. This notice 
# may not be removed or altered from any source distribution.
#
# This license is based on zlib license by Jean-loup Gailly and Mark Adler


# This script takes html output by test_pretty_print.py and uploads it 
# to a web server into an 'openscad_tests' subdir over ssh using sftp. 
# It then modifies the 'index.html' file in that directory to 'add to 
# the list' of reports on the remote web server.
 

#
# Design
#
# On the remote web server there is a directory called 'openscad_tests'
# Inside of it is a file, 'index.html', that lists all the test reports
# stored in that directory. 
# 
# This script uploads the last report created by test_pretty_print.py 
# Then it modifies the remote index.html file (if necessary) to display 
# a link to the new test report.
#
# Each test report is a single .html file, with all of the .png images
# encoded directly into the file using the Data URI and base64 encoding.
# The name of the report file is something like OS_cpu_GLinfo_hash.html, like
# linux_x86_radeon_abcd.html
#
# See examples under 'usage()' below.
#
# Requirements for remote web server and local system:
# 
# 1. Local system must have sftp access to remote server
# This can be tested by runnig this: sftp user@remotehost
# If you are using ssh-agent or an ssh-keyring it will go automatically.
# Otherwise it will request your password to be typed in.
# 
# 2. Remote web server only needs static html. There is no requirement
# for php/cgi/etc. 
# 
# 3. Local system must have the python Paramiko library installed, 
# which in turn requires pycript to also be installed. (to PYTHONPATH)
#
# This script returns '1' on failure, '0' on success (shell-style)
#

# todo: support plain old ftp ??
# todo: support new-fangled sites like dropbox ??

import sys,os,platform,string,getpass

try:
	import paramiko
except:
	x=''' 
please install the paramiko python library in your PYTHONPATH 
If that is not feasible, you can upload the single html report 
file by hand to any site that accepts html code. 
'''

from test_pretty_print import ezsearch, read_sysinfo
from test_cmdline_tool import execute_and_redirect

debug_test_upload = False
dryrun = False
bytecount = 0

def help():
	text='''
test_upload.py

usage:

  test_upload.py --username=uname --host=host --remotepath=/some/path \
	[--dryrun] [--debug]

example1:

  $ ctest # result is Testing/Temporary/linux_x86_nvidia_report_abcd.html
  $ test_upload.py --username=andreis --host=web.sourceforge.net --remotepath=/home/project-web/projectxyz/htdocs/
  $ firefox http://projectxyz.sourceforge.net/openscad_tests/index.html
  # this should display a page with a link to show the test results

example2:

  $ # run under X11, then run under Xvfb, upload both reports to one site
  $ ctest # X11 - result is Testing/Temporary/freebsd_x86_nvidia_abc_report.html
  $ test_upload.py --username=annag --host=fontanka.org --remotepath=/var/www/
	$ export DISPLAY= # dont use 'display' X, use Xvfb (different drivers)
  $ ctest # XVfb - result is Testing/Temporary/freebsd_x86_mesa_xyz_report.html
  $ test_upload.py --username=annag --host=fontanka.org --remotepath=/var/www/
  $ firefox http://fontanka.org/openscad_tests/index.html
  # result is 'index.html' with a link to the two separate test .html files

'''
	print text


def debug(x):
	if debug_test_upload:
		print 'test_upload.py:', x
	sys.stdout.flush()


blankchunk='''<!-- __entry__ -->'''

index_template='''
<html>
<head>
OpenSCAD regression test results
</head>
<body>
 <h3>
  OpenSCAD regression test results
 </h3>
 <ul> 
''' + blankchunk + '''
 </ul>
</body>
</html>
'''

entry_template='''
  <li><a href="__href__">__rept_name__</a></li>
'''


def paramiko_upload( newrept_fname, username, host, remotepath ):
	global bytecount
        debug("running paramiko upload")

        basepath = 'openscad_tests'
	newrept_basefname = os.path.basename( newrept_fname )
        newrept_name = string.split( newrept_basefname,'.html')[0]
	debug("local file: "+ newrept_fname )
	debug("base filename: "+ newrept_basefname )
	debug("report name: "+ newrept_name )



        debug("connect to " + username + "@" + host)
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy)
        client.load_system_host_keys()
	if dryrun:
		debug("dryrun: no client.connect()")
		return 0
        try:
                client.connect(host,username=username)
        except paramiko.PasswordRequiredException, e:
                passw = getpass.getpass('enter passphrase for private ssh key: ')
                client.connect(host,username=username,password=passw)

        #stdin,stdout,stderr=client.exec_command('ls -l')



        debug("find remote path: " + remotepath)
        ftp = client.open_sftp()
	try:
		ftp.chdir( remotepath )
	except:
		debug("failed to change dir to remote path: "+remotepath)
		return 1
	debug("find basepath ( or create ):" + basepath )
        if not basepath in ftp.listdir():
                ftp.mkdir( basepath )
        ftp.chdir( basepath )



        debug("upload local report file to remote file:")
	debug(" local:"+newrept_fname)
	localf = open( newrept_fname, 'r' )
	rept_text = localf.read()
	localf.close()
	debug(" bytes read:"+str(len(rept_text)))

	debug("remote:"+os.path.join(ftp.getcwd(),newrept_basefname))
	f = ftp.file( newrept_basefname, 'w+' )
	f.write( rept_text )
	f.close()
	bytecount += len(rept_text)


	debug( "file uploaded. now, update index.html (or create blank) ")

        if not 'index.html' in ftp.listdir():
                f = ftp.file( 'index.html', 'w+')
                f.write(index_template)
                f.close()
		bytecount += len(index_template)

        f = ftp.file( 'index.html', 'r' )
        text = f.read()
        f.close()

        text2 = entry_template
        text2 = text2.replace( '__href__', newrept_basefname )
        text2 = text2.replace( '__rept_name__', newrept_name )

        if newrept_basefname in text:
                debug( newrept_basefname + " already linked from index.html")
        else:
	        debug("add new report link to index.html")
                text = text.replace( blankchunk, blankchunk+'\n'+text2 )

        f = ftp.file( 'index.html', 'w+' )
        f.write(text)
        f.close()
	bytecount += len(text)

        debug("close connections")
        ftp.close()
        client.close()
	return 0

def upload_unix( reptfile, username, host, remotepath):
	debug("detected unix-like system.")
	return paramiko_upload( reptfile, username, host, remotepath )

def upload_darwin( reptfile, username, host, remotepath ):
	debug("detected osx/darwin")
	return upload_unix( reptfile, username, host, remotepath )
	
def upload_windows( reptfile, username, host, remotepath ):
	debug("detected windows")
	print 'sorry, not implemented on windows'
	return 1
	# use pycript and paramiko
	# the problem is downloading them and installing them 

def upload( reptfile, username, host, remotepath ):
	sysname = platform.system().lower()
	result = 1
	if 'linux' in sysname or 'bsd' in sysname:
		result = upload_unix( reptfile, username, host, remotepath )
	elif 'darwin' in sysname:
		result = upload_darwin( reptfile, username, host, remotepath )
	elif 'windows' in platform.system():
		result = upload_windows( reptfile, username, host, remotepath )
	else:
		print "unknown system type. cant upload, sorry"
	return result

def main():
	if '--debug' in string.join(sys.argv): 
		global debug_test_upload
		debug_test_upload = True
	if '--dryrun' in string.join(sys.argv): 
		global dryrun
		dryrun = True

	debug('running test_upload')
	debug('args: '+str(sys.argv[1:]))

        builddir = ezsearch('--builddir=(.*?) ',string.join(sys.argv)+' ')
        if builddir=='': builddir=os.getcwd()
	sysinfo, sysid = read_sysinfo(os.path.join(builddir,'sysinfo.txt'))
	debug("sysinfo: " + sysinfo[0:60].replace('\n','') + ". . . ")
	debug("sysid: " + sysid )
	if len(sysid)<6:
		print "unable to find valid system id"
		sys.exit(1)

	rept_basename = sysid + '_report' + '.html'
	rept_fname = os.path.join(builddir,'Testing','Temporary',rept_basename )
	debug("report filename:\n" + rept_fname )
		
        username = ezsearch('--username=(.*?) ',string.join(sys.argv)+' ')
        host = ezsearch('--host=(.*?) ',string.join(sys.argv)+' ')
        remotepath = ezsearch('--remotepath=(.*?) ',string.join(sys.argv)+' ')

	if rept_fname=='' or username=='' or host=='' or remotepath=='':
		help()
		sys.exit(1)

	res = upload( rept_fname, username, host, remotepath )	
	if res==1:
		print "upload failed"
		return 1
	else:
		print "upload complete:", bytecount, "bytes written"
		return 0


if __name__ == '__main__':
	sys.exit(main())

