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
# 1. why is hash differing

import string
import sys
import re
import os
import hashlib
import subprocess
import time
import platform
import cgi
try:
    from urllib.error import URLError
    from urllib.request import urlopen
    from urllib.parse import urlencode
except:
    from urllib2 import URLError
    from urllib2 import urlopen
    from urllib import urlencode

def tryread(filename):
    data = None
    try:
        f = open(filename,'rb')
        data = f.read()
        f.close()
    except Exception as e:
        debug( "couldn't open file: [" + filename + "]" )
        debug( str(type(e))+str(e) )
        if filename==None:
            # dont write a bunch of extra errors during test output. 
            # the reporting of test failure is sufficient to indicate a problem
            pass
    return data

def trysave(filename, data):
    dir = os.path.dirname(filename)
    try:
        if not os.path.isdir(dir):
            if not dir == '':
                debug( 'creating' + dir)
                os.mkdir(dir)
        f=open(filename,'wb')
        f.write(data)
        f.close()
    except Exception as e:
        print 'problem writing to',filename
        print type(e), e
        return None
    return True

def ezsearch(pattern, str):
    x = re.search(pattern,str,re.DOTALL|re.MULTILINE)
    if x and len(x.groups())>0: return x.group(1).strip()
    return ''
    
def read_gitinfo():
    # won't work if run from outside of branch. 
    try:
        data = subprocess.Popen(['git', 'remote', '-v'], stdout=subprocess.PIPE).stdout.read()
        origin = ezsearch('^origin *?(.*?)\(fetch.*?$', data)
        upstream = ezsearch('^upstream *?(.*?)\(fetch.*?$', data)
        data = subprocess.Popen(['git', 'branch'], stdout=subprocess.PIPE).stdout.read()
        branch = ezsearch('^\*(.*?)$', data)
        out = 'Git branch: ' + branch + ' from origin ' + origin + '\n'
        out += 'Git upstream: ' + upstream + '\n'
    except:
        out = 'Git branch: Unknown (could not run git)\n'
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
    if 'windows' in osinfo.lower():
        osplain = 'win'

    renderer = ezsearch('GL Renderer:(.*?)\n',data)
    tmp = renderer.split(' ')
    tmp = string.join(tmp[0:min(len(tmp),4)],'-')
    tmp = tmp.split('/')[0]
    renderer = tmp

    data += read_gitinfo()
    data = data.strip()

    # create 4 letter hash and stick on end of sysid
    nondate_data = re.sub("\n.*?ompile date.*?\n", "\n", data).strip()
    hexhash = hashlib.md5(nondate_data).hexdigest()[-4:].upper()
    hash_ = ''.join(chr(ord(c) + 97 - 48) for c in hexhash)

    sysid = '_'.join([osplain, machine, renderer, hash_])
    sysid = sysid.replace('(', '_').replace(')', '_')
    sysid = sysid.lower()

    return data, sysid

class Test:
    def __init__(self, fullname, subpr, passed, output, type, actualfile,
                 expectedfile, scadfile, log):
        self.fullname= fullname
        self.passed, self.output = passed, output
        self.type, self.actualfile = type, actualfile
        self.expectedfile, self.scadfile = expectedfile, scadfile
        self.fulltestlog = log
        self.actualfile_data = None
        self.expectedfile_data = None
        
    def __str__(self):
        x = 'fullname: ' + self.fullname
        x+= '\nactualfile: ' + self.actualfile
        x+= '\nexpectedfile: ' + self.expectedfile
        if self.actualfile_data:
            x+= '\nactualfile_data (#bytes): ' + str(len(self.actualfile_data))
        if self.expectedfile_data:
            x+= '\nexpectedfile_data (#bytes): ' + str(len(self.expectedfile_data))
        x+= '\ntesttype: ' + str(self.type)
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
    hits = map( lambda pattern: ezsearch(pattern, teststring), patterns)
    test = Test(hits[0], hits[1], hits[2]=='Passed', hits[3], hits[4], hits[5], 
                hits[6], hits[7], teststring)
    if len(test.actualfile) > 0:
        test.actualfile_data = tryread(test.actualfile)
    if len(test.expectedfile) > 0:
        test.expectedfile_data = tryread(test.expectedfile)
    debug(" Parsed test\n" + str(test)+'\n')
    return test

def parselog(data):
    startdate = ezsearch('Start testing: (.*?)\n', data)
    enddate = ezsearch('End testing: (.*?)\n', data)
    pattern = '([0-9]*/[0-9]* Testing:.*?time elapsed.*?\n)'
    test_chunks = re.findall(pattern,data, re.S)
    tests = map( parsetest, test_chunks )
    tests = sorted(tests, key = lambda t: t.passed)
    imgcomparer='ImageMagick'
    if '--comparator=diffpng' in data: imgcomparer='diffpng'
    return startdate, tests, enddate, imgcomparer

def load_makefiles(builddir):
    filelist = []
    for root, dirs, files in os.walk(builddir):
        for fname in files: filelist += [ os.path.join(root, fname) ]
    files = [file for file in filelist if 'build.make' in os.path.basename(file) 
             or 'flags.make' in os.path.basename(file)]
    files = [file for file in files if 'esting' not in file and 'emporary' not in file]
    result = {}
    for fname in files:
        result[fname.replace(builddir, '')] = tryread(fname)
    return result


def png_encode64(fname, width=512, data=None, alt=''):
    # en.wikipedia.org/wiki/Data_URI_scheme
    data = data or tryread(fname) or ''
    data_uri = data.encode('base64').replace('\n', '')
    tag = '''<img src="data:image/png;base64,%s" width="%s" %s/>'''
    if alt=="": alt = 'alt="openscad_test_image:' + fname + '" '
    tag %= (data_uri, width, alt)
    return tag


def findlogfile(builddir):
    logpath = os.path.join(builddir, 'Testing', 'Temporary')
    logfilename = os.path.join(logpath, 'LastTest.log.tmp')
    if not os.path.isfile(logfilename):
        logfilename = os.path.join(logpath, 'LastTest.log')
    if not os.path.isfile(logfilename):
        print 'can\'t find and/or open logfile', logfilename
        sys.exit()
    return logfilename

# --- Templating ---
    
class Templates(object):
    html_template = '''<html>
    <head><title>Test run for {sysid}</title>
    {style}
    </head>
    <body>
        <h1>{project_name} test run report</h1>
        <p>
            <b>Sysid</b>:  {sysid}
        </p>
        <p>
            <b>Result summary</b>:  {numpassed} / {numtests} tests passed ({percent}%)
        </p>
        
        <p>
            <b>System info</b>
        </p>
        <pre>{sysinfo}</pre>

        <p>
            <b>Image comparer</b>: {imgcomparer}
        </p>

        <p>
            <b>Tests start time</b>: {startdate}
        </p>
        <p>
            <b>Tests end time</b>: {enddate}
        </p>

        <h2>Image tests</h2>
        {image_tests}

        <h2>Text tests</h2>
        {text_tests}

        <h2>build.make and flags.make</h2>
        {makefiles}
    </body></html>'''

    style = '''
    <style>
        body {
            color: black;
        }

        table {
            border-collapse: collapse;
        }

        table td, th {
            border: 2px solid gray;
        }

        .text-name {
            border: 2px solid black;
            padding: 0.14em;
        }
    </style>'''

    image_template = '''<table>
    <tbody>
    <tr><td colspan="2">{test_name}</td></tr>
    <tr><td> Expected image </td><td> Actual image </td></tr>
    <tr><td> {expected} </td><td> {actual} </td></tr>
    </tbody>
    </table>

    <pre>
{test_log}
    </pre>
    '''

    text_template = '''
    <span class="text-name">{test_name}</span>

    <pre>
{test_log}
    </pre>
    '''

    makefile_template = '''
    <h4>{name}</h4>
    <pre>
        {text}
    </pre>
    '''

    def __init__(self, **defaults):
        self.filled = {}
        self.defaults = defaults

    def fill(self, template, *args, **kwargs):
        kwds = self.defaults.copy()
        kwds.update(kwargs)
        return getattr(self, template).format(*args, **kwds)

    def add(self, template, var, *args, **kwargs):
        self.filled[var] = self.filled.get(var, '') + self.fill(template, *args, **kwargs)
        return self.filled[var]

    def get(self, var):
        return self.filled.get(var, '')


def to_html(project_name, startdate, tests, enddate, sysinfo, sysid, imgcomparer, makefiles):
    passed_tests = [test for test in tests if test.passed]
    failed_tests = [test for test in tests if not test.passed]

    report_tests = failed_tests
    if include_passed:
        report_tests = tests

    percent = '%.0f' % (100.0 * len(passed_tests) / len(tests)) if tests else 'n/a'

    image_test_count = 0
    text_test_count = 0

    templates = Templates()
    for test in report_tests:
        if test.type in ('txt', 'ast', 'csg', 'term', 'echo', 'stl'):
            text_test_count += 1
            templates.add('text_template', 'text_tests',
                          test_name=test.fullname,
                          test_log=cgi.escape(test.fulltestlog))
        elif test.type == 'png':
            image_test_count += 1
            alttxt = 'OpenSCAD test image'

            if not os.path.exists(test.actualfile):
                alttxt = 'image missing for ' + test.fullname
            actual_img = png_encode64(test.actualfile,
                                  data=test.actualfile_data, alt=alttxt)

            if not os.path.exists(test.expectedfile):
                alttxt = 'no img generated for ' + test.fullname
            expected_img = png_encode64(test.expectedfile,
                                    data=test.expectedfile_data, alt=alttxt)

            templates.add('image_template', 'image_tests',
                          test_name=test.fullname,
                          test_log=test.fulltestlog,
                          actual=actual_img,
                          expected=expected_img)
        else:
            raise TypeError('Unknown test type %r' % test.type)

    for mf in sorted(makefiles.keys()):
        mfname = mf.strip().lstrip(os.path.sep)
        text = open(os.path.join(builddir, mfname)).read()
        templates.add('makefile_template', 'makefiles', name=mfname, text=text)

    text_tests = templates.get('text_tests')
    image_tests = templates.get('image_tests')
    makefiles_str = templates.get('makefiles')

    if not include_passed:
        if image_test_count==0:
            image_tests = 'all given tests passed'
        if text_test_count==0:
            text_tests = 'all given tests passed'

    return templates.fill('html_template', style=Templates.style,
                          sysid=sysid, sysinfo=sysinfo,
                          startdate=startdate, enddate=enddate,
                          project_name=project_name,
                          numtests=len(tests),
                          numpassed=len(passed_tests),
                          percent=percent, image_tests=image_tests,
                          text_tests=text_tests, makefiles=makefiles_str,
                          imgcomparer=imgcomparer)

# --- End Templating ---

# --- Web Upload ---

def postify(data):
    return urlencode(data).encode()

def create_page():
    data = {
        'action': 'create',
        'type': 'html'
    }
    try:
        response = urlopen('http://www.dinkypage.com', data=postify(data))
    except:
        return None
    return response.geturl()

def upload_html(page_url, title, html):
    data = {
        'mode': 'editor',
        'title': title,
        'html': html,
        'ajax': '1'
    }
    try:
        response = urlopen(page_url, data=postify(data))
    except URLError, e:
        print 'Upload error: ' + str(e)
        return False
    return 'success' in response.read().decode()

# --- End Web Upload ---

debug_test_pp = False
#debug_test_pp = True
debugfile = None
def debug(x):
    global debugfile
    if debug_test_pp:
        print 'test_pretty_print debug: ' + x
    debugfile.write(x+'\n')

builddir = os.getcwd()
include_passed = False

def main():
    global builddir, debug_test_pp
    global maxretry, dry, include_passed
    project_name = 'OpenSCAD'
    
    if bool(os.getenv("TEST_GENERATE")):
        sys.exit(0)
    
    # --- Command Line Parsing ---
    
    if '--debug' in ' '.join(sys.argv):
        debug_test_pp = True
    maxretry = 10

    if '--include-passed' in sys.argv:
        include_passed = True

    dry = False
    debug('running test_pretty_print')
    if '--dryrun' in sys.argv:
        dry = True

    suffix = ezsearch('--suffix=(.*?) ', ' '.join(sys.argv) + ' ')
    builddir = ezsearch('--builddir=(.*?) ', ' '.join(sys.argv) + ' ')
    if not builddir or not os.path.exists(builddir):
        builddir = os.getcwd()
        print 'warning: couldnt find --builddir, trying to use current dir:',
        print builddir
    debug('build dir set to ' +  builddir)

    upload = False
    if '--upload' in sys.argv:
        upload = True
        debug('will upload test report')

    # Workaround for old cmake's not being able to pass parameters
    # to CTEST_CUSTOM_POST_TEST
    if bool(os.getenv("OPENSCAD_UPLOAD_TESTS")):
        upload = True
    
    # --- End Command Line Parsing ---

    sysinfo, sysid = read_sysinfo(os.path.join(builddir, 'sysinfo.txt'))
    makefiles = load_makefiles(builddir)
    logfilename = findlogfile(builddir)
    testlog = tryread(logfilename)
    debug('found log file: '+logfilename+'\n')
    startdate, tests, enddate, imgcomparer = parselog(testlog)
    if debug_test_pp:
        print 'found sysinfo.txt,',
        print 'found', len(makefiles),'makefiles,',
        print 'found', len(tests),'test results'
        print 'comparer', imgcomparer
    html = to_html(project_name, startdate, tests, enddate, sysinfo, sysid, imgcomparer, makefiles)
    html_basename = sysid + '_report.html'
    html_filename = os.path.join(builddir, 'Testing', 'Temporary', html_basename)
    debug('saving ' + html_filename + ' ' + str(len(html)) + ' bytes')
    trysave(html_filename, html)
    print "report saved:\n", html_filename.replace(os.getcwd()+os.path.sep,'')

    failed_tests = [test for test in tests if not test.passed]
    if upload and failed_tests:
        build = os.getenv("TRAVIS_BUILD_NUMBER")
        if build: filename = 'travis-' + build + '_report.html'
        else: filename = html_basename
        os.system('scp "%s" "%s:%s"' %
                  (html_filename, 'openscad@files.openscad.org', 'www/tests/' + filename) )
        share_url = 'http://files.openscad.org/tests/' + filename;
        print 'html report uploaded:'
        print share_url

#        page_url = create_page()
#        if upload_html(page_url, title='OpenSCAD test results', html=html):
#            share_url = page_url.partition('?')[0]
#            print 'html report uploaded at', share_url
#        else:
#            print 'could not upload html report'

    debug('test_pretty_print complete')

if __name__=='__main__':
    debugfile = open('test_pretty_print.log.txt','w')
    main()
    debugfile.close()
