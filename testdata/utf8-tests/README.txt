1) primary scad filename:
  Tests:
  * file loads
  * Windows title OK
  * Console OK
  a) Pass on the .exe cmd-line
    1) DOS
    2) bash
  b) Pass on the .com cmd-line
    1) DOS
    2) bash
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
3) use
4) font
5) import
  - All file formats
6) surface
7) export
  a) GUI
  b) cmd-line
8) save
9) save as
10) echo


Results:        2014.03         2015.03         master
                XP 8.1          XP 8.1          XP 8.1
1a1             o               X Console issue
1a2 = 1a1
1b1             X cwd must be install dir
                                X Console issue
1b2 = 1b1
1c                 o               X Console issue
1d1                o               X Console issue
1d2                o               X Console issue
1e1             N/A                X title wrong, cannot open file  
1e2             N/A                X title wrong, cannot open file  
1f1                o               X Console issue
1f2                o               X Console issue
2
3
4
5
6
7a
7b
8
9
10
