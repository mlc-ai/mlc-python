import ctypes


class MLCHeader(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("type_index", ctypes.c_int32),
        ("ref_cnt", ctypes.c_int32),
        ("deleter", ctypes.c_void_p),
    ]


class DLDataType(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("code", ctypes.c_uint8),
        ("bits", ctypes.c_uint8),
        ("lanes", ctypes.c_uint16),
    ]


class DLDevice(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("device_type", ctypes.c_int),
        ("device_id", ctypes.c_int),
    ]


class MLCObjPtr(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("ptr", ctypes.c_void_p),
    ]
