import sys, traceback
from openscad import *
# Checking the python-openscad for the worst things a user could potentionally do

def catch_error(code):
    try:
        code()
    except Exception as ex:
        exc_type, exc_obj, exc_tb = sys.exc_info()
        tb_walk = traceback.walk_tb(exc_tb)
        tb_summary = traceback.StackSummary.extract(tb_walk, capture_locals=True)
        print(ex, "in line", tb_summary[1].lineno )    

# Test python_vectorval
catch_error(lambda : cube( "eat that" ))        
catch_error(lambda : cube( sys.stdout ))        
catch_error(lambda : cube( undefined ))        
catch_error(lambda : cube( {} ))        
catch_error(lambda : cube( () ))        
catch_error(lambda : cube( ["a"] ))       
catch_error(lambda : cube( [1,"a"] ))       
catch_error(lambda : cube( [1,2,"a"] ))       

catch_error(lambda : unknownfunc() )        

# Test python_cube
catch_error(lambda : cube([-1,1,1]) )        
catch_error(lambda : cube([1,-1,1]) )        
catch_error(lambda : cube([1,1,-1]) )        
catch_error(lambda : cube(center=5) )        
catch_error(lambda : cube(parx=4) )        

# Test python_sphere
catch_error(lambda : sphere(r=1,d=3) )        
catch_error(lambda : sphere(r=-2) )        
catch_error(lambda : sphere(d=0) )
catch_error(lambda : sphere(r=2,d=5) )
catch_error(lambda : sphere(parx=4) )        

# Test python_cylinder
catch_error(lambda : cylinder(r="Radius",h=2) )        
catch_error(lambda : cylinder(d="Diameter",h=2) )        
catch_error(lambda : cylinder(r=0,h=2) )        
catch_error(lambda : cylinder(d=-1,h=2) )        
catch_error(lambda : cylinder(3,h=-5) )        
catch_error(lambda : cylinder(parx=4) )        

# Test python_polyhedron
catch_error(lambda : polyhedron(points="points", faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=True, faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=17, faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=[], faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=[1], faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=[[1,2]], faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(points=[[1,2,4,6]], faces=[[1,2,3]]) )        
catch_error(lambda : polyhedron(faces=True, points=[[1,2,3]]) )        
catch_error(lambda : polyhedron(faces=17, points=[[1,2,3]]) )        
catch_error(lambda : polyhedron(faces=[], points=[[1,2,3]]) )        
catch_error(lambda : polyhedron(faces=[0], points=[[1,2,3]]) )        
catch_error(lambda : polyhedron(faces=[[0,0]], points=[[1,2,3]]) )        
catch_error(lambda : polyhedron(parx=4) )        

# Test python_square
catch_error(lambda : square("Hallo") )        
catch_error(lambda : square(False) )        
catch_error(lambda : square([1,2,3]) )        
catch_error(lambda : square([1,2],center="hallo") )        
catch_error(lambda : square(parx=4) )        

# Test python_circle
catch_error(lambda : circle("Hallo") )        
catch_error(lambda : circle([1,2]) )        
catch_error(lambda : circle(2,center=True) )        
catch_error(lambda : circle(r=1,d=2) )        
catch_error(lambda : circle(r=-1) )        
catch_error(lambda : circle(d=0) )        
catch_error(lambda : circle(parx=4) )        

# Test python_polygon
catch_error(lambda : polygon(points=True,paths=[1,2,3]))
catch_error(lambda : polygon(points=17,paths=[1,2,3]))
catch_error(lambda : polygon(points=[],paths=[1,2,3]))
catch_error(lambda : polygon(points=[1],paths=[1,2,3]))
catch_error(lambda : polygon(points=[[1,2]],paths=[1,2,3]))
catch_error(lambda : polygon(points=[[1,2,4,6]],paths=[1,2,3]))
catch_error(lambda : polygon(paths=True, points=[[1,2,3]]) )        
catch_error(lambda : polygon(paths=17, points=[[1,2,3]]) )        
catch_error(lambda : polygon(paths=[], points=[[1,2,3]]) )        
catch_error(lambda : polygon(paths=[0], points=[[1,2,3]]) )        
catch_error(lambda : polygon(paths=[[0,0]], points=[[1,2,3]]) )        
catch_error(lambda : polygon(parx=4) ) 

# Test python_scale
catch_error(lambda : scale("hello") )        
catch_error(lambda : scale([1,"string"],1) )        
catch_error(lambda : scale(cube(), "hello") )        
catch_error(lambda : scale(cube(), []) )        
catch_error(lambda : scale(cube(), [1,2,3,4]) )        
catch_error(lambda : scale(cube(), parx=4) )        

# Test python_rotate
catch_error(lambda : rotate("hello") )        
catch_error(lambda : rotate([1,"string"],1) )        
catch_error(lambda : rotate(cube(), "hello") )        
catch_error(lambda : rotate(cube(), [1,2,3,4]) )        
catch_error(lambda : rotate(cube(), parx=4) )        

# Test python_mirror
catch_error(lambda : mirror("hello") )        
catch_error(lambda : mirror([1,"string"],1) )        
catch_error(lambda : mirror(cube(), "hello") )        
catch_error(lambda : mirror(cube(), [1,2,3,4]) )        
catch_error(lambda : mirror(cube(), parx=4) )        
catch_error(lambda : mirror(parx=4) )        

# Test python_translate
catch_error(lambda : translate("hello") )        
catch_error(lambda : translate([1,"string"],1) )        
catch_error(lambda : translate(cube(), "hello") )        
catch_error(lambda : translate(cube(), [1,2,3,4]) )        
catch_error(lambda : translate(cube(), parx=4) )        
catch_error(lambda : translate(parx=4) )        

# Test python_multmatrix
catch_error(lambda : multmatrix(cube(),True) )
catch_error(lambda : multmatrix(cube(),[]) )
catch_error(lambda : multmatrix(cube(),[1]) )
catch_error(lambda : multmatrix(cube(),[[1,2]]) )
catch_error(lambda : multmatrix(cube(),[[0,1,2,3],[4,5,6,7],[8,9,10,11],[12,13,14,15]]) )
catch_error(lambda : multmatrix(parx=4) )        

# Test python_output
catch_error(lambda : output() )        
catch_error(lambda : output("shape") )        
catch_error(lambda : output(undefined) )        
catch_error(lambda : output(parx=4) )        

# Test python_color
catch_error(lambda : color(cube(),4) )        
catch_error(lambda : color(cube(),[0.5,0.5]) )        
catch_error(lambda : color(cube(),False) )        
catch_error(lambda : color(cube(),"green") )        
catch_error(lambda : color(parx=4) )        

# Test python_rotate_extrude
catch_error(lambda : rotate_extrude(sphere(),angle="angle") )        
catch_error(lambda : rotate_extrude(sphere(),angle=[1,2]) )        
catch_error(lambda : rotate_extrude(parx=4) )        

# Test python_linear_extrude
catch_error(lambda : linear_extrude(sphere(),h="height") )        
catch_error(lambda : linear_extrude(sphere(),h=[1,2]) )        
catch_error(lambda : linear_extrude(parx=4) )        

# Test python_csg_sub
catch_error(lambda : difference("obj1","obj2") )        
catch_error(lambda : intersection([cube(),sphere()]) )        
catch_error(lambda : union(3) )        

# Test python_adv_sub
catch_error(lambda : hull("obj1","obj2") )        
catch_error(lambda : fill(cube(),sphere()) )        

# Test python_minkowski
catch_error(lambda : minkowski([cube(),sphere()]) )        
catch_error(lambda : minkowski(parx=4) )        

# Test python_resize
catch_error(lambda : resize("obj1","obj2") )        
catch_error(lambda : resize(cube(),sphere()) )        
catch_error(lambda : resize(cube(),"value") )        
catch_error(lambda : resize(cube(),[1,2,3]) )        

# Test python_roof
catch_error(lambda : roof("shape") )        
catch_error(lambda : roof(3) )        
catch_error(lambda : roof(square(2)) )        
catch_error(lambda : roof(parx=4) )        

# Test python_render
catch_error(lambda : render("shape") )        
catch_error(lambda : render(3) )        
catch_error(lambda : render(cube(2)) )        
catch_error(lambda : render(parx=4) )        

# Test python_surface
catch_error(lambda : surface(3) )        
catch_error(lambda : surface("filename") )        
catch_error(lambda : surface(parx=4) )        

# Test python_offset
catch_error(lambda : offset("shape",3.5) )        
catch_error(lambda : offset(3,3.5) )        
catch_error(lambda : offset(square(2),3.5) )        
catch_error(lambda : offset(parx=4) )        

# Test python_projection
catch_error(lambda : projection("shape") )        
catch_error(lambda : projection(3) )        
catch_error(lambda : projection(cube(2)) )        
catch_error(lambda : projection(parx=4) )        

# Test python_import
catch_error(lambda : osimport(3) )        
catch_error(lambda : osimport("filename") )        
catch_error(lambda : osimport(parx=4) )        


