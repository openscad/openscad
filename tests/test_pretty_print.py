#!/usr/bin/python

#  Copyright (C) 2011 Don Bright <hugh.m.bright@gmail.com> 
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#
# This program 'pretty prints' the ctest output, namely
# files from builddir/Testing/Temporary. 
# html & wiki output are produced in Testing/Temporary/sysid_report
#
# experimental wiki uploading is available by running 
# 
#  python test_pretty_print.py --upload
#

# Design philosophy
#
# 1. parse the data (images, logs) into easy-to-use data structures
# 2. wikifiy the data 
# 3. save the wikified data to disk

# todo
# do something if tests for GL extensions for OpenCSG fail (test fail, no image production)
# copy all images, sysinfo.txt to bundle for html/upload (images 
#  can be altered  by subsequent runs)
# figure out hwo to make the thing run after the test
# figure out how CTEST treats the logfiles.
# why is hash differing
# instead of having special '-info' prerun, put it as yet-another-test
#  and parse the log
# fix windows so that it won't keep asking 'this program crashed' over and over. 
#  (you can set this in the registry to never happen, but itd be better if the program
#   itself was able to disable that temporarily in it's own process)

import string,sys,re,os,hashlib,subprocess,textwrap,time

def tryread(filename):
	data = None
	try:
		f = open(filename,'rb')
		data = f.read()
		f.close()
	except:
		print 'couldn\'t open ',filename
	return data

def trysave(filename,data):
	try:
		if not os.path.isdir(os.path.dirname(filename)):
			#print 'creating',os.path.dirname(filename)
			os.mkdir(os.path.dirname(filename))
		f=open(filename,'wb')
		f.write(data)
		f.close()
	except:
		print 'problem writing to',filename
		return None
	return True

def ezsearch(pattern,str):
	x = re.search(pattern,str,re.DOTALL|re.MULTILINE)
	if x and len(x.groups())>0: return x.group(1).strip()
	return ''
	
def read_gitinfo():
	# won't work if run from outside of branch. 
	data = subprocess.Popen(['git','remote','-v'],stdout=subprocess.PIPE).stdout.read()
	origin = ezsearch('^origin *?(.*?)\(fetch.*?$',data)
	upstream = ezsearch('^upstream *?(.*?)\(fetch.*?$',data)
	data = subprocess.Popen(['git','branch'],stdout=subprocess.PIPE).stdout.read()
	branch = ezsearch('^\*(.*?)$',data)
	out  = 'Git branch: ' + branch + ' from origin ' + origin + '\n'
	out += 'Git upstream: ' + upstream + '\n'
	return out

def read_sysinfo(filename):
	data = tryread(filename)
	if not data: return 'sysinfo: unknown'

	machine = ezsearch('Machine:(.*?)\n',data)
	machine = machine.replace(' ','-').replace('/','-')

	osinfo = ezsearch('OS info:(.*?)\n',data)
	osplain = osinfo.split(' ')[0].strip().replace('/','-')
	if 'windows' in osinfo.lower(): osplain = 'win'

	renderer = ezsearch('GL Renderer:(.*?)\n',data)
	tmp = renderer.split(' ')
	tmp = string.join(tmp[0:3],'-')
	tmp = tmp.split('/')[0]
	renderer = tmp

	data += read_gitinfo()

	data += 'Image comparison: ImageMagick'

	data = data.strip()

	# create 4 letter hash and stick on end of sysid
	nondate_data = re.sub("\n.*?ompile date.*?\n","\n",data).strip()
	hexhash = hashlib.md5()
	hexhash.update(nondate_data)
	hexhash = hexhash.hexdigest()[-4:].upper()
	hash = ''
	for c in hexhash: hash += chr(ord(c)+97-48) 

	sysid = osplain + '_' + machine + '_' + renderer + '_' + hash
	sysid = sysid.lower()

	return data, sysid

class Test:
	def __init__(self,fullname,time,passed,output,type,actualfile,expectedfile,scadfile,log):
		self.fullname,self.time,self.passed,self.output = \
			fullname, time, passed, output
		self.type, self.actualfile, self.expectedfile, self.scadfile = \
			type, actualfile, expectedfile, scadfile
		self.fulltestlog = log

	def __str__(self):
		x = 'fullname: ' + self.fullname
		x+= '\nactualfile: ' + self.actualfile
		x+= '\nexpectedfile: ' + self.expectedfile
		x+= '\ntesttime: ' + self.time
		x+= '\ntesttype: ' + self.type
		x+= '\npassed: ' + str(self.passed)
		x+= '\nscadfile: ' + self.scadfile
		x+= '\noutput bytes: ' + str(len(self.output))
		x+= '\ntestlog bytes: ' + str(len(self.fulltestlog))
		x+= '\n'
		return x

def parsetest(teststring):
	patterns = ["Test:(.*?)\n", # fullname
		"Test time =(.*?) sec\n",
		"Test time.*?Test (Passed)", # pass/fail
		"Output:(.*?)<end of output>",
		'Command:.*?-s" "(.*?)"', # type
		"actual .*?:(.*?)\n",
		"expected .*?:(.*?)\n",
		'Command:.*?(testdata.*?)"' # scadfile 
		]
	hits = map( lambda pattern: ezsearch(pattern,teststring), patterns )
	test = Test(hits[0],hits[1],hits[2]=='Passed',hits[3],hits[4],hits[5],hits[6],hits[7],teststring)
	test.actualfile_data = tryread(test.actualfile)
	test.expectedfile_data = tryread(test.expectedfile)
	return test

def parselog(data):
	startdate = ezsearch('Start testing: (.*?)\n',data)
	enddate = ezsearch('End testing: (.*?)\n',data)
	pattern = '([0-9]*/[0-9]* Testing:.*?time elapsed.*?\n)'
	test_chunks = re.findall(pattern,data,re.S)
	tests = map( parsetest, test_chunks )
	tests = sorted(tests, key = lambda t:t.passed)
	return startdate, tests, enddate

def load_makefiles(builddir):
	filelist = []
	for root, dirs, files in os.walk(builddir):
		for fname in files: filelist += [ os.path.join(root, fname) ]
	files  = filter(lambda x: 'build.make' in os.path.basename(x), filelist)
	files += filter(lambda x: 'flags.make' in os.path.basename(x), filelist)
	files = filter(lambda x: 'esting' not in x and 'emporary' not in x, files)
	result = {}
	for fname in files:
		result[fname.replace(builddir,'')] = open(fname,'rb').read()
	return result

def wikify_filename(fname, wiki_rootpath, sysid):
	wikifname = fname.replace('/','_').replace('\\','_').strip('.')
	return wiki_rootpath + '_' + sysid + '_' + wikifname

def towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles):

	wiki_template = """
<h3>[[WIKI_ROOTPATH]] test run report</h3>

'''Sysid''': SYSID

'''Result summary''': NUMPASSED / NUMTESTS tests passed ( PERCENTPASSED % ) <br>

'''System info''':
<pre>
SYSINFO
</pre>

start time: STARTDATE <br>
end time  : ENDDATE <br>

'''Image tests'''

<REPEAT1>
{| border=1 cellspacing=0 cellpadding=1
|-
| colspan=2 | FTESTNAME  
|-
| Expected image || Actual image
|-
| [[File:EXPECTEDFILE|250px]] || ACTUALFILE_WIKI
|}

<pre>
TESTLOG
</pre>



</REPEAT1>


'''Text tests'''

<REPEAT2>
{|border=1 cellspacing=0 cellpadding=1
|-
| FTESTNAME
|}

<pre>
TESTLOG
</pre>


</REPEAT2>

'''build.make and flags.make'''
<REPEAT3>
*[[MAKEFILE_NAME]]
</REPEAT3>
"""
	txtpages = {}
	imgs = {}
	passed_tests = filter(lambda x: x.passed, tests)
	failed_tests = filter(lambda x: not x.passed, tests)

	tests_to_report = tests
	if failed_only: tests_to_report = failed_tests

	percent = str(int(100.0*len(passed_tests) / len(tests)))
	s = wiki_template
	repeat1 = ezsearch('(<REPEAT1>.*?</REPEAT1>)',s)
	repeat2 = ezsearch('(<REPEAT2>.*?</REPEAT2>)',s)
	repeat3 = ezsearch('(<REPEAT3>.*?</REPEAT3>)',s)
	dic = { 'STARTDATE': startdate, 'ENDDATE': enddate, 'WIKI_ROOTPATH': wiki_rootpath,
		'SYSINFO': sysinfo, 'SYSID':sysid, 
		'NUMTESTS':len(tests), 'NUMPASSED':len(passed_tests), 'PERCENTPASSED':percent }
	for key in dic.keys():
		s = s.replace(key,str(dic[key]))

	for t in tests_to_report:
		if t.type=='txt':
			newchunk = re.sub('FTESTNAME',t.fullname,repeat2)
			newchunk = newchunk.replace('TESTLOG',t.fulltestlog)
			s = s.replace(repeat2, newchunk+repeat2)
		elif t.type=='png':
			tmp = t.actualfile.replace(builddir,'')
			wikiname_a = wikify_filename(tmp,wiki_rootpath,sysid)
			tmp = t.expectedfile.replace(os.path.dirname(builddir),'')
			wikiname_e = wikify_filename(tmp,wiki_rootpath,sysid)
			imgs[wikiname_e] = t.expectedfile_data
			if t.actualfile: 
				actualfile_wiki = '[[File:'+wikiname_a+'|250px]]'
				imgs[wikiname_a] = t.actualfile_data
			else:
				actualfile_wiki = 'No image generated.'
			newchunk = re.sub('FTESTNAME',t.fullname,repeat1)
			newchunk = newchunk.replace('ACTUALFILE_WIKI',actualfile_wiki)
			newchunk = newchunk.replace('EXPECTEDFILE',wikiname_e)
			newchunk = newchunk.replace('TESTLOG',t.fulltestlog)
			s = s.replace(repeat1, newchunk+repeat1)

	makefiles_wikinames = {}
	for mf in sorted(makefiles.keys()):
		tmp = mf.replace('CMakeFiles','').replace('.dir','')
		wikiname = wikify_filename(tmp,wiki_rootpath,sysid)
		newchunk = re.sub('MAKEFILE_NAME',wikiname,repeat3)
		s = s.replace(repeat3, newchunk+repeat3)
		makefiles_wikinames[mf] = wikiname

	s = s.replace(repeat1,'')
	s = s.replace(repeat2,'')
	s = s.replace(repeat3,'')
	s = re.sub('<REPEAT.*?>\n','',s)
	s = re.sub('</REPEAT.*?>','',s)

	mainpage_wikiname = wiki_rootpath + '_' + sysid + '_test_report'
	txtpages[ mainpage_wikiname ] = s
	for mf in sorted(makefiles.keys()):
		txtpages[ makefiles_wikinames[ mf ] ] = '\n*Subreport from [['+mainpage_wikiname+']]\n\n\n<pre>\n'+makefiles[mf]+'\n</pre>'

	return imgs, txtpages

def tohtml(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles):
	# kludge. assume wiki stuff has alreayd run and dumped files properly
	head = '<html><head><title>'+wiki_rootpath+' test run for '+sysid +'</title></head><body>'
	tail = '</body></html>'

	passed_tests = filter(lambda x: x.passed, tests)
	failed_tests = filter(lambda x: not x.passed, tests)
	percent = str(int(100.0*len(passed_tests) / len(tests)))

	tests_to_report = tests
	if failed_only: tests_to_report = failed_tests

	s=''

	s+= '\n<pre>'
	s+= '\nSYSINFO\n'+ sysinfo
	s+= '\n</pre><p>'

	s+= '\n<pre>'
	s+= '\nSTARTDATE: '+ startdate
	s+= '\nENDDATE: '+ enddate
	s+= '\nWIKI_ROOTPATH: '+ wiki_rootpath
	s+= '\nSYSID: '+sysid
	s+= '\nNUMTESTS: '+str(len(tests))
	s+= '\nNUMPASSED: '+str(len(passed_tests))
	s+= '\nPERCENTPASSED: '+ percent
	s+= '\n</pre>'

	for t in tests_to_report:
		if t.type=='txt':
			s+='\n<pre>'+t.fullname+'</pre>\n'
			s+='<p><pre>'+t.fulltestlog+'</pre>\n\n'
		elif t.type=='png':
			tmp = t.actualfile.replace(builddir,'')
			wikiname_a = wikify_filename(tmp,wiki_rootpath,sysid)
			tmp = t.expectedfile.replace(os.path.dirname(builddir),'')
			wikiname_e = wikify_filename(tmp,wiki_rootpath,sysid)
			s+='<table>'
			s+='\n<tr><td colspan=2>'+t.fullname
			s+='\n<tr><td>Expected<td>Actual'
			s+='\n<tr><td><img src='+wikiname_e+' width=250/>'
			s+='\n    <td><img src='+wikiname_a+' width=250/>'
			s+='\n</table>'
			s+='\n<pre>'
			s+=t.fulltestlog
			s+='\n</pre>'

	s+='\n\n<p>\n\n'
	makefiles_wikinames = {}
	for mf in sorted(makefiles.keys()):
		tmp = mf.replace('CMakeFiles','').replace('.dir','')
		wikiname = wikify_filename(tmp,wiki_rootpath,sysid)
		s += '\n<a href='+wikiname+'>'+wikiname+'</a><br>'
	s+='\n'

	return head + s + tail

def wiki_login(wikiurl,api_php_path,botname,botpass):
	site = mwclient.Site(wikiurl,api_php_path)
	site.login(botname,botpass)
	return site

def wiki_upload(wikiurl,api_php_path,botname,botpass,filedata,wikipgname):
	counter = 0
	done = False
	descrip = 'test'
	time.sleep(1)
	while not done:
		try:
			print 'login',botname,'to',wikiurl
			site = wiki_login(wikiurl,api_php_path,botname,botpass)
			print 'uploading...',
			if wikipgname.endswith('png'):
				site.upload(filedata,wikipgname,descrip,ignore=True)
			else:
				page = site.Pages[wikipgname]
				text = page.edit()
				page.save(filedata)
			done = True
			print 'transfer ok'
		except Exception, e:
			print 'Error:', type(e),e
			counter += 1
			if counter>maxretry: 
				print 'giving up. please try a different wiki site'
				done = True
			else:
				print 'wiki',wikiurl,'down. retrying in 15 seconds'
				time.sleep(15)	

def upload(wikiurl,api_php_path='/',wiki_rootpath='test', sysid='null', botname='cakebaby',botpass='anniew',wikidir='.',dryrun=True):
	wetrun = not dryrun
	if dryrun: print 'dry run'
	try:
		global mwclient
		import mwclient
	except:
		print 'please download mwclient 0.6.5 and unpack here:', os.getcwd()
		sys.exit()

	if wetrun: site = wiki_login(wikiurl,api_php_path,botname,botpass)

	wikifiles = os.listdir(wikidir)
	testreport_page = filter( lambda x: 'test_report' in x, wikifiles )
	if (len(testreport_page)>1): 
		print 'multiple test reports found, please clean dir',wikidir
		sys.exit()

	rootpage = testreport_page[0]
	print 'add',rootpage,' to main report page ',wiki_rootpath
	if wetrun:
		page = site.Pages[wiki_rootpath]
		text = page.edit()
		if not '[['+rootpage+']]' in text:
			page.save(text +'\n*[['+rootpage+']]\n')

	wikifiles = os.listdir(wikidir)
	wikifiles = filter(lambda x: not x.endswith('html'), wikifiles)

	print 'upload wiki pages:'
	for wikiname in wikifiles:
		filename = os.path.join(wikidir,wikiname)
		filedata = tryread(filename)
		print 'upload',len(filedata),'bytes from',wikiname
		if wetrun: 
			wiki_upload(wikiurl,api_php_path,botname,botpass,filedata,wikiname)

def findlogfile(builddir):
	logpath = os.path.join(builddir,'Testing','Temporary')
	logfilename = os.path.join(logpath,'LastTest.log.tmp')
	if not os.path.isfile(logfilename):
		logfilename = os.path.join(logpath,'LastTest.log')
	if not os.path.isfile(logfilename):
		print 'cant find and/or open logfile',logfilename
		sys.exit()
	return logpath, logfilename

def main():
	dry = False
	if verbose: print 'running test_pretty_print'
	if '--dryrun' in sys.argv: dry=True
	suffix = ezsearch('--suffix=(.*?) ',string.join(sys.argv)+' ')
	builddir = ezsearch('--builddir=(.*?) ',string.join(sys.argv)+' ')
	if builddir=='': builddir=os.getcwd()
	if verbose: print 'build dir set to', builddir

	sysinfo, sysid = read_sysinfo(os.path.join(builddir,'sysinfo.txt'))
	makefiles = load_makefiles(builddir)
	logpath, logfilename = findlogfile(builddir)
	testlog = tryread(logfilename)
	startdate, tests, enddate = parselog(testlog)
	if verbose:
		print 'found sysinfo.txt,',
		print 'found', len(makefiles),'makefiles,',
		print 'found', len(tests),'test results'

	imgs, txtpages = towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles)

	wikidir = os.path.join(logpath,sysid+'_report')
	if verbose: print 'erasing files in',wikidir
	try: map(lambda x:os.remove(os.path.join(wikidir,x)), os.listdir(wikidir))
	except: pass
	print 'writing',len(imgs),'images and',len(txtpages),'text pages to:\n', ' .'+wikidir.replace(os.getcwd(),'')
	for pgname in sorted(imgs): trysave( os.path.join(wikidir,pgname), imgs[pgname])
	for pgname in sorted(txtpages): trysave( os.path.join(wikidir,pgname), txtpages[pgname])

	htmldata = tohtml(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles)
	trysave( os.path.join(wikidir,'index.html'), htmldata )

	if '--upload' in sys.argv:
		upload(wikisite,wiki_api_path,wiki_rootpath,sysid,'openscadbot',
			'tobdacsnepo',wikidir,dryrun=dry)
		print 'upload attempt complete'

	if verbose: print 'test_pretty_print complete'

#wikisite = 'cakebaby.referata.com'
#wiki_api_path = ''
wikisite = 'cakebaby.wikia.com'
wiki_api_path = '/'
wiki_rootpath = 'OpenSCAD'
builddir = os.getcwd() # os.getcwd()+'/build'
verbose = False
maxretry = 10

failed_only = False
if '--failed-only' in sys.argv: failed_only = True

main()
