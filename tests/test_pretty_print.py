#!/usr/bin/python

# test_pretty_print by don bright 2012. Copyright assigned to Marius Kintel and
# Clifford Wolf 2012. Released under the GPL 2, or later, as described in
# the file named 'COPYING' in OpenSCAD's project root.

#
# This program 'pretty prints' the ctest output, including
# - log files from builddir/Testing/Temporary/ 
# - .png and .txt files from testname-output/*
#
# The result is a single html report file with images data-uri encoded
# into the file. It can be uploaded as a single static file to a web server
# or the 'test_upload.py' script can be used. 


# Design philosophy
#
# 1. parse the data (images, logs) into easy-to-use data structures
# 2. wikifiy the data 
# 3. save the wikified data to disk
# 4. generate html, including base64 encoding of images
# 5. save html file
# 6. upload html to public site and share with others

# todo
#
# 1. Note: Wiki code is deprecated. All future development should move to 
# create html output (or even xml output). Wiki design was based on the 
# wrong assumption of easily accessible public wiki servers with 
# auto-upload scripts available. wiki code should be removed and code 
# simplified.
#
# to still use wiki, use args '--wiki' and/or '--wiki-upload' 
#
# 2. why is hash differing

import string,sys,re,os,hashlib,subprocess,textwrap,time,platform

def tryread(filename):
	data = None
	try:
		f = open(filename,'rb')
		data = f.read()
		f.close()
	except Exception, e:
		print 'couldn\'t open ',filename
		print type(e), e
	return data

def trysave(filename,data):
	dir = os.path.dirname(filename)
	try:
		if not os.path.isdir(dir):
			if not dir == '':
				debug( 'creating' + dir)
				os.mkdir(dir)
		f=open(filename,'wb')
		f.write(data)
		f.close()
	except Exception, e:
		print 'problem writing to',filename
		print type(e), e
		return None
	return True

def ezsearch(pattern,str):
	x = re.search(pattern,str,re.DOTALL|re.MULTILINE)
	if x and len(x.groups())>0: return x.group(1).strip()
	return ''
	
def read_gitinfo():
	# won't work if run from outside of branch. 
	try:
		data = subprocess.Popen(['git','remote','-v'],stdout=subprocess.PIPE).stdout.read()
		origin = ezsearch('^origin *?(.*?)\(fetch.*?$',data)
		upstream = ezsearch('^upstream *?(.*?)\(fetch.*?$',data)
		data = subprocess.Popen(['git','branch'],stdout=subprocess.PIPE).stdout.read()
		branch = ezsearch('^\*(.*?)$',data)
		out  = 'Git branch: ' + branch + ' from origin ' + origin + '\n'
		out += 'Git upstream: ' + upstream + '\n'
	except:
		out = 'Problem running git'
	return out

def read_sysinfo(filename):
	data = tryread(filename)
	if not data: 
		sinfo = platform.sys.platform
		sinfo += '\nsystem cannot create offscreen GL framebuffer object'
		sinfo += '\nsystem cannot create GL based images'
		sysid = platform.sys.platform+'_no_GL_renderer'
		return sinfo, sysid

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
	sysid = sysid.replace('(','_').replace(')','_')
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
		"^ actual .*?:(.*?)\n",
		"^ expected .*?:(.*?)\n",
		'Command:.*?(testdata.*?)"' # scadfile 
		]
	hits = map( lambda pattern: ezsearch(pattern,teststring), patterns )
	test = Test(hits[0],hits[1],hits[2]=='Passed',hits[3],hits[4],hits[5],hits[6],hits[7],teststring)
	if len(test.actualfile) > 0: test.actualfile_data = tryread(test.actualfile)
	if len(test.expectedfile) > 0: test.expectedfile_data = tryread(test.expectedfile)
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

	tests_to_report = failed_tests
	if include_passed: tests_to_report = tests

	try: percent = str(int(100.0*len(passed_tests) / len(tests)))
	except ZeroDivisionError: percent = 'n/a'
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
			if hasattr(t, 'expectedfile_data'): 
                                imgs[wikiname_e] = t.expectedfile_data
			if t.actualfile: 
				actualfile_wiki = '[[File:'+wikiname_a+'|250px]]'
                                if hasattr(t, 'actualfile_data'): 
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

def png_encode64( fname, width=250 ):
	# en.wikipedia.org/wiki/Data_URI_scheme
	try:
		f = open( fname, "rb" )
		data = f.read()
	except:
		data = ''
	data_uri = data.encode("base64").replace("\n","")
	tag  = '<img'
	tag += ' style="border:1px solid gray"'
	tag += ' src="data:image/png;base64,'
	tag +=   data_uri + '"'
	tag += ' width="'+str(width)+'"'
	if data =='':
		tag += ' alt="error: no image generated"'
	else:
		tag += ' alt="openscad_test_image"'
	tag += ' />\n'
	return tag

def tohtml(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles):
	# kludge. assume wiki stuff has alreayd run and dumped files properly
	head = '<html><head><title>'+wiki_rootpath+' test run for '+sysid +'</title></head><body>'
	tail = '</body></html>'

	passed_tests = filter(lambda x: x.passed, tests)
	failed_tests = filter(lambda x: not x.passed, tests)
	try: percent = str(int(100.0*len(passed_tests) / len(tests)))
	except ZeroDivisionError: percent = 'n/a'

	tests_to_report = failed_tests
	if include_passed: tests_to_report = tests

	s=''

	s+= '\n<h3>'
	s+= '\nSystem info\n'
	s+= '\n</h3><p>'
	s+= '<pre>'+sysinfo+'</pre>\n'

	s+= '\n<pre>'
	s+= '\nSTARTDATE: '+ startdate
	s+= '\nENDDATE: '+ enddate
	s+= '\nWIKI_ROOTPATH: '+ wiki_rootpath
	s+= '\nSYSID: '+sysid
	s+= '\nNUMTESTS: '+str(len(tests))
	s+= '\nNUMPASSED: '+str(len(passed_tests))
	s+= '\nPERCENTPASSED: '+ percent
	s+= '\n</pre>'

	if not include_passed:
		s+= '<h3>Failed tests:</h3>\n'

	if len(tests_to_report)==0:
		s+= '<p>none</p>'

	for t in tests_to_report:
		if t.type=='txt':
			s+='\n<pre>'+t.fullname+'</pre>\n'
			s+='<p><pre>'+t.fulltestlog+'</pre>\n\n'
		elif t.type=='png':
			tmp = t.actualfile.replace(builddir,'')
			wikiname_a = wikify_filename(tmp,wiki_rootpath,sysid)
			tmp = t.expectedfile.replace(os.path.dirname(builddir),'')
			wikiname_e = wikify_filename(tmp,wiki_rootpath,sysid)
			# imgtag_e = <img src='+wikiname_e+' width=250/>'
			# imatag_a = <img src='+wikiname_a+' width=250/>'
			imgtag_e = png_encode64( t.expectedfile, 250 )
			imgtag_a = png_encode64( t.actualfile, 250 )
			s+='<table>'
			s+='\n<tr><td colspan=2>'+t.fullname
			s+='\n<tr><td>Expected<td>Actual'
			s+='\n<tr><td>' + imgtag_e + '</td>'
			s+='\n    <td>' + imgtag_a + '</td>'
			s+='\n</table>'
			s+='\n<pre>'
			s+=t.fulltestlog
			s+='\n</pre>'

	s+='\n\n<p>\n\n'

	s+= '<h3> CMake .build files </h3>\n'
	s+= '\n<pre>'
	makefiles_wikinames = {}
	for mf in sorted(makefiles.keys()):
		mfname = mf.strip().lstrip(os.path.sep)
		text = open(os.path.join(builddir,mfname)).read()
		s+= '</pre><h4>'+mfname+'</h4><pre>'
		s+= text
		tmp = mf.replace('CMakeFiles','').replace('.dir','')
		wikiname = wikify_filename(tmp,wiki_rootpath,sysid)
		# s += '\n<a href='+wikiname+'>'+wikiname+'</a><br>'
	s+= '\n</pre>'
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
		if wetrun and len(filedata)>0: 
			wiki_upload(wikiurl,api_php_path,botname,botpass,filedata,wikiname)
		if len(filedata)==0: 
			print 'cancelling empty upload'

def findlogfile(builddir):
	logpath = os.path.join(builddir,'Testing','Temporary')
	logfilename = os.path.join(logpath,'LastTest.log.tmp')
	if not os.path.isfile(logfilename):
		logfilename = os.path.join(logpath,'LastTest.log')
	if not os.path.isfile(logfilename):
		print 'cant find and/or open logfile',logfilename
		sys.exit()
	return logpath, logfilename

def debug(x):
	if debug_test_pp: print 'test_pretty_print: '+x

debug_test_pp=False
builddir=os.getcwd()

def main():
	#wikisite = 'cakebaby.referata.com'
	#wiki_api_path = ''
	global wikisite, wiki_api_path, wiki_rootpath, builddir, debug_test_pp
	global maxretry, dry, include_passed

	wikisite = 'cakebaby.wikia.com'
	wiki_api_path = '/'
	wiki_rootpath = 'OpenSCAD'
	if '--debug' in string.join(sys.argv): debug_test_pp=True
	maxretry = 10

	if bool(os.getenv("TEST_GENERATE")): sys.exit(0)

	include_passed = False
	if '--include-passed' in sys.argv: include_passed = True

	dry = False
	debug( 'running test_pretty_print' )
	if '--dryrun' in sys.argv: dry=True
	suffix = ezsearch('--suffix=(.*?) ',string.join(sys.argv)+' ')
	builddir = ezsearch('--builddir=(.*?) ',string.join(sys.argv)+' ')
	if builddir=='': builddir=os.getcwd()
	debug( 'build dir set to ' +  builddir )

	sysinfo, sysid = read_sysinfo(os.path.join(builddir,'sysinfo.txt'))
	makefiles = load_makefiles(builddir)
	logpath, logfilename = findlogfile(builddir)
	testlog = tryread(logfilename)
	startdate, tests, enddate = parselog(testlog)
	if debug_test_pp:
		print 'found sysinfo.txt,',
		print 'found', len(makefiles),'makefiles,',
		print 'found', len(tests),'test results'

	imgs, txtpages = towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles)

	wikidir = os.path.join(logpath,sysid+'_report')
	debug( 'erasing files in ' + wikidir )
	try: map(lambda x:os.remove(os.path.join(wikidir,x)), os.listdir(wikidir))
	except: pass
	debug( 'output dir:\n' + wikidir.replace(os.getcwd(),'') )
	debug( 'writing ' + str(len(imgs)) + ' images' )
	debug( 'writing ' + str(len(txtpages)-1) + ' text pages' )
	debug( 'writing index.html ' )
	if '--wiki' in string.join(sys.argv):
		print "wiki output is deprecated"
		for pgname in sorted(imgs): trysave( os.path.join(wikidir,pgname), imgs[pgname])
		for pgname in sorted(txtpages): trysave( os.path.join(wikidir,pgname), txtpages[pgname])

	htmldata = tohtml(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles)
	html_basename = sysid+'_report.html'
	html_filename = os.path.join(builddir,'Testing','Temporary',html_basename)
	debug('saving ' +html_filename + ' ' + str(len(htmldata)) + ' bytes')
	trysave( html_filename, htmldata )
	print "report saved:", html_filename.replace(os.getcwd()+os.path.sep,'')

	if '--wiki-upload' in sys.argv:
		print "wiki upload is deprecated."
		upload(wikisite,wiki_api_path,wiki_rootpath,sysid,'openscadbot',
			'tobdacsnepo',wikidir,dryrun=dry)
		print 'upload attempt complete'

	debug( 'test_pretty_print complete' )

if __name__=='__main__':
	main()
