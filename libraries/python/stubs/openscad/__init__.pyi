""" PythonSCAD Stub File for use in editors like Visual Studio Code """
from typing import Union, Optional, overload

PyOpenSCADs = Union["PyOpenSCAD", list["PyOpenSCAD"]]
"""Type for functions that accept either a single OpenSCAD object or a list of objects."""

Color = Union[str, list[float]]
"""Color specification as either a color name string (e.g., "red") or RGB/RGBA values as [r, g, b] or [r, g, b, a]."""

Vector2 = list[float]
"""2D vector represented as [x, y] list."""

Vector3 = list[float]
"""3D vector represented as [x, y, z] list."""

Matrix4x4 = list[list[float]]
"""4x4 transformation matrix as a list of 4 lists of 4 floats."""



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
    """Base class for OpenSCAD objects."""

    origin: Matrix4x4
    """4x4 transformation matrix representing the object's origin. 
    Initialized as identity matrix."""

    def translate(self, v: Vector3) -> "PyOpenSCAD":
        """Translate this object.

        Args:
            v: Translation vector [x, y, z].

        Returns:
            The transformed object. The original object is unaffected.
        """
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
        self, a: Union[float, Vector3], v: Optional[Vector3] = None
    ) -> "PyOpenSCAD":
        """Rotate this object.

        Args:
            a: Rotation angle (degrees) or rotation vector [x, y, z].
            v: Optional rotation axis vector when a is a scalar angle.

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def scale(self, v: Union[float, Vector3]) -> "PyOpenSCAD":
        """Scale this object.

        Args:
            v: Scale factor (uniform) or scale vector [x, y, z].

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def mirror(self, v: Vector3) -> "PyOpenSCAD":
        """Mirror this object.

        Args:
            v: Mirror plane normal vector [x, y, z].

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def multmatrix(self, m: Matrix4x4) -> "PyOpenSCAD":
        """Apply matrix transformation to this object.

        Args:
            m: 4x4 transformation matrix as a list of 4 lists of 4 floats.

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def divmatrix(self, m: Matrix4x4) -> "PyOpenSCAD":
        """Apply inverse matrix transformation to this object.

        Args:
            m: 4x4 matrix as a list of 4 lists of 4 floats for inverse transformation.

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def offset(
        self,
        r: Optional[float] = None,
        delta: Optional[float] = None,
        chamfer: Optional[bool] = None,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> "PyOpenSCAD":
        """Offset this 2D object.

        Args:
            r: Offset radius (rounded corners).
            delta: Offset distance (sharp corners).
            chamfer: If True, creates chamfered corners.
            fn: Number of fragments for curved parts.
            fa: Minimum angle for each fragment.
            fs: Minimum size for each fragment.

        Returns:
            The transformed object. The original object is unaffected.
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

    def color(self, c: Color, alpha: float = 1.0) -> "PyOpenSCAD":
        """Color this object.

        Args:
            c: Color specification - color name string or RGB/RGBA values.
            alpha: Alpha (transparency) value between 0.0 and 1.0. Defaults to 1.0.

        Returns:
            A new object with the color set. The original object is unaffected.
        """
        ...

    def output(self) -> None:
        """sme as show"""
        ...

    def show(self) -> None:
        """Mark this object for output/display."""
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
        convexity: int = 1,
        center: Optional[bool] = None,
        slices: int = 1,
        segments: int = 0,
        scale: Optional[Vector2] = None,
        twist: Optional[float] = None,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> "PyOpenSCAD":
        """Linear extrude this 2D object to 3D.

        Args:
            height: Extrusion height.
            convexity: Convexity parameter for rendering. Defaults to 1.
            center: If True, centers the extrusion.
            slices: Number of slices for twist/scale. Defaults to 1.
            segments: Number of segments. Defaults to 0.
            scale: Scale factor for top vs bottom.
            twist: Twist angle in degrees.
            fn: Number of fragments for curved parts.
            fa: Minimum angle for each fragment.
            fs: Minimum size for each fragment.

        Returns:
            A new object representing the result of the extrusion. The original object is unaffected.
        """
        ...

    def rotate_extrude(
        self,
        convexity: int = 1,
        angle: float = 360.0,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> "PyOpenSCAD":
        """Rotationally extrude this 2D object to 3D.

        Args:
            convexity: Convexity parameter for rendering. Defaults to 1.
            angle: Rotation angle in degrees. Defaults to 360.0.
            fn: Number of fragments for circle approximation.
            fa: Minimum angle for each fragment.
            fs: Minimum size for each fragment.

        Returns:
            A new object representing the result of the extrusion. The original object is unaffected.
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

    def resize(self, newsize: Vector3, convexity: int = 2) -> "PyOpenSCAD":
        """Modifies the size of the object to match the given x,y, and z sizes.

        Args:
            newsize: New size dimensions as [x, y, z].
            convexity: Convexity parameter for rendering. Defaults to 2.

        Returns:
            The transformed object. The original object is unaffected.
        """
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
        self, cut: Optional[bool] = None, convexity: int = 2
    ) -> "PyOpenSCAD":
        """Create a 2D projection from this 3D object.

        Args:
            cut: If True, creates a cross-section at z=0.
            convexity: Convexity parameter for rendering. Defaults to 2.

        Returns:
            The projected 2D object.
        """
        ...

    def mesh(
        self, triangulate: Optional[bool] = None
    ) -> Union[tuple[list[Vector3], list[list[int]]], list[list[Vector2]]]:
        """Export mesh representation of this object.

        Args:
            triangulate: If True, triangulates the mesh.

        Returns:
            For 3D objects: A tuple of (vertices, faces) where:
            - vertices: List of 3D vertex coordinates [[x, y, z], ...]
            - faces: List of face definitions (lists of vertex indices)

            For 2D objects: A list of outlines where:
            - Each outline is a list of 2D vertex coordinates [[x, y], ...]
        """
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
        self, refmat: Matrix4x4, objmat: Optional[Matrix4x4] = None
    ) -> "PyOpenSCAD":
        """Align this object to a reference matrix.

        Args:
            refmat: Reference transformation matrix.
            objmat: Optional object transformation matrix.

        Returns:
            A new object. The original object is unaffected.
        """
        ...


    def render(self, convexity: int = 2) -> "PyOpenSCAD":
        """Force rendering this object.

        Args:
            convexity: Convexity parameter for rendering. Defaults to 2.

        Returns:
            The object that will be forced to render. The original object is unaffected.
        """
        ...

    def union(self, *others: PyOpenSCADs) -> "PyOpenSCAD":
        """Create a union of this object with others.

        Args:
            *others: Other OpenSCAD objects to union with this one.

        Returns:
            A new object representing the union. The original object is unaffected.
        """
        ...

    def difference(self, *others: PyOpenSCADs) -> "PyOpenSCAD":
        """Create a difference by subtracting others from this object.

        Args:
            *others: Other OpenSCAD objects to subtract from this one.

        Returns:
            A new object representing the difference. The original object is unaffected.
        """
        ...

    def intersection(self, *others: PyOpenSCADs) -> "PyOpenSCAD":
        """Create an intersection of this object with others.

        Args:
            *others: Other OpenSCAD objects to intersect with this one.

        Returns:
            A new object representing the intersection. The original object is unaffected.
        """
        ...

    def __getattr__(self, name): ...
    def __setattr__(self, name, value): ...
    def __getitem__(self, name): ...
    def __setitem__(self, name, value): ...

    # Operators:

    def __or__(self, other: PyOPenSCADs) -> "PyOPenSCAD":
        """Create a union of two objects"""
        ...

    def __and__(self, other: PyOPenSCADs) -> "PyOPenSCAD":
        """Create an intersection of two objects"""
        ...

    @overload
    def __sub__(self, other: PyOPenSCADs) -> "PyOPenSCAD":
        """Create a difference of two objects"""
        ...

    def __add__(self, other: Vector3) -> "PyOPenSCAD":
        """Create a new object by translating this object by a vector"""
        ...

    @overload
    def __sub__(self, other: Vector3) -> "PyOPenSCAD":
        """Create a new object by translating this object by the negative of a vector"""
        ...

    @overload
    def __mul__(self, other: float) -> "PyOPenSCAD":
        """Create a new object by scaling this object by a uniform factor in all directions"""
        ...

    @overload
    def __mul__(self, other: Vector3) -> "PyOPenSCAD":
        """Create a new object by scaling this object by a vector of factors in [x, y, z] directions"""
        ...

def square(
    dim: Optional[Union[float, list[float]]] = None, center: Optional[bool] = None
) -> PyOpenSCAD:
    """Create a square primitive.

    Args:
        dim: Dimensions of the square. Can be a single number for a square,
             or a sequence of 2 numbers [width, height] for a rectangle.
             If not specified, creates a unit square.
        center: If True, centers the square at the origin. If False or None,
                places one corner at the origin. Defaults to False.

    Returns:
        A 2D geometric object.
    """
    ...

def circle(
    r: Optional[float] = None,
    d: Optional[float] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a circle primitive.

    Args:
        r: Radius of the circle. Must be positive. Cannot be used with d.
             If neither r nor d is specified, defaults to 1.
        d: Diameter of the circle. Must be positive. Cannot be used with r.
        fn: Number of fragments for circle approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        A 2D geometric object.
    """
    ...

def polygon(
    points: Matrix4x4, paths: Optional[list[list[int]]] = None, convexity: int = 2
) -> PyOpenSCAD:
    """Create a polygon primitive.

    Args:
        points: List of 2D coordinates defining the polygon vertices.
                Each point must be a list of exactly 2 numbers [x, y].
                Must contain at least one point.
        paths: Optional list of paths, where each path is a list of indices
               into the points list. If specified, must contain at least one path.
               Used to define holes or complex polygons.
        convexity: Convexity parameter for rendering optimization. Must be >= 1.
                   Defaults to 2.

    Returns:
        A 2D geometric object.
    """
    ...

def text(
    text: str,
    size: float = 1.0,
    font: Optional[str] = None,
    spacing: float = 1.0,
    direction: str = "ltr",
    language: str = "en",
    script: str = "latin",
    halign: str = "left",
    valign: str = "baseline",
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a text primitive.

    Args:
        text: The text string to render.
        size: Font size. Defaults to 1.0.
        font: Font name to use. If None, uses default font.
        spacing: Spacing between characters. Defaults to 1.0.
        direction: Text direction, either "ltr" (left-to-right) or "rtl". Defaults to "ltr".
        language: Language code (e.g., "en", "de"). Defaults to "en".
        script: Script type (e.g., "latin", "arabic"). Defaults to "latin".
        halign: Horizontal alignment: "left", "center", or "right". Defaults to "left".
        valign: Vertical alignment: "baseline", "top", "center", or "bottom". Defaults to "baseline".
        fn: Number of fragments for curved parts.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        A 2D geometric object.
    """
    ...

def textmetrics(
    text: str,
    size: float = 1.0,
    font: Optional[str] = None,
    spacing: float = 1.0,
    direction: str = "ltr",
    language: str = "en",
    script: str = "latin",
    halign: str = "left",
    valign: str = "baseline",
) -> dict[str, Union[float, list[float]]]:
    """Get text metrics for the given text parameters.

    Args:
        text: The text string to measure.
        size: Font size. Defaults to 1.0.
        font: Font name to use. If None, uses default font.
        spacing: Spacing between characters. Defaults to 1.0.
        direction: Text direction, either "ltr" or "rtl". Defaults to "ltr".
        language: Language code (e.g., "en", "de"). Defaults to "en".
        script: Script type (e.g., "latin", "arabic"). Defaults to "latin".
        halign: Horizontal alignment: "left", "center", or "right". Defaults to "left".
        valign: Vertical alignment: "baseline", "top", "center", or "bottom". Defaults to "baseline".

    Returns:
        A dictionary containing text metrics with keys:
        - "ascent": Font ascent value
        - "descent": Font descent value
        - "offset": [x_offset, y_offset] list
        - "advance": [advance_x, advance_y] list
        - "position": [bbox_x, bbox_y] list
        - "size": [bbox_width, bbox_height] list
    """
    ...

def cube(
    size: Optional[Union[float, Vector3]] = None, center: Optional[bool] = None
) -> PyOpenSCAD:
    """Create a cube primitive.

    Args:
        size: Dimensions of the cube. Can be a single number for a cube,
              or a sequence of 3 numbers [x, y, z] for a rectangular box.
              If not specified, creates a unit cube [1, 1, 1].
        center: If True, centers the cube at the origin. If False or None,
                places one corner at the origin. Defaults to False.

    Returns:
        A 3D geometric object.
    """
    ...

def cylinder(
    h: Optional[float] = None,
    r1: Optional[float] = None,
    r2: Optional[float] = None,
    center: Optional[bool] = None,
    r: Optional[float] = None,
    d: Optional[float] = None,
    d1: Optional[float] = None,
    d2: Optional[float] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a cylinder primitive.

    Args:
        h: Height of the cylinder. Must be positive.
        r1: Radius at bottom. Must be non-negative.
        r2: Radius at top. Must be non-negative.
        center: If True, centers the cylinder at the origin.
        r: Uniform radius for both ends.
        d: Uniform diameter for both ends.
        d1: Diameter at bottom.
        d2: Diameter at top.
        fn: Number of fragments for circle approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        A 3D geometric object.
    """
    ...

def sphere(
    r: Optional[float] = None,
    d: Optional[float] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a sphere primitive.

    Args:
        r: Radius of the sphere. Must be positive. Cannot be used with d.
        d: Diameter of the sphere. Must be positive. Cannot be used with r.
        fn: Number of fragments for sphere approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        A 3D geometric object.
    """
    ...

def polyhedron(
    points: Matrix4x4,
    faces: list[list[int]],
    convexity: int = 2,
    triangles: Optional[list[list[int]]] = None,
) -> PyOpenSCAD:
    """Create a polyhedron primitive.

    Args:
        points: List of 3D coordinates defining the polyhedron vertices.
                Each point must be a list of exactly 3 numbers [x, y, z].
                Must contain at least one point.
        faces: List of face definitions, where each face is a list of indices
               into the points list.
        convexity: Convexity parameter for rendering optimization. Defaults to 2.
        triangles: Optional backwards compatibility parameter for triangular faces.

    Returns:
        A 3D geometric object.
    """
    ...

def frep(exp: PyLibFive, min: List[float], max: List[float], res: int) -> PyOpenSCAD:
    """Create F-Rep (libfive)
    exp : an SDF epression composed from SDF variables and operators, see tutorial
    """
    ...

def ifrep(obj: PyOpenSCAD) -> PyLibFive:
    """Create Inverse F-Rep(experimental)"""
    ...

@overload
def translate(obj: PyOpenSCADs, v: Vector3) -> PyOpenSCAD:
    """Translate an object or list of objects.

    Args:
        obj: Object or list of objects to translate.
        v: Translation vector [x, y, z].

    Returns:
        The transformed object. The original object is unaffected.
    """
    ...

@overload
def translate(matrix: Matrix4x4, v: Vector3) -> Matrix4x4:
    """Apply translation to a 4x4 transformation matrix.

    Args:
        matrix: 4x4 transformation matrix to translate.
        v: Translation vector [x, y, z].

    Returns:
        The transformed matrix with translation applied.
    """
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

@overload
def rotate(
    obj: PyOpenSCADs, a: Union[float, Vector3], v: Optional[Vector3] = None
) -> PyOpenSCAD:
    """Rotate an object or list of objects.

    Args:
        obj: Object or list of objects to rotate.
        a: Rotation angle (degrees) or rotation vector [x, y, z].
        v: Optional rotation axis vector when a is a scalar angle.

    Returns:
        The transformed object. The original object is unaffected.
    """
    ...

@overload
def rotate(
    matrix: Matrix4x4, a: Union[float, Vector3], v: Optional[Vector3] = None
) -> Matrix4x4:
    """Apply rotation to a 4x4 transformation matrix.

    Args:
        matrix: 4x4 transformation matrix to rotate.
        a: Rotation angle (degrees) or rotation vector [x, y, z].
        v: Optional rotation axis vector when a is a scalar angle.

    Returns:
        The transformed matrix with rotation applied.
    """
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

@overload
def scale(obj: PyOpenSCADs, v: Union[float, Vector3]) -> PyOpenSCAD:
    """Scale an object or list of objects.

    Args:
        obj: Object or list of objects to scale.
        v: Scale factor (uniform) or scale vector [x, y, z].

    Returns:
        The transformed object. The original object is unaffected.
    """
    ...

@overload
def scale(matrix: Matrix4x4, v: Union[float, Vector3]) -> Matrix4x4:
    """Apply scaling to a 4x4 transformation matrix.

    Args:
        matrix: 4x4 transformation matrix to scale.
        v: Scale factor (uniform) or scale vector [x, y, z].

    Returns:
        The transformed matrix with scaling applied.
    """
    ...

@overload
def mirror(obj: PyOpenSCADs, v: Vector3) -> PyOpenSCAD:
    """Mirror an object or list of objects.

    Args:
        obj: Object or list of objects to mirror.
        v: Mirror plane normal vector [x, y, z].

    Returns:
        The transformed object. The original object is unaffected.
    """
    ...

@overload
def mirror(matrix: Matrix4x4, v: Vector3) -> Matrix4x4:
    """Apply mirroring to a 4x4 transformation matrix.

    Args:
        matrix: 4x4 transformation matrix to mirror.
        v: Mirror plane normal vector [x, y, z].

    Returns:
        The transformed matrix with mirroring applied.
    """
    ...

def multmatrix(obj: PyOpenSCADs, m: Matrix4x4) -> PyOpenSCAD:
    """Apply matrix transformation to an object.

    Args:
        obj: Object to transform.
        m: 4x4 transformation matrix as a list of 4 lists of 4 floats.

    Returns:
        The transformed object. The original object is unaffected.
    """
    ...

def color(obj: PyOpenSCADs, c: Color, alpha: float = 1.0) -> PyOpenSCAD:
    """Color an object.

    Args:
        obj: Object to color.
        c: Color specification - color name string or RGB/RGBA values.
        alpha: Alpha (transparency) value between 0.0 and 1.0. Defaults to 1.0.

    Returns:
        A new object with the color set. The original object is unaffected.
    """
    ...

def union(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Create a union of multiple objects.

    Args:
        *objects: Variable number of OpenSCAD objects or lists of objects to union.

    Returns:
        A new object representing the union. The original object is unaffected.
    """
    ...

def difference(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Create a difference of multiple objects.

    Args:
        *objects: Variable number of OpenSCAD objects or lists of objects. The first object
                 has all subsequent objects subtracted from it.

    Returns:
        A new object representing the difference. The original object is unaffected.
    """
    ...

def intersection(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Create an intersection of multiple objects.

    Args:
        *objects: Variable number of OpenSCAD objects or lists of objects to intersect.

    Returns:
        A new object representing the intersection. The original object is unaffected.
    """
    ...

def hull(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Create a convex hull of multiple objects.

    Args:
        *objects: Variable number of OpenSCAD objects or lists of objects.

    Returns:
        A new object. The original object is unaffected.
    """
    ...

def linear_extrude(
    obj: PyOpenSCADs,
    height: Optional[float] = None,
    convexity: int = 1,
    center: Optional[bool] = None,
    slices: int = 1,
    segments: int = 0,
    scale: Optional[Vector2] = None,
    twist: Optional[float] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Linear extrude a 2D object to 3D.

    Args:
        obj: 2D object to extrude.
        height: Extrusion height.
        convexity: Convexity parameter for rendering. Defaults to 1.
        center: If True, centers the extrusion.
        slices: Number of slices for twist/scale. Defaults to 1.
        segments: Number of segments. Defaults to 0.
        scale: Scale factor for top vs bottom.
        twist: Twist angle in degrees.
        fn: Number of fragments for curved parts.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        The linearly extruded 3D object. The original object is unaffected.
    """
    ...

def rotate_extrude(
    obj: PyOpenSCADs,
    convexity: int = 1,
    angle: float = 360.0,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Rotationally extrude a 2D object to 3D.

    Args:
        obj: 2D object to extrude.
        convexity: Convexity parameter for rendering. Defaults to 1.
        angle: Rotation angle in degrees. Defaults to 360.0.
        fn: Number of fragments for circle approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        The rotationally extruded 3D object. The original object is unaffected.
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

def offset(
    obj: PyOpenSCADs,
    r: Optional[float] = None,
    delta: Optional[float] = None,
    chamfer: Optional[bool] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Offset a 2D object.

    Args:
        obj: 2D object to offset.
        r: Offset radius (rounded corners).
        delta: Offset distance (sharp corners).
        chamfer: If True, creates chamfered corners.
        fn: Number of fragments for curved parts.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        The offset 2D object. The original object is unaffected.
    """
    ...

def minkowski(*objects: PyOpenSCADs, convexity: int = 2) -> PyOpenSCAD:
    """Create a Minkowski sum of objects.

    Args:
        *objects: Objects or lists of objects to compute Minkowski sum of.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        A new object representing the Minkowski sum. The original object is unaffected.
    """
    ...

def projection(
    obj: PyOpenSCADs, cut: Optional[bool] = None, convexity: int = 2
) -> PyOpenSCAD:
    """Create a 2D projection from a 3D object.

    Args:
        obj: 3D object to project.
        cut: If True, creates a cross-section at z=0.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        The projected 2D object.
    """
    ...

def surface(
    file: str,
    center: Optional[bool] = None,
    convexity: int = 2,
    invert: Optional[bool] = None,
) -> PyOpenSCAD:
    """Create a surface from a heightmap file.

    Args:
        file: Path to the heightmap image file.
        center: If True, centers the surface.
        convexity: Convexity parameter for rendering. Defaults to 2.
        invert: If True, inverts the heightmap.

    Returns:
        A 3d object generated from the imported height map.
    """
    ...

def show(obj: PyOpenSCADs) -> None:
    """Mark an object for output/display.

    Args:
        obj: Object to mark for output.
    """
    ...

def render(obj: PyOPenSCADs, convexity: int = 2) -> PyOPenSCAD:
    """Force rendering an object.

    Args:
        obj: Object to render.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        The object that will be forced to render. The original object is unaffected.
    """
    ...

def output(obj: PyOpenSCADType) -> None:
    """same as show"""
    ...
def resize(
    obj: PyOpenSCADs, newsize: Vector3, convexity: int = 2
) -> PyOpenSCAD:
    """Modifies the size of an object to match the given x,y, and z sizes.

    Args:
        obj: Object to resize.
        newsize: New size dimensions as [x, y, z].
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        The resized object. The original object is unaffected.
    """
    ...

def divmatrix(obj: PyOpenSCADs, m: Matrix4x4) -> PyOpenSCAD:
    """Apply inverse matrix transformation to an object.

    Args:
        obj: Object to transform.
        m: 4x4 matrix as a list of 4 lists of 4 floats for inverse transformation.

    Returns:
        The inverse transformed object.
    """
    ...

def fill(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Create a fill operation on objects.

    Args:
        *objects: Variable number of OpenSCAD objects or lists of objects to fill.

    Returns:
        A new object representing the fill operation result. The original object is unaffected.
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


def export(obj: PyOpenSCADType, file: str) -> None:
    """Export the result to a file
    file:  output file name, format is automatically detected from suffix
    when obj is a dictionary, it allows 3mf export to export several paths
    """
    ...

def mesh(
    obj: PyOpenSCADs, triangulate: Optional[bool] = None
) -> Union[tuple[list[Vector3], list[list[int]]], list[list[Vector2]]]:
    """Export mesh representation of an object.

    Args:
        obj: Object to convert to mesh.
        triangulate: If True, triangulates the mesh.

    Returns:
        For 3D objects: A tuple of (vertices, faces) where:
        - vertices: List of 3D vertex coordinates [[x, y, z], ...]
        - faces: List of face definitions (lists of vertex indices)

        For 2D objects: A list of outlines where:
        - Each outline is a list of 2D vertex coordinates [[x, y], ...]
    """

def highlight(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Highlights Object"""
    ...

def background(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Puts Object into background"""
    ...

def only(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Shows only this object"""
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


def render(obj: PyOpenSCADs, convexity: int = 2) -> PyOpenSCAD:
    """Force rendering an object.

    Args:
        obj: Object to render.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        The object that will be forced to render. The original object is unaffected.
    """
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


def align(
    obj: PyOpenSCAD, refmat: Matrix4x4, objmat: Optional[Matrix4x4] -> None
) -> PyOpenSCAD:
    """Align an object to a reference matrix.

    Args:
        obj: Object to align.
        refmat: Reference transformation matrix.
        objmat: Optional object transformation matrix.

    Returns:
        A new object after alignment. The original object is unaffected.
    """
    ...
