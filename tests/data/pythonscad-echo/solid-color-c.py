"""Regression for solid.c: root ColorNode RGBA or None."""
from openscad import *


def prgba(c):
    if c is None:
        print(None)
    else:
        r, g, b, a = c
        print(round(r, 6), round(g, 6), round(b, 6), round(a, 6))


prgba(cube(10).c)
prgba(cube(10).color("Red").c)
prgba(cube(1).color([0.25, 0.5, 0.75], alpha=0.5).c)
u = cube(1).color("Red") | sphere(d=2).color("Blue")
prgba(u.c)
prgba(u.children()[0].c)
prgba(u.children()[1].c)
prgba(cube(10)["c"])
prgba(cube(10).color("Red")["c"])
