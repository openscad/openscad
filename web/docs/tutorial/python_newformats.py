# New File formats in PythonSCAD

## STEP Format

Although its not smooth, but uses same vertices as xxSCAD used, still its STEP format. It enables potential tool flows and might reduce the amount of programs.

## Foldable Postscript

PythonSCAD is able to turn your model into a Paper-Foldable model(if does not have too many faces and edges).
Just test it on a simple model and explore the limits.

## GCode suitable for Lasercutters

With the new polyline primitive and existing 2d primitives, PythonSCAD can be used to design Lasercut designs.


```py

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
polyline([[0,0],[180,0],[180,180],[0,180],[0,0]]).color("blue").show()
export("myrect.gcode")

```
