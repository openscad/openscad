"""
Test extended add_parameter() functionality.

Tests the new keyword arguments: description, group, range, step, max_length, options
"""

from openscad import *

# Test basic parameters (backward compatibility)
basic_int = add_parameter("basic_int", 42)
basic_float = add_parameter("basic_float", 3.14)
basic_str = add_parameter("basic_str", "hello")
basic_bool = add_parameter("basic_bool", True)

print(f"basic_int: {basic_int}")
print(f"basic_float: {basic_float}")
print(f"basic_str: {basic_str}")
print(f"basic_bool: {basic_bool}")

# Test description and group
with_desc = add_parameter("with_desc", 10, description="A parameter with description")
with_group = add_parameter("with_group", 20, group="MyGroup")
with_both = add_parameter("with_both", 30, description="Has both", group="MyGroup")

print(f"with_desc: {with_desc}")
print(f"with_group: {with_group}")
print(f"with_both: {with_both}")

# Test range with Python's range() - integer slider
slider_int = add_parameter("slider_int", 50, range=range(0, 101, 5))
print(f"slider_int: {slider_int}")

# Test range with tuple - float slider
slider_float = add_parameter("slider_float", 1.0, range=(0.0, 10.0, 0.1))
print(f"slider_float: {slider_float}")

# Test range without step
slider_no_step = add_parameter("slider_no_step", 25, range=(0, 50))
print(f"slider_no_step: {slider_no_step}")

# Test step only (spinbox)
spinbox = add_parameter("spinbox", 45.0, step=0.5)
print(f"spinbox: {spinbox}")

# Test string with max_length
str_limited = add_parameter("str_limited", "short", max_length=20)
print(f"str_limited: {str_limited}")

# Test vector parameter
vec3 = add_parameter("vec3", [10.0, 20.0, 30.0])
print(f"vec3: {vec3}")

# Test vector with range constraint
vec_range = add_parameter("vec_range", [5.0, 10.0, 15.0], range=(0, 100, 1))
print(f"vec_range: {vec_range}")

# Test options as list (dropdown)
dropdown_list = add_parameter("dropdown_list", "red", options=["red", "green", "blue"])
print(f"dropdown_list: {dropdown_list}")

# Test options as dict (labeled dropdown)
dropdown_dict = add_parameter("dropdown_dict", 10, options={10: "Low", 20: "Medium", 30: "High"})
print(f"dropdown_dict: {dropdown_dict}")

# Test numeric options
dropdown_nums = add_parameter("dropdown_nums", 1, options=[1, 2, 3, 4, 5])
print(f"dropdown_nums: {dropdown_nums}")

# Test special groups
hidden_param = add_parameter("hidden_param", False, group="Hidden")
global_param = add_parameter("global_param", "mm", group="Global")

print(f"hidden_param: {hidden_param}")
print(f"global_param: {global_param}")

# Test combined options
full_example = add_parameter("full_example", 50,
    description="A fully configured slider",
    group="Settings",
    range=range(0, 101, 10))
print(f"full_example: {full_example}")

print("All extended add_parameter tests passed!")
