import os
import json
from openscad import *

"""
MachineConfig class which can be used to read lasercutter and 3D
printer machine and material configurations.  The config file is
cached as a JSON export of python dictionaries.
"""
class MachineConfig:

    _config = {}  # the config as read in from the config file
    _working = {} # the, possibly modified, collapsed working config

    def __init__(self, name="PythonSCAD.json"):
        try:
            self._config = self.read(name)
        except:
            if name is not None:
                print("Warning: config file non existant or not read.")
                print("   Generating default.")

            self.gen_color_table()
            self.register("default","default",
                          {"machine":None,
                           "head":None,
                           "material":None})

        self.gen_working(label="default")
        self.write_settings()

    def _check_lasermode(self, value):
        try:
            if self._config["default"]["property"]["lasermode"] != value:
                print("Error: lasermode masmatch.  Ignoring overwrite.")
        except:
            self._config["default"]["property"]["lasermode"] = value
            self.write_settings()

    def register(self, label, itype, iproperty):
        item = {"type":itype,"property":iproperty}
        self._config[label] = item
        self.write_settings()

    def gen_color_table(self):
        self.register("L00","ColorTable",
                      {"power":1,"feed":1,"color":0x000000FF})
        self.register("L01","ColorTable",
                      {"power":1,"feed":1,"color":0x0000FFFF})
        self.register("L02","ColorTable",
                      {"power":1,"feed":1,"color":0xFF0000FF})
        self.register("L03","ColorTable",
                      {"power":1,"feed":1,"color":0x00E000FF})
        self.register("L04","ColorTable",
                      {"power":1,"feed":1,"color":0xD0D000FF})
        self.register("L05","ColorTable",
                      {"power":1,"feed":1,"color":0xFF8000FF})
        self.register("L06","ColorTable",
                      {"power":1,"feed":1,"color":0x00E0E0FF})
        self.register("L07","ColorTable",
                      {"power":1,"feed":1,"color":0xFF00FFFF})
        self.register("L08","ColorTable",
                      {"power":1,"feed":1,"color":0xB4B4B4FF})
        self.register("L09","ColorTable",
                      {"power":1,"feed":1,"color":0x0000A0FF})
        self.register("L10","ColorTable",
                      {"power":1,"feed":1,"color":0xA00000FF})
        self.register("L11","ColorTable",
                      {"power":1,"feed":1,"color":0x00A000FF})
        self.register("L12","ColorTable",
                      {"power":1,"feed":1,"color":0xA0A000FF})
        self.register("L13","ColorTable",
                      {"power":1,"feed":1,"color":0xC08000FF})
        self.register("L14","ColorTable",
                      {"power":1,"feed":1,"color":0x00A0FFFF})
        self.register("L15","ColorTable",
                      {"power":1,"feed":1,"color":0xA000A0FF})
        self.register("L16","ColorTable",
                      {"power":1,"feed":1,"color":0x808080FF})
        self.register("L17","ColorTable",
                      {"power":1,"feed":1,"color":0x7D87B9FF})
        self.register("L18","ColorTable",
                      {"power":1,"feed":1,"color":0xBB7784FF})
        self.register("L19","ColorTable",
                      {"power":1,"feed":1,"color":0x4A6FE3FF})
        self.register("L20","ColorTable",
                      {"power":1,"feed":1,"color":0xD33F6AFF})
        self.register("L21","ColorTable",
                      {"power":1,"feed":1,"color":0x8CD78CFF})
        self.register("L22","ColorTable",
                      {"power":1,"feed":1,"color":0xF0B98DFF})
        self.register("L23","ColorTable",
                      {"power":1,"feed":1,"color":0xF6C4E1FF})
        self.register("L24","ColorTable",
                      {"power":1,"feed":1,"color":0xFA9ED4FF})
        self.register("L25","ColorTable",
                      {"power":1,"feed":1,"color":0x500A78FF})
        self.register("L26","ColorTable",
                      {"power":1,"feed":1,"color":0xB45A00FF})
        self.register("L27","ColorTable",
                      {"power":1,"feed":1,"color":0x004754FF})
        self.register("L28","ColorTable",
                      {"power":1,"feed":1,"color":0x86FA88FF})
        self.register("L29","ColorTable",
                      {"power":1,"feed":1,"color":0xFFDB66FF})
        self.register("T1","ColorTable",
                      {"power":1,"feed":1,"color":0xF36926FF})
        self.register("T2","ColorTable",
                      {"power":1,"feed":1,"color":0x0C96D9FF})

    def read(self, name="PythonSCAD.json"):
        name = self.configfile(name)
        with open(name, 'r', encoding='utf-8') as f:
            cfg = json.loads(f.read())
            return cfg
        
    def write(self, config=None, name="PythonSCAD.json", backup=None):
        name = self.configfile(name)

        if backup is not None:
            import shutil
            shutil.copyfile(name, backup)

        if config is None:
            config = self._config

        jstr = json.dumps(config, indent=4)

        if jstr is not None:
            with open(name,'w') as fout:
                fout.write(jstr)
        
        return

    def write_settings(self, config=None):
        if config is None:
            config = self._config

        mc = machineconfig(config)
        
    def set_config(self, config):
        self._config = config
        self.write_settings()

    def dict(self):
        return self._config

    def get_machine(self, label=None):
        default = self._config["default"]
        machine = default["machine"]
        head = default["head"]

        if label is None:
            return default
        else:
            return default[label]

    def set_working(self, config):
        self._working = config
        return

    def get_types(self):
        try:
            types = set([self._config[x]["type"] for x in self._config])
            return types
        except ValueError as e:
            print(f"An error occurred: {e}")
            return None
    
    def get_label_by_type(self, label):
        try:
            values = set([x for x in self._config
                          if self._config[x]["type"]==label])
            return values
        except ValueError as e:
            print(f"An error occurred: {e}")
            return None
    
    def get_property(self, label):
        try:
            return self._config[label]["property"]
        except ValueError as e:
            print(f"An error occurred: {e}")
            return None

    def working_config(self):
        return self._working
    
    def gen_working(self, label="default"):
        """
        Create a flat representation of all variables associated with
        the default (or user named) configuration
        """
        self._working = {}
        dcfg = self._config[label]["property"]

        try:
            for l in dcfg.keys():
                k = dcfg[l]
                tcfg = self._config[k]["property"]
                for tk in tcfg.keys():
                    self._working[tk] = tcfg[tk]
        except:
            pass

    def configfile(self, name="PythonSCAD.json"):
        name = os.path.expanduser(name)
        xdg = os.getenv("XDG_CONFIG_HOME")
        home = os.getenv("HOME")

        if '/'==name[0] or '\\'==name[0]:
            # FIXME: need to also handle 'C:' naming
            return name

        if (xdg is not None) and (os.path.exists(os.path.join(xgd,name))):
            return os.path.join(xgd,name)

        if home is not None:
            return os.path.join(home,".config","PythonSCAD",name)

        return name

    def get_property_value(self, label, tag):
        try:
            return self._config[label]["property"][tag]
        except ValueError as e:
            print(f"An error occurred: {e}")
            return None

    def set_property_value(self, label, tag, value):
        try:
            self._config[label]["property"][tag] = value
        except ValueError as e:
            print(f"An error occurred: {e}")
            return
        self.write_settings()

    # The followng functions are for manipulating the color table

    def reset_colormap(self):
        """
        reset_colormap: change potentially modified labeled power,
        feed, and color associations back to their default.  The
        default color table is compatible with LightBurn's

        """
        self.gen_color_table()
        self.write_settings()

    def scale_value(self, label1, label2, cfg=None):
        if cfg is None:
            cfg = self._working
        val = cfg[label1] / cfg[label2]
        return val

    def color(self, tag):
        self._check_lasermode(0)
        return self.get_property_value(tag, "color")

    # powermap - return the working labled power
    def power(self, tag):
        return self.get_property_value(tag, "power")

    # feedmap - return the working labled feed
    def feed(self, tag):
        return self.get_property_value(tag, "feed")

    # setpower - overwrite the working labeled power
    def set_power(self, tag, val):
        val = self.set_property_value(tag, "power", val)
        self.write_settings()
        return val

    # setfeed - overwrite the working labeled feed
    def set_feed(self, tag, val):
        val = self.set_property_value(tag, "feed", val)
        self.write_settings()
        return val

    # setcolor - overwrite the working labeled color
    def set_color(self, tag, val):
        val = self.set_property_value(tag, "color", val)
        self.write_settings()
        return val

    def gen_color(self, power=-1,feed=-1):
        self._check_lasermode(1)
        color = 0
        power = int(power)
        feed  = int(feed)
        if power > 1000:
            print("\nError: cannot represent a power factor greater than 100.0%\n")
            return color

        # The alpha bits are mangled into the RGB values to encode a
        # full 12-bits for for power levels 0..1000 and 20-bits for
        # the feed rate.  In order to make the visualizaion of the
        # alpha channel work, the top most 6-bits of the feed rate are
        # shifted into the alpha channel.
        if power != -1: color |= (power << 22)

        cfeed = feed & 0x3FFFFF
        a = (~(cfeed >> 16)) & 0xFF
        gb = (cfeed & 0xFFFF) << 8

        if feed  != -1: color |= (gb | a)

        return color

    # color2str - return the working labled color as an OpenSCAD
    #   compatible string representation of the hex value starting
    #   with a '#'
    def color2str(self, tag):
        return "#{:08X}".format(self.color(tag))

    def gen_color2str(self, power=-1,feed=-1):
        color = self.gen_color(power,feed)
        return f"#{color:08X}"

    def gen_color2hex(self, power=-1,feed=-1):
        color = self.gen_color(power,feed)
        return f"0x{color:08X}"

