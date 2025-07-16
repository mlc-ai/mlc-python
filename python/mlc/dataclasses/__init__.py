from mlc.core.object import Object as PyClass  # for backward compatibility

from .c_class import c_class
from .py_class import py_class
from .utils import (
    Structure,
    add_vtable_method,
    field,
    prototype,
    replace,
    stringify,
    vtable_method,
)
