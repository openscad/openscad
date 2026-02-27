from openscad import *
from pymachineconfig import *

# Create a blank machine config with only a  
mc = MachineConfig(None)

# define a default configuration
mc.register("default","default",
              {"machine":"Creality-Falcon2",
               "head":"LED-40",
               "material":"3mm_ply_LED-40"})

# different machine configurations
mc.register("Creality-Falcon2","machine",
              {"max_feed":25000, #(mm/min)
               "max_width":400, #(mm)
               "max_len":415, #(mm)
               "has_camera":False})
mc.register("XTool-S1","machine",
              {"max_feed":36000, #(mm/min)
               "max_width":319, #(mm)
               "max_len":498, #(mm)
               "has_camera":False})

# different heads which can be independent
# of a given machine
mc.register("LED-40","head",
              {"max_power":40.0,
               "wavelength":455,
               "has_air":True,
               "kerf": 0.075})
mc.register("LED-20","head",
              {"max_power":20.0,
               "wavelength":455,
               "has_air":True,
               "kerf": 0.075})

# materials which are dependent on the head
# characteristics. The machines assume units
# in mm and minutes.
mc.register("3mm_ply_LED-40","material",
              {"thickness":3.0,
               "cut_power":1000,
               "cut_feed":400,
               "engrave_power":300,
               "engrave_feed":6000})
mc.register("0.25in_ply_LED-40","material",
              {"thickness":6.35,
               "cut_power":1000,
               "cut_feed":200,
               "engrave_power":300,
               "engrave_feed":6000})
mc.register("0.75in_pine_LED-40","material",
              {"thickness":19.05,
               "cut_power":1000,
               "cut_feed":200,
               "engrave_power":300,
               "engrave_feed":6000})
               
# generate a working variable set from the default config.
mc.gen_working()

# grab a copy of the working config and write it out
working = mc.working_config()
print("\nHand created configuration.")
print("############")
print("working:",working)
print("############")
print("The default machine set to:")
for k in working.keys():
    print("    ",k,working[k])
print("############")

###############
# What is the name of the default config file.
print("Configfile:",mc.configfile())

###############
# save the configuration
mc.write()
# uncoment to write and save a backup
# mc.write(backup="./backup.json")

###############
# verify the configuration was saved by reading in and printing
mc2 = MachineConfig()
working = mc.working_config()
print("\nRead in saved config file")
print("============")
print("working:",working)
print("============")
print("The default machine set to:")
for k in working.keys():
    print("    ",k,working[k])
print("============")
print("end user defined machine")
print("\n\n")

