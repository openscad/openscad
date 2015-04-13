1) primary scad filename:
  Tests:
  * file loads
  * Windows title OK
  * Console load & echo OK
  Files:
  * utf8-æ.scad
  a) Pass on the .exe cmd-line
    1) DOS   
    2) bash  openscad.exe <filename>
  b) Pass on the .com cmd-line
    1) DOS
    2) bash  openscad.com <filename>
  c) Double-click
  d) Drag-and-drop:
     1) on icon
     2) into mainwin
  e) Launch Screen
     1) Open recent
     2) Open
  f) mainwin
     1) Open recent
     2) File->Open

2) include
   a) utf8-include-a.scad
   b) utf8-include-b.scad
3) use
   a) utf8-use-a.scad
   b) utf8-use-b.scad
4) use font
5) import
  a) dxf
  b) stl
  c) off
6) surface
  a) dat
  b) png
7) GUI export
  a) dxf
  b) svg
  c) stl
  d) off
  e) amf
  f) csg
8) cmd-line export
  a) dxf
  b) svg
  c) stl
  d) off
  e) amf
  f) echo
  g) ast
  h) term
  i) csg
9) save:     utf8-æ.scad -> utf8-ø.scad
10) save as: utf8-æ.scad -> utf8-ø.scad
11) echo: utf8-æ.scad -o out.png
  a) openscad.exe in DOS prompt
  b) openscad.exe in bash
  c) openscad.com in DOS prompt
  d) openscad.com in bash


Results:        2014.03         2015.03         master
                XP 8.1          XP 8.1          XP 8.1
1a1             -               a
1a2		-		a		-
1b1             c		a
1b2		c		a		-
1c                 -               a
1d1                -               a
1d2                -               a		-
1e1             N/A                b		-
1e2             N/A                b		-
1f1                -               a		-
1f2                -               a		-
2a						-
2b						d
3a						-
3b						d
4
5a              X               X               d
5b              X               X               -
5c              X               X               -
6a						-
6b						-
7a						-
7b						-
7c						-
7d						-
7e						-
7f						-
8a						-
8b						-
8c						-
8d						-
8e						-
8f						-
8g						-
8h						-
8i						-
9						-
10						-
11a
11b						-
11c
11d						g

Results
-	OK
a	Console issue
b	title wrong, cannot open file
c	cwd must be install dir
d	Cannot open file with ä or ö
e	Cannot open file
f	Bad lexical cast
g	segmentation fault (also happens with non-utf-8 files)

