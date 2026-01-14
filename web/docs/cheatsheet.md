<style>
body {
    font-family: Arial, sans-serif;
    margin: 2em;
    background-color: #f9f9f9;
}

h1, h2 {
    color: #2c3e50;
}

#searchInput {
    width: 100%;
    font-size: 16px;
    padding: 12px 20px;
    margin-bottom: 24px;
    border: 1px solid #ccc;
    border-radius: 4px;
}
.section {
    margin-bottom: 2em;
}
.grid {
    display: grid;
    grid-template-columns: 1fr 2fr 2fr;
    gap: 1em;
    align-items: start;
}
.grid div {
    background-color: #ecf0f1;
    padding: 1em;
    border-radius: 5px;
    overflow-x: auto;
}

code {
    font-family: Consolas, monospace;
}
</style>
<h1>PythonSCAD Cheat Sheet</h1>

<input type="text" id="searchInput" onkeyup="searchFunction()" placeholder="Search the cheat sheet...">

<!-- ************************************* -->
<!-- edit the sections below to add features, or edit existing -->

<div class="section">
<h2>🧱 Primitives</h2>
<div class="grid">
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

  <div class="section">
    <h2>🔄 Transformations</h2>
    <div class="grid">
    <pre><code>translate(obj, [x, y, z])       # Translate object by x, y, z
rotate(obj, [x, y, z])          # Rotate object by x, y, z degrees
scale(obj, [x, y, z])           # Scale obj by x, y, z factors
mirror([obj, x, y, z])          # Mirror object across the plane defined by x, y, z
right(obj, val)                 # shift  obj by val, alternatively left, front, back, up, down
obj + [x,y,z ]                  # translate object by displacement
rotx(obj, val)                  # Rotation in one axis , alternatively roty, rotz
obj * factor                    # scale obj by factor
</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🔗 Boolean Operations</h2>
    <div class="grid">
    <pre><code>union(obj1, obj2)               # Union of obj1 and obj2, alternatively use obj1 | obj2
difference(obj1, obj2)          # Subtract obj2 from obj1, alternatively use obj1 - obj2
intersection(obj1, obj2)        # Intersection of obj1 and obj2, alternatively use obj1 &amp; obj2
    </code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🌀 Extrusions</h2>
    <div class="grid">
    <pre><code>linear_extrude(shape, height=5) # Extrude 2D shape linearly
rotate_extrude(shape, angle=45) # Extrude 2D shape by rotating it
path_extrude(square(3), path)   # Extrude shape along a specified path</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🎨 Appearance</h2>
    <div class="grid">
    <pre><code>color(obj, "red")               # Apply red color to object
color(obj, [r, g, b])           # Apply RGB color to object</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🧰 Advanced Features</h2>
    <div class="grid">
    <pre><code>hull()(obj1, obj2)              # Create convex hull of obj1 and obj2
minkowski(obj1, obj2)           # Minkowski sum of obj1 and obj2
offset(shape, delta)            # Offset shape by delta</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🧪 Displaying Objects</h2>
    <div class="grid">
    <pre><code>show(obj)                       # Render the object
export(obj,"output.3mf")        # Exporting to a file
</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>🐍 Python-Specific Gadgets</h2>
    <div class="grid">
    <pre><code> # Quickly arrange objects
show([ cube(4).right(5*i) for i in range(5) ])

# Modify the mesh
c = cube(5)
pts, tri = c.mesh()
for pt in pts:
  if pt[2] &gt; 3 and pt[1] &gt; 3:
    pt[2] = pt[2] + 3
polyhedron(pts, tri).show()

show(pillar())</code></pre>
    </div>
  </div>
  <div class="section">
    <h2>Handles</h2>
    <div class="grid">
    <pre><code>c = cube([10,10,10r])
c.right_side = translate(roty(c.origin, 90),[10,5,5]) # create handle on the right side of the cube
c |= cylinder(r=1,h=8).align(c.right_side)  # Attach a cylinder to the new handle</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>PythonSCAD "special" variables</h2>
    <div class="grid">
    <pre><code>fn, fa, fs           # define roundness of things
time, phi                # like in openSCAD , phi = 2*PI*time</code></pre>
    </div>
  </div>

  <div class="section">
    <h2>📚 Additional Resources</h2>
    <ul>
      <li><a href="https://pythonscad.org/">PythonSCAD Official Website</a></li>
      <li><a href="https://github.com/pythonscad/pythonscad">PythonSCAD GitHub Repository</a></li>
      <li><a href="https://www.reddit.com/r/OpenPythonSCAD/wiki/index/">r/OpenPythonSCAD Wiki</a></li>
    </ul>
  </div>




</-- **************************************************-->
<!-- do not alter anything below here-->


<script>
function searchFunction() {
  const input = document.getElementById("searchInput");
  const filter = input.value.toLowerCase();
  const sections = document.querySelectorAll(".section");

  sections.forEach(section => {
    let matches = false;
    const items = section.querySelectorAll(".grid > div");

    // Loop through each <div> inside the grid (3 at a time per item)
    for (let i = 0; i < items.length; i += 3) {
      const func = items[i]?.textContent.toLowerCase() || "";
      const desc = items[i + 1]?.textContent.toLowerCase() || "";
      const example = items[i + 2]?.textContent.toLowerCase() || "";

      const visible = func.includes(filter) || desc.includes(filter) || example.includes(filter);
      // Show or hide the 3 related divs
      items[i].style.display = visible ? "" : "none";
      items[i + 1].style.display = visible ? "" : "none";
      items[i + 2].style.display = visible ? "" : "none";

      if (visible) matches = true;
    }

    // Hide entire section if no matches
    section.style.display = matches ? "" : "none";
  });
}
</script>
