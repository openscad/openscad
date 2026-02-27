from openscad import *
from pymachineconfig import *

# FIXME: overwrite the machine config file with the default for testing
import define_machine

# read in the machine configuration file.
# Note: if one does not exist, it creates default structures
#   to work from, but does not save the file.  The end user
#   needs to do that explicitly.
mc = MachineConfig()
print("\nConfigfile:",mc.configfile())

# overwrite the init-/exit-Codes (with comments)
mc.set_property_value("default", "initCode", "; Testing initCode\r\nG90\r\n")
mc.set_property_value("default", "exitCode", "; Testing exitCode\r\nG0 X0 Y0 Z0\r\n")

# display the working parameters that are read in or created
print("\n############")
working = mc.working_config()
print("The default machine set to:")
for k in working.keys():
    print("  ",k,working[k])
print("############")

# Define parameters for the part
base_width = 75
base_height = 20
base_thickness = 5
engrave_depth = 1
text_string = "PythonSCAD"
font_size = 8

# Create the Base Plate
plate = cube([base_width, base_height, base_thickness])

# Create the Text (using linear_extrude to give it 
text_3d = text(text_string, size=font_size, font="Arial:style=Bold", halign="center", valign="center").linear_extrude(height=base_thickness)

# Position the text
# We need to move the text into the center of the plate and slightly
# above the bottom surface to avoid rendering artifacts (z-fighting)
positioned_text = text_3d.translate([base_width/2, base_height/2, base_thickness - engrave_depth])

# Perform the Engraving (Difference)
#   Subtract the text shape from the base plate
engraved_object = plate - positioned_text

# Render in 3D (commented out to test ExportGCode
#engraved_object.show()

#############################
# Since the GCode generated for the laser cutter does not utilize
# depth (z), offset the engraved portions so that it is not masked by
# the cut parts, but it will be rendered flat on the plane.
#
# Use the color-table power/feed method.  First modify the parameters
#
# for cut:
mc.set_power("L02",900)
mc.set_feed("L02",4000)
# for engrave:
mc.set_power("L01",350)
mc.set_feed("L01",6000)
# Note: the default colors named L01 and L02 are Blue and Red
#   respectively.  The color Red is often associated with cutting,
#   and Blue with engraving.

# engrave the second part
engrave_color = mc.color2str("L01")
text_3d_3 = text_3d
text_3d_3 = text_3d_3.projection(cut=True)
text_3d_3 = text_3d_3.color(engrave_color)
text_3d_3 = text_3d_3.translate([38,10,10])
text_3d_3.show()

# cut the second part
cut_color = mc.color2str("L02")
plate_3 = plate
plate_3 = plate_3.projection(cut=True)
plate_3 = plate_3.color(cut_color)
plate_3.show()

print("cut color (%s): %s"%("L02",cut_color))
print("engrave color (%s): %s"%("L01",engrave_color))

# Note: the working machine's config is cached and sent to
#   export_gcode so that any unszaved colormap modifications are
#   avialable at runtime.  Also, the actual saved colormap does not
#   require modification for each use.

model = text_3d_3 | plate_3
model.show()
model.export("colormap_gcode_colormapped.gcode")
