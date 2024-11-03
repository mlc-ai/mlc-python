from __future__ import annotations

import ctypes
import inspect
import typing
from collections.abc import Callable

from mlc._cython import TypeField, TypeInfo, TypeMethod, type_create, type_field_get_accessor

from .utils import attach_field, attach_method

ClsType = typing.TypeVar("ClsType")


def py_class(
    type_key: str,
    init: bool = True,
    repr: bool = True,
) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        extra_methods: dict[str, Callable] = {}
        methods: list[TypeMethod] = []
        fields: list[TypeField]
        type_cls: type[ClsType]
        type_info: TypeInfo

        # Step 1. Extract fields and methods
        type_hints = typing.get_type_hints(super_type_cls)
        CType, fields = _model_as_ctype(super_type_cls, type_hints)

        # Step 2. Add extra methods
        if init:
            extra_methods["__init__"] = _insert_to_method_list(
                methods,
                name="__init__",
                fn=_method_init(super_type_cls, type_hints, fields),
                is_static=False,
            )
        if repr:
            extra_methods["__repr__"] = _insert_to_method_list(
                methods,
                name="__repr__",
                fn=_method_repr(type_key, fields),
                is_static=True,
            )

        # Step 3. Register the type
        parent_type_info: TypeInfo = _type_parent(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_cls, type_info = type_create(
            super_type_cls,
            type_key=type_key,
            fields=tuple(fields),
            methods=tuple(methods),
            parent_type_index=parent_type_info.type_index,
            num_bytes=ctypes.sizeof(CType),
        )

        # Step 4. Attach fields
        for field in type_info.fields:
            attach_field(
                type_cls,
                name=field.name,
                getter=field.getter,
                setter=field.setter,
                frozen=field.frozen,
            )

        # Step 5. Attach methods
        for method_name, fn in extra_methods.items():
            attach_method(
                type_cls,
                name=method_name,
                method=fn,
            )
        return type_cls

    return decorator


def _type_parent(type_cls: type) -> type:
    from mlc.core.object import PyClass

    if not issubclass(type_cls, PyClass):
        raise TypeError(
            f"Not a subclass of `mlc.PyClass`: `{type_cls.__module__}.{type_cls.__qualname__}`"
        )
    for base in type_cls.__bases__:
        if hasattr(base, "_mlc_type_info"):
            return base
    raise ValueError(
        f"No parent found for `{type_cls.__module__}.{type_cls.__qualname__}`. It must inherit from `mlc.PyClass`."
    )


def _model_as_ctype(
    type_cls: type,
    type_hints: dict[str, type],
) -> tuple[type[ctypes.Structure], list[TypeField]]:
    from mlc._cython import MLCHeader
    from mlc.core import typing as mlc_typing

    fields: list[TypeField] = []
    c_fields = [
        ("_mlc_header", MLCHeader),
    ]
    for field_name, field_ty_py in type_hints.items():
        if field_name.startswith("_"):
            continue
        field_ty = mlc_typing.from_py(field_ty_py)
        field_ty_c = field_ty._ctype()
        c_fields.append((field_name, field_ty_c))
        fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=ctypes.sizeof(field_ty_c),
                frozen=False,
                ty=field_ty,
            )
        )

    class CType(ctypes.Structure):
        _fields_ = c_fields

    for field in fields:
        field.offset = getattr(CType, field.name).offset
        field.getter, field.setter = type_field_get_accessor(field)
    return CType, fields


def _insert_to_method_list(
    methods: list[TypeMethod],
    name: str,
    fn: Callable[..., typing.Any],
    is_static: bool,
) -> Callable[..., typing.Any]:
    from mlc.core import Func

    method = TypeMethod(
        name=name,
        func=Func(fn),
        kind=1 if is_static else 0,
    )

    for i, method in enumerate(methods):
        if method.name == name:
            methods[i] = method
            return fn
    methods.append(method)
    return fn


def _method_init(
    type_cls: type[ClsType],
    type_hints: dict[str, type],
    fields: list[TypeField],
) -> Callable[..., None]:
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
            raise TypeError(
                f"Error in `{type_cls.__module__}.{type_cls.__qualname__}.__init__`: {e}"
            )  # type: ignore[attr-defined]
        return bound

    def method(self: ClsType, *args: typing.Any, **kwargs: typing.Any) -> None:
        self._mlc_pre_init()  # type: ignore[attr-defined]
        args = bind_args(*args, **kwargs).args
        assert len(args) == len(fields)
        for field, value in zip(fields, args):
            setattr(self, field.name, value)

    method.__annotations__ = dict(type_hints) | {"return": None}
    return method


def _method_repr(
    type_key: str,
    fields: list[TypeField],
) -> Callable[[ClsType], str]:
    field_names = tuple(field.name for field in fields)

    def method(self: ClsType) -> str:
        return f"{type_key}({', '.join(f'{name}={getattr(self, name)!r}' for name in field_names)})"

    return method
