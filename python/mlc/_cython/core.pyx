# cython: language_level=3
import ctypes
import itertools
from libc.stdint cimport int32_t, int64_t, uint64_t, uint8_t, uint16_t, int8_t, int16_t
from libc.stdlib cimport malloc, free
from numbers import Integral, Number
from cpython cimport Py_DECREF, Py_INCREF
from . import base


Ptr = base.Ptr
LIB_PATH, LIB = base.lib_path(__file__)

# Section 1. Definition from `mlc/c_api.h`

cdef extern from "mlc/c_api.h" nogil:
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
        kMLCStaticObjectBegin = 32768
        kMLCObject = 32768
        kMLCList = 32769
        kMLCDict = 32770
        kMLCError = 32771
        kMLCFunc = 32772
        kMLCStr = 32773
        # { kMLCTyping
        kMLCTypingBegin = 32774
        kMLCTyping = 32774
        kMLCTypingAny = 32775
        kMLCTypingNone = 32776
        kMLCTypingAtomic = 32777
        kMLCTypingPtr = 32778
        kMLCTypingOptional = 32779
        kMLCTypingUnion = 32780
        kMLCTypingList = 32781
        kMLCTypingDict = 32782
        kMLCTypingEnd = 32783
        # }
        kMLCDynObjectBegin = 65536

    ctypedef union MLCPODValueUnion:
        int64_t v_int64
        double v_float64
        DLDataType v_dtype
        DLDevice v_device
        void* v_ptr
        const char* v_str
        MLCAny* v_obj
        MLCDeleterType deleter
        char[8] v_bytes

    ctypedef struct MLCBoxedPOD:
        MLCAny _mlc_header
        MLCPODValueUnion data

    ctypedef struct MLCAny:
        int32_t type_index
        # union {
        int32_t ref_cnt
        int32_t small_len
        # }
        MLCPODValueUnion v

    ctypedef struct MLCObjPtr:
        MLCAny *ptr

    ctypedef struct MLCByteArray:
        int64_t num_bytes
        const char *bytes

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

    ctypedef struct MLCTypingAny:
        MLCAny _mlc_header

    ctypedef struct MLCTypingNone:
        MLCAny _mlc_header

    ctypedef struct MLCTypingAtomic:
        MLCAny _mlc_header
        int32_t type_index

    ctypedef struct MLCTypingRawPtr:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingOptional:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingUnion:
        MLCAny _mlc_header
        int32_t num_types

    ctypedef struct MLCTypingList:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingDict:
        MLCAny _mlc_header
        MLCObjPtr ty_k
        MLCObjPtr ty_v

    ctypedef struct MLCTypeField:
        const char* name
        int64_t offset
        int32_t num_bytes
        int32_t is_read_only
        MLCAny *ty

    ctypedef struct MLCTypeMethod:
        const char* name
        MLCFunc *func
        int32_t kind

    ctypedef struct MLCTypeInfo:
        int32_t type_index
        const char* type_key
        int32_t type_depth
        int32_t* type_ancestors
        MLCTypeField *fields
        MLCTypeMethod *methods

    ctypedef void* MLCTypeTableHandle

ctypedef MLCAny (*_T_GetLastError)() noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyIncRef)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyDecRef)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_AnyInplaceViewToOwned)(MLCAny *any) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncCreate)(void* self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call, MLCAny* ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncSetGlobal)(MLCTypeTableHandle self, const char *name, MLCAny func, int allow_override) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncGetGlobal)(MLCTypeTableHandle self, const char *name, MLCAny *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_FuncSafeCall)(MLCFunc* func, int32_t num_args, MLCAny *args, MLCAny* ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeIndex2Info)(MLCTypeTableHandle self, int32_t type_index, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeKey2Info)(MLCTypeTableHandle self, const char* type_key, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeRegister)(MLCTypeTableHandle self, int32_t parent_type_index, const char *type_key, int32_t type_index, MLCTypeInfo **out_type_info) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeDefReflection)(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields, MLCTypeField *fields, int64_t num_methods, MLCTypeMethod *methods) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableSet)(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableGet)(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ErrorCreate)(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ErrorGetInfo)(MLCAny err, int32_t* num_strs, const char*** strs) noexcept nogil # no-cython-lint
ctypedef MLCByteArray (*_T_Traceback)(const char *filename, const char *lineno, const char *func_name) noexcept nogil # no-cython-lint
ctypedef void* (*_T_ExtObjCreate)(int32_t bytes, int32_t type_index) noexcept nogil # no-cython-lint
ctypedef void (*_T_ExtObjDelete)(void *objptr) noexcept nogil # no-cython-lint

cdef uint64_t _sym(object symbol):
    symbol: ctypes.CDLL._FuncPtr
    return <uint64_t>(ctypes.cast(symbol, Ptr).value)

cdef _T_GetLastError _C_GetLastError = <_T_GetLastError>_sym(LIB.MLCGetLastError)
cdef _T_AnyIncRef _C_AnyIncRef = <_T_AnyIncRef>_sym(LIB.MLCAnyIncRef)
cdef _T_AnyDecRef _C_AnyDecRef = <_T_AnyDecRef>_sym(LIB.MLCAnyDecRef)
cdef _T_AnyInplaceViewToOwned _C_AnyInplaceViewToOwned = <_T_AnyInplaceViewToOwned>_sym(LIB.MLCAnyInplaceViewToOwned)
cdef _T_FuncCreate _C_FuncCreate = <_T_FuncCreate>_sym(LIB.MLCFuncCreate)
cdef _T_FuncSetGlobal _C_FuncSetGlobal = <_T_FuncSetGlobal>_sym(LIB.MLCFuncSetGlobal)
cdef _T_FuncGetGlobal _C_FuncGetGlobal = <_T_FuncGetGlobal>_sym(LIB.MLCFuncGetGlobal)
cdef _T_FuncSafeCall _C_FuncSafeCall = <_T_FuncSafeCall>_sym(LIB.MLCFuncSafeCall)
cdef _T_TypeIndex2Info _C_TypeIndex2Info = <_T_TypeIndex2Info>_sym(LIB.MLCTypeIndex2Info)
cdef _T_TypeKey2Info _C_TypeKey2Info = <_T_TypeKey2Info>_sym(LIB.MLCTypeKey2Info)
cdef _T_TypeRegister _C_TypeRegister = <_T_TypeRegister>_sym(LIB.MLCTypeRegister)
cdef _T_TypeDefReflection _C_TypeDefReflection = <_T_TypeDefReflection>_sym(LIB.MLCTypeDefReflection)
cdef _T_VTableSet _C_VTableSet = <_T_VTableSet>_sym(LIB.MLCVTableSet)
cdef _T_VTableGet _C_VTableGet = <_T_VTableGet>_sym(LIB.MLCVTableGet)
cdef _T_ErrorCreate _C_ErrorCreate = <_T_ErrorCreate>_sym(LIB.MLCErrorCreate)
cdef _T_ErrorGetInfo _C_ErrorGetInfo = <_T_ErrorGetInfo>_sym(LIB.MLCErrorGetInfo)
cdef _T_Traceback _C_Traceback = <_T_Traceback>_sym(LIB.MLCTraceback)
cdef _T_ExtObjCreate _C_ExtObjCreate = <_T_ExtObjCreate>_sym(LIB.MLCExtObjCreate)
cdef _T_ExtObjDelete _C_ExtObjDelete = <_T_ExtObjDelete>_sym(LIB.MLCExtObjDelete)

PyCode_NewEmpty = ctypes.pythonapi.PyCode_NewEmpty
PyCode_NewEmpty.restype = ctypes.py_object


# Section 2. Type registry and vtable
# TODO: python-side vtable is not necessary
cdef list TYPE_TABLE = []  # mapping: dict[type_index: int, type_cls: type]
cdef dict TYPE_VTABLE = {}  # mapping: dict[type_cls: type, dict[method_name: str, method_func: PyAny]]
cdef list BUILTIN_STR = []  # mapping: dict[type_index: int, method_func: PyAny], i.e. `__str__`
cdef list BUILTIN_INIT = []  # mapping: dict[type_index: int, method_func: PyAny], i.e. `__init__`

cdef inline object _list_get(list source, int index):
    return source[index] if index < len(source) else None

cdef inline object _list_set(list source, int index, object value):
    cdef int32_t delta = index + 1 - len(source)
    if delta > 0:
        source.extend([None] * delta)
    prev, source[index] = source[index], value
    return prev

# Section 3. Error handling

cdef inline void _check_error(int32_t err_code):
    cdef object obj
    if err_code == 0:
        return
    if err_code == -1:
        obj: Str = _any_c2py_no_inc_ref(_C_GetLastError())
        exception = RuntimeError(obj)
    elif err_code == -2:
        obj: PyAny = _any_c2py_no_inc_ref(_C_GetLastError())
        exception = base.translate_exception_from_c(obj)
    else:
        raise RuntimeError("MLC function call failed with error code: %d" % err_code)
    raise exception

cdef inline void _check_error_from(int32_t err_code, MLCAny* c_ret):
    cdef object obj
    if err_code == 0:
        return
    if err_code == -1:
        obj: Str = _any_c2py_no_inc_ref(c_ret[0])
        exception = RuntimeError(obj)
    elif err_code == -2:
        obj: PyAny = _any_c2py_no_inc_ref(c_ret[0])
        exception = base.translate_exception_from_c(obj)
    else:
        raise RuntimeError("MLC function call failed with error code: %d" % err_code)
    c_ret[0] = _MLCAnyNone()
    raise exception

# Section 4. Definition MLC's fundamental types: `PyAny` and `Str`

cdef class PyAny:
    cdef MLCAny _mlc_any

    def __cinit__(self):
        self._mlc_any = _MLCAnyNone()

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

    def __init__(self):
        pass

    def _mlc_init(self, init_func, *init_args) -> None:
        cdef PyAny func
        if isinstance(init_func, str):
            init_func = TYPE_VTABLE[type(self)][init_func]
        try:
            func = <PyAny>init_func
            assert func._mlc_any.type_index == kMLCFunc
        except Exception as e:  # no-cython-lint
            raise TypeError(
                "Unsupported type of `init_func`. "
                f"Expected `str` or `mlc.Func`, but got: {type(init_func)}"
            ) from e
        _func_call_impl(<MLCFunc*>(func._mlc_any.v.v_obj), init_args, &self._mlc_any)

    def __repr__(self) -> str:
        cdef int32_t type_index = self._mlc_any.type_index
        cdef MLCTypeInfo* info = NULL
        cdef object f_str = _list_get(BUILTIN_STR, type_index)
        if f_str is not None:
            return func_call(f_str, (self,))
        info = _type_index2info(type_index)
        for i in range(info.type_depth - 1, -1, -1):
            f_str = _list_get(BUILTIN_STR, info.type_ancestors[i])
            if f_str is not None:
                return func_call(f_str, (self,))
        assert f_str is not None, f"Cannot find `__str__` for type: {type(self)}"

    def __str__(self) -> str:
        return self.__repr__()

    @classmethod
    def _C(cls, str name, *args):
        cdef PyAny func
        try:
            func = TYPE_VTABLE[cls][name]
        except Exception as e:  # no-cython-lint
            if cls not in TYPE_VTABLE:
                raise TypeError(f"Cannot find vtable for type: {cls}") from e
            else:
                raise TypeError(f"Cannot find method `{name}` for type: {cls}") from e
        return func_call(func, args)

cdef class Str(str):
    cdef MLCAny _mlc_any
    __slots__ = ()

    def __cinit__(self):
        self._mlc_any = _MLCAnyNone()

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

cdef inline MLCAny _MLCAnyNone():
    cdef MLCAny x
    x.type_index = kMLCNone
    x.ref_cnt = 0
    x.v.v_int64 = 0
    return x

cdef inline MLCAny _MLCAnyObj(int32_t type_index, MLCAny* obj):
    cdef MLCAny x
    x.type_index = type_index
    x.ref_cnt = 0
    x.v.v_obj = obj
    return x

# Section 5. Conversion MLCAny => Python objects

cdef inline object _any_c2py_no_inc_ref(const MLCAny x):
    cdef int32_t type_index = x.type_index
    cdef MLCStr* mlc_str = NULL
    cdef PyAny any_ret
    cdef Str str_ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int>(x.v.v_int64)
    elif type_index == kMLCFloat:
        return <float>(x.v.v_float64)
    elif type_index == kMLCPtr:
        return ctypes.cast(<unsigned long long>x.v.v_ptr, Ptr)
    elif type_index == kMLCRawStr:
        return str_c2py(x.v.v_str)
    elif type_index == kMLCStr:
        mlc_str = <MLCStr*>(x.v.v_obj)
        str_ret = Str.__new__(Str, str_c2py(mlc_str.data[:mlc_str.length]))
        str_ret._mlc_any = x
        return str_ret
    elif (type_cls := _list_get(TYPE_TABLE, type_index)) is not None:
        any_ret = type_cls.__new__(type_cls)
        any_ret._mlc_any = x
        return any_ret
    raise TypeError(f"MLC does not recognize type index: {type_index}")

cdef inline object _any_c2py_inc_ref(MLCAny x):
    cdef int32_t type_index = x.type_index
    cdef MLCStr* mlc_str = NULL
    cdef PyAny any_ret
    cdef Str str_ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int>(x.v.v_int64)
    elif type_index == kMLCFloat:
        return <float>(x.v.v_float64)
    elif type_index == kMLCPtr:
        return ctypes.cast(<unsigned long long>x.v.v_ptr, Ptr)
    elif type_index == kMLCRawStr:
        return str_c2py(x.v.v_str)
    elif type_index == kMLCStr:
        mlc_str = <MLCStr*>(x.v.v_obj)
        str_ret = Str.__new__(Str, str_c2py(mlc_str.data[:mlc_str.length]))
        str_ret._mlc_any = x
        _check_error(_C_AnyIncRef(&x))
        return str_ret
    elif (type_cls := _list_get(TYPE_TABLE, type_index)) is not None:
        any_ret = type_cls.__new__(type_cls)
        any_ret._mlc_any = x
        _check_error(_C_AnyIncRef(&x))
        return any_ret
    raise TypeError(f"MLC does not recognize type index: {type_index}")

# Section 6. Conversion Python objects => MLCAny

cdef inline MLCAny _any_py2c(object x, list temporary_storage):
    cdef MLCAny y = _MLCAnyNone()
    if x is None:
        ...
    elif isinstance(x, PyAny):
        y = _MLCAnyObj(
            type_index=(<PyAny>x)._mlc_any.type_index,
            obj=(<PyAny>x)._mlc_any.v.v_obj,
        )
    elif isinstance(x, Str):
        y = _MLCAnyObj(
            type_index=(<Str>x)._mlc_any.type_index,
            obj=(<Str>x)._mlc_any.v.v_obj,
        )
    elif isinstance(x, Integral):
        y.type_index = <int64_t>kMLCInt
        y.v.v_int64 = x
    elif isinstance(x, Number):
        y.type_index = <int64_t>kMLCFloat
        y.v.v_float64 = x
    elif isinstance(x, Ptr):
        if x.value is None or x.value == 0:
            ...
        else:
            y.type_index = <int64_t>kMLCPtr
            y.v.v_int64 = x.value
    elif isinstance(x, str):
        x = str_py2c(x)
        temporary_storage.append(x)
        y.type_index = <int64_t>kMLCRawStr
        y.v.v_str = x
    elif callable(x):
        x = _pyany_from_func(x)
        temporary_storage.append(x)
        y = (<PyAny>x)._mlc_any
    elif isinstance(x, tuple):
        y = _any_py2c_list(x, temporary_storage)
    elif isinstance(x, list):
        y = _any_py2c_list(tuple(x), temporary_storage)
    elif isinstance(x, dict):
        y = _any_py2c_dict(tuple(itertools.chain.from_iterable(x.items())), temporary_storage)
    else:
        raise TypeError(f"MLC does not recognize type: {type(x)}")
    return y

cdef inline MLCAny _any_py2c_list(tuple x, list temporary_storage):
    from mlc.core.list import List  # no-cython-lint
    cdef PyAny func = <PyAny>_list_get(BUILTIN_INIT, kMLCList)
    cdef MLCFunc* func_ptr = <MLCFunc*>(func._mlc_any.v.v_obj)
    cdef MLCAny ret = _MLCAnyNone()
    _func_call_impl(func_ptr, x, &ret)
    temporary_storage.append(_pyany_no_inc_ref(ret))
    return ret

cdef inline MLCAny _any_py2c_dict(tuple x, list temporary_storage):
    from mlc.core.dict import Dict  # no-cython-lint
    cdef PyAny func = <PyAny>_list_get(BUILTIN_INIT, kMLCDict)
    cdef MLCFunc* func_ptr = <MLCFunc*>(func._mlc_any.v.v_obj)
    cdef MLCAny ret = _MLCAnyNone()
    _func_call_impl(func_ptr, x, &ret)
    temporary_storage.append(_pyany_no_inc_ref(ret))
    return ret

cdef inline PyAny _pyany_no_inc_ref(MLCAny x):
    cdef PyAny ret = PyAny()
    ret._mlc_any = x
    return ret

cdef inline PyAny _pyany_inc_ref(MLCAny x):
    cdef PyAny ret = PyAny()
    ret._mlc_any = x
    _check_error(_C_AnyIncRef(&ret._mlc_any))
    return ret

# Section 7. Conversion: Python Callable => MLCFunc

cdef void _pyobj_deleter(void* handle) noexcept nogil:
    with gil:
        try:
            Py_DECREF(<object>(handle))
        except Exception as exception:
            # TODO(@junrushao): Will need to handle exceptions more gracefully
            print(f"Error in _pyobj_deleter: {exception}")

cdef inline int32_t _func_safe_call(
    const void* py_func,
    int32_t num_args,
    const MLCAny* c_args,
    MLCAny* c_ret
) noexcept nogil:
    with gil:
        return _func_safe_call_impl(py_func, num_args, c_args, c_ret)

cdef inline int32_t _func_safe_call_impl(
    const void* py_func,
    int32_t num_args,
    const MLCAny* c_args,
    MLCAny* c_ret  # N.B. caller needs to ensure `c_ret` is empty
):
    cdef object py_ret
    cdef list temporary_storage = []
    cdef list py_args = [None] * num_args
    assert c_ret[0].type_index == kMLCNone
    try:
        for i in range(num_args):
            py_args[i] = _any_c2py_inc_ref(c_args[i])
        py_ret = (<object>py_func)(*py_args)
        c_ret[0] = _any_py2c(py_ret, temporary_storage)
        _check_error(_C_AnyInplaceViewToOwned(c_ret))
    except Exception as exception:
        kind, num_bytes_info, bytes_info = base.translate_exception_to_c(exception)
        _check_error(_C_ErrorCreate(kind, num_bytes_info, bytes_info, c_ret))
        return -2
    return 0

cdef inline PyAny _pyany_from_func(object py_func):
    cdef PyAny ret = PyAny()
    Py_INCREF(py_func)
    _check_error(_C_FuncCreate(<void*>(py_func), _pyobj_deleter, _func_safe_call, &ret._mlc_any))
    return ret

# Section 8. Call an MLCFunc

cdef inline void _func_call_impl_with_c_args(
    MLCFunc* c_func,
    int32_t num_args,
    MLCAny* c_args,
    MLCAny* c_ret
):
    cdef int32_t err_code
    with nogil:
        err_code = _C_FuncSafeCall(c_func, num_args, c_args, c_ret)
    _check_error_from(err_code, c_ret)

cdef inline void _func_call_impl(MLCFunc* c_func, tuple py_args, MLCAny* c_ret):
    cdef list temporary_storage = []
    cdef int32_t num_args = len(py_args)
    cdef MLCAny* c_args = <MLCAny*> malloc(num_args * sizeof(MLCAny))
    try:
        for i in range(num_args):
            c_args[i] = _any_py2c(py_args[i], temporary_storage)
        _func_call_impl_with_c_args(c_func, num_args, c_args, c_ret)
    finally:
        free(c_args)

# Section 9. TODO

cdef inline MLCTypeInfo* _type_key2info(str type_key):
    cdef MLCTypeInfo* type_info
    _check_error(_C_TypeKey2Info(NULL, str_py2c(type_key), &type_info))
    if type_info == NULL:
        raise ValueError(f"MLC cannot find type info for type key: {type_key}")
    return type_info

cdef inline MLCTypeInfo* _type_index2info(int32_t type_index):
    cdef MLCTypeInfo* type_info
    _check_error(_C_TypeIndex2Info(NULL, type_index, &type_info))
    if type_info == NULL:
        raise ValueError(f"MLC cannot find type info for type index: {type_index}")
    return type_info

# Section 10. Interface with Python

cpdef bytes str_py2c(str x):
    return x.encode("utf-8")

cpdef str str_c2py(bytes x):
    return x.decode("utf-8")

cpdef object func_call(PyAny func, tuple py_args):
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v.v_obj)
    cdef MLCAny c_ret = _MLCAnyNone()
    _func_call_impl(c_func, py_args, &c_ret)
    return _any_c2py_no_inc_ref(c_ret)

cpdef void func_init(PyAny self, object callable):
    cdef PyAny func = _pyany_from_func(callable)
    self._mlc_any = func._mlc_any
    func._mlc_any = _MLCAnyNone()

cpdef void func_register(str name, bint allow_override, object func):
    cdef PyAny mlc_func = _pyany_from_func(func)
    _check_error(_C_FuncSetGlobal(NULL, str_py2c(name), mlc_func._mlc_any, allow_override))

cpdef object func_get(str name):
    cdef MLCAny c_ret = _MLCAnyNone()
    _check_error(_C_FuncGetGlobal(NULL, str_py2c(name), &c_ret))
    return _any_c2py_no_inc_ref(c_ret)

cpdef list error_get_info(PyAny err):
    cdef list ret
    cdef int32_t num_strs
    cdef const char** strs
    _check_error(_C_ErrorGetInfo(err._mlc_any, &num_strs, &strs))
    ret = [str_c2py(strs[i]) for i in range(num_strs)]
    return ret

cpdef object error_pycode_fake(str filename, str funcname, int lineno):
    return PyCode_NewEmpty(str_py2c(filename), str_py2c(funcname), lineno)

cpdef tuple dtype_as_triple(PyAny obj):
    cdef DLDataType dtype = obj._mlc_any.v.v_dtype
    cdef int code = <int>dtype.code
    cdef int bits = <int>dtype.code
    cdef int lanes = <int>dtype.lanes
    return code, bits, lanes

cpdef tuple device_as_pair(PyAny obj):
    cdef DLDevice device = obj._mlc_any.v.v_device
    return <int>device.device_type, <int>device.device_id

cpdef object type_key2type_info(str type_key, object type_cls):
    cdef MLCTypeInfo* c_info = _type_key2info(type_key)
    cdef int32_t type_index = c_info[0].type_index
    cdef int32_t type_depth = c_info[0].type_depth
    cdef list type_ancestors = [c_info[0].type_ancestors[i] for i in range(type_depth)]
    cdef MLCTypeField* fields_ptr = c_info[0].fields
    cdef MLCTypeMethod* methods_ptr = c_info[0].methods
    cdef list fields = []
    cdef list methods = []
    while fields_ptr != NULL and fields_ptr[0].name != NULL:
        assert fields_ptr[0].ty != NULL
        assert kMLCTypingBegin <= fields_ptr[0].ty[0].type_index <= kMLCTypingEnd
        type_field = base.TypeField(
            name=str_c2py(fields_ptr[0].name),
            offset=fields_ptr[0].offset,
            num_bytes=fields_ptr[0].num_bytes,
            is_read_only=bool(fields_ptr[0].is_read_only),
            ty=_pyany_inc_ref(_MLCAnyObj(
                type_index=fields_ptr[0].ty[0].type_index,
                obj=fields_ptr[0].ty,
            )),
        )
        type_field.getter = _type_field_accessor(type_field)
        fields.append(type_field)
        fields_ptr += 1
    while methods_ptr != NULL and methods_ptr[0].name != NULL:
        assert methods_ptr[0].func != NULL
        assert methods_ptr[0].func[0]._mlc_header.type_index == kMLCFunc
        type_method = base.TypeMethod(
            name=str_c2py(methods_ptr[0].name),
            func=_pyany_inc_ref(_MLCAnyObj(
                type_index=kMLCFunc,
                obj=<MLCAny*>(methods_ptr[0].func),
            )),
            kind=methods_ptr[0].kind,
        )
        methods.append(type_method)
        methods_ptr += 1
    if (_list_set(TYPE_TABLE, type_index, type_cls) is not None):
        raise ValueError(f"Type is already registered: {type_key}")
    for method in methods:
        name = method.name
        func = method.func
        if name == "__str__":
            _list_set(BUILTIN_STR, type_index, func)
        elif name == "__init__":
            _list_set(BUILTIN_INIT, type_index, func)
        TYPE_VTABLE.setdefault(type_cls, {})[name] = func

    return base.TypeInfo(
        type_index=type_index,
        type_key=type_key,
        type_depth=type_depth,
        type_ancestors=tuple(type_ancestors),
        fields=tuple(fields),
        methods=tuple(methods),
    )

cdef _type_field_accessor(object type_field: base.TypeField):
    cdef int64_t offset = type_field.offset
    cdef int32_t num_bytes = type_field.num_bytes
    cdef PyAny ty = type_field.ty
    cdef int32_t ty_type_index = ty._mlc_any.type_index
    cdef int32_t type_index = -1
    if ty_type_index == kMLCTypingAny:
        if num_bytes == sizeof(MLCAny):
            def f(PyAny self):
                cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                return _any_c2py_inc_ref((<MLCAny*>addr)[0])
            return f
    elif ty_type_index == kMLCTypingAtomic or ty_type_index == kMLCTypingList or ty_type_index == kMLCTypingDict:
        if ty_type_index == kMLCTypingAtomic:
            type_index = (<MLCTypingAtomic*>ty._mlc_any.v.v_obj).type_index
        if type_index == -1 or type_index >= kMLCStaticObjectBegin:
            if num_bytes == sizeof(MLCAny*):
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    cdef MLCAny* ptr = (<MLCAny**>addr)[0]
                    if ptr == NULL:
                        return None
                    return _any_c2py_inc_ref(_MLCAnyObj(
                        type_index=ptr[0].type_index,
                        obj=ptr,
                    ))
                return f
        elif type_index == kMLCInt:
            if num_bytes == 1:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<int8_t*>addr)[0]
                return f
            elif num_bytes == 2:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<int16_t*>addr)[0]
                return f
            elif num_bytes == 4:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<int32_t*>addr)[0]
                return f
            elif num_bytes == 8:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<int64_t*>addr)[0]
                return f
        elif type_index == kMLCFloat:
            if num_bytes == 4:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<float*>addr)[0]
                return f
            elif num_bytes == 8:
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    return (<double*>addr)[0]
                return f
        elif type_index == kMLCPtr:
            if num_bytes == sizeof(void*):
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    cdef void* ret = (<void***>addr)[0]
                    return ctypes.cast(<unsigned long long>ret, Ptr)
                return f
        elif type_index == kMLCDataType:
            if num_bytes == sizeof(DLDataType):
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    cdef MLCAny ret = _MLCAnyNone()
                    ret.type_index = kMLCDataType
                    ret.v.v_dtype = (<DLDataType*>addr)[0]
                    return _any_c2py_no_inc_ref(ret)
                return f
        elif type_index == kMLCDevice:
            if num_bytes == sizeof(DLDevice):
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    cdef MLCAny ret = _MLCAnyNone()
                    ret.type_index = kMLCDevice
                    ret.v.v_device = (<DLDevice*>addr)[0]
                    return _any_c2py_no_inc_ref(ret)
                return f
        elif type_index == kMLCRawStr:
            if num_bytes == sizeof(char*):
                def f(PyAny self):
                    cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                    cdef char* addr_str = (<char**>addr)[0]
                    return str_c2py(addr_str)
                return f
    elif ty_type_index == kMLCTypingPtr or ty_type_index == kMLCTypingOptional or ty_type_index == kMLCTypingUnion:
        if num_bytes == sizeof(MLCAny*):
            def f(PyAny self):
                cdef uint64_t addr = <uint64_t>(self._mlc_any.v.v_obj) + offset
                cdef MLCAny* ptr = (<MLCAny**>addr)[0]
                if ptr == NULL:
                    return None
                return _any_c2py_inc_ref(ptr[0])
            return f
    raise ValueError(f"Unsupported {num_bytes}-byte type field: {ty.__str__()}")
