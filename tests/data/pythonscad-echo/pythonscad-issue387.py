"""Test for pythonscad issue #387: Filename is not included in warnings happening in included files"""
from openscad import *
osuse("../scad/issues/pythonscad-issue387-included.scad").test()
