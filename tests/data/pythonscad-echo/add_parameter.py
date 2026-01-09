from openscad import *

# Test with default behavior (feature disabled)
# add_parameter should return the value AND create a global variable

# Test boolean values
result_true = add_parameter("bool_param_true", True)
print(f"Return: {result_true}")  # True
print(f"Global: {bool_param_true}")  # True

result_false = add_parameter("bool_param_false", False)
print(f"Return: {result_false}")  # False
print(f"Global: {bool_param_false}")  # False

# Test integer values
result_int = add_parameter("int_param", 42)
print(f"Return: {result_int}")  # 42
print(f"Global: {int_param}")  # 42

# Test float values
result_float = add_parameter("float_param", 3.14159)
print(f"Return: {result_float}")  # 3.14159
print(f"Global: {float_param}")  # 3.14159

# Test string values
result_str = add_parameter("str_param", "hello")
print(f"Return: {result_str}")  # hello
print(f"Global: {str_param}")  # hello
