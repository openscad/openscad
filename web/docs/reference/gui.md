# GUI and Utility Functions

## rendervars

Control the camera and viewport settings from your Python script. Useful for setting up specific viewing angles.

**Syntax:**

=== "Python"

```python
rendervars(vpd=None, vpf=None, vpr=None, vpt=None)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `vpd` | float | Viewport distance (camera distance from the object) |
| `vpf` | float | Field of view angle in degrees |
| `vpr` | `[x, y, z]` | Viewport rotation in degrees |
| `vpt` | `[x, y, z]` | Viewport translation (the point the camera looks at) |

All parameters are optional. Only the ones you provide will be changed.

**Examples:**

=== "Python"

```python
from openscad import *

cube(10, center=True).show()
rendervars(vpd=150, vpr=[55, 0, 25], vpt=[0, 0, 5])
```

---

## add_menuitem

Add a custom menu item to the PythonSCAD GUI. This function is only available in GUI mode.

**Syntax:**

=== "Python"

```python
add_menuitem(menuname, itemname, callback)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `menuname` | string | Name of the menu to add to (e.g. `"Tools"`) |
| `itemname` | string | Name of the menu item |
| `callback` | callable | Function to call when the item is clicked |

**Examples:**

=== "Python"

```python
from openscad import *

def my_action():
    print("Menu item clicked!")

add_menuitem("Tools", "My Custom Action", my_action)
```

---

## model

Return the current model object.

**Syntax:**

=== "Python"

```python
m = model()
```

**Returns:** The current model as a solid object.

---

## modelpath

Return the absolute file path of the current script.

**Syntax:**

=== "Python"

```python
path = modelpath()
```

**Returns:** A string with the absolute path to the currently running script file.

**Examples:**

=== "Python"

```python
from openscad import *

print(modelpath())
```

---

## version

Return the PythonSCAD version as a list.

**Syntax:**

=== "Python"

```python
v = version()
```

**Returns:** A list like `[year, month, day]`.

**Examples:**

=== "Python"

```python
from openscad import *

print(version())
```

**OpenSCAD reference:** [version](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#OpenSCAD_Version)

---

## version_num

Return the PythonSCAD version as a single number.

**Syntax:**

=== "Python"

```python
n = version_num()
```

**Returns:** An integer encoding the version.

---

## marked

Create a marked value for use with F-Rep / libfive. This wraps a numeric value as a libfive data type.

**Syntax:**

=== "Python"

```python
m = marked(value)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `value` | float | The numeric value to mark |

**Returns:** A libfive `PyData` scalar value.

---

## machineconfig

Set machine configuration parameters. This is used to configure machine-specific settings like bed size.

**Syntax:**

=== "Python"

```python
machineconfig(config)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `config` | dict | Dictionary of configuration key-value pairs |

**Examples:**

=== "Python"

```python
from openscad import *

machineconfig({"bed_x": 200, "bed_y": 200, "bed_z": 200})
```
