from openscad import *

test = osuse("call-openscad-function.scad")

print(f'no_parameter(): {test.no_parameter()}')
print(f'parameter(1): {test.parameter(1)}')
print(f'parameter(2.0): {test.parameter(2.0)}')
print(f'add(1, 2): {test.add(1, 2)}')
print(f'vector(1, 2, 3): {test.vector(1, 2, 3)}')
print(f'default_value(): {test.default_value()}')
print(f'override_default_value(10): {test.override_default_value(1)}')
