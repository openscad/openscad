#!/usr/bin/python
import string,platform,sys,re,os

wiki_basepath = 'OpenSCAD'
logfilename = 'LastTest.log'
builddir = '.'
logpath = os.path.join(builddir,'Testing','Temporary',logfilename)
NO_END = False
if logfilename.endswith('.tmp'): NO_END = True

def read_sysinfo():
	try:
		f=open('sysinfo.txt')
	except:
		return ''
	data=f.read()
	machine_str, osplain_str, renderer_str = '','',''
	machine = re.search('Machine:(.*?)\n',data)
	osinfo = re.search('OS info:(.*?)\n',data)
	renderer = re.search('GL Renderer:(.*?)\n',data)
	if machine: machine_str = machine.group(1).strip()
	if osinfo: osplain_str = osinfo.group(1).strip().split(' ')[0].strip()
	if renderer: 
		tmp = renderer.group(1).strip().split(' ')
		renderer_str = string.join(tmp[0:3],'-')
	platform = osplain_str + '_' + machine_str + '_' + renderer_str
	platform = platform.lower()
	return data, platform

def readlog():
	try:
		print 'reading',logpath
		f = open(logpath)
	except:
		print 'couldnt open ',logpath
		return None

	data = f.read()
	return data

class Test:
	def __init__(self,fullname,time,passed,output,type,outputfile,expectedfile):
		self.fullname,self.time,self.passed,self.output = \
			fullname, time, passed, output
		self.type = type
		self.outputfile = outputfile
		self.expectedfile = expectedfile

	def __str__(self):
		x = 'fullname: ' + self.fullname
		x+= '\noutputfile: ' + self.outputfile
		x+= '\nexpectedfile: ' + self.expectedfile
		x+= '\ntesttime: ' + self.time
		x+= '\ntesttype: ' + self.type
		x+= '\npassfail: ' + self.passfail
		x+= '\noutput: \n' + self.output
		x+= '\n'
		return x

def gettest_strings(data):
	chunks = data.split('----------------------------------------------------------')
	print 'read',len(chunks), 'chunks'
	if NO_END:
		enddate = 'n/a (cancelled)'
		chunks.pop()
	else:
		enddate = chunks.pop().replace('End testing: ','').strip()
	chunks.reverse()
	startdate = chunks.pop().replace('Start testing: ','').strip()
	chunks.reverse()
	tests=[]
	chunksize = 3
	for i in range(0,len(chunks),chunksize):
		testchunks = chunks[i:i+chunksize]
		test = string.join(testchunks,'-----')
		tests += [test]
		#print '----------<<<<<<<<<<<<<<<<'
		#print test
		#print '----------<<<<<<<<<<<<<<<<'
		test = ''
	sysinfo, platform = read_sysinfo()
	return startdate, tests, enddate, sysinfo, platform

def parsetest(teststring):
	s = teststring
	def regex(pat,str):
		x=re.search(pat,str,re.DOTALL|re.MULTILINE)
		if x: 
			if len(x.groups())>0: 
				return x.group(1).strip()
		return ''
	testfullname = regex("Test:(.*?)\n",s)
	testtime = regex("Test time =(.*?)\n",s).replace(' sec','')
	passfail = regex("Test time.*?Test (Passed)",s)
	command = regex("Command:(.*?)\n",s)
	tmp = command.split(' "')
	testtype = ''
	try:
		testtype = tmp[3].strip('"')
	except:
		print 'failed to parse log', teststring
	goodimg = regex("expected image:(.*?)\n",s)
	actualimg = regex("actual image:(.*?)\n",s)
	if passfail=='Passed': passed = True
	else: passed = False
	output = regex("Output:(.*?)<end of output>",s).replace('-----','')
	test = Test(testfullname, testtime, passed, output, testtype, actualimg, goodimg )
	return test

def parse(data):
	startdate, test_strs, enddate, sysinfo, platform = gettest_strings(data)
	print 'found', len(test_strs),'test results'
	tests = []
	for i in range(len(test_strs)):
		test = parsetest(test_strs[i])
		tests += [test]
	return startdate, tests, enddate, sysinfo, platform

def towiki(startdate, tests, enddate, sysinfo, platform):
	def convert_path(fulltestname,platform,path):
		# convert system path name (image file) to wiki path name
		testprogram = fulltestname[0:fulltestname.find('_')]
		testprogram = testprogram.replace('test','')
		filename = os.path.basename(path)
		filename = filename.replace('-actual','')
		filename = filename.replace('-tests','')
		filename = filename.replace('-expected','')
		try:
			platform = platform[0].upper() + platform[1:]
		except:
			platform = 'error'
		newpath = testprogram + '_' + filename
		# must use _ not / b/c of wikinet.org weird name mangling
		newpath = wiki_basepath + '_' + platform + '_' + newpath
		return newpath
		
	x='''
<h3>OpenSCAD test run</h3>

platform: PLATFORM

detailed system info: 
<pre>
SYSINFO
</pre>

runtime: STARTDATE to ENDDATE

Failed tests

{|TABLESTYLE
! Testname !! expected output !! actual output
|-
| FTESTNAME || [[File:EXPECTEDIMG|thumb|250px]] || [[File:ACTUALIMG|thumb|250px]]
|}

Passed tests

{|TABLESTYLE 
! Testname 
|-
| PTESTNAME
|}

'''

	repeat1='''
|-
| FTESTNAME || [[File:EXPECTEDIMG|thumb|250px]] || [[File:ACTUALIMG|thumb|250px]]
'''

	repeat2='''
|-
| PTESTNAME
'''

	x = x.replace('TABLESTYLE','border=1 cellspacing=0 cellpadding=1 align="center"')
	x = x.replace('STARTDATE',startdate)
	x = x.replace('ENDDATE',enddate)
	x = x.replace('SYSINFO',sysinfo)
	x = x.replace('PLATFORM',platform)

	for t in tests:
		if t.passed:
			tmp = str(repeat2)
			tmp = tmp.replace('PTESTNAME',t.fullname)
			x = x.replace(repeat2,tmp + repeat2)
		else:
			tmp = str(repeat1)
			tmp = tmp.replace('FTESTNAME',t.fullname)
			wiki_imgpath1 = convert_path(t.fullname,'expected',t.expectedfile)
			tmp = tmp.replace('EXPECTEDIMG',wiki_imgpath1)
			wiki_imgpath2 = convert_path(t.fullname,platform,t.outputfile)
			tmp = tmp.replace('ACTUALIMG',wiki_imgpath2)
			x = x.replace(repeat1,tmp + repeat1)

	x = x.replace(repeat1,'')
	x = x.replace(repeat2,'')
	return x

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

def testsort(tests):
	passed = []
	failed = []
	for t in tests:
		if t.passed: passed+=[t]
		else: failed +=[t]
	return failed+passed

def save(data,filename):
	try:
		f=open(filename,'w')
	except:
		print 'couldnt open ',filename, 'for writing'
		return None
	print 'writing',len(data),'bytes to',filename
	f.write(data)
	f.close()

def main():
	data = readlog()
	startdate, tests, enddate, sysinfo, platform = parse(data)
	tests = testsort(tests)
	wikidata = towiki(startdate, tests, enddate, sysinfo, platform)
	htmldata = wikitohtml(wikidata) 
	save(wikidata, platform+'.wiki')
	save(htmldata, platform+'.html')

main()

