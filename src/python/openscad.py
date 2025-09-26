import _openscad

class OpenSCADWrapper:
    def __init__(self, openscad_obj):
        self._obj = openscad_obj

    def __getattr__(self, name):
        attr = getattr(self._obj, name)
        # If it's a method that returns a PyOpenSCAD object, wrap the result
        if callable(attr):
            def wrapped_method(*args, **kwargs):
                result = attr(*args, **kwargs)
                # Check if result is a PyOpenSCAD object that should be wrapped
                if hasattr(result, '__class__') and result.__class__.__name__ == 'PyOpenSCAD':
                    return OpenSCADWrapper(result)
                return result
            return wrapped_method
        return attr

    def _wrap_if_needed(self, result):
        """Helper to wrap PyOpenSCAD objects"""
        if hasattr(result, '__class__') and result.__class__.__name__ == 'PyOpenSCAD':
            return OpenSCADWrapper(result)
        return result

    # Operator overloads - delegate to the wrapped object and wrap results
    def __or__(self, other):
        """Union operator |"""
        other_obj = other._obj if isinstance(other, OpenSCADWrapper) else other
        return self._wrap_if_needed(self._obj | other_obj)

    def __and__(self, other):
        """Intersection operator &"""
        other_obj = other._obj if isinstance(other, OpenSCADWrapper) else other
        return self._wrap_if_needed(self._obj & other_obj)

    def __sub__(self, other):
        """Difference operator -"""
        other_obj = other._obj if isinstance(other, OpenSCADWrapper) else other
        return self._wrap_if_needed(self._obj - other_obj)

    def __add__(self, other):
        """Addition operator + (if defined)"""
        other_obj = other._obj if isinstance(other, OpenSCADWrapper) else other
        return self._wrap_if_needed(self._obj + other_obj)

    def center(self):
        return OpenSCADWrapper(self._obj.translate([0 - (self._obj.position[i] + self._obj.size[i] / 2) for i in range(3)]))

    def hello_world(self):
        # Your convenience method
        return "Hello, World!"

# Get all callable functions from _openscad module
_openscad_functions = [name for name in dir(_openscad) if callable(getattr(_openscad, name)) and not name.startswith('_')]

# Dynamically create wrapper functions
def _create_wrapper_function(func_name):
    """Creates a wrapper function for the given openscad function"""
    original_func = getattr(_openscad, func_name)

    def wrapper(*args, **kwargs):
        result = original_func(*args, **kwargs)
        # Check if result is a PyOpenSCAD object that should be wrapped
        if hasattr(result, '__class__') and result.__class__.__name__ == 'PyOpenSCAD':
            return OpenSCADWrapper(result)
        return result

    # Copy function metadata
    wrapper.__name__ = func_name
    wrapper.__doc__ = getattr(original_func, '__doc__', None)
    return wrapper

# Add all openscad functions to this module's namespace
globals().update({func_name: _create_wrapper_function(func_name) for func_name in _openscad_functions})
