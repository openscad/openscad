# PythonSCAD Stub File for use in editors like Visual Studio Code
class PyLibFive:
    pass

class PyLibFive:
    def x(self) -> PyLibFive:
        """Return X coorinate
        """
        ...

    def y(self) ->PyLibFive:
        """Return Y coorinate
        """
        ...

    def z(self) ->PyLibFive:
        """Return Z coorinate
        """
        ...

    def sqrt(self, v) ->PyLibFive:
        """Return Z coorinate
        """
        ...

    def square(self, v) ->PyLibFive:
        """ Calculates the square of v
        """
        ...

    def abs(self, v) ->PyLibFive:
        """ Calculates the absolute Value of v
        """
        ...

    def max(self, a, b) ->PyLibFive:
        """ Calculates the maximum from a and b
        """
        ...

    def min(self, a, b) ->PyLibFive:
        """ Calculates the minimum from a and b
        """
        ...

    def sin(self, v) ->PyLibFive:
        """ Calculates the sine of v
        """
        ...

    def cos(self, v) ->PyLibFive:
        """ Calculates the cosine of v
        """
        ...

    def tan(self, v) ->PyLibFive:
        """ Calculates the tangens of v
        """
        ...

    def asin(self, v) ->PyLibFive:
        """ Calculates the arc sine of v
        """
        ...

    def acos(self, v) ->PyLibFive:
        """ Calculates the arc cosine of v
        """
        ...

    def atan(self, v) ->PyLibFive:
        """ Calculates the arc tangent of v
        """
        ...

    def exp(self, v) ->PyLibFive:
        """ Calculates the exponent of v
        """
        ...

    def log(self, v) ->PyLibFive:
        """ Calculates the logarithm of v
        """
        ...

    def pow(self, a, b) ->PyLibFive:
        """ Calculates the power from a over b
        """
        ...

    def comp(self, a, b) ->PyLibFive:
        """ Calculates the libfive comparision of a and b
        """
        ...

    def atan2(self, y, x) ->PyLibFive:
        """ Calculates aractangent from y/x
        """
        ...

    def print(self) ->PyLibFive:
        """ Print the Tree for debugging
        """
        ...


class PyOpenSCAD:
    pass


class PyOpenSCAD:
    edge

def square(dim:float | list[float], center:bool) -> PyOpenSCAD:
    """Create a Square
    """
    ...

def circle(r:float,d:float,angle:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Creates a Circle
    """
    ...

def polygon(points:list[float], paths:list[int], convexity:int) -> PyOpenSCAD:
    """Creates a Polygon 
    points: array of vertices
    paths: how to combine them
    """
    ...

def spline(points:list[float]) -> PyOpenSCAD:
    """Creates a smooth Polygon
    """
    ...

def text(text, size:float, font:str, spacing:float, direction:str, language:str, script:str, halign:float, valign:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Creates a Text
    """
    ...

def textmetrics(text, size:float, font:str, spacing:float, direction:str, language:str, script:str, halign:float, valign:float) -> list[float]:
    """Get textmetrics from a label
    """
    ...

def cube(size:list[float], center:bool) -> PyOpenSCAD:
    """Creates a Cube
    """
    ...

def cylinder(h:float,r1:float, r2:float, center:bool, r:float, d:float, d1:float, d2:float, angle:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Creates a Cylinder
    """
    ...

def sphere(r:float, d:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Creates a Sphere
    """
    ...

def polyhedron(points:list[float], faces:list[int], convexity:int, triangles:list[int]) -> PyOpenSCAD:
    """Creates a Polyhedron
    """
    ...

def frep(exp:PyLibFive, min:list[float], max:list[float], res:int) -> PyOpenSCAD:
    """Create F-Rep (libfive)
    exp : an SDF epression composed from SDF variables and operators, see tutorial
    """
    ...

def ifrep(obj:PyOpenSCAD) -> PyLibFive:
    """Create Inverse F-Rep(experimental)
    """
    ...

def translate(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Move Object by an offset
    """
    ...

def right(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Moves an Object to the right
    """
    ...

def left(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Moves an Object to the left
    """
    ...

def back(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Moves Object backwards
    """
    ...

def front(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Moves Object frontwards
    """
    ...

def up(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Move Object upwards
    """
    ...

def down(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Move Object downwards
    """
    ...

def rotx(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Rotate Object around X Axis
    """
    ...

def roty(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Rotate Object around Y Axis
    """
    ...

def rotz(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Rotate Object around Z Axis
    """
    ...

def rotate(obj:PyOpenSCAD, a:float, v:list[float]) -> PyOpenSCAD:
    """Rotate Object around X, Y and Z Axis
    """
    ...

def scale(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Scale Object by a factor
    """
    ...

def mirror(obj:PyOpenSCAD, v:list[float]) -> PyOpenSCAD:
    """Mirror Object 
    """
    ...

def multmatrix(obj:PyOpenSCAD, m:list[float]) -> PyOpenSCAD:
    """Apply an 4x4 Eigen vector to an Object or handle
    """
    ...

def divmatrix(obj:PyOpenSCAD, m:list[float]) -> PyOpenSCAD:
    """Apply inverse of an 4x4 Eigen vecor to an Object or handle
    """
    ...

def offset(obj:PyOpenSCAD, r:float, delta:float, chamfer:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """2D or 3D Offset of an Object
    """
    ...

def roof(obj:PyOpenSCAD, method:str, convexity:int, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Create Roof from an 2D Shape
    """
    ...

def pull(obj:PyOpenSCAD, src:list[float], dst:list[float]) -> PyOpenSCAD:
    """Pull apart Object, basically between src and dst it creates a prisma with the x-section
    src: anchor
    dst: how much to pull
    """
    ...
def wrap(obj:PyOpenSCAD, r:float) -> PyOpenSCAD:
    """Wraps an object around a virtual cylinder
    src: Object
    r: Radius of the Cylinder
    """
    ...

def color(obj:PyOpenSCAD, c:str, alpha:float, texture:int) -> PyOpenSCAD:
    """Colorize Object
    texture: id from texture command
    """
    ...

def output(obj:PyOpenSCAD) -> None:
    """Output the result to the display
    """
    ...

def show(obj:PyOpenSCAD) -> None:
    """Same as output
    """
    ...

def separate(obj:PyOpenSCAD) -> None:
    """Splits an object into a list of geometricaly distinct objects
    """
    ...

def export(obj:PyOpenSCAD, file:str) -> None:
    """Export the result to a file
    file:  output file name, format is automatically detected from suffix
    when obj is a dictionary, it allows 3mf export to export several paths
    """
    ...

def find_face(obj:PyOpenSCAD,m:lis[float] ) :
    """Find the face of the object which matches the given normal vector most
    m: vector of the normal
    """
    ...

def sitonto(obj:PyOpenSCAD,x:lis[float],y:lis[float]],z:lis[float] ):
    """
    Translates an object into a new coordinate system with the new x,y,z vecrtor given
    x: new X vector
    y: new Y vector
    z: new Z vector
    """
    ...

def linear_extrude(obj:PyOpenSCAD, height:float, v:list[float], layer:str, convexity:int, origin:list[float], scale:float, center:bool, slices:int, segments:int, twist:float, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Linear_extrude an 2D Object
    v: direction of extrusion
    """
    ...

def rotate_extrude(obj:PyOpenSCAD, layer, convexity:int, scale:float, twist:float, origin:list[float], offset:list[float], v:list[float], method:str, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Rotate_extrude an 2D Object
    v: direction of extrusion
    """
    ...

def path_extrude(obj:PyOpenSCAD, path:list[float], xdir:list[float], convexity:int, origin:list[float], scale:float, twist:float, closed:bool, fn:int, fa:float, fs:float) -> PyOpenSCAD:
    """Path_extrude an 2D Object
    xdir: intial vector of x axis
    """
    ...


def union(obj:PyOpenSCAD, r:float, fn:int) -> PyOpenSCAD:
    """Union several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def difference(obj:PyOpenSCAD, r:float, fn:int) -> PyOpenSCAD:
    """Difference several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def intersection(obj:PyOpenSCAD, r:float, fn:int) -> PyOpenSCAD:
    """Intersection several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def hull(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Hull several Objects
    """
    ...

def minkowski(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Minkowski sum several Objects
    """
    ...

def fill(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Fill Objects(remove holes)
    """
    ...

def resize(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Resize an Object
    """
    ...


def concat(obj:PyOpenSCAD, r:float, fn:int) -> PyOpenSCAD:
    """Concatenate the Triangles of several objects together without any CSG
    """
    ...

def highlight(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Highlights Object
    """
    ...

def background(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Puts Object into background
    """
    ...

def only(obj:PyOpenSCAD) -> PyOpenSCAD:
    """Shows only this object
    """
    ...

def projection(obj:PyOpenSCAD, cut:bool, convexity:int) -> PyOpenSCAD:
    """Crated 2D Projection from a 3D Object
    """
    ...

def surface(file, center:bool, convexity:int, invert:bool) -> PyOpenSCAD:
    """Create a Surface Object
    """
    ...

def texture(file, uv:float):
    """Specify a Texture (JPEG) to be used with color
    file: path to a jpg file
    uv: size of the mapped texture square in 3D space
    """
    ...

def mesh(obj:PyOpenSCAD):
    """Retrieves Mesh Data from a 2D or 3D object
    """
    ...

def faces(obj:PyOpenSCAD):
    """returns a list of all the faces of the object
    """
    ...

def edges(obj:PyOpenSCAD):
    """returns a list of all the edges of a face
    """
    ...

def oversample(obj:PyOpenSCAD,n:int, round:bool) -> PyOpenSCAD:
    """Create artificial intermediate points into straight lines
    n: factor  of the oversampling
    bool: whether to round the oversampling
    """
    ...

def debug(obj:PyOpenSCAD, faces:vector[ind]) -> None:
    """Turns listed faces red in the given object
    """
    ...

def fillet(obj:PyOpenSCAD, r:float, sel:PyOpenSCAD, fn:int) -> PyOpenSCAD:
    """Create nice roundings for sharp edges
    r: radius of the fillet
    sel: Object which overlaps the "selected" edges
    fn: number of points for fillet x-section
    """
    ...

def render(obj:PyOpenSCAD,convexity:int) -> PyOpenSCAD:
    """Renders Object even in preview mode
    """
    ...

def osimport(file:str, layer:str, convexity:int, origin:list[float], scale:float, width:float, height:float, filename:str, center:bool, dpi:float, id:int) -> PyOpenSCAD:
    """Imports Object from disc
    """
    ...

def nimport(url:str) -> PyOpenSCAD:
    """Same as python inport , but with an URL instead
    """
    ...

def osuse(path:str) -> PyOpenSCAD:
    """ OpenSCAD/use a Library in PythonSCAD
    """
    ...

def osinclude(path:str) -> PyOpenSCAD:
    """ OpenSCAD/include a Library in PythonSCAD
    """
    ...

def version() -> list[float]:
    """Outputs pythonscad Version
    """
    ...

def version_num() -> list[float]:
    """Outputs pythoncad Version
    """
    ...

def add_parameter(name:str, default) -> None:
    """Adds Parameter for use in Customizer
    """
    ...

def scad(code:str) -> PyOpenSCAD:
    """Evaluate Code in SCAD syntax
    """
    ...


def align(obj:PyOpenSCAD, refmat:list[float], objmat:list[float]) -> PyOpenSCAD:
    """Aligns an Object to another
    refmat: handle matrix of the reference object
    objmat: handle matrix of the new object
    """
    ...

def add_menuitem(menuname:str, itemname:str, callback: str) -> 
    """Add custom function to the PythonSCAD Banner menu
    """
    ...

def model() -> PyOpenSCAD:
    """Returns the parsed Module shown
    This can be used process the model from within custom menu items
    """
    ...

def modelpath() -> str:
    """Returns the path of the python file on the host computer
    """
    ...

def marked(number:double) -> PyOpenSCAD:
    """Returns a "marked" number 
    Marked numbers are just numbers, but they are enabled for interactive model dragging
    """
    ...






