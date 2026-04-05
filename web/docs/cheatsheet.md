# PythonSCAD Cheat Sheet

<input type="text" id="searchInput" onkeyup="searchFunction()" placeholder="Search the cheat sheet...">

<!-- ************************************* -->
<!-- edit the sections below to add features, or edit existing -->

## 3D Primitives

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/primitives3d/#cube">cube</a>([x, y, z], center)</code></div>
      <div>Create a box with specified dimensions</div>
      <div><code>cube([10, 20, 30]).show()</code></div>

      <div class="func"><code><a href="../reference/primitives3d/#sphere">sphere</a>(r | d)</code></div>
      <div>Create a sphere with radius r or diameter d</div>
      <div><code>sphere(15).show()</code></div>

      <div class="func"><code><a href="../reference/primitives3d/#cylinder">cylinder</a>(h, r|d, center)</code></div>
      <div>Create a cylinder or cone with height h and radii</div>
      <div><code>cylinder(h=20, r1=10, r2=5).show()</code></div>

      <div class="func"><code><a href="../reference/primitives3d/#polyhedron">polyhedron</a>(points, faces)</code></div>
      <div>Create a 3D solid from vertices and face indices</div>
      <div><code>polyhedron(pts, tris).show()</code></div>

      <div class="func"><code><a href="../reference/surface/#surface">surface</a>(file, center)</code></div>
      <div>Generate a 3D surface from a data file or image</div>
      <div><code>surface(file="terrain.dat").show()</code></div>

      <div class="func"><code><a href="../reference/sheet/#sheet">sheet</a>(func, imin, imax, ...)</code></div>
      <div>Generate a 3D surface from a Python function</div>
      <div><code>sheet(myfunc, 0, 10, 0, 10).show()</code></div>
  </div>
</div>

## 2D Primitives

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/primitives2d/#square">square</a>([x, y], center)</code></div>
      <div>Create a 2D rectangle</div>
      <div><code>square([10, 20]).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#circle">circle</a>(r | d, angle)</code></div>
      <div>Create a 2D circle or arc sector</div>
      <div><code>circle(r=5, angle=90).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#polygon">polygon</a>(points, paths)</code></div>
      <div>Create a 2D polygon from a list of points</div>
      <div><code>polygon([[0,0], [10,0], [5,10]]).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#polyline">polyline</a>(points)</code></div>
      <div>Create an open 2D polyline (e.g. for laser cutting)</div>
      <div><code>polyline([[0,0], [10,0], [5,10]]).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#spline">spline</a>(points, fn)</code></div>
      <div>Create a smooth 2D curve through given points</div>
      <div><code>spline([[0,6],[10,-5],[20,10]], fn=20).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#text">text</a>(t, size, font, ...)</code></div>
      <div>Create 2D text geometry</div>
      <div><code>text("Hello", size=10).show()</code></div>

      <div class="func"><code><a href="../reference/primitives2d/#textmetrics">textmetrics</a>(t, size, font, ...)</code></div>
      <div>Get bounding box metrics for text without creating geometry</div>
      <div><code>m = textmetrics("Hello", size=10)</code></div>
  </div>
</div>

## 1D Primitives

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/primitives1d/#edge">edge</a>(size, center)</code></div>
      <div>Create a 1D edge with a given length</div>
      <div><code>edge(size=10, center=True).show()</code></div>
  </div>
</div>

## Transformations

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/transformations/#translate">translate</a>(obj, [x,y,z])</code></div>
      <div>Move object by x, y, z</div>
      <div><code>cube(5).translate([10, 0, 0]).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#rotate">rotate</a>(obj, [x,y,z])</code></div>
      <div>Rotate object by angles in degrees</div>
      <div><code>cube(5).rotate([45, 0, 0]).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#scale">scale</a>(obj, [x,y,z])</code></div>
      <div>Scale object by factors along each axis</div>
      <div><code>cube(5).scale([1, 2, 1]).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#mirror">mirror</a>(obj, [x,y,z])</code></div>
      <div>Mirror object across a plane defined by the normal vector</div>
      <div><code>cube(5).mirror([1, 0, 0]).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#resize">resize</a>(obj, [x,y,z], auto)</code></div>
      <div>Resize object to exact dimensions</div>
      <div><code>sphere(5).resize([10, 10, 20]).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#multmatrix">multmatrix</a>(obj, m)</code></div>
      <div>Apply a 4x4 transformation matrix</div>
      <div><code>cube(5).multmatrix(mat).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#divmatrix">divmatrix</a>(obj, m)</code></div>
      <div>Apply the inverse of a 4x4 transformation matrix</div>
      <div><code>cube(5).divmatrix(mat).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#color">color</a>(obj, c, alpha)</code></div>
      <div>Apply color to object by name, hex, or [r,g,b,a]</div>
      <div><code>cube(5).color("Tomato").show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#offset">offset</a>(obj, r | delta, chamfer)</code></div>
      <div>Offset a 2D shape inward or outward</div>
      <div><code>square(10).offset(r=2).show()</code></div>

      <div class="func"><code><a href="../reference/pull/#pull">pull</a>(obj, src, dst)</code></div>
      <div>Pull apart an object by inserting void at a point</div>
      <div><code>cube(5).pull([1,1,3], [4,-2,5]).show()</code></div>

      <div class="func"><code><a href="../reference/wrap/#wrap">wrap</a>(obj, target, r|d)</code></div>
      <div>Wrap a flat object around a cylinder</div>
      <div><code>square(10).wrap(cylinder(r=5,h=10)).show()</code></div>

      <div class="func"><code><a href="../reference/align/#align">align</a>(obj, refmat, objmat, flip)</code></div>
      <div>Align object to a handle / reference matrix</div>
      <div><code>cyl.align(cyl.right_center, cyl.origin).show()</code></div>

      <div class="func"><code><a href="../reference/roof/#roof">roof</a>(obj, method)</code></div>
      <div>Create a roof from a 2D polygon (experimental)</div>
      <div><code>polygon(pts).roof().show()</code></div>
  </div>
</div>

### Convenience Transforms

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/transformations/#convenience-transforms">right</a> / left / front / back / up / down</code></div>
      <div>Translate along a single axis</div>
      <div><code>cube(5).right(10).up(3).show()</code></div>

      <div class="func"><code><a href="../reference/transformations/#convenience-rotations">rotx</a> / roty / rotz</code></div>
      <div>Rotate around a single axis</div>
      <div><code>cube(5).rotx(45).show()</code></div>
  </div>
</div>

## Boolean Operations

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/booleans/#union">union</a>(obj1, obj2, r, fn)</code></div>
      <div>Combine objects; optionally fillet edges with r</div>
      <div><code>union(cube(5), sphere(3)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#difference">difference</a>(obj1, obj2, r, fn)</code></div>
      <div>Subtract obj2 from obj1; optionally fillet with r</div>
      <div><code>difference(cube(10), sphere(7)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#intersection">intersection</a>(obj1, obj2)</code></div>
      <div>Keep only the overlapping volume</div>
      <div><code>intersection(cube(10), sphere(7)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#hull">hull</a>(obj1, obj2, ...)</code></div>
      <div>Create the convex hull of objects</div>
      <div><code>hull(cube(3), sphere(2).right(10)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#fill">fill</a>(obj1, obj2, ...)</code></div>
      <div>Fill concavities in a 2D shape</div>
      <div><code>fill(polygon(pts)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#minkowski">minkowski</a>(obj1, obj2)</code></div>
      <div>Minkowski sum of two objects</div>
      <div><code>minkowski(cube(10), sphere(2)).show()</code></div>

      <div class="func"><code><a href="../reference/booleans/#concat">concat</a>(obj1, obj2, ...)</code></div>
      <div>Concatenate meshes without boolean operations</div>
      <div><code>concat(part1, part2, part3).show()</code></div>
  </div>
</div>

## Extrusions

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/extrusions/#linear_extrude">linear_extrude</a>(obj, height, twist, ...)</code></div>
      <div>Extrude a 2D shape or function linearly</div>
      <div><code>circle(5).linear_extrude(height=10).show()</code></div>

      <div class="func"><code><a href="../reference/extrusions/#rotate_extrude">rotate_extrude</a>(obj, angle, v, ...)</code></div>
      <div>Extrude a 2D shape by rotating; supports helix via v</div>
      <div><code>circle(3).right(10).rotate_extrude(angle=360).show()</code></div>

      <div class="func"><code><a href="../reference/extrusions/#path_extrude">path_extrude</a>(obj, path, ...)</code></div>
      <div>Extrude a 2D shape along an arbitrary 3D path</div>
      <div><code>path_extrude(square(1), [[0,0,0],[0,0,10],[10,0,10]]).show()</code></div>

      <div class="func"><code><a href="../reference/extrusions/#skin">skin</a>(obj1, obj2, ..., segments, interpolate)</code></div>
      <div>Skin across multiple 2D profiles placed in 3D space</div>
      <div><code>skin(square(4).roty(40), circle(2).up(10)).show()</code></div>
  </div>
</div>

## Operators on Solids

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/operators/#add">obj + [x,y,z]</a></code></div>
      <div>Translate object by displacement vector</div>
      <div><code>(cube(5) + [10, 0, 0]).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#subtract">obj1 - obj2</a></code></div>
      <div>Difference (or translate by negated vector)</div>
      <div><code>(cube(10) - sphere(7)).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#multiply">obj * factor</a></code></div>
      <div>Scale object by a factor or vector</div>
      <div><code>(cube(5) * 2).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#or">obj1 | obj2</a></code></div>
      <div>Union of two objects</div>
      <div><code>(cube(5) | sphere(3)).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#and">obj1 &amp; obj2</a></code></div>
      <div>Intersection of two objects</div>
      <div><code>(cube(10) &amp; sphere(7)).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#matmul">obj @ matrix</a></code></div>
      <div>Apply a 4x4 transformation matrix (multmatrix)</div>
      <div><code>(cube(5) @ mat).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#xor">obj1 ^ obj2</a></code></div>
      <div>Hull of two objects (or explode with vector)</div>
      <div><code>(cube(3) ^ sphere(2).right(10)).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#mod">obj % vector</a></code></div>
      <div>Minkowski (with solid) or rotate (with vector)</div>
      <div><code>(cube(10) % sphere(1)).show()</code></div>

      <div class="func"><code><a href="../reference/operators/#unary">+obj / -obj / ~obj</a></code></div>
      <div>Debug modifiers: highlight / background / show-only</div>
      <div><code>show(+cube(5) | -sphere(3))</code></div>
  </div>
</div>

## Object Properties and Attributes

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/properties/#size">obj.size</a></code></div>
      <div>Bounding box dimensions [w, h, d] or [w, h]</div>
      <div><code>print(cube(10).size)</code></div>

      <div class="func"><code><a href="../reference/properties/#position">obj.position</a></code></div>
      <div>Bounding box minimum corner coordinates</div>
      <div><code>print(cube(10).position)</code></div>

      <div class="func"><code><a href="../reference/properties/#bbox">obj.bbox</a></code></div>
      <div>Bounding box as a solid (cube for 3D, square for 2D)</div>
      <div><code>cube(10).bbox.color("red").show()</code></div>

      <div class="func"><code><a href="../reference/properties/#mesh">obj.mesh</a>(triangulate, color)</code></div>
      <div>Extract vertices and triangles as Python lists</div>
      <div><code>pts, tris = cube(10).mesh()</code></div>

      <div class="func"><code><a href="../reference/properties/#faces">obj.faces</a>(triangulate)</code></div>
      <div>Get a list of face solids with orientation matrices</div>
      <div><code>for f in sphere(5).faces(): f.show()</code></div>

      <div class="func"><code><a href="../reference/properties/#edges">obj.edges</a>()</code></div>
      <div>Get a list of edge solids from a face</div>
      <div><code>edges = square(10).edges()</code></div>

      <div class="func"><code><a href="../reference/properties/#inside">obj.inside</a>(point)</code></div>
      <div>Check if a point is inside the solid</div>
      <div><code>cube(10).inside([5, 5, 5])</code></div>

      <div class="func"><code><a href="../reference/properties/#children">obj.children</a>()</code></div>
      <div>Get child nodes as a tuple</div>
      <div><code>parts = union(cube(5), sphere(3)).children()</code></div>

      <div class="func"><code><a href="../reference/properties/#dynamic-attributes">obj.points / obj.paths / obj.matrix</a></code></div>
      <div>Access node-specific data (polygon points, face matrix, etc.)</div>
      <div><code>pts = my_polygon.points</code></div>
  </div>
</div>

## Object Model

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/object_model/#method-chaining">obj.method(...)</a></code></div>
      <div>All transforms and operations available as methods</div>
      <div><code>cube(5).translate([10,0,0]).color("red").show()</code></div>

      <div class="func"><code><a href="../reference/object_model/#clone">obj.clone()</a></code></div>
      <div>Create a deep copy of the solid</div>
      <div><code>copy = cube(5).clone()</code></div>

      <div class="func"><code><a href="../reference/object_model/#dict">obj.dict()</a></code></div>
      <div>Return the object's metadata dictionary</div>
      <div><code>print(obj.dict())</code></div>

      <div class="func"><code><a href="../reference/object_model/#attribute-access">obj["key"] / obj.key</a></code></div>
      <div>Store and retrieve arbitrary metadata on solids</div>
      <div><code>c["name"] = "my cube"; print(c["name"])</code></div>

      <div class="func"><code><a href="../reference/object_model/#iteration">for child in obj</a></code></div>
      <div>Iterate over child nodes (yields ChildRef)</div>
      <div><code>for ch in union(a, b): ch.show()</code></div>

      <div class="func"><code><a href="../reference/object_model/#memberfunction">memberfunction</a>(name, func)</code></div>
      <div>Register a user-defined method on all solids</div>
      <div><code>memberfunction("double", lambda s: s.scale([2,2,2]))</code></div>
  </div>
</div>

## Mesh Operations

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/repair/#explode">explode</a>(obj, v)</code></div>
      <div>Explode a solid outward by a vector</div>
      <div><code>cube(5).explode([1,1,1]).show()</code></div>

      <div class="func"><code><a href="../reference/repair/#oversample">oversample</a>(obj, n, round)</code></div>
      <div>Subdivide mesh edges for finer detail</div>
      <div><code>cube(5).oversample(2).show()</code></div>

      <div class="func"><code><a href="../reference/repair/#debug">debug</a>(obj, faces)</code></div>
      <div>Visualize mesh faces for debugging</div>
      <div><code>cube(5).debug().show()</code></div>

      <div class="func"><code><a href="../reference/repair/#repair">repair</a>(obj, color)</code></div>
      <div>Make a solid watertight / manifold</div>
      <div><code>osimport("broken.stl").repair().show()</code></div>

      <div class="func"><code><a href="../reference/fillet/#fillet">fillet</a>(obj, r, sel, fn, minang)</code></div>
      <div>Add rounded fillets or chamfers to edges</div>
      <div><code>cube(10).fillet(2, fn=5).show()</code></div>

      <div class="func"><code><a href="../reference/repair/#separate">separate</a>(obj)</code></div>
      <div>Split a solid into disconnected parts</div>
      <div><code>parts = separate(my_solid)</code></div>
  </div>
</div>

## Display and Export

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/display/#show">show</a>(obj)</code></div>
      <div>Render and display the object in the viewport</div>
      <div><code>cube(10).show()</code></div>

      <div class="func"><code><a href="../reference/display/#export">export</a>(obj, file)</code></div>
      <div>Export object to a file (STL, 3MF, etc.)</div>
      <div><code>cube(10).export("output.stl")</code></div>

      <div class="func"><code><a href="../reference/display/#render">render</a>(obj, convexity)</code></div>
      <div>Force full geometry evaluation</div>
      <div><code>cube(10).render().show()</code></div>

      <div class="func"><code><a href="../reference/display/#projection">projection</a>(obj, cut)</code></div>
      <div>Project a 3D object to 2D</div>
      <div><code>sphere(10).projection(cut=True).show()</code></div>

      <div class="func"><code><a href="../reference/display/#group">group</a>(obj)</code></div>
      <div>Group objects without performing boolean operations</div>
      <div><code>group(cube(5) | sphere(3)).show()</code></div>

      <div class="func"><code><a href="../reference/display/#highlight">highlight</a> / <a href="../reference/display/#background">background</a> / <a href="../reference/display/#only">only</a></code></div>
      <div>Debug modifiers for visualization</div>
      <div><code>highlight(cube(5)).show()</code></div>
  </div>
</div>

## I/O and Integration

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/io/#osimport">osimport</a>(file, ...)</code></div>
      <div>Import geometry from file (STL, OFF, AMF, 3MF, SVG, DXF)</div>
      <div><code>osimport("model.stl").show()</code></div>

      <div class="func"><code><a href="../reference/io/#osuse">osuse</a>(file)</code></div>
      <div>Use an OpenSCAD library file (like <code>use &lt;...&gt;</code>)</div>
      <div><code>osuse("library.scad")</code></div>

      <div class="func"><code><a href="../reference/io/#osinclude">osinclude</a>(file)</code></div>
      <div>Include an OpenSCAD file (like <code>include &lt;...&gt;</code>)</div>
      <div><code>osinclude("config.scad")</code></div>

      <div class="func"><code><a href="../reference/io/#scad">scad</a>(code)</code></div>
      <div>Execute inline OpenSCAD code from Python</div>
      <div><code>scad("cube(10);")</code></div>

      <div class="func"><code><a href="../reference/io/#nimport">nimport</a>(url)</code></div>
      <div>Import a model from a network URL (GUI only)</div>
      <div><code>nimport("https://example.com/model.stl")</code></div>
  </div>
</div>

## Math Functions

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/math/#trigonometry">Sin</a> / Cos / Tan / Asin / Acos / Atan</code></div>
      <div>Trigonometric functions (degrees, like OpenSCAD)</div>
      <div><code>y = Sin(45)  # 0.7071...</code></div>

      <div class="func"><code><a href="../reference/math/#norm">norm</a>(vec)</code></div>
      <div>Calculate the length of a vector</div>
      <div><code>length = norm([3, 4])  # 5.0</code></div>

      <div class="func"><code><a href="../reference/math/#dot">dot</a>(vec1, vec2)</code></div>
      <div>Dot product of two vectors</div>
      <div><code>d = dot([1,0,0], [0,1,0])  # 0.0</code></div>

      <div class="func"><code><a href="../reference/math/#cross">cross</a>(vec1, vec2)</code></div>
      <div>Cross product of two 3D vectors</div>
      <div><code>c = cross([1,0,0], [0,1,0])  # [0,0,1]</code></div>
  </div>
</div>

## Special Variables

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/variables/#fn">fn</a> / <a href="../reference/variables/#fa">fa</a> / <a href="../reference/variables/#fs">fs</a></code></div>
      <div>Control roundness: number of segments / min angle / min size</div>
      <div><code>fn = 50; circle(10).show()</code></div>

      <div class="func"><code><a href="../reference/variables/#time">time</a> / <a href="../reference/variables/#phi">phi</a></code></div>
      <div>Animation step and phi = 2 * PI * time</div>
      <div><code>cube(5).rotate([0, 0, time * 360]).show()</code></div>
  </div>
</div>

## Customizer

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
  <div class="func"><code><a href="../reference/customizer/#add_parameter">add_parameter</a>(name, default)</code></div>
  <div>Add a customizer parameter</div>
  <div><code>width = add_parameter("width", 10)</code></div>

  <div class="func"><code><a href="../reference/customizer/#slider">add_parameter</a>(..., range=)</code></div>
  <div>Slider with min/max (use range() or tuple)</div>
  <div><code>add_parameter("x", 50, range=range(0, 101))</code></div>

  <div class="func"><code><a href="../reference/customizer/#dropdown">add_parameter</a>(..., options=)</code></div>
  <div>Dropdown menu</div>
  <div><code>add_parameter("c", "red", options=["red", "green"])</code></div>

  <div class="func"><code><a href="../reference/customizer/#groups">add_parameter</a>(..., group=)</code></div>
  <div>Organize into tabs</div>
  <div><code>add_parameter("x", 10, group="Size")</code></div>

  <div class="func"><code><a href="../reference/customizer/#description">add_parameter</a>(..., description=)</code></div>
  <div>Add help text</div>
  <div><code>add_parameter("x", 10, description="Width in mm")</code></div>

  <div class="func"><code><a href="../reference/customizer/#step">add_parameter</a>(..., step=)</code></div>
  <div>Spinbox with custom step</div>
  <div><code>add_parameter("angle", 45.0, step=0.5)</code></div>

  <div class="func"><code><a href="../reference/customizer/#max_length">add_parameter</a>(..., max_length=)</code></div>
  <div>String max length</div>
  <div><code>add_parameter("name", "hi", max_length=20)</code></div>
  </div>
</div>

## GUI and Utility

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/gui/#rendervars">rendervars</a>(vpd, vpf, vpr, vpt)</code></div>
      <div>Set camera/viewport from code</div>
      <div><code>rendervars(vpd=150, vpr=[55, 0, 25])</code></div>

      <div class="func"><code><a href="../reference/gui/#add_menuitem">add_menuitem</a>(menu, item, callback)</code></div>
      <div>Add a custom menu item to the GUI (GUI only)</div>
      <div><code>add_menuitem("Tools", "My Tool", my_func)</code></div>

      <div class="func"><code><a href="../reference/gui/#model">model</a>() / <a href="../reference/gui/#modelpath">modelpath</a>()</code></div>
      <div>Get the current model or script file path</div>
      <div><code>path = modelpath()</code></div>

      <div class="func"><code><a href="../reference/gui/#version">version</a>() / <a href="../reference/gui/#version_num">version_num</a>()</code></div>
      <div>Get PythonSCAD version info</div>
      <div><code>print(version())</code></div>

      <div class="func"><code><a href="../reference/gui/#marked">marked</a>(value)</code></div>
      <div>Create a marked F-Rep value (for libfive)</div>
      <div><code>m = marked(3.14)</code></div>

      <div class="func"><code><a href="../reference/gui/#machineconfig">machineconfig</a>(config)</code></div>
      <div>Set machine configuration dictionary</div>
      <div><code>machineconfig({"bed_x": 200, "bed_y": 200})</code></div>
  </div>
</div>

## Handles

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/align/#origin">obj.origin</a></code></div>
      <div>Default handle (identity matrix) on every solid</div>
      <div><code>c.top = translate(c.origin, [5, 5, 10])</code></div>

      <div class="func"><code><a href="../reference/align/#align">obj.align</a>(refmat, objmat, flip)</code></div>
      <div>Align object to a handle on another object</div>
      <div><code>cyl.align(c.right_center, cyl.origin).show()</code></div>
  </div>
</div>

## F-Rep / libfive

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code><a href="../reference/frep/#frep">frep</a>(exp, min, max, res)</code></div>
      <div>Mesh a signed distance function (libfive expression)</div>
      <div><code>frep(sdf, [-4,-4,-4], [4,4,4], 20).show()</code></div>

      <div class="func"><code><a href="../reference/frep/#ifrep">ifrep</a>(obj)</code></div>
      <div>Convert a mesh to a libfive implicit function</div>
      <div><code>tree = ifrep(cube(5))</code></div>

      <div class="func"><code><a href="../reference/frep/#libfive-module">import libfive as lv</a></code></div>
      <div>Low-level SDF building blocks: lv.x(), lv.y(), lv.z(), lv.sqrt(), lv.sin(), ...</div>
      <div><code>c = lv.x()**2 + lv.y()**2 + lv.z()**2 - 4</code></div>
  </div>
</div>

## Additional Resources

<div class="cheatsheet-section">
  <ul>
    <li><a href="https://pythonscad.org/">PythonSCAD Official Website</a></li>
    <li><a href="https://github.com/pythonscad/pythonscad">PythonSCAD GitHub Repository</a></li>
    <li><a href="https://www.reddit.com/r/OpenPythonSCAD/wiki/index/">r/OpenPythonSCAD Wiki</a></li>
    <li><a href="https://en.wikibooks.org/wiki/OpenSCAD_User_Manual">OpenSCAD User Manual (Wikibooks)</a></li>
  </ul>
</div>

<!-- do not alter anything below here-->

<script>
function searchFunction() {
  const input = document.getElementById("searchInput");
  const filter = input.value.toLowerCase();
  const sections = document.querySelectorAll(".cheatsheet-section");

  sections.forEach(section => {
    let matches = false;
    const items = section.querySelectorAll(".cheatsheet-grid > div");
    const preElements = section.querySelectorAll(".cheatsheet-grid > pre");

    // Loop through each <div> inside the grid (3 at a time per item)
    for (let i = 0; i < items.length; i += 3) {
      const func = items[i]?.textContent.toLowerCase() || "";
      const desc = items[i + 1]?.textContent.toLowerCase() || "";
      const example = items[i + 2]?.textContent.toLowerCase() || "";

      const visible = func.includes(filter) || desc.includes(filter) || example.includes(filter);
      // Show or hide the 3 related divs
      items[i].style.display = visible ? "" : "none";
      if (items[i + 1]) items[i + 1].style.display = visible ? "" : "none";
      if (items[i + 2]) items[i + 2].style.display = visible ? "" : "none";

      if (visible) matches = true;
    }

    // Also check pre elements for matches
    preElements.forEach(pre => {
      const content = pre.textContent.toLowerCase();
      const visible = content.includes(filter);
      pre.style.display = visible ? "" : "none";
      if (visible) matches = true;
    });

    // Hide entire section if no matches
    section.style.display = matches ? "" : "none";

    // Also hide/show the preceding heading (h2)
    const heading = section.previousElementSibling;
    if (heading && /^H[1-6]$/.test(heading.tagName)) {
      heading.style.display = matches ? "" : "none";
    }
  });
}
</script>
