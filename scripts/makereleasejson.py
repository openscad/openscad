#!/usr/bin/env python3

# Used by github-release.sh

import sys
import json

v = sys.argv[1]
print(json.JSONEncoder().encode({
'tag_name': 'openscad-'+v,
'name': 'OpenSCAD '+v,
'body': open('./releases/'+v+'.md').read()
}))
