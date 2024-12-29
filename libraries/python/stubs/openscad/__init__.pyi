from __future__ import annotations

""" PythonSCAD Stub File for use in editors like Visual Studio Code """

from typing import Union, List, Optional, overload, Self
from enum import Enum

PyOpenSCADType = Union[PyOpenSCAD, List["PyOpenSCADType"]]

# TODO: grab these from PythonSCAD
FN = 32
FA = 12
FS = 2


class PyLibFive:
    def __init__(self, x: float, y: float, z: float):
        ...

    def x(self) -> Self:
        """Return X coordinate"""
        ...

    def y(self) -> Self:
        """Return Y coordinate"""
        ...

    def z(self) -> Self:
        """Return Z coordinate"""
        ...

    def sqrt(self, v) -> Self:
        """Return Z coordinate"""
        ...

    def square(self, v) -> Self:
        """Calculates the square of v"""
        ...

    def abs(self, v) -> Self:
        """Calculates the absolute Value of v"""
        ...

    def max(self, a, b) -> Self:
        """Calculates the maximum from a and b"""
        ...

    def min(self, a, b) -> Self:
        """Calculates the minimum from a and b"""
        ...

    def sin(self, v) -> Self:
        """Calculates the sine of v"""
        ...

    def cos(self, v) -> Self:
        """Calculates the cosine of v"""
        ...

    def tan(self, v) -> Self:
        """Calculates the tangents of v"""
        ...

    def asin(self, v) -> Self:
        """Calculates the arc sine of v"""
        ...

    def acos(self, v) -> Self:
        """Calculates the arc cosine of v"""
        ...

    def atan(self, v) -> Self:
        """Calculates the arc tangent of v"""
        ...

    def exp(self, v) -> Self:
        """Calculates the exponent of v"""
        ...

    def log(self, v) -> Self:
        """Calculates the logarithm of v"""
        ...

    def pow(self, a, b) -> Self:
        """Calculates the power from a over b"""
        ...

    def comp(self, a, b) -> Self:
        """Calculates the libfive comparision of a and b"""
        ...

    def atan2(self, y, x) -> Self:
        """Calculates aractangent from y/x"""
        ...

    def print(self) -> Self:
        """Print the Tree for debugging"""
        ...

class PyOpenSCAD:
    def translate(self, v: List[float]) -> Self:
        """Move Object by an offset"""
        ...

    def right(self, v: List[float]) -> Self:
        """Moves an Object to the right"""
        ...

    def left(self, v: List[float]) -> Self:
        """Moves an Object to the left"""
        ...

    def back(self, v: List[float]) -> Self:
        """Moves Object backwards"""
        ...

    def front(self, v: List[float]) -> Self:
        """Moves Object frontwards"""
        ...

    def up(self, v: List[float]) -> Self:
        """Move Object upwards"""
        ...

    def down(self, v: List[float]) -> Self:
        """Move Object downwards"""
        ...

    def rotx(self, v: List[float]) -> Self:
        """Rotate Object around X Axis"""
        ...

    def roty(self, v: List[float]) -> Self:
        """Rotate Object around Y Axis"""
        ...

    def rotz(self, v: List[float]) -> Self:
        """Rotate Object around Z Axis"""
        ...

    def rotate(
        self, 
        a: Union[float, List[float], None] = None,
        v: Optional[List[float]] = None
    ) -> Self:
        """Rotate Object around axis or axes
        Two ways to call:
        1. rotate([x, y, z]) - rotates x° around X axis, y° around Y axis, z° around Z axis
        2. rotate(a, [x, y, z]) - rotates a° around vector [x,y,z]
        """
        ...

    def scale(self, v: List[float]) -> Self:
        """Scale Object by a factor"""
        ...

    def mirror(self, v: List[float]) -> Self:
        """Mirror Object"""
        ...

    def multmatrix(self, m: List[float]) -> Self:
        """Apply an 4x4 Eigen vector to an Object or handle"""
        ...

    def divmatrix(self, m: List[float]) -> Self:
        """Apply inverse of an 4x4 Eigen vector to an Object or handle"""
        ...

    def offset(
        self,
        r: float,
        delta: float = 0,
        chamfer: bool = False,
        fn: int = FN,
        fa: float = FA,
        fs: float = FS,
    ) -> Self:
        """2D or 3D Offset of an Object
        r: radius for round corners (can't be used with delta)
        delta: distance to offset (can't be used with r)
        chamfer: when true, creates chamfered edges
        """
        ...

    class RoofMethod(Enum):
        TOP = "top"
        LOFT = "loft"
    def roof(
        self,
        method: str = RoofMethod.TOP.name,
        convexity: int = 2,
        fn: int = FN,
        fa: float = FA,
        fs: float = FS,
    ) -> Self:
        """Create Roof from an 2D Shape"""
        ...

    def pull(
        self, 
        src: Self, 
        dst: Self
    ) -> Self:
        """Pull apart Object, basically between src and dst it creates a prisma with the x-section
        src: anchor
        dst: how much to pull
        """
        ...

    def color(
        self, 
        c: Optional[str] = None, 
        alpha: Optional[float] = None, 
        texture: Optional[int] = None
    ) -> Self:
        """Colorize Object
        texture: id from texture command
        """
        ...

    def output(self) -> None:
        """Output the result to the display"""
        ...

    def show(self) -> None:
        """Same as output"""
        ...

    def export(self, file: str) -> None:
        """Export the result to a file
        file: output file name, format is automatically detected from suffix
        when obj is a dictionary, it allows 3mf export to export several paths
        """
        ...

    def linear_extrude(
        self,
        height: Optional[float] = None,
        center: bool = False,
        convexity: int = 2,
        twist: float = 0,
        slices: int = 10,
        scale: Union[float, List[float]] = 1,
        fn: int = FN,
        fa: float = FA,
        fs: float = FS,
    ) -> Self:
        """Linear_extrude an 2D Object into 3D
        height: height of extrusion
        center: if true, centers the geometry vertically
        convexity: parameter for preview
        twist: degrees of twist over the entire height
        slices: number of intermediate points
        scale: relative scale at the top
        """
        ...

    def rotate_extrude(
        self,
        angle: float = 360,
        convexity: int = 2,
        fn: int = FN,
        fa: float = FA,
        fs: float = FS,
    ) -> Self:
        """Rotate_extrude an 2D Object around Z axis
        angle: degrees to sweep (default: 360)
        convexity: parameter for preview
        """
        ...

    def path_extrude(
        self,
        path: List[float],
        xdir: List[float],
        convexity: int = 2,
        origin: List[float] = [0, 0, 0],
        scale: float = 1,
        twist: float = 0,
        closed: bool = False,
        fn: int = FN,
        fa: float = FA,
        fs: float = FS,
    ) -> Self:
        """Path_extrude an 2D Object
        xdir: initial vector of x axis
        """
        ...

    def resize(
        self,
        newsize: List[float],
        auto: List[bool],
    ) -> Self:
        """Resize an Object"""
        ...

    def highlight(self) -> Self:
        """Highlights Object"""
        ...

    def background(self) -> Self:
        """Puts Object into background"""
        ...

    def only(self) -> Self:
        """Shows only this object"""
        ...

    def projection(
        self, 
        cut: Optional[bool] = None, 
        convexity: Optional[int] = None
    ) -> Self:
        """Create 2D Projection from a 3D Object"""
        ...

    def mesh(self):
        """Retrieves Mesh Data from a 2D or 3D object"""
        ...

    def oversample(
        self, 
        n: Optional[int] = None, 
        round: Optional[bool] = None
    ) -> Self:
        """Create artificial intermediate points into straight lines
        n: factor of the oversampling
        round: whether to round the oversampling
        """
        ...

    def fillet(
        self, 
        r: Optional[float] = None, 
        sel: Optional[Self] = None, 
        fn: Optional[int] = None
    ) -> Self:
        """Create nice roundings for sharp edges
        r: radius of the fillet
        sel: Object which overlaps the "selected" edges
        fn: number of points for fillet x-section
        """
        ...

    def align(
        self, 
        refmat: Optional[List[float]] = None, 
        objmat: Optional[List[float]] = None
    ) -> Self:
        """Aligns an Object to another
        refmat: handle matrix of the reference object
        objmat: handle matrix of the new object
        """
        ...

def square(dim: float | List[float], center: bool) -> PyOpenSCAD:
    """Create a Square"""
    ...

@overload
def circle(
    r: float,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None
) -> PyOpenSCAD:
    """Creates a Circle using radius"""
    ...

@overload
def circle(
    *,
    d: float,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None
) -> PyOpenSCAD:
    """Creates a Circle using diameter"""
    ...

def polygon(points: List[List[float]], paths: List[List[int]], convexity: int = 3) -> PyOpenSCAD:
    """Creates a Polygon
    points: array of vertices
    paths: how to combine them
    """
    ...

def text(
    text,
    size: float,
    font: str,
    spacing: float,
    direction: str,
    language: str,
    script: str,
    halign: float,
    valign: float,
    fn: int,
    fa: float,
    fs: float,
) -> PyOpenSCAD:
    """Creates a Text"""
    ...

def textmetrics(
    text,
    size: float,
    font: str,
    spacing: float,
    direction: str,
    language: str,
    script: str,
    halign: float,
    valign: float,
) -> List[float]:
    """Get textmetrics from a label"""
    ...

def cube(size: List[float], center: bool) -> PyOpenSCAD:
    """Creates a Cube"""
    ...

@overload
def cylinder(
    h: float,
    r: float,
    center: bool = False,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Cylinder with uniform radius"""
    ...

@overload
def cylinder(
    h: float,
    r1: float,
    r2: float,
    center: bool = False,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Cylinder with different top and bottom radii"""
    ...

@overload
def cylinder(
    h: float,
    *,
    d: float,
    center: bool = False,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Cylinder with uniform diameter"""
    ...

@overload
def cylinder(
    h: float,
    *,
    d1: float,
    d2: float,
    center: bool = False,
    angle: Optional[float] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Cylinder with different top and bottom diameters"""
    ...

@overload
def sphere(
    r: float,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Sphere using radius"""
    ...

@overload
def sphere(
    *,
    d: float,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Creates a Sphere using diameter"""
    ...

def polyhedron(
    points: List[float], faces: List[int], convexity: int, triangles: List[int]
) -> PyOpenSCAD:
    """Creates a Polyhedron"""
    ...

def frep(exp: PyLibFive, min: List[float], max: List[float], res: int) -> PyOpenSCAD:
    """Create F-Rep (libfive)
    exp : an SDF epression composed from SDF variables and operators, see tutorial
    """
    ...

def ifrep(obj: PyOpenSCAD) -> PyLibFive:
    """Create Inverse F-Rep(experimental)"""
    ...

def translate(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Move Object by an offset"""
    ...

def right(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Moves an Object to the right"""
    ...

def left(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Moves an Object to the left"""
    ...

def back(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Moves Object backwards"""
    ...

def front(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Moves Object frontwards"""
    ...

def up(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Move Object upwards"""
    ...

def down(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Move Object downwards"""
    ...

def rotx(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Rotate Object around X Axis"""
    ...

def roty(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Rotate Object around Y Axis"""
    ...

def rotz(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Rotate Object around Z Axis"""
    ...

def rotate(obj: PyOpenSCADType, a: float, v: List[float]) -> PyOpenSCAD:
    """Rotate Object around X, Y and Z Axis"""
    ...

def scale(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Scale Object by a factor"""
    ...

def mirror(obj: PyOpenSCADType, v: List[float]) -> PyOpenSCAD:
    """Mirror Object"""
    ...

def multmatrix(obj: PyOpenSCADType, m: List[float]) -> PyOpenSCAD:
    """Apply an 4x4 Eigen vector to an Object or handle"""
    ...

def divmatrix(obj: PyOpenSCADType, m: List[float]) -> PyOpenSCAD:
    """Apply inverse of an 4x4 Eigen vecor to an Object or handle"""
    ...

def offset(
    obj: PyOpenSCADType,
    r: Optional[float] = None,
    delta: Optional[float] = None,
    chamfer: Optional[bool] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """2D or 3D Offset of an Object
    r: radius for round corners (can't be used with delta)
    delta: distance to offset (can't be used with r)
    chamfer: when true, creates chamfered edges
    """
    ...

def roof(
    obj: PyOpenSCADType, method: str, convexity: int, fn: int, fa: float, fs: float
) -> PyOpenSCAD:
    """Create Roof from an 2D Shape"""
    ...

def pull(obj: PyOpenSCADType, src: List[float], dst: List[float]) -> PyOpenSCAD:
    """Pull apart Object, basically between src and dst it creates a prisma with the x-section
    src: anchor
    dst: how much to pull
    """
    ...

def color(obj: PyOpenSCADType, c: str, alpha: float, texture: int) -> PyOpenSCAD:
    """Colorize Object
    texture: id from texture command
    """
    ...

def show(obj: PyOpenSCADType) -> None:
    """Same as output"""
    ...

def export(obj: PyOpenSCADType, file: str) -> None:
    """Export the result to a file
    file:  output file name, format is automatically detected from suffix
    when obj is a dictionary, it allows 3mf export to export several paths
    """
    ...

def linear_extrude(
    obj: PyOpenSCADType,
    height: float,
    center: bool = False,
    convexity: int = 2,
    twist: float = 0,
    slices: int = 10,
    scale: Union[float, List[float]] = 1,
    fn: int = FN,
    fa: float = FA,
    fs: float = FS,
) -> PyOpenSCAD:
    """Linear_extrude an 2D Object into 3D
    height: height of extrusion
    center: if true, centers the geometry vertically
    convexity: parameter for preview
    twist: degrees of twist over the entire height
    slices: number of intermediate points
    scale: relative scale at the top
    """
    ...

def rotate_extrude(
    obj: PyOpenSCADType,
    angle: Optional[float] = 360,
    convexity: Optional[int] = None,
    fn: Optional[int] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Rotate_extrude an 2D Object around Z axis
    angle: degrees to sweep (default: 360)
    convexity: parameter for preview
    """
    ...

def path_extrude(
    obj: PyOpenSCADType,
    path: List[float],
    xdir: List[float],
    convexity: int,
    origin: List[float],
    scale: float,
    twist: float,
    closed: bool,
    fn: int,
    fa: float,
    fs: float,
) -> PyOpenSCAD:
    """Path_extrude an 2D Object
    xdir: intial vector of x axis
    """
    ...

def union(
    *objects: PyOpenSCADType,
    r: Optional[float] = None,
    fn: Optional[int] = None
) -> PyOpenSCAD:
    """Union several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def difference(
    *objects: PyOpenSCADType,
    r: Optional[float] = None,
    fn: Optional[int] = None
) -> PyOpenSCAD:
    """Difference several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def intersection(
    *objects: PyOpenSCADType,
    r: Optional[float] = None,
    fn: Optional[int] = None
) -> PyOpenSCAD:
    """Intersection several Objects
    r: radius of fillet
    fn: number of points for fillet x-section
    """
    ...

def hull(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Hull several Objects"""
    ...

def minkowski(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Minkowski sum several Objects"""
    ...

def fill(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Fill Objects(remove holes)"""
    ...

def resize(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Resize an Object"""
    ...

def highlight(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Highlights Object"""
    ...

def background(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Puts Object into background"""
    ...

def only(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Shows only this object"""
    ...

def projection(obj: PyOpenSCADType, cut: bool, convexity: int) -> PyOpenSCAD:
    """Crated 2D Projection from a 3D Object"""
    ...

def surface(file, center: bool, convexity: int, invert: bool) -> PyOpenSCAD:
    """Create a Surface Object"""
    ...

def texture(file, uv: float):
    """Specify a Texture (JPEG) to be used with color
    file: path to a jpg file
    uv: size of the mapped texture square in 3D space
    """
    ...

def mesh(obj: PyOpenSCAD):
    """Retrieves Mesh Data from a 2D or 3D object"""
    ...

def oversample(obj: PyOpenSCADType, n: int, round: bool) -> PyOpenSCAD:
    """Create artificial intermediate points into straight lines
    n: factor  of the oversampling
    bool: whether to round the oversampling
    """
    ...

def fillet(obj: PyOpenSCADType, r: float, sel: PyOpenSCADType, fn: int) -> PyOpenSCAD:
    """Create nice roundings for sharp edges
    r: radius of the fillet
    sel: Object which overlaps the "selected" edges
    fn: number of points for fillet x-section
    """
    ...

def group(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Groups several Objects"""
    ...

def render(obj: PyOpenSCADType, convexity: int) -> PyOpenSCAD:
    """Renders Object even in preview mode"""
    ...

def osimport(
    file: str,
    layer: str,
    convexity: int,
    origin: List[float],
    scale: float,
    width: float,
    height: float,
    filename: str,
    center: bool,
    dpi: float,
    id: int,
) -> PyOpenSCAD:
    """Imports Object from disc"""
    ...

def version() -> List[float]:
    """Outputs pythonscad Version"""
    ...

def version_num() -> List[float]:
    """Outputs pythoncad Version"""
    ...

def add_parameter(name: str, default) -> None:
    """Adds Parameter for use in Customizer"""
    ...

def scad(code: str) -> PyOpenSCAD:
    """Evaluate Code in SCAD syntax"""
    ...

def align(obj: PyOpenSCADType, refmat: List[float], objmat: List[float]) -> PyOpenSCAD:
    """Aligns an Object to another
    refmat: handle matrix of the reference object
    objmat: handle matrix of the new object
    """
    ...

def output(obj: PyOpenSCADType) -> None:
    """Output the result to the display"""
    ...
