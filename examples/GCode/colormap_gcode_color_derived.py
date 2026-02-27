from openscad import *
from pymachineconfig import *

# read in the machine configuration file.
#
# Note: if one does not exist, it creates a default colormap, and
#   blank machine configuration.  For this example to work without a
#   pre-defined configuration we are using the "default_machine"
#   provided in the examples. (below)
import define_machine

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
# Use the direct power/feed method.
#
# for cut (power=100% and feed=401mm/min):
cut_color = mc.gen_color2str(power=1000,feed=401)
print("cut color:",cut_color)
# for engrave (power=55.0% and feed=6000mm/min)
engrave_color = mc.gen_color2str(power=550,feed=6000)
print("engrave color:",engrave_color)
# Note: the colors are specifically generated to encode the power and
#   feed into the RGB representation.  Since LaserGRBL and LightBurn
#   use 0..1000 to represent 0 to 100% power, the first 8 bits of the
#   color (ie Red) is used to for power.  This reduces power's step
#   resolution to 0.4% from 0.1%, and give 16-bits (GB of the RGB) to
#   feed.  If we reduce feed from 16-bits to 14-bits would result in
#   the maximum feed of 16K mm/min, which is less that some common
#   machine's capabilities.

# engrave the part
text_3d_2 = text_3d
text_3d_2 = text_3d_2.projection(cut=True)
text_3d_2 = text_3d_2.color(engrave_color)
text_3d_2 = text_3d_2.translate([38,10,10])
text_3d_2.show()

# now cut it out
plate_2 = plate
plate_2 = plate_2.projection(cut=True)
plate_2 = plate_2.color(cut_color)
plate_2.show()

model = text_3d_2 | plate_2
model.show()
model.export("colormap_gcode_color_derived.gcode")
