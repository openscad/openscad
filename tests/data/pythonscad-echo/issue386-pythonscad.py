"""Test for pythonscad issue #386: $-variables are not handled correctly when using osuse()"""
from openscad import *
result = osuse("../scad/issues/issue386-pythonscad-included.scad").test()
print(f"test(): {result}")
