from __future__ import annotations

import ctypes
import dataclasses
import functools
import inspect
import typing
import warnings
from collections.abc import Callable
from io import StringIO

from . import c_struct

if typing.TYPE_CHECKING:
    from .core import PyAny, TypeAnn  # type: ignore[import-not-found]

Ptr = ctypes.c_void_p
_C_VoidPointer = int


@dataclasses.dataclass(eq=False)
class TypeField:
    name: str
    offset: int
    getter: _C_VoidPointer
    setter: _C_VoidPointer
    type_ann: TypeAnn
    is_read_only: bool
    is_owned_obj_ptr: bool


@dataclasses.dataclass(eq=False)
class TypeMethod:
    name: str
    func: PyAny
    kind: int


@dataclasses.dataclass(eq=False)
class TypeInfo:
    type_index: int
    type_key: str
    type_depth: int
    type_ancestors: tuple[int, ...]
    getter: _C_VoidPointer
    setter: _C_VoidPointer
    fields: tuple[TypeField, ...]
    methods: tuple[TypeMethod, ...]

    def prototype(self) -> str:
        io = StringIO()
        print(f"class {self.type_key}:", file=io)
        print(f"  # type_index: {self.type_index}; type_ancestors: {self.type_ancestors}", file=io)
        for field in self.fields:
            print(f"  {field.name}: {field.type_ann}", file=io)
        for fn in self.methods:
            if fn.kind == 0:
                print(f"  def {fn.name}(self, *args): ...", file=io)
            else:
                print("  @staticmethod", file=io)
                print(f"  def {fn.name}(*args): ...", file=io)
        return io.getvalue().rstrip()


def type_ann_flatten(type_ann: type) -> list[typing.Any]:
    flattened: list[typing.Any] = []

    def f(ann: type) -> None:  # noqa: PLR0912 (ignore "too many branches")
        for candidate in [int, float, str, Ptr]:
            if ann is candidate:
                flattened.append(ann)
                return
        if (ann is list) or (ann is typing.List):  # noqa: UP006
            flattened.append(list)
            flattened.append(typing.Any)
        elif (ann is dict) or (ann is typing.Dict):  # noqa: UP006
            flattened.append(dict)
            flattened.append(typing.Any)
            flattened.append(typing.Any)
        elif (ann is typing.Any) or (ann is Ellipsis):
            flattened.append(typing.Any)
        elif (type_info := getattr(ann, "_mlc_type_info", None)) is not None:
            flattened.append(type_info.type_index)
        elif (origin := typing.get_origin(ann)) is not None:
            from mlc.core import Dict, List

            args = typing.get_args(ann)
            if (origin is list) or (origin is List):
                if len(args) != 1:
                    raise ValueError(f"Unsupported type: {ann}")
                flattened.append(list)
                f(args[0])
            elif (origin is dict) or (origin is Dict):
                if len(args) != 2:
                    raise ValueError(f"Unsupported type: {ann}")
                flattened.append(dict)
                f(args[0])
                f(args[1])
            elif origin is tuple:
                raise ValueError("Unsupported type: `tuple`. Use `list` instead.")
        else:
            raise ValueError(f"Unsupported type: {ann}")

    f(type_ann)
    return flattened


ClsType = typing.TypeVar("ClsType")


def _attach_method(
    cls: ClsType,
    name: str,
    method: typing.Callable,
    annotations: dict[str, type] | None = None,
) -> None:
    method.__module__ = cls.__module__
    method.__name__ = name
    method.__qualname__ = f"{cls.__qualname__}.{name}"  # type: ignore[attr-defined]
    method.__doc__ = f"Method `{name}` of class `{cls.__qualname__}`"  # type: ignore[attr-defined]
    method.__annotations__ = annotations or {}
    setattr(cls, name, method)


def c_class(type_key: str) -> Callable[[type[ClsType]], type[ClsType]]:
    from . import core

    def field_to_property(f: TypeField) -> property:
        def f_getter(this: typing.Any, _key: str = f.name) -> typing.Any:
            return core.object_get_attr(this, f.offset, f.getter)

        def f_setter(this: typing.Any, value: typing.Any, _key: str = f.name) -> None:
            return core.object_set_attr(this, f.offset, f.setter, value)

        return property(
            fget=f_getter if f.getter else None,
            fset=f_setter if (f.setter and not f.is_read_only) else None,
            doc=f"{type_key}::{f.name}",
        )

    def method_to_callable(f: TypeMethod) -> typing.Callable:
        def mem_fn(this: typing.Any, *args: typing.Any) -> typing.Any:
            return core.func_call(f.func, (this, *args))

        @staticmethod  # type: ignore[misc]
        def static_fn(*args: typing.Any) -> typing.Any:
            return core.func_call(f.func, args)

        fn: typing.Callable
        if f.kind == 0:  # member method
            fn = mem_fn
        elif f.kind == 1:  # static method
            fn = static_fn
        else:
            raise ValueError(f"Unknown method kind: {f.kind}")
        fn.__name__ = f.name
        fn.__qualname__ = f"{type_key}.{f.name}"
        return fn

    def decorator(cls: type[ClsType]) -> type[ClsType]:
        @functools.wraps(cls, updated=())
        class SubCls(cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

        cls_fields = typing.get_type_hints(cls)
        info: TypeInfo = core.type_info_from_cxx(type_key)
        for field in info.fields:
            str_field = str(field.type_ann)
            if field.name in cls_fields:
                type_ann = core.TypeAnn(cls_fields.pop(field.name))
                str_cls_field = str(type_ann)
                if str_cls_field != str_field:
                    warnings.warn(
                        f"In `{cls.__qualname__}`, attribute `{field.name}` type mismatch. "
                        f"Expected `{str_field}`, but it is defined as `{str_cls_field}`."
                    )
            else:
                warnings.warn(
                    f"In `{cls.__qualname__}`, attribute `{field.name}` not found. "
                    f"Add `{field.name}: {str_field}` to class definition."
                )
            setattr(SubCls, field.name, field_to_property(field))
        for method in info.methods:
            if not method.name.startswith("_"):
                _attach_method(SubCls, method.name, method_to_callable(method))
        setattr(SubCls, "_mlc_type_info", info)
        core.register_vtable(
            SubCls,
            info,
            vtable={method.name: method.func for method in info.methods},
        )
        return SubCls

    return decorator


def py_class(
    type_key: str,
    init: bool = True,
    repr: bool = True,
) -> Callable[[type[ClsType]], type[ClsType]]:
    from . import core

    def extract_type_fields(cls: type) -> tuple[tuple[TypeField, ...], int]:
        fields: list[TypeField] = [
            TypeField(
                name=name,
                offset=-1,  # We'll fill this later
                getter=0,  # NULL
                setter=0,  # NULL
                type_ann=core.TypeAnn(ann),
                is_read_only=False,  # Assuming all fields are writable by default
                is_owned_obj_ptr=False,  # Assuming no owned object pointers by default
            )
            for name, ann in typing.get_type_hints(cls).items()
            if not name.startswith("_")
        ]

        class Cls(ctypes.Structure):
            _fields_ = [("_mlc_header", c_struct.MLCHeader)] + [
                (field.name, field.type_ann._ctype()) for field in fields
            ]

        for field in fields:
            field.offset = getattr(Cls, field.name).offset
            field.getter, field.setter = field.type_ann.get_getter_setter()
            assert field.getter, f"Getter not found for `{field.name}: {field.type_ann}`"
            assert field.setter, f"Setter not found for `{field.name}: {field.type_ann}`"
        return tuple(fields), ctypes.sizeof(Cls)

    def parent_type_cls(cls: type) -> type:
        for base in cls.__bases__:
            if hasattr(base, "_mlc_type_info"):
                return base
        raise ValueError(f"Parent type not found for `{cls.__qualname__}`")

    def field_to_property(f: TypeField) -> property:
        def f_getter(this: typing.Any, _key: str = f.name) -> typing.Any:
            return core.object_get_attr(this, f.offset, f.getter)

        def f_setter(this: typing.Any, value: typing.Any, _key: str = f.name) -> None:
            return core.object_set_attr_py(this, f.offset, f.setter, value, f.type_ann)

        return property(
            fget=f_getter if f.getter else None,
            fset=f_setter if (f.setter and not f.is_read_only) else None,
            doc=f"{type_key}::{f.name}",
        )

    def decorator(cls: type[ClsType]) -> type[ClsType]:
        from mlc.core.object import PyObject

        if not issubclass(cls, PyObject):
            raise TypeError(f"Not a subclass of `mlc.PyObject`: `{cls.__qualname__}`")

        info: TypeInfo = core.type_info_from_py(type_key, parent_type_cls(cls))
        info.fields, sizeof = extract_type_fields(cls)
        type_index = info.type_index

        @functools.wraps(cls, updated=())
        class SubCls(cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

        _attach_method(
            SubCls,
            "_mlc_pre_init",
            lambda self: core.ext_obj_init(self, type_index, sizeof),
        )
        if init:
            _attach_method(
                SubCls,
                "__init__",
                _method_init(SubCls, info.fields),
                annotations={ty: ann for ty, ann in typing.get_type_hints(cls).items()}
                | {"return": None},
            )
        if repr:
            _attach_method(
                SubCls,
                "__repr__",
                _method_repr(SubCls, info.fields),
                annotations={"return": str},
            )

        for field in info.fields:
            setattr(SubCls, field.name, field_to_property(field))
        setattr(SubCls, "_mlc_type_info", info)
        core.register_fields(info)
        core.register_vtable(
            SubCls,
            info,
            vtable={},
        )
        return SubCls

    return decorator


def _method_init(cls: ClsType, fields: tuple[TypeField, ...]) -> Callable[..., None]:
    sig = inspect.Signature(
        parameters=[
            inspect.Parameter(
                name=field.name,
                kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
            )
            for field in fields
        ],
    )

    def bind_args(*args: typing.Any, **kwargs: typing.Any) -> inspect.BoundArguments:
        try:
            bound = sig.bind(*args, **kwargs)
            bound.apply_defaults()
        except TypeError as e:
            raise TypeError(f"Error in `{cls.__qualname__}` constructor: {e}")  # type: ignore[attr-defined]
        return bound

    def method(self: ClsType, *args: typing.Any, **kwargs: typing.Any) -> None:
        self._mlc_pre_init()  # type: ignore[attr-defined]
        args = bind_args(*args, **kwargs).args
        assert len(args) == len(fields)
        for field, value in zip(fields, args):
            setattr(self, field.name, value)

    return method


def _method_repr(cls: ClsType, fields: tuple[TypeField, ...]) -> Callable[[ClsType], str]:
    field_names = [field.name for field in fields]

    def method(self: ClsType) -> str:
        cls_name = cls.__qualname__  # type: ignore[attr-defined]
        return f"{cls_name}({', '.join(f'{name}={getattr(self, name)!r}' for name in field_names)})"

    return method
