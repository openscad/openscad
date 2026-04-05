# Customizer

## add_parameter

Add a parameter that appears in the PythonSCAD Customizer GUI. Users can modify these parameters through the GUI without editing code.

**Syntax:**

=== "Python"

```python
value = add_parameter(name, default, description="", group="Parameters",
                      range=None, step=None, max_length=None, options=None)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | string | — | Parameter name (displayed in the GUI) |
| `default` | any | — | Default value (type determines the widget: int, float, bool, string, list) |
| `description` | string | `""` | Help text shown in the customizer |
| `group` | string | `"Parameters"` | Tab name for organizing parameters |
| `range` | range or tuple | `None` | Min/max for slider widgets |
| `step` | float | `None` | Step increment for spinbox widgets |
| `max_length` | int | `None` | Maximum string length |
| `options` | list or dict | `None` | Dropdown menu options |

**Returns:** The current value of the parameter (either the default or the user-modified value).

---

### Basic Usage

The parameter type is inferred from the default value:

=== "Python"

```python
from openscad import *

width = add_parameter("width", 10)          # integer
scale = add_parameter("scale", 1.5)         # float
name = add_parameter("name", "default")     # string
enabled = add_parameter("enabled", True)    # boolean (checkbox)

cube([width, width, width]).show()
```

---

### Slider

Use `range` to create a slider widget:

=== "Python"

```python
from openscad import *

# Integer slider (range() is exclusive, so use 101 for max 100)
quality = add_parameter("quality", 50, range=range(0, 101, 5))

# Float slider using tuple (min, max) or (min, max, step)
scale = add_parameter("scale", 1.0, range=(0.1, 10.0, 0.1))
```

---

### Step (Spinbox)

Use `step` for a spinbox without a slider:

=== "Python"

```python
from openscad import *

angle = add_parameter("angle", 45.0, step=0.5)
```

---

### Dropdown

Use `options` for a dropdown menu:

=== "Python"

```python
from openscad import *

# Simple list
color = add_parameter("color", "red", options=["red", "green", "blue"])

# Labeled options (value: label)
quality = add_parameter("quality", 10, options={10: "Low", 20: "Medium", 30: "High"})
```

---

### Groups

Organize parameters into tabs using `group`:

=== "Python"

```python
from openscad import *

width = add_parameter("width", 10, group="Dimensions")
height = add_parameter("height", 20, group="Dimensions")
color = add_parameter("color", "red", group="Appearance")

# Special groups
debug = add_parameter("debug", False, group="Hidden")    # not shown in UI
units = add_parameter("units", "mm", group="Global")     # appears on all tabs
```

---

### Description

Add help text with `description`:

=== "Python"

```python
from openscad import *

width = add_parameter("width", 10,
    description="Width of the model in mm",
    group="Dimensions")
```

---

### max_length

Limit string input length:

=== "Python"

```python
from openscad import *

name = add_parameter("name", "hello", max_length=20)
```

---

### Vectors

Vector parameters (lists) support constraints applied to all elements:

=== "Python"

```python
from openscad import *

size = add_parameter("size", [10, 20, 30], range=(1, 100, 1))
```
