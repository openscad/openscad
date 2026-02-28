# PythonSCAD Cheat Sheet

<input type="text" id="searchInput" onkeyup="searchFunction()" placeholder="Search the cheat sheet...">

<!-- ************************************* -->
<!-- edit the sections below to add features, or edit existing -->

## 🧱 Primitives

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
      <div class="func"><code>cube([x, y, z])</code></div>
      <div>Create a cube with specified dimensions</div>
      <div><code>cube([10, 20, 30]).show()</code></div>

      <div class="func"><code>sphere(r)</code></div>
      <div>Create a sphere with radius r</div>
      <div><code>sphere(15).show()</code></div>

      <div class="func"><code>cylinder(h, r1, r2)</code></div>
      <div>Create a cylinder with height h and radii r1 and r2</div>
      <div><code>cylinder(h=20, r1=10, r2=5).show()</code></div>

      <div class="func"><code>polyhedron(points)</code></div>
      <div>Create a 3D polyhedron with given points and triangles</div>
      <div><code>polyhedron([[0,0], [10,0], [5,10]]).show()</code></div>

      <div class="func"><code>square([x, y])</code></div>
      <div>Create a 2D square</div>
      <div><code>square([10, 10]).show()</code></div>

      <div class="func"><code>circle(r)</code></div>
      <div>Create a 2D circle</div>
      <div><code>circle(5).show()</code></div>

      <div class="func"><code>polygon(points)</code></div>
      <div>Create a 2D polygon with given points</div>
      <div><code>polygon([[0,0], [10,0], [5,10]]).show()</code></div>

      <div class="func"><code>spline(points)</code></div>
      <div>Create a 2D polygon with given points</div>
      <div><code>spline([[0,0], [10,0], [5,10]],fn=20).show()</code></div>

  </div>
</div>

## 🔄 Transformations

```python
translate(obj, [x, y, z])       # Translate object by x, y, z
rotate(obj, [x, y, z])          # Rotate object by x, y, z degrees
scale(obj, [x, y, z])           # Scale obj by x, y, z factors
mirror([obj, x, y, z])          # Mirror object across the plane defined by x, y, z
right(obj, val)                 # shift  obj by val, alternatively left, front, back, up, down
obj + [x,y,z ]                  # translate object by displacement
rotx(obj, val)                  # Rotation in one axis , alternatively roty, rotz
obj * factor                    # scale obj by factor
```

## 🔗 Boolean Operations

```python
union(obj1, obj2)               # Union of obj1 and obj2, alternatively use obj1 | obj2
difference(obj1, obj2)          # Subtract obj2 from obj1, alternatively use obj1 - obj2
intersection(obj1, obj2)        # Intersection of obj1 and obj2, alternatively use obj1 &amp; obj2
```

## 🌀 Extrusions

```python
linear_extrude(shape, height=5) # Extrude 2D shape linearly
rotate_extrude(shape, angle=45) # Extrude 2D shape by rotating it
path_extrude(square(3), path)   # Extrude shape along a specified path
```

## 🎨 Appearance

```python
color(obj, "red")               # Apply red color to object
color(obj, [r, g, b])           # Apply RGB color to object
```

## 🧰 Advanced Features

```python
hull()(obj1, obj2)              # Create convex hull of obj1 and obj2
minkowski(obj1, obj2)           # Minkowski sum of obj1 and obj2
offset(shape, delta)            # Offset shape by delta
```


## 🧪 Displaying Objects

```python
show(obj)                       # Render the object
export(obj,"output.3mf")        # Exporting to a file
```

## 📐 Object Properties

```python
obj.size                        # Bounding box dimensions [w, h, d] (3D) or [w, h] (2D)
obj.position                    # Bounding box min corner [x, y, z] (3D) or [x, y] (2D)
obj.bbox                        # Bounding box as a solid (cube for 3D, square for 2D)
```

## 🐍 Python-Specific Gadgets

```python
# Quickly arrange objects
show([ cube(4).right(5*i) for i in range(5) ])

# Modify the mesh
c = cube(5)
pts, tri = c.mesh()
for pt in pts:
if pt[2] &gt; 3 and pt[1] &gt; 3:
  pt[2] = pt[2] + 3
polyhedron(pts, tri).show()

show(pillar())
```

## 📌 Handles

```python
c = cube([10,10,10r])
c.right_side = translate(roty(c.origin, 90),[10,5,5]) # create handle on the right side of the cube
c |= cylinder(r=1,h=8).align(c.right_side)  # Attach a cylinder to the new handle
```

## 🐍 Special variables

```python
fn, fa, fs           # define roundness of things
time, phi            # like in openSCAD , phi = 2*PI*time
```

## 🎛️ Customizer

<div class="cheatsheet-section">
  <div class="cheatsheet-grid">
  <div class="func"><code>add_parameter(name, default)</code></div>
  <div>Add a customizer parameter</div>
  <div><code>width = add_parameter("width", 10)</code></div>

  <div class="func"><code>add_parameter(..., range=)</code></div>
  <div>Slider with min/max (use range() or tuple)</div>
  <div><code>add_parameter("x", 50, range=range(0, 101))</code></div>

  <div class="func"><code>add_parameter(..., options=)</code></div>
  <div>Dropdown menu</div>
  <div><code>add_parameter("c", "red", options=["red", "green"])</code></div>

  <div class="func"><code>add_parameter(..., group=)</code></div>
  <div>Organize into tabs</div>
  <div><code>add_parameter("x", 10, group="Size")</code></div>

  <div class="func"><code>add_parameter(..., description=)</code></div>
  <div>Add help text</div>
  <div><code>add_parameter("x", 10, description="Width in mm")</code></div>

  <div class="func"><code>add_parameter(..., step=)</code></div>
  <div>Spinbox with custom step</div>
  <div><code>add_parameter("angle", 45.0, step=0.5)</code></div>

  <div class="func"><code>add_parameter(..., max_length=)</code></div>
  <div>String max length</div>
  <div><code>add_parameter("name", "hi", max_length=20)</code></div>
  </div>
</div>

## 📚 Additional Resources

<div class="cheatsheet-section">
  <ul>
    <li><a href="https://pythonscad.org/">PythonSCAD Official Website</a></li>
    <li><a href="https://github.com/pythonscad/pythonscad">PythonSCAD GitHub Repository</a></li>
    <li><a href="https://www.reddit.com/r/OpenPythonSCAD/wiki/index/">r/OpenPythonSCAD Wiki</a></li>
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
