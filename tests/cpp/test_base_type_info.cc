#include "./common.h"
#include <gtest/gtest.h>
#include <mlc/core/all.h>

namespace {
using namespace mlc;

static_assert(base::IsObj<Object>, "IsObj<Object> == true");
static_assert(base::IsObj<FuncObj>, "IsObj<Func> == true");
static_assert(base::IsObj<StrObj>, "IsObj<Str> == true");

struct SubType : public Object {
  int data;
  explicit SubType(int data) : data(data) {
    if (data == 1) {
      throw std::runtime_error("New Error");
    }
  }
  MLC_DEF_DYN_TYPE(MLC_CPPTESTS_EXPORTS, SubType, Object, "test.SubType");
};

struct TestObj : public Object {
  int x;
  explicit TestObj(int x) : x(x) {}
  MLC_DEF_DYN_TYPE(MLC_CPPTESTS_EXPORTS, TestObj, Object, "test.TestObj");
};

struct SubTestObj : public TestObj {
  int y;
  explicit SubTestObj(int x, int y) : TestObj(x), y(y) {}
  MLC_DEF_DYN_TYPE(MLC_CPPTESTS_EXPORTS, SubTestObj, TestObj, "test.SubTestObj");
};

void CheckAncestor(int32_t num, const int32_t *ancestors, std::vector<int32_t> expected) {
  EXPECT_EQ(num, expected.size());
  for (int i = 0; i < num; ++i) {
    EXPECT_EQ(ancestors[i], expected[i]);
  }
}

TEST(StaticTypeInfo, Object) {
  EXPECT_EQ(Object::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCObject));
  EXPECT_STRCASEEQ(Object::_type_key, "object.Object");
  EXPECT_EQ(Object::_type_depth, 0);
  CheckAncestor(Object::_type_depth, Object::_type_ancestors, {});
}

TEST(StaticTypeInfo, FuncObj) {
  EXPECT_EQ(FuncObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCFunc));
  EXPECT_STRCASEEQ(FuncObj::_type_key, "object.Func");
  EXPECT_EQ(FuncObj::_type_depth, 1);
  CheckAncestor(FuncObj::_type_depth, FuncObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(StaticTypeInfo, StrObj) {
  EXPECT_EQ(StrObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
  EXPECT_STRCASEEQ(StrObj::_type_key, "object.Str");
  EXPECT_EQ(StrObj::_type_depth, 1);
  CheckAncestor(StrObj::_type_depth, StrObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(StaticTypeInheritance, None) {
  Ref<Object> obj;
  // FIXME: The lines below are going to segfault
  // EXPECT_STREQ(obj->GetTypeKey(), "None");
  // EXPECT_FALSE(obj->IsInstance<Object>());
  // EXPECT_FALSE(obj->IsInstance<Func>());
  // EXPECT_FALSE(obj->IsInstance<Str>());
}

TEST(StaticTypeInheritance, Object) {
  Ref<Object> obj = Ref<Object>::New();
  EXPECT_STREQ(obj->GetTypeKey(), "object.Object");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Func_0) {
  Ref<FuncObj> obj = Ref<FuncObj>::New([](int64_t x) { return x + 1; });
  EXPECT_STREQ(obj->GetTypeKey(), "object.Func");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Func_1) {
  Ref<FuncObj> obj = Ref<FuncObj>::New([](int64_t x) { return x + 1; });
  EXPECT_STREQ(obj->GetTypeKey(), "object.Func");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Str_0) {
  Ref<StrObj> obj = Ref<StrObj>::New("Hello, World!");
  EXPECT_STREQ(obj->GetTypeKey(), "object.Str");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_TRUE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Str_1) {
  Ref<Object> obj = Ref<StrObj>::New("Hello, World!");
  EXPECT_STREQ(obj->GetTypeKey(), "object.Str");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_TRUE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeSubclass, NoException) {
  Ref<SubType> obj = Ref<SubType>::New(0);
  EXPECT_EQ(obj->data, 0);
}

TEST(StaticTypeSubclass, Exception) {
  try {
    Ref<SubType>::New(1);
    FAIL() << "No exception thrown";
  } catch (std::runtime_error &ex) {
    EXPECT_STREQ(ex.what(), "New Error");
  }
}

TEST(DynTypeInfo, TestObj) {
  EXPECT_GE(TestObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin));
  EXPECT_STRCASEEQ(TestObj::_type_key, "test.TestObj");
  EXPECT_EQ(TestObj::_type_depth, 1);
  CheckAncestor(TestObj::_type_depth, TestObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(DynTypeInfo, SubTestObj) {
  EXPECT_GE(SubTestObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin));
  EXPECT_NE(SubTestObj::_type_index, TestObj::_type_index);
  EXPECT_STRCASEEQ(SubTestObj::_type_key, "test.SubTestObj");
  EXPECT_EQ(SubTestObj::_type_depth, 2);
  CheckAncestor(SubTestObj::_type_depth, SubTestObj::_type_ancestors,
                {static_cast<int32_t>(MLCTypeIndex::kMLCObject), TestObj::_type_index});
}

TEST(DynTypeInheritance, TestObj) {
  Ref<TestObj> obj = Ref<TestObj>::New(10);
  EXPECT_EQ(obj->x, 10);
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<TestObj>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(DynTypeInheritance, SubTestObj) {
  Ref<SubTestObj> obj = Ref<SubTestObj>::New(10, 20);
  EXPECT_EQ(obj->x, 10);
  EXPECT_EQ(obj->y, 20);
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<TestObj>());
  EXPECT_TRUE(obj->IsInstance<SubTestObj>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

template <typename Callable> void CheckSignature(Callable callable, const char *expected) {
  EXPECT_STREQ(::mlc::base::FuncCanonicalize<Callable>::Sig().c_str(), expected);
  Func func(std::move(callable));
  EXPECT_NE(func.get(), nullptr);
}

TEST(FuncTraits, Signature) {
  using CStr = const char *;
  using CxxStr = std::string;
  using VoidPtr = void *;
  const char *cstr = "Hello";
  CheckSignature([]() -> void {}, "() -> void");
  CheckSignature([](Any, const Any, const Any &, Any &&) -> Any { return Any(); },
                 "(0: Any, 1: Any, 2: Any, 3: Any) -> Any");
  CheckSignature([](AnyView, const AnyView, const AnyView &, AnyView &&) -> AnyView { return AnyView(); },
                 "(0: AnyView, 1: AnyView, 2: AnyView, 3: AnyView) -> AnyView");
  CheckSignature([](int, const int, const int &, int &&) -> int { return 0; },
                 "(0: int, 1: int, 2: int, 3: int) -> int");
  CheckSignature([](double, const double, const double &, double &&) -> double { return 0.0; },
                 "(0: float, 1: float, 2: float, 3: float) -> float");
  CheckSignature([&](CStr, const CStr, const CStr &, CStr &&) -> CStr { return cstr; },
                 "(0: char *, 1: char *, 2: char *, 3: char *) -> char *");
  CheckSignature([](CxxStr, const CxxStr, const CxxStr &, CxxStr &&) -> CxxStr { return CxxStr(); },
                 "(0: char *, 1: char *, 2: char *, 3: char *) -> char *");
  CheckSignature([](VoidPtr, const VoidPtr, const VoidPtr &, VoidPtr &&) -> VoidPtr { return nullptr; },
                 "(0: Ptr, 1: Ptr, 2: Ptr, 3: Ptr) -> Ptr");
  CheckSignature([](DLDataType, const DLDataType, const DLDataType &, DLDataType &&) -> DLDataType { return {}; },
                 "(0: dtype, 1: dtype, 2: dtype, 3: dtype) -> dtype");
  CheckSignature([](DLDevice, const DLDevice, const DLDevice &, DLDevice &&) -> DLDevice { return {}; },
                 "(0: Device, 1: Device, 2: Device, 3: Device) -> Device");
  CheckSignature([](StrObj *, const StrObj *) -> Str { return Str{Null}; },
                 "(0: object.StrObj *, 1: object.StrObj *) -> str");
  CheckSignature([](Ref<StrObj>, const Ref<StrObj>, const Ref<StrObj> &,
                    Ref<StrObj> &&) -> Ref<StrObj> { return Ref<StrObj>::New("Test"); },
                 "(0: Ref<object.StrObj>, 1: Ref<object.StrObj>, 2: Ref<object.StrObj>, 3: Ref<object.StrObj>) -> "
                 "Ref<object.StrObj>");
  CheckSignature([](Str, const Str, const Str &, Str &&) -> Str { return Str{Null}; },
                 "(0: str, 1: str, 2: str, 3: str) -> str");
  CheckSignature(
      [](Optional<int64_t>, Optional<ObjectRef>, Optional<Str>, Optional<DLDevice>, Optional<DLDataType>) -> void {},
      "(0: Optional<int>, 1: Optional<object.Object>, 2: Optional<object.StrObj>, 3: Optional<Device>, 4: "
      "Optional<dtype>) -> void");
  CheckSignature([](bool, Optional<bool>, Ref<bool>) -> bool { return false; },
                 "(0: bool, 1: Optional<bool>, 2: Ref<bool>) -> bool");
  CheckSignature([](bool, Optional<bool>, Ref<bool>) -> Ref<bool> { return false; },
                 "(0: bool, 1: Optional<bool>, 2: Ref<bool>) -> Ref<bool>");
  CheckSignature([](bool, Optional<bool>, Ref<bool>) -> Optional<bool> { return false; },
                 "(0: bool, 1: Optional<bool>, 2: Ref<bool>) -> Optional<bool>");
}

} // namespace
