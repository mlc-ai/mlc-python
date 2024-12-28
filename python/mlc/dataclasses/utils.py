from __future__ import annotations

import ctypes
import dataclasses
import inspect
import typing
from collections.abc import Callable
from io import StringIO
from typing import Any, Literal, TypeVar, get_type_hints

from mlc._cython import (
    MISSING,
    Field,
    TypeField,
    TypeInfo,
    TypeMethod,
    type_add_method,
    type_index2type_methods,
)
from mlc.core import typing as mlc_typing

KIND_MAP = {None: 0, "nobind": 1, "bind": 2, "var": 3}
SUB_STRUCTURES = {"nobind": 0, "bind": 1}


class DefaultFactory(typing.NamedTuple):
    fn: Callable[[], typing.Any]


def method_init(
    type_cls: type,
    fields: list[Field],
) -> Callable[..., None]:
    annotations: dict[str, typing.Any] = {"return": None}
    params_without_defaults: list[inspect.Parameter] = []
    params_with_defaults: list[inspect.Parameter] = []
    ordering = [0] * len(fields)
    for i, field in enumerate(fields):
        assert field.name is not None
        name: str = field.name
        annotations[name] = typing.Any  # TODO: Handle annotations better
        if field.default_factory is MISSING:
            ordering[i] = len(params_without_defaults)
            params_without_defaults.append(
                inspect.Parameter(
                    name=name,
                    kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
                )
            )
        else:
            ordering[i] = -len(params_with_defaults) - 1
            params_with_defaults.append(
                inspect.Parameter(
                    name=name,
                    kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
                    default=DefaultFactory(fn=field.default_factory),
                )
            )
    for i, order in enumerate(ordering):
        if order < 0:
            ordering[i] = len(params_without_defaults) - order - 1
    sig = inspect.Signature(
        parameters=[
            *params_without_defaults,
            *params_with_defaults,
        ],
    )

    signature_str = (
        f"{type_cls.__module__}.{type_cls.__qualname__}.__init__("
        + ", ".join(p.name for p in sig.parameters.values())
        + ")"
    )

    def bind_args(*args: typing.Any, **kwargs: typing.Any) -> inspect.BoundArguments:
        bound = sig.bind(*args, **kwargs)
        bound.apply_defaults()
        return bound

    def method(self: type, *args: typing.Any, **kwargs: typing.Any) -> None:
        e = None
        try:
            args = bind_args(*args, **kwargs).args
            args = tuple(arg.fn() if isinstance(arg, DefaultFactory) else arg for arg in args)
            args = tuple(args[order] for order in ordering)
            self._mlc_init(*args)  # type: ignore[attr-defined]
        except Exception as _e:
            e = TypeError(f"Error in `{signature_str}`: {_e}").with_traceback(_e.__traceback__)
        if e is not None:
            raise e
        try:
            post_init = self.__post_init__  # type: ignore[attr-defined]
        except AttributeError:
            pass
        else:
            post_init()

    method.__signature__ = sig  # type: ignore[attr-defined]
    method.__annotations__ = annotations
    return method


def field(
    *,
    default: Any = MISSING,
    default_factory: Any = MISSING,
    structure: Literal["bind", "nobind"] | None = "nobind",
) -> Any:
    if default is not MISSING and default_factory is not MISSING:
        raise ValueError("Cannot specify both `default` and `default_factory`")
    if default is not MISSING:
        default_factory = lambda: default
    return Field(
        structure=structure,
        default_factory=default_factory,
        name=None,
    )


def inspect_dataclass_fields(  # noqa: PLR0912
    type_key: str,
    type_cls: type,
    parent_type_info: TypeInfo,
) -> tuple[list[TypeField], list[Field]]:
    def _get_num_bytes(field_ty: Any) -> int:
        if hasattr(field_ty, "_ctype"):
            return ctypes.sizeof(field_ty._ctype())
        return 0

    # Step 1. Inspect and extract all the `TypeField`s
    type_hints = get_type_hints(type_cls)
    type_fields: list[TypeField] = []
    for type_field in parent_type_info.fields:
        field_name = type_field.name
        field_ty = type_field.ty
        if type_hints.pop(field_name, None) is None:
            raise ValueError(
                f"Missing field `{type_key}::{field_name}`, "
                f"which appears in its parent class `{parent_type_info.type_key}`."
            )
        type_fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=_get_num_bytes(field_ty),
                frozen=False,
                ty=field_ty,
            )
        )
    for field_name, field_ty_py in type_hints.items():
        if field_name.startswith("_"):
            continue
        field_ty = mlc_typing.from_py(field_ty_py)
        type_fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=_get_num_bytes(field_ty),
                frozen=False,
                ty=field_ty,
            )
        )
    # Step 2. Convert `TypeField`s to dataclass `Field`s
    d_fields: list[Field] = []
    for type_field in type_fields:
        lhs = type_field.name
        if lhs.startswith("_"):
            continue
        rhs = getattr(type_cls, lhs, MISSING)
        if isinstance(rhs, property):
            for parent_d_field in parent_type_info.d_fields:
                if parent_d_field.name == lhs:
                    rhs = parent_d_field
                    break
        if rhs is MISSING:
            d_field = field()
        elif isinstance(rhs, (int, float, str, bool, type(None))):
            d_field = field(default=rhs)
        elif isinstance(rhs, Field):
            d_field = rhs
        else:
            raise ValueError(f"Cannot recognize field: {type_field.name}: {rhs}")
        d_field.name = type_field.name
        d_fields.append(d_field)
    return type_fields, d_fields


@dataclasses.dataclass
class Structure:
    kind: Literal["bind", "nobind", "var"] | None
    fields: tuple[str, ...] = dataclasses.field(default_factory=tuple)

    def _type_check(self) -> None:
        if not isinstance(self.kind, (str, type(None))):
            raise ValueError(
                f"Invalid `Structure.kind`. Expected `str`, but got: {type(self.kind)}"
            )
        if self.kind is None and self.fields:
            raise ValueError(
                "Invalid `Structure.fields`. Expected empty tuple, but got non-empty tuple"
            )
        if not isinstance(self.fields, tuple):
            raise ValueError(
                f"Invalid `Structure.fields`. Expected `tuple`, but got: {type(self.fields)}"
            )
        for field in self.fields:
            if not isinstance(field, str):
                raise ValueError(
                    f"Invalid element in `Structure.fields`. Expected `str`, but got: {type(field)}"
                )

    def __post_init__(self) -> None:
        self._type_check()
        fields = []
        if self.kind not in KIND_MAP:
            raise ValueError(
                f"Invalid `_mlc_structure`. Expected kind in `{KIND_MAP}`, but got `{self.kind}`"
            )
        for name_kind in self.fields:
            name, kind = _parse_name_kind(name_kind)
            fields.append(f"{name}:{kind}")
        self.fields = tuple(fields)


def _parse_name_kind(name_kind: str) -> tuple[str, str]:
    if ":" in name_kind:
        name, kind = name_kind.split(":")
    else:
        name, kind = name_kind, "nobind"
    if kind not in SUB_STRUCTURES:
        raise ValueError(
            f"Invalid `_mlc_structure`. Expected kind in `{SUB_STRUCTURES}`, but got `{name_kind}`"
        )
    return name, kind


def structure_to_c(
    struct: Structure,
    fields: list[TypeField],
) -> tuple[
    int,  # structure_kind
    list[int],  # sub_structure_indices
    list[int],  # sub_structure_kinds
]:
    field_to_indices = {field.name: i for i, field in enumerate(fields)}
    sub_indices: list[int] = []
    sub_kinds: list[int] = []
    for name, kind in map(_parse_name_kind, struct.fields):
        if name not in field_to_indices:
            raise ValueError(
                f"Invalid field name in `_mlc_structure`. Expected field name in `{field_to_indices}`, "
                f"but got: {name}"
            )
        sub_indices.append(field_to_indices[name])
        sub_kinds.append(SUB_STRUCTURES[kind])
    return KIND_MAP[struct.kind], sub_indices, sub_kinds


def structure_parse(
    structure: Literal["bind", "nobind", "var"] | None,
    d_fields: list[Field],
) -> Structure:
    assert structure in (None, "bind", "nobind", "var")
    if structure is None:
        return Structure(kind=None, fields=())
    struct_fields: list[str] = []
    for d_field in d_fields:
        sub_structure = d_field.structure
        assert sub_structure in (None, "nobind", "bind")
        if sub_structure is not None:
            struct_fields.append(f"{d_field.name}:{sub_structure}")
    return Structure(
        kind=structure,
        fields=tuple(struct_fields),
    )


def get_parent_type(type_cls: type) -> type:
    for base in type_cls.__bases__:
        if hasattr(base, "_mlc_type_info"):
            return base
    raise ValueError(
        f"No parent type found for `{type_cls.__module__}.{type_cls.__qualname__}`. "
        f"The type must inherit from `mlc.Object`."
    )


def add_vtable_method(
    type_cls: type,
    name: str,
    is_static: bool,
    method: Callable[..., Any],
) -> None:
    if (type_info := getattr(type_cls, "_mlc_type_info", None)) is None:
        raise ValueError(f"Invalid type: {type_cls}")
    assert isinstance(type_info, TypeInfo)
    type_add_method(
        type_index=type_info.type_index,
        name=name,
        py_callable=method,
        kind=int(is_static),
    )


FuncType = TypeVar("FuncType", bound=Callable[..., Any])


def vtable_method(is_static: bool) -> Callable[[FuncType], FuncType]:
    def decorator(method: FuncType) -> FuncType:
        method._mlc_is_static_func = is_static  # type: ignore[attr-defined]
        return method

    return decorator


def add_vtable_methods_for_type_cls(type_cls: type, type_index: int) -> None:
    for name, func in vars(type_cls).items():
        if not callable(func):
            continue
        # kind: 0  =>  member method
        # kind: 1  =>  static method
        if (is_static := getattr(func, "_mlc_is_static_func", None)) is not None:
            type_add_method(type_index, name, func, kind=int(is_static))
        elif name in ("__ir_print__",):
            type_add_method(type_index, name, func, kind=0)


def prototype_py(type_info: type | TypeInfo) -> str:
    if not isinstance(type_info, TypeInfo):
        if (type_info := getattr(type_info, "_mlc_type_info", None)) is None:  # type: ignore[assignment]
            raise ValueError(f"Invalid type: {type_info}")
    assert isinstance(type_info, TypeInfo)
    cls_name = type_info.type_key.rsplit(".", maxsplit=1)[-1]

    io = StringIO()
    print(f"@mlc.dataclasses.c_class({type_info.type_key!r})", file=io)
    print(f"class {cls_name}:", file=io)
    for field in type_info.fields:
        ty = str(field.ty)
        if ty == "char*":
            ty = "str"
        print(f"  {field.name}: {ty}", file=io)
    fn: TypeMethod
    for fn in type_index2type_methods(type_info.type_index):
        if fn.name.startswith("_"):
            continue
        if fn.kind == 0:
            print(f"  def {fn.name}(self, *args): ...", file=io)
        else:
            print("  @staticmethod", file=io)
            print(f"  def {fn.name}(*args): ...", file=io)
    return io.getvalue().rstrip()


def prototype_cxx(type_info: type | TypeInfo) -> str:
    if not isinstance(type_info, TypeInfo):
        if (type_info := getattr(type_info, "_mlc_type_info", None)) is None:  # type: ignore[assignment]
            raise ValueError(f"Invalid type: {type_info}")
    assert isinstance(type_info, TypeInfo)

    parent_type_info = type_info.get_parent()
    namespaces = type_info.type_key.split(".")
    cls_name = namespaces[-1]
    parent_ref_name = mlc_typing.AtomicType(type_index=parent_type_info.type_index).cxx_str()
    parent_obj_name = (
        parent_ref_name + "Obj"  ##
        if parent_ref_name != "::mlc::ObjectRef"
        else "::mlc::Object"
    )

    fields: list[tuple[str, str]] = []
    for field in type_info.fields:
        fields.append((field.name, field.ty.cxx_str()))

    io = StringIO()
    # Step 1. Namespace
    # Step 2. Object class
    for ns in namespaces[:-1]:
        print(f"namespace {ns} {{", file=io)
    print(f"struct {cls_name}Obj : public ::mlc::Object {{", file=io)
    # Step 2.1. Fields
    for name, ty in fields:
        print(f"  {ty} {name};", file=io)
    # Step 2.2. Constructor
    print(f"  explicit {cls_name}Obj(", file=io, end="")
    for i, (name, ty) in enumerate(fields):
        if i != 0:
            print(", ", file=io, end="")
        print(f"{ty} {name}", file=io, end="")
    print("): ", file=io, end="")
    for i, (name, _) in enumerate(fields):
        if i != 0:
            print(", ", file=io, end="")
        print(f"{name}({name})", file=io, end="")
    print(" {}", file=io)
    # Step 2.3. Macro to define object type
    print(
        f'  MLC_DEF_DYN_TYPE({cls_name}Obj, {parent_obj_name}, "{type_info.type_key}");',
        file=io,
    )
    print(f"}};  // struct {cls_name}Obj\n", file=io)
    # Step 3. Object reference class
    print(f"struct {cls_name} : public {parent_ref_name} {{", file=io)
    # Step 3.1. Define fields for reflection
    print(f"  MLC_DEF_OBJ_REF({cls_name}, {cls_name}Obj, {parent_ref_name})", file=io)
    for name, _ in fields:
        print(f'    .Field("{name}", &{cls_name}Obj::{name})', file=io)
    # Step 3.2. Define `__init__` method for reflection
    print(f'    .StaticFn("__init__", ::mlc::InitOf<{cls_name}Obj', file=io, end="")
    for i, (_, ty) in enumerate(fields):
        print(f", {ty}", file=io, end="")
    print(">);", file=io)
    print(f"}};  // struct {cls_name}", file=io)
    # Step 4. End namespace
    for ns in reversed(namespaces[:-1]):
        print(f"}}  // namespace {ns}", file=io)
    return io.getvalue().rstrip()
