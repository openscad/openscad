#!/usr/bin/python
#
# This program 'pretty prints' the ctest output, namely
# files from builddir/Testing/Temporary. 
# html & wiki output are produced
# wiki uploading is available by running 
# 
#  python test_pretty_print.py --upload

# todo
# ban opencsg<2.0 from opencsgtest
# copy all images, sysinfo.txt to bundle for html/upload (images 
#  can be altered  by subsequent runs)
# figure out hwo to make the thing run after the test
# figure out how CTEST treats the logfiles.
# why is hash differing
# instead of having special '-info' prerun, put it as yet-another-test
#  and parse the log

import string,sys,re,os,hashlib,subprocess

def tryread(filename):
	data = None
	try:
		print 'reading', filename
		f = open(filename)
		data = f.read()
		f.close()
	except:
		print 'couldn\'t open ',filename
	return data

def trysave(data,filename):
	try:
		f=open(filename,'w')
		print 'writing',len(data),'bytes to',filename
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
	nondate_data = re.sub("\n.*?ompile date.*?\n","",data)
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
	return test

def parselog(data):
	startdate = ezsearch('Start testing: (.*?)\n',data)
	enddate = ezsearch('End testing: (.*?)\n',data)
	pattern = '([0-9]*/[0-9]* Testing:.*?time elapsed.*?\n)'
	test_chunks = re.findall(pattern,data,re.S)
	tests = map( parsetest, test_chunks )
	print 'found', len(tests),'test results'
	return startdate, tests, enddate

def wikify_filename(testname,filename,sysid):
	# translate from local system to wiki style filename.
	result = wiki_rootpath+'_'+testname+'_'
	expected = ezsearch('(expected....$)',filename)
	if expected!='': result += expected
	actual = ezsearch(os.sep+'.*?-output.*?(actual.*)',filename)
	if actual!='': 
		result += sysid+'_'+actual
	return result.replace('/','_')


def towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid):

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


"""
	passed_tests = filter(lambda x: x.passed, tests)
	failed_tests = filter(lambda x: not x.passed, tests)
	percent = str(int(100.0*len(passed_tests) / len(tests)))
	manifest = {}
	s = wiki_template
	repeat1 = ezsearch('(<REPEAT1>.*?</REPEAT1>)',s)
	repeat2 = ezsearch('(<REPEAT2>.*?</REPEAT2>)',s)
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
			manifest[t.actualfile] = wikify_filename(t.fullname,t.actualfile,sysid)
			manifest[t.expectedfile] = wikify_filename(t.fullname,t.expectedfile,sysid)
			if t.actualfile: 
				actualfile_wiki = '[[File:'+manifest[t.actualfile]+'|250px]]'
			else:
				actualfile_wiki = 'No image generated.'
			newchunk = re.sub('FTESTNAME',t.fullname,repeat1)
			newchunk = newchunk.replace('ACTUALFILE_WIKI',actualfile_wiki)
			newchunk = newchunk.replace('EXPECTEDFILE',manifest[t.expectedfile])
			newchunk = newchunk.replace('TESTLOG',t.fulltestlog)
			s = s.replace(repeat1, newchunk+repeat1)

	s = s.replace(repeat1,'')
	s = s.replace(repeat2,'')
	s = re.sub('<REPEAT.*?>\n','',s)
	s = re.sub('</REPEAT.*?>','',s)
	return manifest, s


def wikitohtml(wiki_rootpath, sysid, wikidata, manifest):
	head = '<html><head><title>'+wiki_rootpath+' test run for '+sysid +'</title></head><body>'
	revmanifest = dict((val,key) for key, val in manifest.iteritems())
	x=re.sub('\{\|(.*?)\n','<table \\1>\n',wikidata)
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

def upload_dryrun(wikiurl,api_php_path,wikidata,manifest,wiki_rootpath,sysid,botname,botpass):
	print 'dry run. no files to be uploaded'
	print 'log in', wikiurl, api_php_path, botname, botpass
	print 'save ' + '*[['+wiki_rootpath+sysid+']]' + ' to page ' + wiki_rootpath
	print 'save ', len(wikidata), ' bytes to page ',wiki_rootpath+sysid
	for localfile in manifest.keys():
		if localfile:
			localf=open(localfile,'rb')
			wikifile = manifest[localfile]
			print 'upload',localfile,wikifile

def upload(wikiurl,api_php_path,wikidata,manifest,wiki_rootpath,sysid,botname,botpass,dryrun=True,forceupload=False):
	if dryrun: 
		upload_dryrun(wikiurl,api_php_path,wikidata,manifest,wiki_rootpath,sysid,botname,botpass)
		return None
	try:
		import mwclient
	except:
		print 'please download mwclient and unpack here:', os.cwd()
	print 'open site',wikiurl
	if not api_php_path == '':
		site = mwclient.Site(wikiurl,api_php_path)
	else:
		site = mwclient.Site(wikiurl)
		
	print 'bot login'
	site.login(botname,botpass)

	print 'edit ',wiki_rootpath
	page = site.Pages[wiki_rootpath]
	text = page.edit()
	rootpage = wiki_rootpath + sysid
	if not '[['+rootpage+']]' in text:
		page.save(text +'\n*[['+rootpage+']]\n')

	print 'upload wiki data to',rootpage
	page = site.Pages[rootpage]
	text = page.edit()
	page.save(wikidata)

	print 'upload images'
	for localfile in sorted(manifest.keys()):
		if localfile:
			localf = open(localfile,'rb')
			wikifile = manifest[localfile]
			skip=False
			if 'expected.png' in wikifile.lower():
				image = site.Images[wikifile]
				if image.exists and forceupload==False:
					print 'skipping',wikifile, '(already on wiki)'
					skip=True
			if not skip:
				print 'uploading',wikifile,'...'
				site.upload(localf,wikifile,wiki_rootpath + ' test', ignore=True)

#wikisite = 'cakebaby.referata.com'
#wiki_api_path = ''
wikisite = 'cakebaby.wikia.com'
wiki_api_path = '/'
wiki_rootpath = 'OpenSCAD'
builddir = os.getcwd()
logpath = os.path.join(builddir,'Testing','Temporary')
logfilename = os.path.join(logpath,'LastTest.log')

def main():
	testlog = tryread(logfilename)
	startdate, tests, enddate = parselog(testlog)
	tests = sorted(tests, key = lambda t:t.passed)
	sysinfo, sysid = read_sysinfo('sysinfo.txt')
	if '--forceupload' in sys.argv: forceupl=True
	else: forceupl=False
	manifest, wikidata = towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid)
	trysave(wikidata, os.path.join(logpath,sysid+'.wiki'))
	htmldata = wikitohtml(wiki_rootpath, sysid, wikidata, manifest)
	trysave(htmldata, os.path.join(logpath,sysid+'.html'))
	if '--upload' in sys.argv:
		upload(wikisite,wiki_api_path,wikidata,manifest,wiki_rootpath,sysid,'openscadbot','tobdacsnepo',dryrun=False,forceupload=forceupl)
main()

