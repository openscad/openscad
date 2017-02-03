import py
import os.path
from openscad_utils import *


temppath = py.test.ensuretemp('MCAD')

def pytest_generate_tests(metafunc):
    if "modpath" in metafunc.funcargnames:
        for fpath, modnames in collect_test_modules().items():
            basename = os.path.splitext(os.path.split(str(fpath))[1])[0]
            #os.system("cp %s %s/" % (fpath, temppath))
            if "modname" in metafunc.funcargnames:
                for modname in modnames:
                    print modname
                    metafunc.addcall(id=basename+"/"+modname, funcargs=dict(modname=modname, modpath=fpath))
            else:
                metafunc.addcall(id=os.path.split(str(fpath))[1], funcargs=dict(modpath=fpath))


def test_module_compile(modname, modpath):
    tempname = modpath.basename + '-' + modname + '.scad'
    fpath = temppath.join(tempname)
    stlpath = temppath.join(tempname + ".stl")
    f = fpath.open('w')
    code = """
//generated testfile
use <%s>

%s();
""" % (modpath, modname)
    print code
    f.write(code)
    f.flush()
    output = call_openscad(path=fpath, stlpath=stlpath, timeout=15)
    print output
    assert output[0] is 0
    for s in ("warning", "error"):
        assert s not in output[2].strip().lower()
    assert len(stlpath.readlines()) > 2

def test_file_compile(modpath):
    stlpath = temppath.join(modpath.basename + "-test.stl")
    output = call_openscad(path=modpath, stlpath=stlpath)
    print output
    assert output[0] is 0
    for s in ("warning", "error"):
        assert s not in output[2].strip().lower()
    assert len(stlpath.readlines()) == 2


