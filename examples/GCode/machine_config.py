from openscad import *
from pymachineconfig import *
# set up a machine definition fron scratch.
# WARNING: excuting this can overwrite your config
import define_machine

# read in the machine config.  If it does not exist, generate a default
mc = MachineConfig()

# find the full path of the config file it wants to work with.
print("\nConfigfile:",mc.configfile())

working = mc.working_config()
print("working:",working)
print("\n############")
print("The default machine set to:")
for k in working.keys():
    print("  ",k,working[k])
print("############")

# test code
types = mc.get_types()
print("Types = ",str(types))
for t in types:
    ret = mc.get_label_by_type(t)
    print("    type: '%s'  values: %s"%(t,str(ret)))

cut_scale = mc.scale_value("cut_feed","max_feed")
print("cut scale:",cut_scale)

engrave_scale = mc.scale_value("engrave_feed","max_feed")
print("engrave scale:",engrave_scale)

test_power = mc.get_property_value("Creality-Falcon2", "max_feed")
print("max_power =",test_power)

mc.set_property_value("Creality-Falcon2",  "max_feed", 12000)

test_power = mc.get_property_value("Creality-Falcon2", "max_feed")
print("max_power =",test_power)

###############

#color_table = mc.get_property_value("ColorTable", "L03")
feed = mc.feed("L03")
print("color_table['L03']['feed']=",feed)

mc.set_feed("L03",450)

color_table = mc.feed("L03")
print("color_table['L03'] =",color_table)
print("   modified feed:",mc.feed("L03"))

###############

mc.reset_colormap()
print("   reset feed:",mc.feed("L03"))

print("L03 color:",mc.color2str("L03"))
