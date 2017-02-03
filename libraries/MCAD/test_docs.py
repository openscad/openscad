import py
import os.path

dirpath = py.path.local("./")

def pytest_generate_tests(metafunc):
    if "filename" in metafunc.funcargnames:
        for fpath in dirpath.visit('*.scad'):
            metafunc.addcall(id=fpath.basename, funcargs=dict(filename=fpath.basename))
        for fpath in dirpath.visit('*.py'):
            name = fpath.basename
            if not (name.startswith('test_') or name.startswith('_')):
                metafunc.addcall(id=fpath.basename, funcargs=dict(filename=fpath.basename))

def test_README(filename):
    README = dirpath.join('README').read()

    assert filename in README
