""" PythonSCAD Stub File for use in editors like Visual Studio Code """

from enum import Enum
from typing import Callable, Any, Iterator, List, Literal, Mapping, Optional, Self, Sequence, TYPE_CHECKING, Union, overload

if TYPE_CHECKING:
    import numpy as np
    import numpy.typing as npt

PyOpenSCADs = Union["PyOpenSCAD", list["PyOpenSCAD"]]
"""Type for functions that accept either a single OpenSCAD object or a list of objects."""

Color = Union[str, Sequence[float]]
"""Color specification as either a color name string (e.g., "red") or RGB/RGBA values as [r, g, b] or [r, g, b, a]."""

Vector1 = Sequence[float]
"""1D vector represented as an [x] coordinate sequence."""

Vector2 = Sequence[float]
"""2D vector represented as an [x, y] coordinate sequence."""

Vector3 = Sequence[float]
"""3D vector represented as an [x, y, z] coordinate sequence."""

Matrix4x4 = Sequence[Sequence[float]]
"""4x4 transformation matrix represented as four coordinate sequences."""

class PyOpenSCADVector(Sequence[float]):
    """3D vector object returned by ``vector()``."""

    def dot(self, other: Self) -> float:
        """Dot product of two vectors."""
        ...

    def norm(self) -> float:
        """Length (magnitude) of this vector."""
        ...

class PyLibFive:
    """Implicit-surface expression node (C type ``PyData``).

    Built via the separate ``libfive`` extension module (``x()``, ``y()``, ``z()``,
    arithmetic, etc.), not via a Python constructor on this type.
    """

    ...

class ChildRef:
    """Mutable reference yielded while iterating over a solid's children."""

    value: "PyOpenSCAD"
    def __getattr__(self, name: str) -> Any: ...

class ChildIterator(Iterator[ChildRef]):
    """Iterator over mutable child references."""

    def __iter__(self) -> Self: ...
    def __next__(self) -> ChildRef: ...

class PyOpenSCAD:
    """Base class for OpenSCAD objects."""

    origin: Matrix4x4
    """4x4 transformation matrix representing the object's origin.
    Initialized as identity matrix."""

    def __iter__(self) -> ChildIterator: ...
    def __len__(self) -> int: ...

    def translate(self, v: Vector3| "npt.NDArray[np.float64]") -> "PyOpenSCAD":
        """Translate this object.

        Args:
            v: Translation vector [x, y, z].

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def right(self, v: float) -> Self:
        """Move object along +X by ``v``."""
        ...

    def left(self, v: float) -> Self:
        """Move object along -X by ``v``."""
        ...

    def back(self, v: float) -> Self:
        """Move object along +Y by ``v``."""
        ...

    def front(self, v: float) -> Self:
        """Move object along -Y by ``v``."""
        ...

    def up(self, v: float) -> Self:
        """Move object along +Z by ``v``."""
        ...

    def down(self, v: float) -> Self:
        """Move object along -Z by ``v``."""
        ...

    def rotx(self, v: float) -> Self:
        """Rotate object around the X axis by ``v`` degrees."""
        ...

    def roty(self, v: float) -> Self:
        """Rotate object around the Y axis by ``v`` degrees."""
        ...

    def rotz(self, v: float) -> Self:
        """Rotate object around the Z axis by ``v`` degrees."""
        ...

    def rotate(
        self, a: Union[float, Vector3| "npt.NDArray[np.float64]"], v: Optional[Vector3| "npt.NDArray[np.float64]"] = None, ref: Optional[Vector3] = None
    ) -> "PyOpenSCAD":
        """Rotate this object.

        Args:
            a: Rotation angle (degrees) or rotation vector [x, y, z].
            v: Optional rotation axis vector when a is a scalar angle.
            ref: Optional specify center of rotation

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def scale(self, v: Union[float, Vector3| "npt.NDArray[np.float64]"]) -> "PyOpenSCAD":
        """Scale this object.

        Args:
            v: Scale factor (uniform) or scale vector [x, y, z].

        Returns:
            The transformed object. The original object is unaffected.
        """
        ...

    def mirror(self, v: Vector3| "npt.NDArray[np.float64]") -> "PyOpenSCAD":
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
        VORONOI = "voronoi"
        STRAIGHT = "straight"
    def roof(
        self,
        method: str = "voronoi",
        convexity: int = 2,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> Self:
        """Build a roof/hip shape over a 2D outline.

        Args:
            method: Roof method (defaults to ``"voronoi"`` when omitted).
            convexity: Convexity parameter for rendering. Defaults to 2.
            fn: Number of fragments for curved parts.
            fa: Minimum angle for each fragment.
            fs: Minimum size for each fragment.
        """
        ...

    def pull(
        self,
        src: Vector3 | "npt.NDArray[np.float64]",
        dst: Vector3 | "npt.NDArray[np.float64]",
    ) -> Self:
        """Stretch part of an object between two 3D points.

        Args:
            src: Anchor point ``[x, y, z]``.
            dst: Pull direction/end point ``[x, y, z]``.
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
        height: Optional[Union[float, Vector3 | "npt.NDArray[np.float64]"]] = None,
        convexity: int = 1,
        origin: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
        scale: Optional[Union[float, Vector2 | "npt.NDArray[np.float64]"]] = None,
        center: Optional[bool] = None,
        slices: int = 1,
        segments: int = 0,
        twist: Optional[Union[float, Any]] = None,
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
        scale: float = 1.0,
        angle: float = 360.0,
        twist: Optional[Union[float, Any]] = None,
        origin: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
        offset: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
        v: Optional[Vector3 | "npt.NDArray[np.float64]"] = None,
        method: str = "centered",
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
        path: Sequence[Sequence[float]] | "npt.NDArray[np.float64]",
        xdir: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
        convexity: int = 1,
        origin: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
        scale: Optional[Union[float, Sequence[float] | "npt.NDArray[np.float64]"]] = None,
        twist: Optional[Union[float, Any]] = None,
        closed: bool = False,
        allow_intersect: bool = False,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> Self:
        """Extrude a 2D shape along a 3D path.

        Args:
            path: Path as a sequence of 3D (or homogeneous 4D) points.
            xdir: Initial local X axis direction (defaults to ``[1, 0, 0]``).
        """
        ...

    def resize(
        self,
        newsize: Union[float, Vector1, Vector2, Vector3, Sequence[float]],
        auto: Union[bool, Sequence[Union[bool, int, float]]] = False,
        convexity: int = 2,
    ) -> "PyOpenSCAD":
        """Modifies the size of an object to match the given x,y, and z sizes.

        Args:
            newsize: New size dimensions as [x], [x, y], or [x, y, z]. Use 0 to keep an axis unchanged. A scalar applies the same size to all three axes.
            auto: When True, auto-scale axes with 0 size proportionally. May also be a 1-3 element sequence of bools or numbers (nonzero = true).
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
        self, cut: bool = False, detail: bool = False, convexity: int = 2
    ) -> "PyOpenSCAD":
        """Create a 2D projection from this 3D object.

        Args:
            cut: If True, creates a cross-section at z=0.
            detail: If True (and ``cut`` is False), preserve more silhouette edges.
                Honored only with the Manifold backend; ignored otherwise.
            convexity: Convexity parameter for rendering. Defaults to 2.

        Returns:
            The projected 2D object.
        """
        ...

    @overload
    def mesh(
        self, triangulate: Optional[bool] = None, *, color: Literal[False] = False
    ) -> Union[
        tuple[list[Vector3], list[list[int]]],
        list[list[Vector2]],
        None,
    ]:
        ...

    @overload
    def mesh(
        self, triangulate: Optional[bool] = None, *, color: Literal[True]
    ) -> Union[
        tuple[list[Vector3], list[list[int]], list[Sequence[float]], list[int]],
        list[list[Union[Sequence[float], Vector2]]],
        None,
    ]:
        """Export mesh representation of this object.

        Args:
            triangulate: If True, triangulates the mesh.
            color: If True, include per-face or per-outline color data.

        Returns:
            For 3D objects without color: ``(vertices, faces)``.
            For 3D objects with color: ``(vertices, faces, colors, color_indices)``.
            For 2D objects: outline vertex lists (optionally prefixed with RGBA color).
            Returns ``None`` when geometry cannot be exported.
        """
        ...

    def oversample(
        self,
        size: float = 2,
        texture: Optional[str] = None,
        projection: Optional[str] = None,
        texturewidth: float = 1,
        textureheight: float = 1,
        texturedepth: float = 1,
    ) -> Self:
        """Subdivide/refine a mesh, optionally applying a texture."""
        ...

    def fillet(
        self,
        r: float = 1.0,
        sel: Optional[PyOpenSCADs] = None,
        fn: Optional[float] = None,
        minang: float = 30,
    ) -> Self:
        """Round edges of a solid."""
        ...

    def align(
        self, refmat: Matrix4x4, objmat: Optional[Matrix4x4] = None, flip: bool = False
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

    def separate(self) -> list["PyOpenSCAD"]:
        """Split a compound object into connected parts."""
        ...

    def wrap(
        self,
        target: Optional[PyOpenSCAD] = None,
        r: Optional[float] = None,
        d: Optional[float] = None,
        fn: Optional[float] = None,
        fa: Optional[float] = None,
        fs: Optional[float] = None,
    ) -> Self:
        """Wrap this object around a cylinder or a 2D outline."""
        ...

    def explode(self, v: Sequence[Union[float, Sequence[float]]]) -> Self:
        """Move this object's parts apart along one or more direction specs."""
        ...

    def inside(self, point: Vector3 | "npt.NDArray[np.float64]") -> Optional[bool]:
        """Test whether ``point`` lies inside this object."""
        ...

    def bbox(self) -> Optional["PyOpenSCAD"]:
        """Return a bounding-box shape for this object."""
        ...

    def faces(self, triangulate: Optional[bool] = None) -> Optional[list]:
        """Return face index lists for this object."""
        ...

    def children(self) -> tuple["PyOpenSCAD", ...]:
        """Return direct child solids as a tuple."""
        ...

    def edges(self) -> Optional[list["PyOpenSCAD"]]:
        """Return 1D edge objects for 2D outline geometry."""
        ...

    def clone(self, obj: PyOpenSCADs) -> None:
        """Replace this object in-place with a clone of ``obj``."""
        ...

    def hasattr(self, keyword: str) -> bool:
        """Return whether ``keyword`` exists in this object's attribute dict."""
        ...

    def getattr(self, keyword: str) -> Any:
        """Return an attribute from this object's dict, or ``None`` if missing."""
        ...

    def setattr(self, keyword: str, setvalue: Any) -> None:
        """Set an attribute on this object's dict."""
        ...

    def dict(self) -> dict[str, Any]:
        """Return this object's attribute dictionary."""
        ...

    def __getattr__(self, name: str) -> Any: ...
    def __setattr__(self, name: str, value: Any) -> None: ...
    @overload
    def __getitem__(self, name: str) -> Any: ...
    @overload
    def __getitem__(self, index: int) -> "PyOpenSCAD": ...
    def __setitem__(self, name: str, value: Any) -> None: ...

    # Operators:

    def __or__(self, other: PyOpenSCADs) -> "PyOpenSCAD":
        """Create a union of two objects"""
        ...

    def __and__(self, other: PyOpenSCADs) -> "PyOpenSCAD":
        """Create an intersection of two objects"""
        ...

    @overload
    def __sub__(self, other: PyOpenSCADs) -> "PyOpenSCAD":
        """Create a difference of two objects"""
        ...

    @overload
    def __add__(self, other: PyOpenSCAD) -> "PyOpenSCAD":
        """Create a union when adding two solids"""
        ...

    @overload
    def __add__(self, other: Vector3) -> "PyOpenSCAD":
        """Create a new object by translating this object by a vector"""
        ...

    @overload
    def __sub__(self, other: Vector3) -> "PyOpenSCAD":
        """Create a new object by translating this object by the negative of a vector"""
        ...

    @overload
    def __mul__(self, other: float) -> "PyOpenSCAD":
        """Create a new object by scaling this object by a uniform factor in all directions"""
        ...

    @overload
    def __mul__(self, other: Vector3) -> "PyOpenSCAD":
        """Create a new object by scaling this object by a vector of factors in [x, y, z] directions"""
        ...

    @overload
    def __matmul__(self, other: Matrix4x4) -> Union["PyOpenSCAD", Matrix4x4]:
        """Apply a 4x4 transformation matrix to this object or matrix"""
        ...

    def __invert__(self) -> "PyOpenSCAD":
        """Mark this object with the ``only`` modifier (``!``)"""
        ...

    def __neg__(self) -> "PyOpenSCAD":
        """Mark this object for highlight rendering (``#``)"""
        ...

    def __pos__(self) -> "PyOpenSCAD":
        """Mark this object as background (``%``)"""
        ...

    @overload
    def __xor__(self, other: PyOpenSCAD) -> "PyOpenSCAD":
        """Create the hull from self and other"""
        ...

    @overload
    def __xor__(self, other: Sequence[Union[float, Sequence[float]]]) -> "PyOpenSCAD":
        """Create the explosion object from self and the direction spec"""
        ...

    @overload
    def __mod__(self, other: PyOpenSCAD) -> "PyOpenSCAD":
        """Create a new object from the minkowski sum from self and other"""
        ...

    @overload
    def __mod__(self, other: Vector3) -> "PyOpenSCAD":
        """Rotate object by the rotations given in x/y/z of the Vector"""
        ...

def square(
    dim: Optional[Union[float, list[float]| "npt.NDArray[np.float64]"]] = None, center: Optional[bool] = None
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
    angle: Optional[float] = None,
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
    points: Sequence[Sequence[float]] | "npt.NDArray[np.float64]",
    paths: Sequence[Sequence[int]] | "npt.NDArray[np.int64]" | None = None,
    convexity: int = 2,
    *,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a polygon primitive.

    Args:
        points: List of coordinates defining the polygon vertices.
                Each point is ``[x, y]`` or ``[x, y, r]`` where optional ``r``
                fillets that corner. Must contain at least one point.
        paths: Optional list of paths, where each path is a list of indices
               into the points list. If specified, must contain at least one path.
               Used to define holes or complex polygons.
        convexity: Convexity parameter for rendering optimization. Must be >= 1.
                   Defaults to 2.
        fn: Number of fragments for corner-fillet arcs.
        fa: Minimum angle for each fillet-arc fragment.
        fs: Minimum size for each fillet-arc fragment.

    Returns:
        A 2D geometric object.
    """
    ...

def polyline(
    points: Sequence[Sequence[float]] | "npt.NDArray[np.float64]",
) -> PyOpenSCAD:
    """Create an open polyline through ``points``.

    Points are ``[x, y]`` (2D) or ``[x, y, z]``. If any point has a non-zero
    ``z``, the result is a 3D polyline.
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
    points: Sequence[Sequence[float]] | "npt.NDArray[np.float64]",
    faces: list[list[int]],
    convexity: int = 2,
    triangles: Optional[list[list[int]]] = None,
    colors: Optional[Sequence[Sequence[float]] | "npt.NDArray[np.float64]"] = None,
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


def organic(pts: list[list[float]], r: float) -> PyOpenSCAD:
    """Build an organic shape from a point cloud and radius ``r``."""
    ...


def frep(
    exp: Union[PyLibFive, Any],
    min: Sequence[float],
    max: Sequence[float],
    res: float = 10,
) -> PyOpenSCAD:
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


def right(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along +X."""
    ...

def left(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along -X."""
    ...

def back(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along +Y."""
    ...

def front(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along -Y."""
    ...

def up(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along +Z."""
    ...

def down(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Move object along -Z."""
    ...

@overload
def rotate(
    obj: PyOpenSCADs, a: Union[float, Vector3], v: Optional[Vector3] = None, ref: Optional[Vector3] = None
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
    matrix: Matrix4x4, a: Union[float, Vector3], v: Optional[Vector3] = None, ref: Optional[Vector3] = None
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

def rotx(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Rotate object around the X axis."""
    ...

def roty(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Rotate object around the Y axis."""
    ...

def rotz(obj: PyOpenSCAD, v: float) -> PyOpenSCAD:
    """Rotate object around the Z axis."""
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
    height: Optional[Union[float, Vector3 | "npt.NDArray[np.float64]"]] = None,
    convexity: int = 1,
    origin: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
    scale: Optional[Union[float, Vector2 | "npt.NDArray[np.float64]"]] = None,
    center: Optional[bool] = None,
    slices: int = 1,
    segments: int = 0,
    twist: Optional[Union[float, Any]] = None,
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
    scale: float = 1.0,
    angle: float = 360.0,
    twist: Optional[Union[float, Any]] = None,
    origin: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
    offset: Optional[Vector2 | "npt.NDArray[np.float64]"] = None,
    v: Optional[Vector3 | "npt.NDArray[np.float64]"] = None,
    method: str = "centered",
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
    obj: PyOpenSCAD,
    path: Sequence[Sequence[float]] | "npt.NDArray[np.float64]",
    xdir: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
    convexity: int = 1,
    origin: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
    scale: Optional[Union[float, Sequence[float] | "npt.NDArray[np.float64]"]] = None,
    twist: Optional[Union[float, Any]] = None,
    closed: bool = False,
    allow_intersect: bool = False,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Extrude a 2D shape along a 3D path."""
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

def minkowski(obj1: PyOpenSCADs, obj2: PyOpenSCADs, convexity: int = 2) -> PyOpenSCAD:
    """Create a Minkowski sum of two objects."""
    ...

def projection(
    obj: PyOpenSCADs,
    cut: bool = False,
    detail: bool = False,
    convexity: int = 2,
) -> PyOpenSCAD:
    """Create a 2D projection from a 3D object.

    Args:
        obj: 3D object to project.
        cut: If True, creates a cross-section at z=0.
        detail: If True (and ``cut`` is False), preserve more silhouette edges.
            Honored only with the Manifold backend; ignored otherwise.
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

def render(obj: PyOpenSCADs, convexity: int = 2) -> PyOpenSCAD:
    """Force rendering an object.

    Args:
        obj: Object to render.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        The object that will be forced to render. The original object is unaffected.
    """
    ...

def output(obj: PyOpenSCAD) -> None:
    """same as show"""
    ...
def resize(
    obj: PyOpenSCADs,
    newsize: Union[float, Vector1, Vector2, Vector3, Sequence[float]],
    auto: Union[bool, Sequence[Union[bool, int, float]]] = False,
    convexity: int = 2,
) -> PyOpenSCAD:
    """Modifies the size of an object to match the given x,y, and z sizes.

    Args:
        obj: Object to resize.
        newsize: New size dimensions as [x], [x, y], or [x, y, z]. Use 0 to keep an axis unchanged. A scalar applies the same size to all three axes.
        auto: When True, auto-scale axes with 0 size proportionally. May also be a 1-3 element sequence of bools or numbers (nonzero = true).
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
    obj: PyOpenSCAD,
    method: str = "voronoi",
    convexity: int = 2,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Build a roof/hip shape over a 2D outline."""
    ...

def pull(
    obj: PyOpenSCAD,
    src: Vector3 | "npt.NDArray[np.float64]",
    dst: Vector3 | "npt.NDArray[np.float64]",
) -> PyOpenSCAD:
    """Stretch part of an object between two 3D points."""
    ...


@overload
def export(obj: PyOpenSCAD, file: str) -> None:
    """Export the result to a file
    file:  output file name, format is automatically detected from suffix
    """
    ...

@overload
def export(obj: Mapping[str, PyOpenSCADs], file: str) -> None:
    """Export the result to a file
    file:  output file name, format is automatically detected from suffix
    when obj is a dictionary, it allows 3mf export to export several paths
    """
    ...

@overload
def mesh(
    obj: PyOpenSCADs, triangulate: Optional[bool] = None, *, color: Literal[False] = False
) -> Union[
    tuple[list[Vector3], list[list[int]]],
    list[list[Vector2]],
    None,
]:
    ...

@overload
def mesh(
    obj: PyOpenSCADs, triangulate: Optional[bool] = None, *, color: Literal[True]
) -> Union[
    tuple[list[Vector3], list[list[int]], list[Sequence[float]], list[int]],
    list[list[Union[Sequence[float], Vector2]]],
    None,
]:
    """Export mesh representation of an object."""
    ...

def inside(obj: PyOpenSCADs, point: Vector3 | "npt.NDArray[np.float64]") -> Optional[bool]:
    """Test whether ``point`` lies inside ``obj``."""
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



def oversample(
    obj: PyOpenSCAD,
    size: float = 2,
    texture: Optional[str] = None,
    projection: Optional[str] = None,
    texturewidth: float = 1,
    textureheight: float = 1,
    texturedepth: float = 1,
) -> PyOpenSCAD:
    """Subdivide/refine a mesh, optionally applying a texture."""
    ...

def fillet(
    obj: PyOpenSCAD,
    r: float = 1.0,
    sel: Optional[PyOpenSCADs] = None,
    fn: Optional[float] = None,
    minang: float = 30,
) -> PyOpenSCAD:
    """Round edges of a solid."""
    ...

def group(obj: PyOpenSCAD) -> PyOpenSCAD:
    """Groups several Objects"""
    ...


def rendervars(
    vpd: Optional[float] = None,
    vpf: Optional[float] = None,
    vpr: Optional[Vector3] = None,
    vpt: Optional[Vector3] = None,
) -> None:
    """Set camera/viewport parameters from Python code.

    All parameters are optional. Only the ones provided will be updated;
    the rest keep their current values.

    Args:
        vpd: Viewer distance (camera distance from the object).
        vpf: Field of view angle in degrees.
        vpr: Viewport rotation as [x, y, z] in degrees.
        vpt: Viewport translation as [x, y, z] (the point the camera looks at).
    """
    ...

def machineconfig(
    config: Mapping[str, Any],
) -> None:
    """Specify extended parameters to be used for gcode Export

    Args:
        config: Python Dict tree with settings

    machineconfig({
        "default":{
            "property":{
                "initCode":"G90", # Potential init code
                "exitCode":"",    # Potential exit code
                "feedAddX":-1,    # reduce feedrate away from laser tube
                "feedAddY":-1,    # same
            }
        },
        "stroke":{ # create mapping blue color to laserpower/laserfeed
            "property":{
                "color":0x0000ffff,
                "feed":1000,
                "power":50
            }
        }
    })

    """
    ...

@overload
def osimport(
    file: str,
    layer: Optional[str] = None,
    convexity: int = 2,
    origin: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
    scale: float = 1,
    width: float = 1,
    height: float = 1,
    center: bool = False,
    dpi: float = 72.0,
    id: Optional[str] = None,
    stroke: bool = False,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
    split_by_color: Literal[False] = False,
) -> PyOpenSCAD:
    """Imports an object from disk.

    Args:
        stroke: When True, turns open SVG paths into polygons. When False (the
                default), SVG paths are converted to polylines instead.
        split_by_color: SVG only. When True, returns a dict of ``{hex_color: PyOpenSCAD}``
                with one 2D object per distinct color found in the SVG, instead of a
                single merged object. Useful for exporting each color as a separate
                named/colored object (e.g. for multi-toolhead 3MF export via
                ``export()``'s dict form). Raises ValueError for non-SVG files or if
                the SVG has no colored shapes.

    Example:
        >>> parts = osimport("logo.svg", split_by_color=True)
        >>> solids = {name: obj.linear_extrude(2) for name, obj in parts.items()}
        >>> export(solids, "logo.3mf")
    """
    ...

@overload
def osimport(
    file: str,
    layer: Optional[str] = None,
    convexity: int = 2,
    origin: Optional[Sequence[float] | "npt.NDArray[np.float64]"] = None,
    scale: float = 1,
    width: float = 1,
    height: float = 1,
    center: bool = False,
    dpi: float = 72.0,
    id: Optional[str] = None,
    stroke: bool = False,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
    *,
    split_by_color: Literal[True],
) -> dict[str, PyOpenSCAD]:
    """See the non-overloaded osimport() docstring."""
    ...

@overload
def osimport(
    file: str,
    layer: Optional[str],
    convexity: int,
    origin: Optional[Sequence[float] | "npt.NDArray[np.float64]"],
    scale: float,
    width: float,
    height: float,
    center: bool,
    dpi: float,
    id: Optional[str],
    stroke: bool,
    fn: Optional[float],
    fa: Optional[float],
    fs: Optional[float],
    split_by_color: Literal[True],
) -> dict[str, PyOpenSCAD]:
    """Positional form of the split-by-color overload."""
    ...

def osuse(file: str) -> PyOpenSCAD:
    """Import an OpenSCAD library, exposing its modules and functions.

    Args:
        file: Path to the OpenSCAD (.scad) file to import.

    Returns:
        An object with the library's modules and functions as attributes.
        Modules can be called to create geometry, functions can be called
        to compute and return values.

    Example:
        >>> lib = osuse("mylib.scad")
        >>> # Call a module to create geometry
        >>> obj = lib.my_module(r=10, h=20)
        >>> # Call a function to compute a value
        >>> width = lib.get_width()
        >>> area = lib.calculate_area(width=5, height=10)
    """
    ...

def version() -> List[int]:
    """Outputs PythonSCAD semantic version components."""
    ...

def version_num() -> int:
    """Outputs PythonSCAD semantic version as major * 1000000 + minor * 1000 + patch."""
    ...

def version_string() -> str:
    """Outputs the full PythonSCAD version string."""
    ...

def _register_parameter(
    name: str,
    default: Any,
    description: str | None = ...,
    group: str | None = ...,
    range: Any = ...,
    step: float | None = ...,
    max_length: int | None = ...,
    options: list[Any] | dict[Any, str] | None = ...,
) -> Any:
    """Internal pure Customizer parameter registration helper."""
    ...

def add_parameter(
    name: str,
    default: Any,
    description: str | None = ...,
    group: str | None = ...,
    range: Any = ...,
    step: float | None = ...,
    max_length: int | None = ...,
    options: list[Any] | dict[Any, str] | None = ...,
) -> Any:
    """Add a Customizer parameter and return its effective value."""
    ...

def scad(code: str) -> PyOpenSCAD:
    """Evaluate Code in SCAD syntax"""
    ...


def align(
    obj: PyOpenSCAD, refmat: Matrix4x4, objmat: Optional[Matrix4x4] = None, flip: bool = False
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

def edge(size: float = 1, center: Optional[bool] = None) -> PyOpenSCAD:
    """Create a 1D edge primitive.

    Args:
        size: Length of the edge. Can be a single number or [length].
        center: If True, centers the edge at the origin.

    Returns:
        A 1D geometric object.
    """
    ...

def spline(points: list[Vector2], fn: Optional[int] = None, fa: Optional[float] = None, fs: Optional[float] = None) -> PyOpenSCAD:
    """Create a spline curve from points.

    Args:
        points: List of 2D control points.
        fn: Number of fragments for curve approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        A 2D geometric object representing the spline.
    """
    ...

def wrap(
    obj: PyOpenSCADs,
    target: Optional[PyOpenSCAD] = None,
    r: Optional[float] = None,
    d: Optional[float] = None,
    fn: Optional[float] = None,
    fa: Optional[float] = None,
    fs: Optional[float] = None,
) -> PyOpenSCAD:
    """Wrap an object around a cylinder or a 2D outline.

    Args:
        obj: Object to wrap.
        target: Optional 2D target object to wrap around.
        r: Radius of the cylinder.
        d: Diameter of the cylinder.
        fn: Number of fragments for circle approximation.
        fa: Minimum angle for each fragment.
        fs: Minimum size for each fragment.

    Returns:
        The wrapped object.
    """
    ...

def separate(obj: PyOpenSCADs) -> list[PyOpenSCAD]:
    """Split an object into separate parts.

    Args:
        obj: Object to split into parts.

    Returns:
        A list of separate objects, one for each connected component.
    """
    ...

def skin(
    *objects: PyOpenSCADs,
    convexity: int = 2,
    align_angle: Optional[float] = None,
    segments: Optional[int] = None,
    interpolate: Optional[float] = None,
) -> PyOpenSCAD:
    """Create a skin surface connecting multiple 2D cross-sections.

    Args:
        *objects: Multiple 2D cross-section objects to connect.
        convexity: Convexity parameter for rendering. Defaults to 2.

    Returns:
        A 3D object skinned across the cross-sections.
    """
    ...

def concat(*objects: PyOpenSCADs) -> PyOpenSCAD:
    """Concatenate multiple objects into a single object.

    Args:
        *objects: Variable number of objects to concatenate.

    Returns:
        A single object containing all input objects.
    """
    ...

def patch(
    outer: Union[Sequence[Vector3], PyOpenSCAD],
    holes: Optional[Sequence[Union[Sequence[Vector3], PyOpenSCAD]]] = None,
    proj: Optional[Callable[[Vector3], Vector2]] = None,
    grid_spacing_uv: float = 1.0,
    displacement: Optional[Callable[[Vector3], float]] = None,
    use_tangents: bool = False,
) -> PyOpenSCAD:
    """Build a triangulated surface from an outer boundary ring and holes.

    Args:
        outer: The outer boundary ring, as a list of 3D points or a 2D shape.
        holes: Zero or more inner boundary rings to cut out of the surface,
               each a list of 3D points or a 2D shape.
        proj: Optional function mapping a 3D point to 2D (u, v) coordinates,
              used to flatten the boundary for triangulation. If not given,
              chosen automatically based on the shape of the input.
        grid_spacing_uv: Target spacing between interior mesh points, in the
              flattened domain's own units. Defaults to 1.0.
        displacement: Optional function mapping a 3D point to a scalar,
              applied along the local surface normal at interior points.
        use_tangents: If True, use each ring's own plane normal as a tangent
              to bow the surface instead of a flat interpolation between
              rings. Only meaningful when a ring is given as a 2D shape.

    Returns:
        A 3D surface object.
    """
    ...

def sheet(
    func,
    imin: float,
    imax: float,
    jmin: float,
    jmax: float,
    fs: Optional[float] = None,
    iclose: Optional[bool] = None,
    jclose: Optional[bool] = None,
) -> PyOpenSCAD:
    """Create a parametric surface sheet.

    Args:
        func: Function that takes (i, j) and returns a 3D point.
        imin: Minimum value for first parameter.
        imax: Maximum value for first parameter.
        jmin: Minimum value for second parameter.
        jmax: Maximum value for second parameter.
        fs: Fragment size for tesselation.
        iclose: If True, closes the surface in the i direction.
        jclose: If True, closes the surface in the j direction.

    Returns:
        A 3D parametric surface object.
    """
    ...

def bbox(obj: PyOpenSCADs) -> Optional[PyOpenSCAD]:
    """Return a bounding-box shape for ``obj``."""
    ...

def size(obj: PyOpenSCADs) -> Optional[Union[Vector2, Vector3]]:
    """Get the size dimensions of an object's bounding box."""
    ...

def position(obj: PyOpenSCADs) -> Optional[Vector3]:
    """Get the minimum corner of an object's bounding box."""
    ...

def faces(obj: PyOpenSCADs, triangulate: Optional[bool] = None) -> Optional[list]:
    """Export a list of faces from an object."""
    ...

def children(obj: PyOpenSCADs) -> tuple[PyOpenSCAD, ...]:
    """Return direct child solids of ``obj`` as a tuple."""
    ...

def edges(obj: PyOpenSCADs) -> Optional[list[PyOpenSCAD]]:
    """Export 1D edge objects from 2D outline geometry."""
    ...

def explode(obj: PyOpenSCADs, v: Sequence[Union[float, Sequence[float]]]) -> PyOpenSCAD:
    """Explode a solid with a vector list.

    Args:
        obj: Object to explode.
        v: List of vectors for explosion directions.

    Returns:
        The exploded object.
    """
    ...

def debug(obj: PyOpenSCADs, faces: Optional[list] = None) -> PyOpenSCAD:
    """Debug an object or specific faces.

    Args:
        obj: Object to debug.
        faces: Optional list of specific faces to debug.

    Returns:
        Debug visualization of the object.
    """
    ...

def repair(obj: PyOpenSCADs, color: Optional[Color] = None) -> PyOpenSCAD:
    """Repair an object to make it watertight.

    Args:
        obj: Object to repair.
        color: Optional color for repaired areas.

    Returns:
        The repaired object.
    """
    ...

def osinclude(file: str) -> PyOpenSCAD:
    """Include an OpenSCAD library (deprecated - use osuse instead).

    Args:
        file: Path to the OpenSCAD (.scad) file to include.

    Returns:
        An object with the library's modules and functions as attributes.

    Note:
        This function is deprecated. Use osuse() instead.
    """
    ...

def model() -> Optional[PyOpenSCAD]:
    """Yield the current top-level model object, or ``None`` if unset."""
    ...

def modelpath() -> str:
    """Get the absolute path to the current script.

    Returns:
        Absolute path to the script file.
    """
    ...

def memberfunction(membername: str, memberfunc: Any, docstring: Optional[str] = None) -> None:
    """Register an additional OpenSCAD member function.

    Args:
        membername: Name of the member function.
        memberfunc: Python function to register.
        docstring: Documentation string for the function.
    """
    ...

def marked(value: float) -> PyLibFive:
    """Wrap a numeric value as a marked libfive expression node."""
    ...

def vector(x: float, y: float, z: float) -> PyOpenSCADVector:
    """Create a 3D vector object."""
    ...

def Sin(value: float) -> float:
    """Calculate sine of an angle in degrees.

    Args:
        value: Angle in degrees.

    Returns:
        Sine of the angle.
    """
    ...

def Cos(value: float) -> float:
    """Calculate cosine of an angle in degrees.

    Args:
        value: Angle in degrees.

    Returns:
        Cosine of the angle.
    """
    ...

def Tan(value: float) -> float:
    """Calculate tangent of an angle in degrees.

    Args:
        value: Angle in degrees.

    Returns:
        Tangent of the angle.
    """
    ...

def Asin(value: float) -> float:
    """Calculate arcsine in degrees.

    Args:
        value: Value between -1 and 1.

    Returns:
        Arcsine in degrees.
    """
    ...

def Acos(value: float) -> float:
    """Calculate arccosine in degrees.

    Args:
        value: Value between -1 and 1.

    Returns:
        Arccosine in degrees.
    """
    ...

def Atan(value: float) -> float:
    """Calculate arctangent in degrees.

    Args:
        value: Value to calculate arctangent for.

    Returns:
        Arctangent in degrees.
    """
    ...

def norm(vec: Vector3) -> float:
    """Calculate the length/magnitude of a vector.

    Args:
        vec: 3D vector [x, y, z].

    Returns:
        The length of the vector.
    """
    ...

def dot(vec1: Vector3, vec2: Vector3) -> float:
    """Calculate the dot product of two vectors.

    Args:
        vec1: First 3D vector [x, y, z].
        vec2: Second 3D vector [x, y, z].

    Returns:
        The dot product of the two vectors.
    """
    ...

def cross(vec1: Vector3, vec2: Vector3) -> Vector3:
    """Calculate the cross product of two vectors.

    Args:
        vec1: First 3D vector [x, y, z].
        vec2: Second 3D vector [x, y, z].

    Returns:
        The cross product as a 3D vector.
    """
    ...

# Exported type aliases from the C extension module.
Openscad = PyOpenSCAD
