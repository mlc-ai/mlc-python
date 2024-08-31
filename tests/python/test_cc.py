import mlc
import pytest

CPP_SOURCE = """
#ifdef _WIN32
#pragma comment(lib, "mlc_registry.lib")
#endif
#include <mlc/all.h>
#include <string>

struct MyObj : public mlc::Object {
  std::string x;
  int32_t y;
  MyObj(std::string x, int y) : x(x), y(y) {}
  int32_t YPlusOne() const { return y + 1; }
  MLC_DEF_DYN_TYPE(MyObj, Object, "mlc.MyObj");
};

struct MyObjRef : public mlc::ObjectRef {
  MLC_DEF_OBJ_REF(MyObjRef, MyObj, mlc::ObjectRef)
      .Field("x", &MyObj::x)
      .FieldReadOnly("y", &MyObj::y)
      .Method("__init__", mlc::InitOf<MyObj, std::string, int32_t>)
      .Method("YPlusOne", &MyObj::YPlusOne);
};
"""


def test_jit_load() -> None:
    mlc.cc.jit_load(CPP_SOURCE)

    @mlc.register_type("mlc.MyObj")
    class MyObj(mlc.Object):
        x: str
        y: int

        def __init__(self, x: str, y: int) -> None:
            self._mlc_init("__init__", x, y)

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
