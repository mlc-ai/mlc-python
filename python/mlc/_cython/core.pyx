# cython: language_level=3
import ctypes
import platform
import itertools
import typing
from libc.stdint cimport int32_t, int64_t, uint64_t, uint8_t, uint16_t
from libc.stdlib cimport malloc, free
from libcpp.vector cimport vector
from numbers import Integral, Number
from cpython cimport Py_DECREF, Py_INCREF
from pathlib import Path
from .exception_utils import translate_exception_to_c, translate_exception_from_c
from .type_info import TypeField, TypeMethod, TypeInfo, Ptr, type_ann_flatten
from . import c_struct

cdef extern from "mlc/c_api.h" nogil:
    ctypedef void (*MLCDeleterType)(void *) nogil # no-cython-lint
    ctypedef void (*MLCFuncCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret) nogil # no-cython-lint
    ctypedef int32_t (*MLCFuncSafeCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret) noexcept nogil # no-cython-lint
    ctypedef int32_t (*MLCAttrGetterSetter)(MLCTypeField*, void *addr, MLCAny *) noexcept nogil # no-cython-lint

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
        MLCAttrGetterSetter getter
        MLCAttrGetterSetter setter
        MLCTypeInfo **type_annotation
        int32_t is_read_only
        int32_t is_owned_obj_ptr

    ctypedef struct MLCTypeMethod:
        const char* name
        MLCFunc *func
        int32_t kind

    ctypedef struct MLCTypeInfo:
        int32_t type_index
        char* type_key
        int32_t type_depth
        int32_t* type_ancestors
        MLCAttrGetterSetter getter
        MLCAttrGetterSetter setter
        MLCTypeField *fields
        MLCTypeMethod *methods

    ctypedef void* MLCTypeTableHandle

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
ctypedef void* (*_T_ExtObjCreate)(int32_t bytes, int32_t type_index) noexcept nogil # no-cython-lint
ctypedef void (*_T_ExtObjDelete)(void *objptr) noexcept nogil # no-cython-lint

cdef uint64_t _sym(object symbol):
    symbol: ctypes.CDLL._FuncPtr
    return <uint64_t>(ctypes.cast(symbol, Ptr).value)

SYSTEM = platform.system()
DSO_SUFFIX = ".so" if SYSTEM == "Linux" else ".dylib" if SYSTEM == "Darwin" else ".dll"
LIB_PATH = Path(__file__).parent.parent / "lib" / f"libmlc_registry{DSO_SUFFIX}"
if not LIB_PATH.exists():
    raise FileNotFoundError(f"Cannot find the MLC registry library at {LIB_PATH}")
LIB = ctypes.CDLL(str(LIB_PATH), ctypes.RTLD_GLOBAL)

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
cdef _T_ExtObjCreate _C_ExtObjCreate = <_T_ExtObjCreate>_sym(LIB.MLCExtObjCreate) # no-cython-lint
cdef _T_ExtObjDelete _C_ExtObjDelete = <_T_ExtObjDelete>_sym(LIB.MLCExtObjDelete) # no-cython-lint

PyCode_NewEmpty = ctypes.pythonapi.PyCode_NewEmpty
PyCode_NewEmpty.restype = ctypes.py_object


cdef class TypeAnn:
    cdef vector[MLCTypeInfo*] _vector

    def __init__(self, type_ann) -> None:
        cdef list py_type_annotation = type_ann_flatten(type_ann)
        translation = {
            typing.Any: kMLCNone,
            None: kMLCNone,
            int: kMLCInt,
            float: kMLCFloat,
            str: kMLCStr,
            Ptr: kMLCPtr,
            list: kMLCList,
            dict: kMLCDict,
        }
        for elem in py_type_annotation:
            if elem in translation:
                self._vector.push_back(_type_index2info(translation[elem]))
            elif isinstance(elem, int):
                self._vector.push_back(_type_index2info(elem))
            else:
                raise TypeError(f"Unsupported type annotation: {elem}")
        self._vector.push_back(NULL)

    @staticmethod
    cdef TypeAnn from_c(MLCTypeInfo** c_type_annotation):
        cdef TypeAnn ann = TypeAnn.__new__(TypeAnn)
        cdef int32_t i = 0
        while c_type_annotation[i] is not NULL:
            ann._vector.push_back(c_type_annotation[i])
            i += 1
        ann._vector.push_back(NULL)
        return ann

    def __str__(self) -> str:
        return self.__repr__()

    def __repr__(self) -> str:
        cdef MLCTypeInfo** ann = self._vector.data()
        cdef int32_t i = 0

        def f():
            nonlocal i
            cdef MLCTypeInfo* info = ann[i]
            i += 1
            if info[0].type_index == kMLCNone:
                return "Any"
            elif info[0].type_index == kMLCList:
                elem = f()
                return f"list[{elem}]"
            elif info[0].type_index == kMLCDict:
                key = f()
                value = f()
                return f"dict[{key}, {value}]"
            elif info[0].type_index in [kMLCRawStr, kMLCStr]:
                return "str"
            else:
                return str_c2py(info[0].type_key)

        return f()

    def _ctype(self):
        cdef MLCTypeInfo** ann = self._vector.data()
        cdef int32_t type_index = ann[0][0].type_index
        if type_index >= kMLCStaticObjectBegin:
            return c_struct.MLCObjPtr
        translation = {
            kMLCInt: ctypes.c_int64,
            kMLCFloat: ctypes.c_double,
            kMLCPtr: Ptr,
            kMLCDataType: c_struct.DLDataType,
            kMLCDevice: c_struct.DLDevice,
            kMLCRawStr: Ptr,
        }
        if type_index in translation:
            return translation[type_index]
        raise TypeError(f"Unsupported type index: {type_index}")

    def get_getter_setter(self) -> tuple[int, int]:
        cdef MLCTypeInfo* info = self._vector.data()[0]
        return <uint64_t>info[0].getter, <uint64_t>info[0].setter

cdef list TYPE_TABLE = []
cdef dict TYPE_VTABLE = {}
cdef list BUILTIN_STR = []
cdef list BUILTIN_INIT = []

cdef inline object _list_get(list source, int index):
    return source[index] if index < len(source) else None

cdef inline object _list_set(list source, int index, object value):
    cdef int32_t delta = index + 1 - len(source)
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
    c_ret[0] = _MLCAny()
    raise exception

cdef inline MLCAny _MLCAny():
    cdef MLCAny x
    x.type_index = kMLCNone
    x.ref_cnt = 0
    x.v_int64 = 0
    return x

cdef class PyAny:
    cdef MLCAny _mlc_any

    def __cinit__(self):
        self._mlc_any = _MLCAny()

    def __init__(self):
        pass

    def _mlc_init(self, init_func, *init_args) -> None:
        cdef PyAny func
        try:
            if isinstance(init_func, str):
                func = TYPE_VTABLE[type(self)][init_func]
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
        cdef PyAny func = TYPE_VTABLE[type(self)][name]
        return func_call(func, args)

    def __dealloc__(self):
        _check_error(_C_AnyDecRef(&self._mlc_any))

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

    def __repr__(self) -> str:
        global BUILTIN_STR
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
    cdef int32_t type_index = x.type_index
    cdef PyAny ret
    if type_index == kMLCNone:
        return None
    elif type_index == kMLCInt:
        return <int>(x.v_int64)
    elif type_index == kMLCFloat:
        return <float>(x.v_float64)
    elif type_index == kMLCPtr:
        return ctypes.cast(<unsigned long long>x.v_ptr, Ptr)
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
    cdef MLCAny y = _MLCAny()
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
    elif isinstance(x, Ptr):
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
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v_obj)
    cdef PyAny ret = PyAny()
    _func_call_impl(c_func, x, &ret._mlc_any)
    temporary_storage.append(ret)
    return ret._mlc_any

cdef inline MLCAny _any_py2c_dict(tuple x, list temporary_storage):
    from mlc.core.dict import Dict  # no-cython-lint
    cdef PyAny func = <PyAny>_list_get(BUILTIN_INIT, kMLCDict)
    cdef MLCFunc* func_ptr = <MLCFunc*>(func._mlc_any.v_obj)
    cdef PyAny ret = PyAny()
    _func_call_impl(func_ptr, x, &ret._mlc_any)
    temporary_storage.append(ret)
    return ret._mlc_any

cdef inline PyAny _any_typed_py2c(object x, MLCTypeInfo** ann, int32_t* i):
    cdef PyAny y = PyAny()
    cdef MLCTypeInfo* info = ann[i[0]]
    cdef int32_t type_index = 0
    assert info is not NULL
    type_index = info.type_index
    i[0] += 1
    if type_index == kMLCNone:  # it means `typing.Any`
        temporary_storage = []
        y._mlc_any = _any_py2c(x, temporary_storage)
        _check_error(_C_AnyInplaceViewToOwned(&y._mlc_any))
    elif type_index == kMLCInt:
        if isinstance(x, Integral):
            y._mlc_any.type_index = <int64_t>kMLCInt
            y._mlc_any.v_int64 = int(x)
        else:
            raise TypeError(f"Expected `Integral`, but got: {type(x)}")
    elif type_index == kMLCFloat:
        if isinstance(x, Number):
            y._mlc_any.type_index = <int64_t>kMLCFloat
            y._mlc_any.v_float64 = float(x)
        else:
            raise TypeError(f"Expected `Number`, but got: {type(x)}")
    elif type_index == kMLCPtr:
        if x is None:
            y._mlc_any.type_index = <int64_t>kMLCNone
            y._mlc_any.v_int64 = 0
        elif isinstance(x, Ptr):
            if x.value is None or x.value == 0:
                y._mlc_any.type_index = <int64_t>kMLCNone
                y._mlc_any.v_int64 = 0
            else:
                y._mlc_any.type_index = <int64_t>kMLCPtr
                y._mlc_any.v_int64 = x.value
        else:
            raise TypeError(f"Expected `Ptr`, i.e. `ctypes.c_void_p`, but got: {type(x)}")
    elif type_index == kMLCDataType:
        from mlc.core.dtype import DataType  # no-cython-lint
        y = DataType(x)
    elif type_index == kMLCDevice:
        from mlc.core.device import Device  # no-cython-lint
        y = Device(x)
    elif type_index == kMLCStr:
        if isinstance(x, Str):
            y._mlc_any = (<Str>x)._mlc_any
            _check_error(_C_AnyIncRef(&y._mlc_any))
        elif isinstance(x, str):
            x = str_py2c(x)
            y._mlc_any.type_index = <int64_t>kMLCRawStr
            y._mlc_any.v_str = x
            _check_error(_C_AnyInplaceViewToOwned(&y._mlc_any))
        else:
            raise TypeError(f"Expected `str`, but got: {type(x)}")
    elif type_index == kMLCFunc:
        from mlc.core.func import Func  # no-cython-lint
        if isinstance(x, Func):
            y._mlc_any = (<PyAny>x)._mlc_any
            _check_error(_C_AnyIncRef(&y._mlc_any))
        elif callable(x):
            y._mlc_any = _func_py2c(x)
        else:
            raise TypeError(f"Expected `callable`, but got: {type(x)}")
    elif type_index == kMLCList:
        from mlc.core.list import List  # no-cython-lint
        if isinstance(x, (tuple, list, List)):
            y = _any_typed_py2c_list(list(x), ann, i)
        else:
            raise TypeError(f"Expected `list`, but got: {type(x)}")
    elif type_index == kMLCDict:
        from mlc.core.dict import Dict  # no-cython-lint
        if isinstance(x, (dict, Dict)):
            y = _any_typed_py2c_dict(list(x.keys()), list(x.values()), ann, i)
        else:
            raise TypeError(f"Expected `dict`, but got: {type(x)}")
    elif type_index >= <int32_t>kMLCStaticObjectBegin:
        if isinstance(x, PyAny) and (<PyAny>x)._mlc_any.type_index == type_index:
            y._mlc_any = (<PyAny>x)._mlc_any
            _check_error(_C_AnyIncRef(&y._mlc_any))
        else:
            raise TypeError(f"Expected `{info[0].type_key}`, but got: {type(x)}")
    elif type_index == kMLCRawStr:
        raise TypeError("Raw string is not supported")
    else:
        raise TypeError(f"Unsupported type index: {type_index}")
    return y

cdef inline PyAny _any_typed_py2c_list(list x, MLCTypeInfo** ann, int32_t* i):
    cdef int32_t i_save = i[0]
    cdef PyAny func = <PyAny>_list_get(BUILTIN_INIT, kMLCList)
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v_obj)
    cdef int32_t num_args = len(x)
    cdef PyAny ret = PyAny()
    cdef MLCAny* c_args = <MLCAny*> malloc(num_args * sizeof(MLCAny))
    try:
        for j in range(num_args):
            i[0] = i_save
            x[j] = _any_typed_py2c(x[j], ann, i)
            c_args[j] = (<PyAny>x[j])._mlc_any
        _func_call_impl_with_c_args(c_func, num_args, c_args, &ret._mlc_any)
    finally:
        free(c_args)
    return ret

cdef inline PyAny _any_typed_py2c_dict(list ks, list vs, MLCTypeInfo** ann, int32_t* i):
    cdef int32_t i_save = i[0]
    cdef PyAny func = <PyAny>_list_get(BUILTIN_INIT, kMLCDict)
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v_obj)
    cdef int32_t num_args = len(ks)
    cdef PyAny ret = PyAny()
    cdef MLCAny* c_args = <MLCAny*> malloc(num_args * 2 * sizeof(MLCAny))
    assert c_func is not NULL
    try:
        for j in range(num_args):
            i[0] = i_save
            ks[j] = _any_typed_py2c(ks[j], ann, i)
            c_args[j * 2] = (<PyAny>ks[j])._mlc_any
        i_save = i[0]
        for j in range(num_args):
            i[0] = i_save
            vs[j] = _any_typed_py2c(vs[j], ann, i)
            c_args[j * 2 + 1] = (<PyAny>vs[j])._mlc_any
        _func_call_impl_with_c_args(c_func, num_args * 2, c_args, &ret._mlc_any)
    finally:
        free(c_args)
    return ret

cdef void _pyobj_deleter(void* handle) noexcept nogil:
    with gil:
        try:
            Py_DECREF(<object>(handle))
        except Exception as exception:
            # TODO(@junrushao): Will need to handle exceptions more gracefully
            print(f"Error in _pyobj_deleter: {exception}")

cdef inline MLCAny _func_py2c(object py_func):
    cdef MLCAny c_ret = _MLCAny()
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

cdef inline MLCTypeInfo* _type_index2info(int32_t type_index):
    cdef MLCTypeInfo* type_info
    _check_error(_C_TypeIndex2Info(NULL, type_index, &type_info))
    if type_info == NULL:
        raise ValueError(f"MLC cannot find type info for type index: {type_index}")
    return type_info

cpdef object func_call(PyAny func, tuple py_args):
    cdef MLCFunc* c_func = <MLCFunc*>(func._mlc_any.v_obj)
    cdef MLCAny c_ret = _MLCAny()
    _func_call_impl(c_func, py_args, &c_ret)
    return _any_c2py(c_ret)

cpdef object type_info_from_cxx(str type_key):
    cdef MLCTypeInfo* c_info = _type_key2info(type_key)
    cdef int32_t type_index = c_info[0].type_index
    cdef int32_t type_depth = c_info[0].type_depth
    cdef list type_ancestors = [c_info[0].type_ancestors[i] for i in range(type_depth)]
    cdef MLCAttrGetterSetter getter = c_info[0].getter
    cdef MLCAttrGetterSetter setter = c_info[0].setter
    cdef MLCTypeField* fields_ptr = c_info[0].fields
    cdef MLCTypeMethod* methods_ptr = c_info[0].methods
    cdef list fields = []
    cdef list methods = []
    cdef PyAny method_func
    while fields_ptr != NULL and fields_ptr[0].name != NULL:
        fields.append(TypeField(
            name=str_c2py(fields_ptr[0].name),
            offset=fields_ptr[0].offset,
            getter=<uint64_t>fields_ptr[0].getter,
            setter=<uint64_t>fields_ptr[0].setter,
            type_ann=TypeAnn.from_c(fields_ptr[0].type_annotation),
            is_read_only=bool(fields_ptr[0].is_read_only),
            is_owned_obj_ptr=bool(fields_ptr[0].is_owned_obj_ptr),
        ))
        fields_ptr += 1
    while methods_ptr != NULL and methods_ptr[0].name != NULL:
        method_func = PyAny()
        method_func._mlc_any.type_index = kMLCFunc
        method_func._mlc_any.v_obj = <MLCAny*>(methods_ptr[0].func)
        _check_error(_C_AnyIncRef(&method_func._mlc_any))
        methods.append(TypeMethod(
            name=str_c2py(methods_ptr[0].name),
            func=method_func,
            kind=methods_ptr[0].kind,
        ))
        methods_ptr += 1
    return TypeInfo(
        type_index=type_index,
        type_key=type_key,
        type_depth=type_depth,
        type_ancestors=tuple(type_ancestors),
        getter=<uint64_t>getter,
        setter=<uint64_t>setter,
        fields=tuple(fields),
        methods=tuple(methods),
    )

cpdef object type_info_from_py(str type_key, object parent_type_cls):
    cdef MLCTypeInfo* c_info = NULL
    cdef object parent_type_info = parent_type_cls._mlc_type_info
    _check_error(_C_TypeRegister(NULL, parent_type_info.type_index, str_py2c(type_key), -1, &c_info))
    return type_info_from_cxx(type_key)


cpdef void register_vtable(object type_cls, object type_info, dict vtable):
    type_info: TypeInfo
    cdef int32_t type_index = type_info.type_index
    cdef str type_key = type_info.type_key
    if (_list_set(TYPE_TABLE, type_index, type_cls) is not None):
        raise ValueError(f"Type is already registered: {type_key}")
    for name, value in vtable.items():
        if not isinstance(value, PyAny):
            raise TypeError(f"Invalid type of value in vtable: {type(value)}")
        if name == "__str__":
            _list_set(BUILTIN_STR, type_index, value)
        elif name == "__init__":
            _list_set(BUILTIN_INIT, type_index, value)
        TYPE_VTABLE.setdefault(type_cls, {})[name] = value

cpdef void register_fields(object type_info):
    type_info: TypeInfo
    cdef vector[MLCTypeField] fields
    cdef list temporary_storage = []
    cdef TypeAnn type_ann
    cdef int32_t type_index

    for field in type_info.fields:
        name = str_py2c(field.name)
        temporary_storage.append(name)
        type_ann = <TypeAnn>(field.type_ann)
        type_index = type_ann._vector[0][0].type_index
        fields.push_back(MLCTypeField(
            name=name,
            offset=field.offset,
            getter=<MLCAttrGetterSetter>field.getter,
            setter=<MLCAttrGetterSetter>field.setter,
            type_annotation=(<TypeAnn>(field.type_ann))._vector.data(),
            is_read_only=False,
            is_owned_obj_ptr=type_index >= kMLCStaticObjectBegin,
        ))
    _check_error(_C_TypeDefReflection(NULL, type_info.type_index, fields.size(), fields.data(), 0, NULL))

cpdef void register_func(str name, bint allow_override, object func):
    mlc_func: PyAny = PyAny._new_from_mlc_any(_func_py2c(func))
    _check_error(_C_FuncSetGlobal(NULL, str_py2c(name), mlc_func._mlc_any, allow_override))

cpdef object get_global_func(str name):
    cdef MLCAny c_ret = _MLCAny()
    _check_error(_C_FuncGetGlobal(NULL, str_py2c(name), &c_ret))
    return PyAny._new_from_mlc_any(c_ret)

cpdef object object_get_attr(object this, int64_t offset, uint64_t getter):
    cdef void* addr = <void*>((<uint64_t>_extract_obj_ptr(this)) + offset)
    cdef MLCAttrGetterSetter c_getter = <MLCAttrGetterSetter>(getter)
    cdef MLCAny c_ret = _MLCAny()
    _check_error_from(c_getter(NULL, addr, &c_ret), &c_ret)
    return _any_c2py(c_ret)

cpdef void object_set_attr(object this, int64_t offset, uint64_t setter, object value):
    cdef void* addr = <void*>((<uint64_t>_extract_obj_ptr(this)) + offset)
    cdef MLCAttrGetterSetter c_setter = <MLCAttrGetterSetter>(setter)
    cdef list temporary_storage = []
    cdef MLCAny c_val = _any_py2c(value, temporary_storage)
    _check_error_from(c_setter(NULL, addr, &c_val), &c_val)

cpdef void object_set_attr_py(object this, int64_t offset, uint64_t setter, object value, TypeAnn type_ann):
    cdef void* addr = <void*>((<uint64_t>_extract_obj_ptr(this)) + offset)
    cdef MLCAttrGetterSetter c_setter = <MLCAttrGetterSetter>(setter)
    cdef int32_t cast_i = 0
    cdef PyAny value_cast = _any_typed_py2c(value, type_ann._vector.data(), &cast_i)
    cdef MLCAny c_val = value_cast._mlc_any
    _check_error_from(c_setter(NULL, addr, &c_val), &c_val)
    value_cast._mlc_any = _MLCAny()

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

cpdef void ext_obj_init(PyAny self, int32_t type_index, int32_t sizeof_):
    self._mlc_any.type_index = type_index
    self._mlc_any.v_obj = <MLCAny*>(_C_ExtObjCreate(type_index, sizeof_))

cpdef object testing_cast(object x, TypeAnn type_ann):
    cdef int32_t cast_i = 0
    cdef PyAny cast_ret = _any_typed_py2c(x, type_ann._vector.data(), &cast_i)
    cdef int32_t type_index = cast_ret._mlc_any.type_index
    cdef PyAny ret
    cdef Str str_ret
    if type_index == kMLCNone:
        return None
    if type_index == kMLCStr:
        str_ret = Str._new_from_mlc_any(cast_ret._mlc_any)
        _check_error(_C_AnyIncRef(&str_ret._mlc_any))
        return str_ret
    ret = PyAny._new_from_mlc_any(cast_ret._mlc_any)
    _check_error(_C_AnyIncRef(&ret._mlc_any))
    return ret
