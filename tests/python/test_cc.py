import mlc
import mlc.dataclasses as mlcd
import pytest
from mlc._cython import SYSTEM


@pytest.mark.xfail(
    condition=SYSTEM == "Windows",
    reason="`vcvarsall.bat` not found for some reason",
)
def test_jit_load() -> None:
    mlc.cc.jit_load("""
    #define MLC_JIT_EXPORTS 1
    #include <mlc/core/all.h>
    #include <string>

    struct MyObj : public mlc::Object {
    mlc::Str x;
    int32_t y;
    MyObj(mlc::Str x, int y) : x(x), y(y) {}
    int32_t YPlusOne() const { return y + 1; }
    MLC_DEF_DYN_TYPE(MLC_JIT_EXPORTS, MyObj, Object, "mlc.MyObj");
    };

    struct MyObjRef : public mlc::ObjectRef {
    MLC_DEF_OBJ_REF(MLC_JIT_EXPORTS, MyObjRef, MyObj, mlc::ObjectRef)
        .Field("x", &MyObj::x)
        .FieldReadOnly("y", &MyObj::y)
        .StaticFn("__init__", mlc::InitOf<MyObj, mlc::Str, int32_t>)
        .MemFn("YPlusOne", &MyObj::YPlusOne);
    };
    """)

    @mlcd.c_class("mlc.MyObj")
    class MyObj(mlc.Object):
        x: str
        y: int

        def YPlusOne(self) -> int:
            return type(self)._C(b"YPlusOne", self)

    obj = MyObj("hello", 42)
    assert obj.x == "hello"
    assert obj.y == 42
    assert obj.YPlusOne() == 43

    obj.x = "world"
    assert obj.x == "world"
    with pytest.raises(TypeError):
        obj.x = 42  # type: ignore[assignment]
    with pytest.raises(AttributeError):
        obj.y = 42
    del obj
