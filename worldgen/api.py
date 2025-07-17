from typing import Callable, Dict, Any

_REGISTRY: Dict[str, Callable] = {}
_SCHEMA: Dict[str, Dict[str, Any]] = {}  # JSON schema per function

def register_api(name: str, schema: Dict[str, Any]):
    """
    Decorator to register a function and its schema for API and trace access.
    """
    def decorator(func: Callable) -> Callable:
        _REGISTRY[name] = func
        _SCHEMA[name] = schema
        return func
    return decorator

# Example of a function that could be registered in another module:
#
# @register_api("carve_cave", {"type": "object", "properties": {"seed": {"type": "integer"}}})
# def carve_cave(vox_array, seed):
#     # Implementation...
#     pass 