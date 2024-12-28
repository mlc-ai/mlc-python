# cython: language_level=3
import ctypes
import itertools
from libcpp.vector cimport vector
from libc.stdint cimport int32_t, int64_t, uint64_t, uint8_t, uint16_t, int8_t, int16_t
from libc.stdlib cimport malloc, free
from numbers import Integral, Number
from cpython cimport Py_DECREF, Py_INCREF
from . import base

Ptr = base.Ptr
PyCode_NewEmpty = ctypes.pythonapi.PyCode_NewEmpty
PyCode_NewEmpty.restype = ctypes.py_object
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
        kMLCStaticObjectBegin = 1000
        # kMLCCore [1000: 1100) {
        kMLCCoreBegin = 1000
        kMLCObject = 1000
        kMLCList = 1001
        kMLCDict = 1002
        kMLCError = 1003
        kMLCFunc = 1004
        kMLCStr = 1005
        kMLCCoreEnd = 1100
        # }
        # kMLCTyping [1100: 1200) {
        kMLCTypingBegin = 1100
        kMLCTyping = 1100
        kMLCTypingAny = 1101
        kMLCTypingAtomic = 1102
        kMLCTypingPtr = 1103
        kMLCTypingOptional = 1104
        kMLCTypingList = 1105
        kMLCTypingDict = 1106
        kMLCTypingEnd = 1200
        # }
        # [Section] Dynamic Boxed: [kMLCDynObjectBegin, +oo)
        kMLCDynObjectBegin = 100000

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

    ctypedef struct MLCAny:
        int32_t type_index
        # union {
        int32_t ref_cnt
        int32_t small_len
        # }
        MLCPODValueUnion v

    ctypedef struct MLCBoxedPOD:
        MLCAny _mlc_header
        MLCPODValueUnion data

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

    ctypedef struct MLCTypingPtr:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingOptional:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingList:
        MLCAny _mlc_header
        MLCObjPtr ty

    ctypedef struct MLCTypingDict:
        MLCAny _mlc_header
        MLCObjPtr ty_k
        MLCObjPtr ty_v

    ctypedef struct MLCTypeField:
        const char* name
        int32_t index
        int64_t offset
        int32_t num_bytes
        int32_t frozen
        MLCAny *ty

    ctypedef struct MLCTypeMethod:
        const char* name
        MLCFunc *func
        int32_t kind  # 0: member method, 1: static method

    ctypedef struct MLCTypeInfo:
        int32_t type_index
        const char* type_key
        uint64_t type_key_hash
        int32_t type_depth
        int32_t* type_ancestors
        MLCTypeField *fields
        MLCTypeMethod *methods
        int32_t structure_kind
        int32_t *sub_structure_indices
        int32_t *sub_structure_kinds

    ctypedef void* MLCTypeTableHandle
    ctypedef void* MLCVTableHandle

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
ctypedef int32_t (*_T_TypeRegisterFields)(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields, MLCTypeField *fields) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeRegisterStructure)(MLCTypeTableHandle self, int32_t type_index, int32_t structure_kind, int64_t num_sub_structures, int32_t *sub_structure_indices, int32_t *sub_structure_kinds) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_TypeAddMethod)(MLCTypeTableHandle self, int32_t type_index, MLCTypeMethod method) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableGetGlobal)(MLCTypeTableHandle self, const char *key, MLCVTableHandle *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableGetFunc)(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor, MLCAny *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_VTableSetFunc)(MLCVTableHandle vtable, int32_t type_index, MLCFunc *func, int32_t override_mode) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ErrorCreate)(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ErrorGetInfo)(MLCAny err, int32_t* num_strs, const char*** strs) noexcept nogil # no-cython-lint
ctypedef MLCByteArray (*_T_Traceback)(const char *filename, const char *lineno, const char *func_name) noexcept nogil # no-cython-lint
ctypedef int32_t (*_T_ExtObjCreate)(int32_t bytes, int32_t type_index, MLCAny *ret) noexcept nogil # no-cython-lint
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
cdef _T_TypeRegisterFields _C_TypeRegisterFields = <_T_TypeRegisterFields>_sym(LIB.MLCTypeRegisterFields)
cdef _T_TypeRegisterStructure _C_TypeRegisterStructure = <_T_TypeRegisterStructure>_sym(LIB.MLCTypeRegisterStructure)
cdef _T_TypeAddMethod _C_TypeAddMethod = <_T_TypeAddMethod>_sym(LIB.MLCTypeAddMethod)
cdef _T_VTableGetGlobal _C_VTableGetGlobal = <_T_VTableGetGlobal>_sym(LIB.MLCVTableGetGlobal)
cdef _T_VTableGetFunc _C_VTableGetFunc = <_T_VTableGetFunc>_sym(LIB.MLCVTableGetFunc)
cdef _T_VTableSetFunc _C_VTableSetFunc = <_T_VTableSetFunc>_sym(LIB.MLCVTableSetFunc)
cdef _T_ErrorCreate _C_ErrorCreate = <_T_ErrorCreate>_sym(LIB.MLCErrorCreate)
cdef _T_ErrorGetInfo _C_ErrorGetInfo = <_T_ErrorGetInfo>_sym(LIB.MLCErrorGetInfo)
cdef _T_Traceback _C_Traceback = <_T_Traceback>_sym(LIB.MLCTraceback)
cdef _T_ExtObjCreate _C_ExtObjCreate = <_T_ExtObjCreate>_sym(LIB.MLCExtObjCreate)
cdef _T_ExtObjDelete _C_ExtObjDelete = <_T_ExtObjDelete>_sym(LIB.MLCExtObjDelete)

# Section 2. Utility functions

cdef inline object _list_get(list source, int32_t index):
    try:
        return source[index]
    except IndexError:
        return None

cdef inline object _list_set(list source, int32_t index, object value):
    while len(source) <= index:
        source.extend([None] * len(source))
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
    __slots__ = ()

    def __cinit__(self):
        self._mlc_any = _MLCAnyNone()

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

    def __init__(self):
        pass

    def _mlc_init(self, *init_args) -> None:
        cdef int32_t type_index = type(self)._mlc_type_info.type_index
        cdef MLCFunc* func = _vtable_get_func_ptr(_VTABLE_INIT, type_index, False)
        _func_call_impl(func, init_args, &self._mlc_any)

    def __repr__(self) -> str:
        cdef int32_t type_index = self._mlc_any.type_index
        cdef MLCAny c_ret = _MLCAnyNone()
        cdef MLCFunc* func = _vtable_get_func_ptr(_VTABLE_STR, type_index, True)
        _func_call_impl(func, (self, ), &c_ret)
        return _any_c2py_no_inc_ref(c_ret)

    def __str__(self) -> str:
        return self.__repr__()

    def __reduce__(self):
        return (base.new_object, (type(self),), self.__getstate__())

    def __getstate__(self):
        return {"mlc_json": func_call(_SERIALIZE, (self,))}

    def __setstate__(self, state):
        cdef PyAny ret = func_call(_DESERIALIZE, (state["mlc_json"], ))
        cdef MLCAny tmp = self._mlc_any
        self._mlc_any = ret._mlc_any
        ret._mlc_any = tmp

    def _mlc_json(self):
        return func_call(_SERIALIZE, (self,))

    @staticmethod
    def _mlc_from_json(mlc_json):
        return func_call(_DESERIALIZE, (mlc_json,))

    @staticmethod
    def _mlc_eq_s(PyAny lhs, PyAny rhs, bind_free_vars: bool, assert_mode: bool) -> bool:
        return bool(func_call(_STRUCUTRAL_EQUAL, (lhs, rhs, bind_free_vars, assert_mode)))

    @staticmethod
    def _mlc_hash_s(PyAny x) -> object:
        cdef object ret = func_call(_STRUCUTRAL_HASH, (x,))
        if ret < 0:
            ret += 2 ** 63
        return ret

    @classmethod
    def _C(cls, bytes name, *args):
        cdef int32_t type_index = cls._mlc_type_info.type_index
        cdef MLCVTableHandle vtable = _vtable_get_global(name)
        cdef MLCFunc* func = NULL
        cdef MLCAny c_ret = _MLCAnyNone()
        try:
            func = _vtable_get_func_ptr(vtable, type_index, True)
        except Exception as e:  # no-cython-lint
            raise TypeError(f"Cannot find method `{name}` for type: {cls}") from e
        _func_call_impl(func, args, &c_ret)
        return _any_c2py_no_inc_ref(c_ret)

cdef class Str(str):
    cdef MLCAny _mlc_any
    __slots__ = ()

    def __cinit__(self):
        self._mlc_any = _MLCAnyNone()

    def __init__(self, value):
        cdef str value_unicode = self
        cdef bytes value_c = str_py2c(value_unicode)
        self._mlc_any = _MLCAnyRawStr(value_c)
        _check_error(_C_AnyInplaceViewToOwned(&self._mlc_any))

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

    def __reduce__(self):
        return (Str, (str(self),))


cdef inline MLCAny _MLCAnyNone():
    cdef MLCAny x
    x.type_index = kMLCNone
    x.ref_cnt = 0
    x.v.v_int64 = 0
    return x

cdef inline MLCAny _MLCAnyObj(MLCAny* obj):
    cdef MLCAny x
    assert obj != NULL
    x.type_index = obj.type_index
    x.ref_cnt = 0
    x.v.v_obj = obj
    return x

cdef inline MLCAny _MLCAnyInt(int64_t value):
    cdef MLCAny x
    x.type_index = kMLCInt
    x.ref_cnt = 0
    x.v.v_int64 = value
    return x

cdef inline MLCAny _MLCAnyFloat(double value):
    cdef MLCAny x
    x.type_index = kMLCFloat
    x.ref_cnt = 0
    x.v.v_float64 = value
    return x

cdef inline MLCAny _MLCAnyPtr(uint64_t ptr):
    cdef MLCAny x
    if ptr == 0:
        x.type_index = kMLCNone
        x.ref_cnt = 0
        x.v.v_int64 = 0
    else:
        x.type_index = kMLCPtr
        x.ref_cnt = 0
        x.v.v_int64 = ptr
    return x

cdef inline MLCAny _MLCAnyDevice(DLDevice device):
    cdef MLCAny x
    x.type_index = kMLCDevice
    x.ref_cnt = 0
    x.v.v_device = device
    return x

cdef inline MLCAny _MLCAnyDataType(DLDataType dtype):
    cdef MLCAny x
    x.type_index = kMLCDataType
    x.ref_cnt = 0
    x.v.v_dtype = dtype
    return x

cdef inline MLCAny _MLCAnyRawStr(bytes value):
    cdef MLCAny x
    x.type_index = kMLCRawStr
    x.ref_cnt = 0
    x.v.v_str = value
    return x

cdef inline object _DataType(DLDataType dtype):
    cdef PyAny any_ret
    cdef object type_cls = _list_get(TYPE_INDEX_TO_INFO, kMLCDataType).type_cls
    any_ret = type_cls.__new__(type_cls)
    any_ret._mlc_any.type_index = kMLCDataType
    any_ret._mlc_any.v.v_dtype = dtype
    return any_ret

cdef inline object _Device(DLDevice device):
    cdef PyAny any_ret
    cdef object type_cls = _list_get(TYPE_INDEX_TO_INFO, kMLCDevice).type_cls
    any_ret = type_cls.__new__(type_cls)
    any_ret._mlc_any.type_index = kMLCDevice
    any_ret._mlc_any.v.v_device = device
    return any_ret

cdef inline object _Ptr(void* ptr):
    return ctypes.cast(<uint64_t>ptr, Ptr)

cdef inline tuple _flatten_dict_to_tuple(dict x):
    return tuple(itertools.chain.from_iterable(x.items()))

cdef inline uint64_t _addr_from_ptr(object ptr):
    if ptr is None:
        return 0
    assert isinstance(ptr, Ptr)
    return <uint64_t>(ptr.value) if ptr.value else <uint64_t>0

# Section 5. Conversion MLCAny => Python objects

cdef inline object _any_c2py_no_inc_ref(const MLCAny x):
    cdef int32_t type_index = x.type_index
    cdef MLCStr* mlc_str = NULL
    cdef PyAny any_ret
    cdef Str str_ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int64_t>(x.v.v_int64)
    elif type_index == kMLCFloat:
        return <double>(x.v.v_float64)
    elif type_index == kMLCPtr:
        return _Ptr(x.v.v_ptr)
    elif type_index == kMLCRawStr:
        return str_c2py(x.v.v_str)
    elif type_index == kMLCStr:
        mlc_str = <MLCStr*>(x.v.v_obj)
        str_ret = Str.__new__(Str, str_c2py(mlc_str.data[:mlc_str.length]))
        str_ret._mlc_any = x
        return str_ret
    elif (type_cls := _list_get(TYPE_INDEX_TO_INFO, type_index)) is not None:
        type_cls = type_cls.type_cls
        any_ret = type_cls.__new__(type_cls)
        any_ret._mlc_any = x
        return any_ret
    raise ValueError(f"MLC does not recognize type: {type_index}")

cdef inline object _any_c2py_inc_ref(MLCAny x):
    cdef int32_t type_index = x.type_index
    cdef MLCStr* mlc_str = NULL
    cdef PyAny any_ret
    cdef Str str_ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int64_t>(x.v.v_int64)
    elif type_index == kMLCFloat:
        return <double>(x.v.v_float64)
    elif type_index == kMLCPtr:
        return _Ptr(x.v.v_ptr)
    elif type_index == kMLCRawStr:
        return str_c2py(x.v.v_str)
    elif type_index == kMLCStr:
        mlc_str = <MLCStr*>(x.v.v_obj)
        str_ret = Str.__new__(Str, str_c2py(mlc_str.data[:mlc_str.length]))
        str_ret._mlc_any = x
        _check_error(_C_AnyIncRef(&x))
        return str_ret
    elif (type_cls := _list_get(TYPE_INDEX_TO_INFO, type_index)) is not None:
        type_cls = type_cls.type_cls
        any_ret = type_cls.__new__(type_cls)
        any_ret._mlc_any = x
        _check_error(_C_AnyIncRef(&x))
        return any_ret
    raise ValueError(f"MLC does not recognize type: {type_index}")

cdef inline PyAny _pyany_no_inc_ref(MLCAny x):
    cdef PyAny ret = PyAny()
    ret._mlc_any = x
    return ret

cdef inline PyAny _pyany_inc_ref(MLCAny x):
    cdef PyAny ret = PyAny()
    ret._mlc_any = x
    _check_error(_C_AnyIncRef(&ret._mlc_any))
    return ret

# Section 6. Conversion Python objects => MLCAny

cdef inline MLCAny _any_py2c(object x, list temporary_storage):
    cdef MLCAny y
    if x is None:
        y = _MLCAnyNone()
    elif isinstance(x, PyAny):
        y = (<PyAny>x)._mlc_any
    elif isinstance(x, Str):
        y = (<Str>x)._mlc_any
    elif isinstance(x, Integral):
        y = _MLCAnyInt(<int64_t>x)
    elif isinstance(x, Number):
        y = _MLCAnyFloat(<double>x)
    elif isinstance(x, Ptr):
        y = _MLCAnyPtr(_addr_from_ptr(x))
    elif isinstance(x, str):
        x = str_py2c(x)
        y = _MLCAnyRawStr(x)
        temporary_storage.append(x)
    elif callable(x):
        x = _pyany_from_func(x)
        y = (<PyAny>x)._mlc_any
        temporary_storage.append(x)
    elif isinstance(x, tuple):
        y = _any_py2c_list(x, temporary_storage)
    elif isinstance(x, list):
        y = _any_py2c_list(tuple(x), temporary_storage)
    elif isinstance(x, dict):
        y = _any_py2c_dict(_flatten_dict_to_tuple(x), temporary_storage)
    else:
        raise TypeError(f"MLC does not recognize type: {type(x)}")
    return y

cdef inline MLCAny _any_py2c_list(tuple x, list temporary_storage):
    cdef MLCAny ret = _MLCAnyNone()
    _func_call_impl(_LIST_INIT, x, &ret)
    temporary_storage.append(_pyany_no_inc_ref(ret))
    return ret

cdef inline MLCAny _any_py2c_dict(tuple x, list temporary_storage):
    cdef MLCAny ret = _MLCAnyNone()
    _func_call_impl(_DICT_INIT, x, &ret)
    temporary_storage.append(_pyany_no_inc_ref(ret))
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
    assert c_ret.type_index == kMLCNone
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

# Section 9. Type information

cdef inline MLCTypeInfo* _type_index2c_type_info(int32_t type_index):
    cdef MLCTypeInfo* c_info = NULL
    _check_error(_C_TypeIndex2Info(NULL, type_index, &c_info))
    if c_info == NULL:
        raise ValueError(f"MLC cannot find type info for type index: {type_index}")
    return c_info

cdef inline MLCTypeInfo* _type_key2c_type_info(str type_key):
    cdef MLCTypeInfo* c_info = NULL
    _check_error(_C_TypeKey2Info(NULL, str_py2c(type_key), &c_info))
    if c_info == NULL:
        raise ValueError(f"MLC cannot find type info for type key: {type_key}")
    return c_info

cdef inline object _type_index2py_type_info(int32_t type_index):
    return _type_info_c2py(_type_index2c_type_info(type_index))

cdef inline object _type_key2py_type_info(str type_key):
    return _type_info_c2py(_type_key2c_type_info(type_key))

cdef inline object _type_info_c2py(MLCTypeInfo* c_info):
    cdef int32_t type_index = c_info.type_index
    cdef int32_t type_depth = c_info.type_depth
    cdef MLCTypeField* fields_ptr = c_info.fields
    cdef str type_key = str_c2py(c_info.type_key)
    cdef tuple type_ancestors = tuple(c_info.type_ancestors[i] for i in range(type_depth))
    cdef list fields = []
    cdef object ret
    if (ret := _list_get(TYPE_INDEX_TO_INFO, type_index)) is not None:
        return ret
    while fields_ptr != NULL and fields_ptr.name != NULL:
        assert fields_ptr.ty != NULL
        assert kMLCTypingBegin <= fields_ptr.ty.type_index <= kMLCTypingEnd
        type_field = base.TypeField(
            name=str_c2py(fields_ptr.name),
            offset=fields_ptr.offset,
            num_bytes=fields_ptr.num_bytes,
            frozen=bool(fields_ptr.frozen),
            ty=_pyany_inc_ref(_MLCAnyObj(fields_ptr.ty)),
        )
        type_field.getter, type_field.setter = _type_field_accessor(type_field)
        fields.append(type_field)
        fields_ptr += 1
    ret = base.TypeInfo(
        type_cls=None,
        type_index=type_index,
        type_key=type_key,
        type_depth=type_depth,
        type_ancestors=tuple(type_ancestors),
        fields=tuple(fields),
    )
    _list_set(TYPE_INDEX_TO_INFO, type_index, ret)
    return ret

cdef inline MLCVTableHandle _vtable_get_global(bytes key):
    cdef MLCVTableHandle ret = NULL
    _C_VTableGetGlobal(NULL, key, &ret)
    return ret

cdef inline PyAny _vtable_get_func(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor):
    cdef PyAny ret = PyAny()
    _check_error(_C_VTableGetFunc(vtable, type_index, allow_ancestor, &ret._mlc_any))
    if ret._mlc_any.type_index == kMLCNone:
        raise ValueError(f"Cannot find function for type: {str_c2py(_type_index2c_type_info(type_index).type_key)}")
    elif ret._mlc_any.type_index != kMLCFunc:
        raise ValueError(f"Expected function, but got: {str_c2py(_type_index2c_type_info(ret.type_index).type_key)}")
    return ret

cdef inline MLCFunc* _vtable_get_func_ptr(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor):
    cdef PyAny ret = _vtable_get_func(vtable, type_index, allow_ancestor)
    return <MLCFunc*>(ret._mlc_any.v.v_obj)

# Section 10. Type conversion

ctypedef MLCAny (*FTypeCheckerConvert)(object _self, object value, list temporary_storage)

cdef class TypeChecker:
    cdef object _self
    cdef FTypeCheckerConvert convert

cdef class TypeCheckerAny:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        return _any_py2c(value, temporary_storage)

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAny.convert
        return ret

cdef class TypeCheckerAtomicInt:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        if isinstance(value, Integral):
            return _MLCAnyInt(<int64_t>(value))
        raise TypeError(f"Expected `int`, but got: {type(value)}")

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAtomicInt.convert
        return ret

cdef class TypeCheckerAtomicFloat:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        if isinstance(value, Number):
            return _MLCAnyFloat(<double>(value))
        raise TypeError(f"Expected `float`, but got: {type(value)}")

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAtomicInt.convert
        return ret

cdef class TypeCheckerAtomicPtr:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        if isinstance(value, None):
            return _MLCAnyNone()
        if isinstance(value, Ptr):
            return _MLCAnyPtr(_addr_from_ptr(value))
        raise TypeError(f"Expected `Ptr`, but got: {type(value)}")

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAtomicPtr.convert
        return ret

cdef class TypeCheckerAtomicDevice:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        cdef MLCAny ret = _MLCAnyNone()
        value = base.device_normalize(value)
        _func_call_impl(_DEVICE_INIT, (value,), &ret)
        return ret

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAtomicDevice.convert
        return ret

cdef class TypeCheckerAtomicDType:
    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        cdef MLCAny ret = _MLCAnyNone()
        value = base.dtype_normalize(value)
        _func_call_impl(_DTYPE_INIT, (value,), &ret)
        return ret

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerAtomicDType.convert
        return ret

cdef class TypeCheckAtomicObject:
    cdef int32_t type_index
    cdef int32_t type_depth
    cdef MLCFunc* any_to_ref

    def __init__(self, int32_t type_index):
        self.type_index = type_index
        self.type_depth = _type_index2py_type_info(type_index).type_depth
        try:
            self.any_to_ref = _vtable_get_func_ptr(_VTABLE_ANY_TO_REF, type_index, False)
        except Exception as e:  # no-cython-lint
            self.any_to_ref = NULL

    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        cdef TypeCheckAtomicObject self = <TypeCheckAtomicObject>_self
        cdef PyAny v
        cdef tuple type_ancestors  # ancestors of `value`'s type
        cdef MLCAny ret
        cdef str type_key
        # (1) Fast path: check `type_index` first
        try:
            assert value is not None
            v = <PyAny?>value
        except:   # no-cython-lint
            ...   # value is not `PyAny`, fall through
        else:
            # (1.1) Faster: `type_index` direct match
            if v._mlc_any.type_index == self.type_index:
                return v._mlc_any
            # (1.2) Slower: `type_index` in `value`'s `type_ancestors`
            type_ancestors = type(value)._mlc_type_info.type_ancestors
            try:
                if type_ancestors[self.type_depth] == self.type_index:
                    return v._mlc_any
            except IndexError:  # fall through
                ...
        # (2) Slow path: convert `value` to `MLCAny`, and then convert to `Ref<TObject>`
        if self.any_to_ref is not NULL:
            ret = _MLCAnyNone()
            _func_call_impl(self.any_to_ref, (value,), &ret)  # maybe throw exception
            temporary_storage.append(_pyany_no_inc_ref(ret))
            return ret
        type_key = _type_index2py_type_info(self.type_index).type_key
        raise TypeError(f"Expected type: `{type_key}`, but got: {type(value)}")

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckAtomicObject.convert
        return ret

cdef class TypeCheckerOptional:
    cdef TypeChecker sub

    def __init__(self, TypeChecker sub):
        self.sub = sub

    @staticmethod
    cdef MLCAny convert(object _self, object value, list temporary_storage):
        cdef TypeCheckerOptional self = <TypeCheckerOptional>_self
        if value is None:
            return _MLCAnyNone()
        return _type_checker_call(self.sub, value, temporary_storage)

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerOptional.convert
        return ret

cdef class TypeCheckerList:
    cdef TypeChecker sub

    def __init__(self, TypeChecker sub):
        self.sub = sub

    @staticmethod
    cdef MLCAny convert(object _self, object _value, list temporary_storage):
        from mlc.core.list import List as mlc_list

        cdef TypeCheckerList self = <TypeCheckerList>_self
        cdef tuple value
        cdef int32_t num_args = 0
        cdef MLCAny* c_args = NULL
        cdef MLCAny ret = _MLCAnyNone()
        if not isinstance(_value, (list, tuple, mlc_list)):
            raise TypeError(f"Expected `list` or `tuple`, but got: {type(_value)}")
        value = tuple(_value)
        num_args = len(value)
        c_args = <MLCAny*> malloc(num_args * sizeof(MLCAny))
        try:
            for i in range(num_args):
                c_args[i] = _type_checker_call(self.sub, value[i], temporary_storage)
            _func_call_impl_with_c_args(_LIST_INIT, num_args, c_args, &ret)
        finally:
            free(c_args)
        temporary_storage.append(_pyany_no_inc_ref(ret))
        return ret

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerList.convert
        return ret

cdef class TypeCheckerDict:
    cdef TypeChecker sub_k
    cdef TypeChecker sub_v

    def __init__(self, TypeChecker sub_k, TypeChecker sub_v):
        self.sub_k = sub_k
        self.sub_v = sub_v

    @staticmethod
    cdef MLCAny convert(object _self, object _value, list temporary_storage):
        cdef TypeCheckerDict self = <TypeCheckerDict>_self
        cdef tuple value
        cdef int32_t num_args = 0
        cdef MLCAny* c_args = NULL
        cdef MLCAny ret = _MLCAnyNone()
        cdef TypeChecker sub_k = self.sub_k
        cdef TypeChecker sub_v = self.sub_v
        if not isinstance(_value, dict):
            raise TypeError(f"Expected `dict`, but got: {type(_value)}")
        value = _flatten_dict_to_tuple(_value)
        num_args = len(value)
        c_args = <MLCAny*> malloc(num_args * sizeof(MLCAny))
        try:
            for i in range(0, num_args, 2):
                c_args[i] = _type_checker_call(sub_k, value[i], temporary_storage)
            for i in range(1, num_args, 2):
                c_args[i] = _type_checker_call(sub_v, value[i], temporary_storage)
            _func_call_impl_with_c_args(_DICT_INIT, num_args, c_args, &ret)
        finally:
            free(c_args)
        temporary_storage.append(_pyany_no_inc_ref(ret))
        return ret

    cdef TypeChecker get(self):
        cdef TypeChecker ret = TypeChecker()
        ret._self = self
        ret.convert = TypeCheckerDict.convert
        return ret

cdef inline MLCAny _type_checker_call(TypeChecker self, object value, list temporary_storage):
    return self.convert(self._self, value, temporary_storage)

cdef inline TypeChecker _type_checker_from_ty(MLCAny* ty):
    cdef int32_t atomic_type_index = -1
    cdef MLCAny* sub_ty_k = NULL
    cdef MLCAny* sub_ty_v = NULL
    if ty.type_index == kMLCTypingAny:
        return TypeCheckerAny().get()
    elif ty.type_index == kMLCTypingAtomic:
        atomic_type_index = (<MLCTypingAtomic*>ty).type_index
        if atomic_type_index == kMLCInt:
            return TypeCheckerAtomicInt().get()
        elif atomic_type_index == kMLCFloat:
            return TypeCheckerAtomicFloat().get()
        elif atomic_type_index == kMLCPtr:
            return TypeCheckerAtomicPtr().get()
        elif atomic_type_index == kMLCDevice:
            return TypeCheckerAtomicDevice().get()
        elif atomic_type_index == kMLCDataType:
            return TypeCheckerAtomicDType().get()
        elif atomic_type_index >= kMLCStaticObjectBegin:
            return TypeCheckAtomicObject(atomic_type_index).get()
        else:
            return None
    elif ty.type_index == kMLCTypingOptional:
        sub_ty_k = (<MLCTypingOptional*>ty).ty.ptr
        return TypeCheckerOptional(_type_checker_from_ty(sub_ty_k)).get()
    elif ty.type_index == kMLCTypingList:
        sub_ty_k = (<MLCTypingList*>ty).ty.ptr
        return TypeCheckerList(_type_checker_from_ty(sub_ty_k)).get()
    elif ty.type_index == kMLCTypingDict:
        sub_ty_k = (<MLCTypingDict*>ty).ty_k.ptr
        sub_ty_v = (<MLCTypingDict*>ty).ty_v.ptr
        return TypeCheckerDict(
            _type_checker_from_ty(sub_ty_k),
            _type_checker_from_ty(sub_ty_v),
        ).get()
    elif ty.type_index == kMLCTypingPtr:
        return None  # TODO
    else:
        raise ValueError(f"Unsupported type index: {ty.type_index}")

cdef tuple _type_field_accessor(object type_field: base.TypeField):
    cdef int64_t offset = type_field.offset
    cdef int32_t num_bytes = type_field.num_bytes
    cdef MLCAny* ty = (<PyAny?>(type_field.ty))._mlc_any.v.v_obj
    cdef int32_t idx = -1
    cdef TypeChecker checker
    if (ty.type_index, num_bytes) == (kMLCTypingAny, sizeof(MLCAny)):
        def f(PyAny self): return _any_c2py_inc_ref((<MLCAny*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0])
        def g(PyAny self, object value):  # no-cython-lint
            cdef MLCAny* addr = <MLCAny*>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
            cdef MLCAny save = addr[0]
            cdef list temporary_storage = []
            addr[0] = _any_py2c(value, temporary_storage)
            _check_error(_C_AnyInplaceViewToOwned(addr))
            _check_error(_C_AnyDecRef(&save))
        return f, g
    elif ty.type_index in (kMLCTypingAtomic, kMLCTypingList, kMLCTypingDict):
        idx = (<MLCTypingAtomic*>ty).type_index if ty.type_index == kMLCTypingAtomic else -1
        if (idx == -1 or idx >= kMLCStaticObjectBegin) and num_bytes == sizeof(MLCAny*):
            def f(PyAny self):
                cdef MLCAny* ptr = (<MLCAny**>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
                return _any_c2py_inc_ref(_MLCAnyObj(ptr)) if ptr != NULL else None
            checker = _type_checker_from_ty(ty)
            def g(PyAny self, object value):  # no-cython-lint
                cdef MLCAny **addr = <MLCAny**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny save = _MLCAnyNone() if addr[0] == NULL else _MLCAnyObj(addr[0])
                cdef list temporary_storage = []
                cdef MLCAny ret
                try:
                    ret = _type_checker_call(checker, value, temporary_storage)
                    if ret.type_index == kMLCNone:
                        addr[0] = NULL
                    elif ret.type_index >= kMLCStaticObjectBegin:
                        addr[0] = ret.v.v_obj
                        _check_error(_C_AnyIncRef(&ret))
                    else:
                        raise TypeError(f"Unexpected type index: {ret.type_index}")
                else:
                    _check_error(_C_AnyDecRef(&save))
            return f, g
        elif (idx, num_bytes) == (kMLCInt, 1):
            def f(PyAny self): return (<int8_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, int8_t value): (<int8_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCInt, 2):
            def f(PyAny self): return (<int16_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, int16_t value): (<int16_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCInt, 4):
            def f(PyAny self): return (<int32_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, int32_t value): (<int32_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCInt, 8):
            def f(PyAny self): return (<int64_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, int64_t value): (<int64_t*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCFloat, 4):
            def f(PyAny self): return (<float*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, double value): (<float*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCFloat, 8):
            def f(PyAny self): return (<double*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0]
            def g(PyAny self, double value): (<double*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = value
            return f, g
        elif (idx, num_bytes) == (kMLCPtr, sizeof(void*)):
            def f(PyAny self): return _Ptr((<void**>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0])
            def g(PyAny self, object ptr): (<void**>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0] = <void*>_addr_from_ptr(ptr)  # no-cython-lint
            return f, g
        elif (idx, num_bytes) == (kMLCDataType, sizeof(DLDataType)):
            def f(PyAny self): return _DataType((<DLDataType*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0])
            def g(PyAny self, object dtype):  # no-cython-lint
                cdef DLDataType* addr = <DLDataType*>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                _func_call_impl(_DTYPE_INIT, (base.dtype_normalize(dtype), ), &ret)
                addr[0] = ret.v.v_dtype
            return f, g
        elif (idx, num_bytes) == (kMLCDevice, sizeof(DLDevice)):
            def f(PyAny self): return _Device((<DLDevice*>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0])
            def g(PyAny self, object device):  # no-cython-lint
                cdef DLDevice* addr = <DLDevice*>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                _func_call_impl(_DEVICE_INIT, (base.device_normalize(device), ), &ret)
                addr[0] = ret.v.v_device
            return f, g
        elif (idx, num_bytes) == (kMLCRawStr, sizeof(char*)):
            def f(PyAny self): return str_c2py((<char**>(<uint64_t>(self._mlc_any.v.v_obj) + offset))[0])
            return f, None  # raw str is always read-only
    elif ty.type_index in (kMLCTypingPtr, kMLCTypingOptional) and num_bytes == sizeof(MLCAny*):
        if ty.type_index == kMLCTypingPtr:
            ty = (<MLCTypingPtr*>ty).ty.ptr
        elif ty.type_index == kMLCTypingOptional:
            ty = (<MLCTypingOptional*>ty).ty.ptr
        # Valid types:
        # - Optional[Atomic/List[...]/Dict[...]]
        # Invalid types:
        # - Optional[Any/None]
        # - Optional[Ptr/Optional/Union[...]]
        if ty.type_index == kMLCTypingAtomic:
            type_index = (<MLCTypingAtomic*>ty).type_index
        elif ty.type_index == kMLCTypingList:
            type_index = kMLCList
        elif ty.type_index == kMLCTypingDict:
            type_index = kMLCDict
        else:
            raise ValueError(f"Unsupported type field: {type_field.ty.__str__()}")
        if type_index >= kMLCStaticObjectBegin:
            def f(PyAny self):
                cdef MLCAny** addr = <MLCAny**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny* ptr = addr[0]
                return _any_c2py_inc_ref(_MLCAnyObj(ptr)) if ptr != NULL else None
            checker = TypeCheckerOptional(_type_checker_from_ty(ty)).get()
            def g(PyAny self, object value):  # no-cython-lint
                cdef MLCAny **addr = <MLCAny**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny save = _MLCAnyNone() if addr[0] == NULL else _MLCAnyObj(addr[0])
                cdef list temporary_storage = []
                cdef MLCAny ret
                try:
                    ret = _type_checker_call(checker, value, temporary_storage)
                    if ret.type_index == kMLCNone:
                        addr[0] = NULL
                    elif ret.type_index >= kMLCStaticObjectBegin:
                        addr[0] = ret.v.v_obj
                        _check_error(_C_AnyIncRef(&ret))
                    else:
                        raise TypeError(f"Unexpected type index: {ret.type_index}")
                else:
                    _check_error(_C_AnyDecRef(&save))
            return f, g
        elif type_index == kMLCInt:
            def f(PyAny self):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCBoxedPOD* ptr = addr[0]
                return ptr.data.v_int64 if ptr != NULL else None

            def g(PyAny self, object value):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                cdef MLCAny[2] args = [_MLCAnyPtr(<uint64_t>addr), _MLCAnyNone()]
                if value is not None:
                    args[1] = _MLCAnyInt(<int64_t?>value)
                _func_call_impl_with_c_args(_INT_NEW, 2, args, &ret)
            return f, g
        elif type_index == kMLCFloat:
            def f(PyAny self):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCBoxedPOD* ptr = addr[0]
                return ptr.data.v_float64 if ptr != NULL else None

            def g(PyAny self, object value):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                cdef MLCAny[2] args = [_MLCAnyPtr(<uint64_t>addr), _MLCAnyNone()]
                if value is not None:
                    args[1] = _MLCAnyFloat(<double?>value)
                _func_call_impl_with_c_args(_FLOAT_NEW, 2, args, &ret)
            return f, g
        elif type_index == kMLCPtr:
            def f(PyAny self):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCBoxedPOD* ptr = addr[0]
                return _Ptr(ptr.data.v_ptr) if ptr != NULL else None

            def g(PyAny self, object value):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                cdef MLCAny[2] args = [_MLCAnyPtr(<uint64_t>addr), _MLCAnyNone()]
                if value is not None:
                    args[1] = _MLCAnyPtr(_addr_from_ptr(value))
                _func_call_impl_with_c_args(_PTR_NEW, 2, args, &ret)
            return f, g
        elif type_index == kMLCDataType:
            def f(PyAny self):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCBoxedPOD* ptr = addr[0]
                return _DataType(ptr.data.v_dtype) if ptr != NULL else None

            def g(PyAny self, object value):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny[2] args = [_MLCAnyPtr(<uint64_t>addr), _MLCAnyNone()]
                cdef MLCAny ret = _MLCAnyNone()
                if value is not None:
                    _func_call_impl(_DTYPE_INIT, (base.dtype_normalize(value), ), &args[1])
                _func_call_impl_with_c_args(_DTYPE_NEW, 2, args, &ret)
            return f, g
        elif type_index == kMLCDevice:
            def f(PyAny self):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCBoxedPOD* ptr = addr[0]
                return _Device(ptr.data.v_device) if ptr != NULL else None

            def g(PyAny self, object value):
                cdef MLCBoxedPOD** addr = <MLCBoxedPOD**>(<uint64_t>(self._mlc_any.v.v_obj) + offset)
                cdef MLCAny ret = _MLCAnyNone()
                cdef MLCAny[2] args = [_MLCAnyPtr(<uint64_t>addr), _MLCAnyNone()]
                if value is not None:
                    _func_call_impl(_DEVICE_INIT, (base.device_normalize(value), ), &args[1])
                _func_call_impl_with_c_args(_DEVICE_NEW, 2, args, &ret)
            return f, g
    raise ValueError(f"Unsupported {num_bytes}-byte type field: {type_field.ty.__str__()}")


# Section 11. Interface with Python

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

cpdef PyAny func_get_untyped(str name):
    cdef PyAny ret = PyAny()
    _check_error(_C_FuncGetGlobal(NULL, str_py2c(name), &ret._mlc_any))
    return ret

cpdef list error_get_info(PyAny err):
    cdef list ret
    cdef int32_t num_strs
    cdef const char** strs
    _check_error(_C_ErrorGetInfo(err._mlc_any, &num_strs, &strs))
    ret = [str_c2py(strs[i]) for i in range(num_strs)]
    return ret

cpdef object error_pycode_fake(str filename, str funcname, int32_t lineno):
    return PyCode_NewEmpty(str_py2c(filename), str_py2c(funcname), lineno)

cpdef tuple dtype_as_triple(PyAny obj):
    cdef DLDataType dtype = obj._mlc_any.v.v_dtype
    cdef int32_t code = <int32_t>dtype.code
    cdef int32_t bits = <int32_t>dtype.code
    cdef int32_t lanes = <int32_t>dtype.lanes
    return code, bits, lanes

cpdef tuple device_as_pair(PyAny obj):
    cdef DLDevice device = obj._mlc_any.v.v_device
    return <int32_t>device.device_type, <int32_t>device.device_id

cpdef object type_key2py_type_info(str type_key):
    return _type_key2py_type_info(type_key)

cpdef object type_cast(PyAny typing_obj, object value):
    cdef TypeChecker checker = _type_checker_from_ty(typing_obj._mlc_any.v.v_obj)
    cdef list temporary_storage = []
    cdef MLCAny ret = _type_checker_call(checker, value, temporary_storage)
    return _any_c2py_inc_ref(ret)

cpdef tuple type_field_get_accessor(object type_field):
    assert isinstance(type_field, base.TypeField)
    return _type_field_accessor(type_field)

cpdef PyAny type_create_instance(object cls, int32_t type_index, int32_t num_bytes):
    cdef PyAny self = PyAny.__new__(cls)
    assert self._mlc_any.type_index == kMLCNone
    _check_error(_C_ExtObjCreate(num_bytes, type_index, &self._mlc_any))
    return self

cpdef void type_register_fields(int32_t type_index, list fields):
    cdef bytes name
    cdef list temporary_storage = []
    cdef PyAny ty
    cdef vector[MLCTypeField] mlc_fields

    for i, field in enumerate(fields):
        name = str_py2c(field.name)
        temporary_storage.append(name)
        ty = <PyAny?>(field.ty)
        mlc_fields.push_back(MLCTypeField(
            name=name,
            index=i,
            offset=field.offset,
            num_bytes=field.num_bytes,
            frozen=field.frozen,
            ty=ty._mlc_any.v.v_obj,
        ))
    _check_error(_C_TypeRegisterFields(NULL, type_index, len(fields), mlc_fields.data()))

cpdef void type_register_structure(
    int32_t type_index,
    int32_t struct_kind,
    tuple sub_structure_indices,
    tuple sub_structure_kinds,
):
    cdef vector[int32_t] mlc_sub_structure_indices
    cdef vector[int32_t] mlc_sub_structure_kinds
    for i in sub_structure_indices:
        mlc_sub_structure_indices.push_back(i)
    for i in sub_structure_kinds:
        mlc_sub_structure_kinds.push_back(i)
    _check_error(_C_TypeRegisterStructure(
        NULL, type_index,
        struct_kind, len(sub_structure_indices),
        mlc_sub_structure_indices.data(), mlc_sub_structure_kinds.data()
    ))

cpdef void type_add_method(
    int32_t type_index,
    str name,
    py_callable,
    int32_t kind,
):
    cdef PyAny func
    if isinstance(py_callable, PyAny):
        func = <PyAny?>py_callable
    else:
        func = _pyany_from_func(py_callable)
    _C_TypeAddMethod(NULL, type_index, MLCTypeMethod(
        name=str_py2c(name),
        func=<MLCFunc*>(func._mlc_any.v.v_obj),
        kind=kind,
    ))

cpdef list type_index2type_methods(int32_t type_index):
    cdef MLCTypeInfo* c_info = _type_index2c_type_info(type_index)
    cdef MLCTypeMethod* methods_ptr = c_info.methods
    cdef list methods = []
    while methods_ptr != NULL and methods_ptr.name != NULL:
        assert methods_ptr.func != NULL
        assert methods_ptr.func._mlc_header.type_index == kMLCFunc
        methods.append(base.TypeMethod(
            name=str_c2py(methods_ptr.name),
            func=_pyany_inc_ref(_MLCAnyObj(<MLCAny*>methods_ptr.func)),
            kind=methods_ptr.kind,
        ))
        methods_ptr += 1
    return methods

cpdef object type_index2cached_py_type_info(int32_t type_index):
    return TYPE_INDEX_TO_INFO[type_index]


def make_mlc_init(list fields):
    cdef tuple _setters = tuple(field.setter for field in fields)

    def _mlc_init(PyAny self, *args):
        cdef tuple setters = _setters
        cdef int32_t num_args = len(args)
        cdef int32_t i = 0
        cdef object e = None
        assert num_args == len(setters)
        while i < num_args:
            try:
                setters[i](self, args[i])
            except Exception as _e:  # no-cython-lint
                e = ValueError(f"Failed to set field `{fields[i].name}`: {str(_e)}. Got: {args[i]}")
                e = e.with_traceback(_e.__traceback__)
            if e is not None:
                raise e
            i += 1

    return _mlc_init


def type_create(int32_t parent_type_index, str type_key):
    cdef MLCTypeInfo* c_info = NULL
    cdef object type_info
    cdef int32_t type_index
    _check_error(_C_TypeRegister(NULL, parent_type_index, str_py2c(type_key), -1, &c_info))
    type_index = c_info.type_index
    type_info = base.TypeInfo(
        type_cls=None,
        type_index=type_index,
        type_key=type_key,
        type_depth=c_info.type_depth,
        type_ancestors=tuple(c_info.type_ancestors[i] for i in range(c_info.type_depth)),
        fields=(),
    )
    if (prev := _list_set(TYPE_INDEX_TO_INFO, type_index, type_info)) is not None:
        raise ValueError(
            f"Type index {type_index} already exists, "
            f"where current type key is `{type_key}`, "
            f"previous type key is `{prev.type_key}`"
        )
    return type_info


cdef list TYPE_INDEX_TO_INFO = [None]  # mapping: (type_index: int) ==> (type_info: base.TypeInfo)
cdef PyAny _SERIALIZE = func_get_untyped("mlc.core.JSONSerialize")  # Any -> str
cdef PyAny _DESERIALIZE = func_get_untyped("mlc.core.JSONDeserialize")  # str -> Any
cdef PyAny _STRUCUTRAL_EQUAL = func_get_untyped("mlc.core.StructuralEqual")
cdef PyAny _STRUCUTRAL_HASH = func_get_untyped("mlc.core.StructuralHash")

cdef MLCVTableHandle _VTABLE_INIT = _vtable_get_global(b"__init__")
cdef MLCVTableHandle _VTABLE_STR = _vtable_get_global(b"__str__")
cdef MLCVTableHandle _VTABLE_NEW_REF = _vtable_get_global(b"__new_ref__")
cdef MLCVTableHandle _VTABLE_ANY_TO_REF = _vtable_get_global(b"__any_to_ref__")

cdef MLCFunc* _INT_NEW = _vtable_get_func_ptr(_VTABLE_NEW_REF, kMLCInt, False)
cdef MLCFunc* _FLOAT_NEW = _vtable_get_func_ptr(_VTABLE_NEW_REF, kMLCFloat, False)
cdef MLCFunc* _PTR_NEW = _vtable_get_func_ptr(_VTABLE_NEW_REF, kMLCPtr, False)
cdef MLCFunc* _DTYPE_NEW = _vtable_get_func_ptr(_VTABLE_NEW_REF, kMLCDataType, False)
cdef MLCFunc* _DEVICE_NEW = _vtable_get_func_ptr(_VTABLE_NEW_REF, kMLCDevice, False)

cdef MLCFunc* _DTYPE_INIT = _vtable_get_func_ptr(_VTABLE_INIT, kMLCDataType, False)
cdef MLCFunc* _DEVICE_INIT = _vtable_get_func_ptr(_VTABLE_INIT, kMLCDevice, False)
cdef MLCFunc* _LIST_INIT = _vtable_get_func_ptr(_VTABLE_INIT, kMLCList, False)
cdef MLCFunc* _DICT_INIT = _vtable_get_func_ptr(_VTABLE_INIT, kMLCDict, False)
