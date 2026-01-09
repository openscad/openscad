from openscad import *

# Test with pure function feature enabled
# add_parameter should ONLY return the value, NOT create a global variable

# Test boolean values
result_true = add_parameter("bool_param_true", True)
print(f"Return: {result_true}")  # True

# Verify global variable was NOT created
try:
    print(f"Global: {bool_param_true}")
    print("ERROR: Global variable should not exist")
except NameError:
    print("Global: undefined (correct)")

# Test integer values
result_int = add_parameter("int_param", 42)
print(f"Return: {result_int}")  # 42

try:
    print(f"Global: {int_param}")
    print("ERROR: Global variable should not exist")
except NameError:
    print("Global: undefined (correct)")

# Test float values
result_float = add_parameter("float_param", 3.14159)
print(f"Return: {result_float}")  # 3.14159

try:
    print(f"Global: {float_param}")
    print("ERROR: Global variable should not exist")
except NameError:
    print("Global: undefined (correct)")

# Test string values
result_str = add_parameter("str_param", "hello")
print(f"Return: {result_str}")  # hello

try:
    print(f"Global: {str_param}")
    print("ERROR: Global variable should not exist")
except NameError:
    print("Global: undefined (correct)")
