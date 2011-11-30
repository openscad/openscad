#!/usr/bin/python
#
# This program 'pretty prints' the ctest output, namely
# files from builddir/Testing/Temporary. 
# html & wiki output are produced
# wiki uploading is available by running 
# 
#  python test_pretty_print.py --upload

# todo
# do something if tests for opencsg extensions fail (fail, no image production)
# copy all images, sysinfo.txt to bundle for html/upload (images 
#  can be altered  by subsequent runs)
# figure out hwo to make the thing run after the test
# figure out how CTEST treats the logfiles.
# why is hash differing
# instead of having special '-info' prerun, put it as yet-another-test
#  and parse the log

import string,sys,re,os,hashlib,subprocess,textwrap

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
			print 'creating',os.path.dirname(filename)
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

'''Failed image tests'''

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


'''Failed text tests'''

{|border=1 cellspacing=0 cellpadding=1
! Testname 
<REPEAT2>
|-
| FTESTNAME
</REPEAT2>
|}

'''build.make and flags.make'''
<REPEAT3>
*[[MAKEFILE_NAME]]
</REPEAT3>
"""
	txtpages = {}
	imgs = {}
	passed_tests = filter(lambda x: x.passed, tests)
	failed_tests = filter(lambda x: not x.passed, tests)
	percent = str(int(100.0*len(passed_tests) / len(tests)))
	manifest = {}
	s = wiki_template
	repeat1 = ezsearch('(<REPEAT1>.*?</REPEAT1>)',s)
	repeat2 = ezsearch('(<REPEAT2>.*?</REPEAT2>)',s)
	repeat3 = ezsearch('(<REPEAT3>.*?</REPEAT3>)',s)
	dic = { 'STARTDATE': startdate, 'ENDDATE': enddate, 'WIKI_ROOTPATH': wiki_rootpath,
		'SYSINFO': sysinfo, 'SYSID':sysid, 
		'NUMTESTS':len(tests), 'NUMPASSED':len(passed_tests), 'PERCENTPASSED':percent }
	for key in dic.keys():
		s = s.replace(key,str(dic[key]))
	testlogs = ''
	for t in failed_tests:
		testlogs += '\n\n'+t.fulltestlog
		if t.type=='txt':
			newchunk = re.sub('FTEST_OUTPUTFILE',t.fullname,repeat2)
			newchunk = re.sub('FTESTNAME',t.fullname,repeat2)
			s = s.replace(repeat2, newchunk+repeat2)
		elif t.type=='png':
			tmp = t.actualfile.replace(builddir,'')
			wikiname_a = wikify_filename(tmp,wiki_rootpath,sysid)
			# erase /home/whatever/openscad/tests/regression
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
		txtpages[ makefiles_wikinames[ mf ] ] = '<pre>\n'+makefiles[mf]+'\n</pre>'

	return imgs, txtpages

def wikitohtml(wiki_rootpath, sysid, wikidata, manifest):
	head = '<html><head><title>'+wiki_rootpath+' test run for '+sysid +'</title></head><body>'
	revmanifest = dict((val,key) for key, val in manifest.iteritems())
	x=re.sub('\{\|(.*?)\n','<table \\1>\n',wikidata)
	x=re.sub('\|(.*?colspan.*?)\|','<td \\1>',x)
	x=re.sub("'''(.*?)'''","<b>\\1</b>",x)
	filestrs=re.findall('\[\[File\:(.*?)\|.*?\]\]',x)
	for f in filestrs:
		newfile_html='<img src="'+revmanifest[f]+'" width=250 />'
		x=re.sub('\[\[File\:'+f+'\|.*?\]\]',newfile_html,x)
	dic = { '|}':'</table>', '|-':'<tr>', '||':'<td>', '|':'<td>', 
		'!!':'<th>', '!':'<tr><th>', '\n\n':'\n<p>\n'} #order matters
	for key in dic: x=x.replace(key,dic[key])
	x=re.sub("\[\[(.*?)\]\]","\\1",x)
	return head + x + '</body></html>'

def upload(wikiurl,api_php_path='/',wiki_rootpath='test', sysid='null', botname='cakebaby',botpass='anniew',wikidir='.',dryrun=True):
	wetrun = not dryrun
	if dryrun: print 'dry run'
	try:
		import mwclient
	except:
		print 'please download mwclient and unpack here:', os.getcwd()
		sys.exit()
	print 'opening site:',wikiurl
	if wetrun:
		site = mwclient.Site(wikiurl,api_php_path)
		
	print 'bot login:', botname
	if wetrun: site.login(botname,botpass)

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
	print 'upload wiki pages:'
	for wikiname in wikifiles:
		filename = os.path.join(wikidir,wikiname)
		filedata = tryread(filename)
		print 'upload',len(filedata),'bytes from',wikiname,'...',
		sys.stdout.flush()
		if wikiname.endswith('.png'):
			localf = open(filename,'rb') # mwclient needs file handle
			descrip = wiki_rootpath + ' test'
			if wetrun:
				site.upload(localf,wikiname,descrip,ignore=True)
			print 'image uploaded'
		else: # textpage
			if wetrun:
				page = site.Pages[wikiname]
				text = page.edit()
				page.save(filedata)
			print 'text page uploaded'

def findlogfile(builddir):
	logpath = os.path.join(builddir,'Testing','Temporary')
	logfilename = os.path.join(logpath,'LastTest.log')
	return logpath, logfilename

def main():
	dry = False
	if '--dryrun' in sys.argv: dry=True
	print 'build dir set to', builddir

	sysinfo, sysid = read_sysinfo(os.path.join(builddir,'sysinfo.txt'))
	makefiles = load_makefiles(builddir)
	logpath, logfilename = findlogfile(builddir)
	testlog = tryread(logfilename)
	startdate, tests, enddate = parselog(testlog)
	print 'found sysinfo.txt'
	print 'found', len(makefiles),'makefiles'
	print 'found', len(tests),'test results'

	imgs, txtpages = towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, makefiles)

	wikidir = os.path.join(logpath,'wiki')
	print 'writing',len(imgs),'images and',len(txtpages),'wiki pages to:\n ', wikidir
	for k in sorted(imgs): trysave( os.path.join(wikidir,k), imgs[k])
	for k in sorted(txtpages): trysave( os.path.join(wikidir,k), txtpages[k])

	if '--upload' in sys.argv:
		upload(wikisite,wiki_api_path,wiki_rootpath,sysid,'openscadbot',
			'tobdacsnepo',wikidir,dryrun=dry)
	
#wikisite = 'cakebaby.referata.com'
#wiki_api_path = ''
wikisite = 'cakebaby.wikia.com'
wiki_api_path = '/'
wiki_rootpath = 'OpenSCAD'
builddir = os.getcwd() # os.getcwd()+'/build'

main()
