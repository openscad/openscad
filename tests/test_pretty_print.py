#!/usr/bin/python
#
# this program reads the ctest logfiles from builddir/Testing/Temporary
# and generates two 'pretty printed' outputs in that directory:
#
# 1. mediawiki format, for uploading to a wiki.
#    automated uploading is available by following these steps:
# 
#  download mwclient
#  python ctest_pretty_print.py --upload 
#
# 2. html format, for easy local viewing
#
# sysinfo.txt:
#   sysinfo.txt should have been created by ctest by the main test run. 
#   it is the output of opencsgtest --info 


# todo
# 1. add sysinfo about ... git branch
# 2. consolidate image flip
# 3. uploading, can it be done in one file
# 4. consolidate pretty print

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

	hasher = hashlib.md5()
	hasher.update(data)
	hexhash = hasher.hexdigest()[-4:].upper()
	# make all letters for aesthetic reasons
	hash = ''
	for c in hexhash: hash += chr(ord(c)+97-48) 

	sysid = osplain + '_' + machine + '_' + renderer + '_' + hash
	sysid = sysid.lower()
	
	data += read_gitinfo()

	return data, sysid

class Test:
	def __init__(self,fullname,time,passed,output,type,actualfile,expectedfile,scadfile):
		self.fullname,self.time,self.passed,self.output = \
			fullname, time, passed, output
		self.type, self.actualfile, self.expectedfile, self.scadfile = \
			type, actualfile, expectedfile, scadfile

	def __str__(self):
		x = 'fullname: ' + self.fullname
		x+= '\nactualfile: ' + self.actualfile
		x+= '\nexpectedfile: ' + self.expectedfile
		x+= '\ntesttime: ' + self.time
		x+= '\ntesttype: ' + self.type
		x+= '\npassed: ' + str(self.passed)
		x+= '\nscadfile: ' + self.scadfile
		x+= '\noutput bytes: ' + str(len(self.output))
		x+= '\n'
		return x

def parsetest(teststring):
	patterns = ["Test:(.*?)\n", # fullname
		"Test time =(.*?) sec\n",
		"Test time.*?Test (Passed)",
		"Output:(.*?)<end of output>",
		'Command:.*?-s" "(.*?)"', # type
		"actual .*?:(.*?)\n",
		"expected .*?:(.*?)\n",
		'Command:.*?(testdata.*?)"' # scadfile 
		]
	hits = map( lambda pattern: ezsearch(pattern,teststring), patterns )
	test = Test(hits[0],hits[1],hits[2]=='Passed',hits[3],hits[4],hits[5],hits[6],hits[7])
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


def towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, testlog):
	wiki_template = '''
<h3>OpenSCAD test run</h3>

sysid: SYSID

detailed system info: 
<pre>
SYSINFO
</pre>

start time: STARTDATE 
end time  : ENDDATE

Failed image tests

{|border=1 cellspacing=0 cellpadding=1 align="center"
! Testname !! expected output !! actual output
<REPEAT1>
|-
| FTESTNAME || [[File:EXPECTEDFILE|thumb|250px]] || ACTUALFILE_WIKI
</REPEAT1>
|}

Failed text tests (see test log, below, for diff output)

{|border=1 cellspacing=0 cellpadding=1 align="center"
<REPEAT2>
|-
| FTESTNAME
</REPEAT2>
|}

Passed tests

{|border=1 cellspacing=0 cellpadding=1 align="center"
! Testname 
<REPEAT3>
|-
| PTESTNAME
</REPEAT3>
|}

LastTest.log

<pre>
LASTTESTLOG
</pre>

'''
	manifest = {}
	s = wiki_template
	repeat1 = ezsearch('(<REPEAT1>.*?</REPEAT1>)',s)
	repeat2 = ezsearch('(<REPEAT2>.*?</REPEAT2>)',s)
	repeat3 = ezsearch('(<REPEAT3>.*?</REPEAT3>)',s)
	dic = { 'STARTDATE': startdate, 'ENDDATE': enddate,
		'SYSINFO': sysinfo, 'SYSID':sysid, 'LASTTESTLOG':testlog }
	for key in dic.keys():
		s = re.sub(key,dic[key],s)
	for t in tests:
		print t.passed, t.type, t.fullname, t.expectedfile, t.actualfile
		manifest[t.actualfile] = wikify_filename(t.fullname,t.actualfile,sysid)
		manifest[t.expectedfile] = wikify_filename(t.fullname,t.expectedfile,sysid)
		if t.passed:
			newchunk = re.sub('PTESTNAME',t.fullname,repeat3)
			s = s.replace(repeat3, newchunk+repeat3)
		elif not t.passed and t.type=='txt':
			newchunk = re.sub('FTEST_OUTPUTFILE',t.fullname,repeat2)
			newchunk = re.sub('FTESTNAME',t.fullname,repeat2)
			s = s.replace(repeat2, newchunk+repeat2)
		elif not t.passed and t.type=='png':
			if t.actualfile: 
				actualfile_wiki = '[[File:'+manifest[t.actualfile]+'|thumb|250px]]'
			else:
				actualfile_wiki = 'No file generated. See test output for more info'
			newchunk = re.sub('FTESTNAME',t.fullname,repeat1)
			newchunk = newchunk.replace('ACTUALFILE_WIKI',actualfile_wiki)
			newchunk = newchunk.replace('EXPECTEDFILE',manifest[t.expectedfile])
			s = s.replace(repeat1, newchunk+repeat1)
	s = s.replace(repeat1,'')
	s = s.replace(repeat2,'')
	s = s.replace(repeat3,'')
	s = re.sub('<REPEAT.*?>\n','',s)
	s = re.sub('</REPEAT.*?>','',s)
	return manifest, s

def wikitohtml(data):
	# not pretty 
	data = data.replace('\n\n','\n<p>\n')
	data = re.sub('\{\|.*?\n','<table border=1>\n',data)
	data = re.sub('\n\! ','\n<tr>\n<td>',data)
	data = data.replace(' !! ','<td>')
	data = data.replace('|-','<tr>')
	data = re.sub('\n\| ','\n<td>',data)
	data = data.replace(' || ','<td>')
	data = data.replace('|}','\n</table>')
	data = re.sub('[[File:(.*?)|.*?]]','<img src="(\1)">',data)
	return data

wiki_rootpath = 'OpenSCAD'
builddir = os.getcwd()
logpath = os.path.join(builddir,'Testing','Temporary')
logfilename = os.path.join(logpath,'LastTest.log')

def main():
	testlog = tryread(logfilename)
	startdate, tests, enddate = parselog(testlog)
	tests = sorted(tests, key = lambda t:t.passed)
	sysinfo, sysid = read_sysinfo('sysinfo.txt')
	manifest, wikidata = towiki(wiki_rootpath, startdate, tests, enddate, sysinfo, sysid, testlog)
	htmldata = wikitohtml(wikidata) 
	#save(wikidata, os.path.join(logpath,sysid+'.wiki'))
	#save(htmldata, os.path.join(logpath,sysid+'.html'))
	trysave(wikidata, sysid+'.wiki')
	trysave(htmldata, sysid+'.html')

main()

