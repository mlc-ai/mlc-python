# cython: language_level=3
from libc.stdint cimport int32_t, int64_t, uint64_t, uint8_t, uint16_t
from libc.stdlib cimport malloc, free
from numbers import Integral, Number
from cpython cimport Py_DECREF, Py_INCREF
from pathlib import Path
import ctypes
import platform
from .exception_utils import translate_exception_to_c, translate_exception_from_c

cdef extern from "mlc/ffi/c_api.h" nogil:
    ctypedef void (*MLCDeleterType)(void *) nogil # no-cython-lint
    ctypedef void (*MLCFuncCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret) nogil # no-cython-lint
    ctypedef int32_t (*MLCFuncSafeCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret) noexcept nogil # no-cython-lint

    ctypedef struct DLDataType:
        uint8_t code
        uint8_t bits
        uint16_t lanes

    ctypedef enum DLDeviceType:
        kDLCPU = 0

    ctypedef struct DLDevice:
        DLDeviceType device_type
        int32_t device_id

    ctypedef struct DLTensor:
        void* data
        DLDevice device
        int32_t ndim
        DLDataType dtype
        int64_t* shape
        int64_t* strides
        uint64_t byte_offset

    ctypedef struct DLManagedTensor:
        DLTensor dl_tensor
        void* manager_ctx
        MLCDeleterType deleter

    cdef enum MLCTypeIndex:
        kMLCNone = 0
        kMLCInt = 1
        kMLCFloat = 2
        kMLCPtr = 3
        kMLCDataType = 4
        kMLCDevice = 5
        kMLCRawStr = 6
        kMLCStaticObjectBegin = 64
        kMLCObject = 64
        kMLCList = 65
        kMLCDict = 66
        kMLCError = 67
        kMLCFunc = 68
        kMLCStr = 69
        kMLCDynObjectBegin = 128

    ctypedef struct MLCAny:
        int32_t type_index
        # union {
        int32_t ref_cnt
        int32_t small_len
        # }
        # union {
        int64_t v_int64
        double v_float64
        DLDataType v_dtype
        DLDevice v_device
        void* v_ptr
        const char* v_str
        MLCAny* v_obj
        MLCDeleterType deleter
        char[8] v_bytes
        # }

    ctypedef int32_t (*MLCAttrSetter)(void *addr, MLCAny *view) noexcept nogil # no-cython-lint
    ctypedef int32_t (*MLCAttrGetter)(void *addr, MLCAny *view) noexcept nogil # no-cython-lint

    ctypedef struct MLCError:
        MLCAny _mlc_header
        const char *kind

    ctypedef struct MLCStr:
        MLCAny _mlc_header
        int64_t length
        char *data

    ctypedef struct MLCFunc:
        MLCAny _mlc_header
        MLCFuncCallType call
        MLCFuncSafeCallType safe_call

    ctypedef struct MLCList:
        MLCAny _mlc_header
        int64_t capacity
        int64_t size
        void* data

    ctypedef struct MLCDict:
        MLCAny _mlc_header
        int64_t capacity
        int64_t size
        void* data

    ctypedef struct MLCTypeField:
        const char* name
        int64_t offset
        MLCAttrGetter getter
        MLCAttrSetter setter

    ctypedef struct MLCTypeMethod:
        const char* name
        MLCFunc *func

    ctypedef struct MLCTypeInfo:
        int32_t type_index
        char* type_key
        int32_t type_depth
        int32_t* type_ancestors
        MLCTypeField *fields
        MLCTypeMethod *methods

    ctypedef void* MLCTypeTableHandle

SYSTEM = platform.system()
DSO_SUFFIX = ".so" if SYSTEM == "Linux" else ".dylib" if SYSTEM == "Darwin" else ".dll"
LIB_PATH = Path(__file__).parent.parent / "lib" / f"libmlc_registry{DSO_SUFFIX}"
if not LIB_PATH.exists():
    raise FileNotFoundError(f"Cannot find the MLC registry library at {LIB_PATH}")
LIB = ctypes.CDLL(str(LIB_PATH), ctypes.RTLD_GLOBAL)

ctypedef MLCAny (*_T_GetLastError)() noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyIncRef)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyDecRef)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyInplaceViewToOwned)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncCreate)(void* py_func, MLCDeleterType deleter, MLCFuncSafeCallType safe_call, MLCAny* ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncSetGlobal)(void*, const char *name, MLCAny func, int allow_override) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncGetGlobal)(void*, const char *name, MLCAny *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncSafeCall)(MLCFunc* py_func, int32_t num_args, MLCAny *args, MLCAny* ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeIndex2Info)(MLCTypeTableHandle self, int32_t type_index, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeKey2Info)(MLCTypeTableHandle self, const char* type_key, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeRegister)(MLCTypeTableHandle self, int32_t parent_type_index, const char *type_key, int32_t type_index, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeDefReflection)(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields, MLCTypeField *fields, int64_t num_methods, MLCTypeMethod *methods) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableSet)(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableGet)(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ErrorCreate)(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret) noexcept nogil  # no-cython-lint
ctypedef int32_t (*_T_ErrorGetInfo)(MLCAny error, int32_t* num_strs, const char*** strs) noexcept nogil # no-cython-lint

cdef uint64_t _sym(object symbol):
    symbol: ctypes.CDLL._FuncPtr
    return <uint64_t>(ctypes.cast(symbol, ctypes.c_void_p).value)

cdef _T_GetLastError _C_GetLastError = <_T_GetLastError>_sym(LIB.MLCGetLastError) # no-cython-lint
cdef _T_AnyIncRef _C_AnyIncRef = <_T_AnyIncRef>_sym(LIB.MLCAnyIncRef) # no-cython-lint
cdef _T_AnyDecRef _C_AnyDecRef = <_T_AnyDecRef>_sym(LIB.MLCAnyDecRef) # no-cython-lint
cdef _T_AnyInplaceViewToOwned _C_AnyInplaceViewToOwned = <_T_AnyInplaceViewToOwned>_sym(LIB.MLCAnyInplaceViewToOwned) # no-cython-lint
cdef _T_FuncCreate _C_FuncCreate = <_T_FuncCreate>_sym(LIB.MLCFuncCreate) # no-cython-lint
cdef _T_FuncSetGlobal _C_FuncSetGlobal = <_T_FuncSetGlobal>_sym(LIB.MLCFuncSetGlobal) # no-cython-lint
cdef _T_FuncGetGlobal _C_FuncGetGlobal = <_T_FuncGetGlobal>_sym(LIB.MLCFuncGetGlobal) # no-cython-lint
cdef _T_FuncSafeCall _C_FuncSafeCall = <_T_FuncSafeCall>_sym(LIB.MLCFuncSafeCall) # no-cython-lint
cdef _T_TypeIndex2Info _C_TypeIndex2Info = <_T_TypeIndex2Info>_sym(LIB.MLCTypeIndex2Info) # no-cython-lint
cdef _T_TypeKey2Info _C_TypeKey2Info = <_T_TypeKey2Info>_sym(LIB.MLCTypeKey2Info) # no-cython-lint
cdef _T_TypeRegister _C_TypeRegister = <_T_TypeRegister>_sym(LIB.MLCTypeRegister) # no-cython-lint
cdef _T_TypeDefReflection _C_TypeDefReflection = <_T_TypeDefReflection>_sym(LIB.MLCTypeDefReflection) # no-cython-lint
cdef _T_VTableSet _C_VTableSet = <_T_VTableSet>_sym(LIB.MLCVTableSet) # no-cython-lint
cdef _T_VTableGet _C_VTableGet = <_T_VTableGet>_sym(LIB.MLCVTableGet) # no-cython-lint
cdef _T_ErrorCreate _C_ErrorCreate = <_T_ErrorCreate>_sym(LIB.MLCErrorCreate) # no-cython-lint
cdef _T_ErrorGetInfo _C_ErrorGetInfo = <_T_ErrorGetInfo>_sym(LIB.MLCErrorGetInfo) # no-cython-lint

PyCode_NewEmpty = ctypes.pythonapi.PyCode_NewEmpty
PyCode_NewEmpty.restype = ctypes.py_object

cdef list TYPE_TABLE = []
cdef list BUILTIN_STR = []

cdef inline object _list_get(list source, int index):
    return source[index] if index < len(source) else None

cdef inline object _list_set(list source, int index, object value):
    cdef int delta = index + 1 - len(source)
    if delta > 0:
        source.extend([None] * delta)
    prev, source[index] = source[index], value
    return prev

cdef inline void _check_error(int32_t err_code):
    if err_code == 0:
        return
    if err_code == -1:
        exception = RuntimeError(Str._new_from_mlc_any(_C_GetLastError()))
    elif err_code == -2:
        exception = translate_exception_from_c(PyAny._new_from_mlc_any(_C_GetLastError()))
    else:
        raise RuntimeError("MLC function call failed with error code: %d" % err_code)
    raise exception

cdef inline void _check_error_from(int32_t err_code, MLCAny* c_ret):
    if err_code == 0:
        return
    if err_code == -1:
        exception = RuntimeError(Str._new_from_mlc_any(c_ret[0]))
    elif err_code == -2:
        exception = translate_exception_from_c(PyAny._new_from_mlc_any(c_ret[0]))
    else:
        raise RuntimeError("MLC function call failed with error code: %d" % err_code)
    _mlc_any_clear(c_ret)
    raise exception

cdef inline void _mlc_any_clear(MLCAny* x):
    x.type_index = kMLCNone
    x.ref_cnt = 0
    x.v_int64 = 0

cdef class PyAny:
    cdef MLCAny _mlc_any

    def __cinit__(self):
        _mlc_any_clear(&self._mlc_any)

    def __init__(self):
        pass

    def _mlc_init(self, init_func, *init_args) -> None:
        cdef PyAny func
        try:
            if init_func is None:
                init_func = "__init__"
            if isinstance(init_func, str):
                func = type(self)._mlc_vtable[init_func]
            else:
                func = <PyAny>init_func
            assert func._mlc_any.type_index == kMLCFunc
        except:  # no-cython-lint
            raise TypeError(
                "Unsupported type of `init_func`. "
                f"Expected `str` or `mlc.Func`, but got: {type(init_func)}"
            )
        _func_call_impl(<MLCFunc*>(func._mlc_any.v_obj), init_args, &self._mlc_any)

    @staticmethod
    def _C(str name, *args):
        cdef PyAny self = args[0]
        cdef PyAny func = type(self)._mlc_vtable[name]
        return func_call(func, args)

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

    @staticmethod
    cdef inline PyAny from_raw(MLCAny* source):
        cdef PyAny ret = PyAny.__new__(PyAny)
        ret._mlc_any = source[0]
        return ret

    @staticmethod
    cdef inline object _new_from_mlc_any(const MLCAny source):
        cdef int32_t type_index = source.type_index
        cdef object type_cls = _list_get(TYPE_TABLE, type_index)
        cdef PyAny ret
        if type_index == kMLCNone:
            return None
        if type_index == kMLCStr:
            return Str._new_from_mlc_any(source)
        ret = type_cls.__new__(type_cls)
        ret._mlc_any = source
        return ret

    def __str__(self) -> str:
        global BUILTIN_STR
        cdef object f_str = _list_get(BUILTIN_STR, self._mlc_any.type_index)
        assert f_str is not None, f"Cannot find `__str__` for type: {type(self)}"
        return func_call(f_str, (self,))

cdef class Str(str):
    cdef MLCAny _mlc_any
    __slots__ = ()

    @staticmethod
    cdef inline Str _new_from_mlc_any(const MLCAny source):
        cdef MLCStr* mlc_str = <MLCStr*>(source.v_obj)
        cdef Str ret = Str.__new__(Str, str_c2py(mlc_str.data[:mlc_str.length]))
        ret._mlc_any = source
        return ret

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))


cdef inline object _any_c2py(const MLCAny x):
    # Note: Ownership is left for the caller to manage properly
    # i.e. if the caller wants to share ownership, it has to call
    # `MLCAnyIncRef` accordingly on its end.
    cdef int type_index = x.type_index
    cdef PyAny ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int>(x.v_int64)
    elif type_index == kMLCFloat:
        return <float>(x.v_float64)
    elif type_index == kMLCPtr:
        return ctypes.cast(<unsigned long long>x.v_ptr, ctypes.c_void_p)
    elif type_index == kMLCRawStr:
        return str_c2py(x.v_str)
    elif type_index == kMLCStr:
        return Str._new_from_mlc_any(x)
    elif (type_cls := _list_get(TYPE_TABLE, type_index)) is not None:
        ret = type_cls.__new__(type_cls)
        ret._mlc_any = x
        return ret
    else:
        raise TypeError(f"MLC does not recognize type index: {type_index}")

cdef inline MLCAny _any_py2c(object x, list temporary_storage):
    cdef MLCAny y
    y.ref_cnt = 0
    if x is None:
        y.type_index = <int64_t>kMLCNone
        y.v_int64 = 0
    elif isinstance(x, PyAny):
        y.type_index = (<PyAny>x)._mlc_any.type_index
        y.v_obj = (<PyAny>x)._mlc_any.v_obj
    elif isinstance(x, Str):
        y.type_index = (<Str>x)._mlc_any.type_index
        y.v_obj = (<Str>x)._mlc_any.v_obj
    elif isinstance(x, Integral):
        y.type_index = <int64_t>kMLCInt
        y.v_int64 = x
    elif isinstance(x, Number):
        y.type_index = <int64_t>kMLCFloat
        y.v_float64 = x
    elif isinstance(x, ctypes.c_void_p):
        if x.value is None or x.value == 0:
            y.type_index = <int64_t>kMLCNone
            y.v_int64 = 0
        else:
            y.type_index = <int64_t>kMLCPtr
            y.v_int64 = x.value
    elif isinstance(x, str):
        x = str_py2c(x)
        temporary_storage.append(x)
        y.type_index = <int64_t>kMLCRawStr
        y.v_str = x
    elif callable(x):
        x = PyAny._new_from_mlc_any(_func_py2c(x))
        temporary_storage.append(x)
        y = (<PyAny>x)._mlc_any
    elif isinstance(x, (tuple, list)):
        from ..list import List  # no-cython-lint
        x = List(x)
        temporary_storage.append(x)
        y = (<PyAny>x)._mlc_any
    elif isinstance(x, dict):
        from ..dict import Dict  # no-cython-lint
        x = Dict(x)
        temporary_storage.append(x)
        y = (<PyAny>x)._mlc_any
    else:
        raise TypeError(f"MLC does not recognize type: {type(x)}")
    return y

cdef void _pyobj_deleter(void* handle) noexcept nogil:
    with gil:
        try:
            Py_DECREF(<object>(handle))
        except Exception as exception:
            # TODO(@junrushao): Will need to handle exceptions more gracefully
            print(f"Error in _pyobj_deleter: {exception}")

cdef inline MLCAny _func_py2c(object py_func):
    cdef MLCAny c_ret
    _mlc_any_clear(&c_ret)
    Py_INCREF(py_func)
    _check_error(_C_FuncCreate(<void*>(py_func), _pyobj_deleter, func_safe_call, &c_ret))
    return c_ret

cdef inline int32_t _func_safe_call_impl(
    const void* py_func,
    int32_t num_args,
    const MLCAny* c_args,
    MLCAny* c_ret
):
    cdef object py_ret
    cdef list temporary_storage = []
    cdef list py_args = [None] * num_args
    try:
        for i in range(num_args):
            py_ret = py_args[i] = _any_c2py(c_args[i])
            if isinstance(py_ret, PyAny):
                _check_error(_C_AnyIncRef(&(<PyAny>py_ret)._mlc_any))
        py_ret = (<object>py_func)(*py_args)
        # FIXME(@junrushao): `c_ret` should be empty, otherwise there will be a leak
        assert c_ret[0].type_index == kMLCNone
        c_ret[0] = _any_py2c(py_ret, temporary_storage)
        _check_error(_C_AnyInplaceViewToOwned(c_ret))
    except Exception as exception:
        kind, num_bytes_info, bytes_info = translate_exception_to_c(exception)
        _check_error(_C_ErrorCreate(kind, num_bytes_info, bytes_info, c_ret))
        return -2
    return 0

cdef inline int32_t func_safe_call(
    const void* py_func,
    int32_t num_args,
    const MLCAny* c_args,
    MLCAny* c_ret
) noexcept nogil:
    with gil:
        return _func_safe_call_impl(py_func, num_args, c_args, c_ret)

cdef inline void _func_call_impl(MLCFunc* c_func, tuple py_args, MLCAny* c_ret):
    cdef int32_t err_code
    cdef list temporary_storage = []
    cdef int32_t num_args = len(py_args)
    cdef MLCAny* c_args = <MLCAny*> malloc(num_args * sizeof(MLCAny))
    try:
        for i in range(num_args):
            c_args[i] = _any_py2c(py_args[i], temporary_storage)
        with nogil:
            err_code = _C_FuncSafeCall(c_func, num_args, c_args, c_ret)
    finally:
        free(c_args)
    _check_error_from(err_code, c_ret)

cpdef object func_call(PyAny func, tuple py_args):
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v_obj)
    cdef MLCAny c_ret
    _mlc_any_clear(&c_ret)
    _func_call_impl(c_func, py_args, &c_ret)
    return _any_c2py(c_ret)

cdef inline MLCAny* _extract_obj_ptr(object source):
    cdef MLCAny ret
    if isinstance(source, Str):
        return (<Str>source)._mlc_any.v_obj
    if isinstance(source, PyAny):
        ret = (<PyAny>source)._mlc_any
        if ret.type_index >= kMLCStaticObjectBegin:
            return ret.v_obj
    raise TypeError(f"Invalid type: {type(source)}")

cdef inline MLCTypeInfo* _type_key2info(str type_key):
    cdef MLCTypeInfo* type_info
    _check_error(_C_TypeKey2Info(NULL, str_py2c(type_key), &type_info))
    if type_info == NULL:
        raise ValueError(f"MLC cannot find type info for type key: {type_key}")
    return type_info

cdef inline object _make_property_getter(str name, int64_t offset, MLCAttrGetter c_getter):
    if c_getter == NULL:
        return None

    def f_getter(this, str _key = name):
        cdef MLCAny c_ret
        cdef void* addr = <void*>((<uint64_t>_extract_obj_ptr(this)) + offset)
        _mlc_any_clear(&c_ret)
        _check_error_from(c_getter(addr, &c_ret), &c_ret)
        return _any_c2py(c_ret)
    return f_getter

cdef inline object _make_property_setter(str name, int64_t offset, MLCAttrSetter c_setter):
    if c_setter == NULL:
        return None

    def f_setter(this, object value, str _key = name) -> None:
        cdef list temporary_storage = []
        cdef MLCAny c_val = _any_py2c(value, temporary_storage)
        cdef void* addr = <void*>((<uint64_t>_extract_obj_ptr(this)) + offset)
        _check_error_from(c_setter(addr, &c_val), &c_val)
    return f_setter

cdef inline object _make_method(MLCAny* c_func):
    cdef PyAny ret = PyAny.__new__(PyAny)
    ret._mlc_any.type_index = c_func[0].type_index
    ret._mlc_any.ref_cnt = 0
    ret._mlc_any.v_obj = c_func
    _check_error(_C_AnyIncRef(&ret._mlc_any))
    return ret

cpdef object type_reflect(type type_cls, str type_key, dict attrs, dict vtable):
    cdef MLCTypeInfo* type_info = _type_key2info(type_key)
    cdef int32_t type_index = type_info[0].type_index
    cdef str name
    cdef int64_t offset = 0
    cdef PyAny func
    cdef MLCAny* c_func = NULL
    cdef MLCTypeField* fields = type_info[0].fields
    cdef MLCTypeMethod* methods = type_info[0].methods
    global BUILTIN_STR
    while fields != NULL and fields[0].name != NULL:
        name = str_c2py(fields[0].name)
        offset = fields[0].offset
        attrs[name] = property(
            fget=_make_property_getter(name, offset, fields[0].getter),
            fset=_make_property_setter(name, offset, fields[0].setter),
            doc=f"{type_key}::{name}"
        )
        fields += 1
    while methods != NULL and methods[0].name != NULL:
        name = str_c2py(methods[0].name)
        c_func = <MLCAny*>(methods[0].func)
        func = _make_method(c_func)
        vtable[name] = func
        if name == "__str__":
            _list_set(BUILTIN_STR, type_index, func)
        if not name.startswith("_"):
            from ..func import Func  # no-cython-lint
            _check_error(_C_AnyIncRef(&func._mlc_any))
            attrs[name] = PyAny._new_from_mlc_any(func._mlc_any)
        methods += 1
    return type_index

cpdef void register_type(type type_cls, int32_t type_index, str type_key):
    if (_list_set(TYPE_TABLE, type_index, type_cls) is not None):
        raise ValueError(f"Type is already registered: {type_key}")

cpdef void register_func(str name, bint allow_override, object func):
    mlc_func: PyAny = PyAny._new_from_mlc_any(_func_py2c(func))
    _check_error(_C_FuncSetGlobal(NULL, str_py2c(name), mlc_func._mlc_any, allow_override))

cpdef object get_global_func(str name):
    cdef MLCAny c_ret
    _mlc_any_clear(&c_ret)
    _check_error(_C_FuncGetGlobal(NULL, str_py2c(name), &c_ret))
    return PyAny._new_from_mlc_any(c_ret)

cpdef bytes str_py2c(str x):
    return x.encode("utf-8")

cpdef str str_c2py(bytes x):
    return x.decode("utf-8")

cpdef object pycode_fake(str filename, str funcname, int lineno):
    return PyCode_NewEmpty(str_py2c(filename), str_py2c(funcname), lineno)

cpdef list error_get_info(PyAny error):
    cdef list ret
    cdef int32_t num_strs
    cdef const char** strs
    _check_error(_C_ErrorGetInfo(error._mlc_any, &num_strs, &strs))
    ret = [str_c2py(strs[i]) for i in range(num_strs)]
    return ret

cpdef tuple dtype_as_triple(PyAny obj):
    cdef DLDataType dtype = obj._mlc_any.v_dtype
    cdef int code = <int>dtype.code
    cdef int bits = <int>dtype.code
    cdef int lanes = <int>dtype.lanes
    return code, bits, lanes

cpdef tuple device_as_pair(PyAny obj):
    cdef DLDevice device = obj._mlc_any.v_device
    return <int>device.device_type, <int>device.device_id

cpdef void func_init(PyAny self, object callable):
    self._mlc_any = _func_py2c(callable)

cpdef void list_dict_init(PyAny self, tuple py_args):
    cdef PyAny init = <PyAny>(type(self)._mlc_vtable["__init__"])
    cdef MLCFunc* c_func = <MLCFunc*>(init._mlc_any.v_obj)
    _func_call_impl(c_func, py_args, &self._mlc_any)
