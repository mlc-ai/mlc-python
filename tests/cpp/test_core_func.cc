#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {
using namespace mlc;

// Helper functions for testing
int TestFuncAdd(int a, int b) { return a + b; }
std::string TestFuncStrConcat(const std::string &a, const std::string &b) { return a + b; }

TEST(Func, ConstructFromFreeFunction) {
  Func func(TestFuncAdd);
  Any result = func(5, 3);
  EXPECT_EQ(result.operator int(), 8);
}

TEST(Func, ConstructFromNonCaptureLambda) {
  Func func([](int a, int b) { return a * b; });
  Any result = func(4, 7);
  EXPECT_EQ(result.operator int(), 28);
}

TEST(Func, ConstructFromCapturingLambda) {
  int capture = 10;
  Func func([capture](int x) { return capture + x; });
  Any result = func(5);
  EXPECT_EQ(result.operator int(), 15);
}

TEST(Func, ConstructFromRefCapturingLambda) {
  int capture = 10;
  Func func([&capture](int x) { capture += x; });
  Any result = func(5);
  EXPECT_EQ(result.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCNone));
  EXPECT_EQ(capture, 15);
}

TEST(Func, ConstructFromStdFunction) {
  std::function<double(double, double)> std_func = [](double a, double b) { return a / b; };
  Func func(std::move(std_func));
  Any result = func(10.0, 2.0);
  EXPECT_DOUBLE_EQ(result.operator double(), 5.0);
}

TEST(Func, ConstructFromVoidReturn) {
  bool called = false;
  Func func([&called]() { called = true; });
  func();
  EXPECT_TRUE(called);
}

TEST(Func, ConstructFromPacked) {
  Func func([](int num_args, const AnyView *, Any *ret) { *ret = num_args; });
  int result = func(1, 3.14, "Hello", nullptr);
  EXPECT_EQ(result, 4);
}

TEST(Func, FunctionWithStringArguments) {
  Func func(TestFuncStrConcat);
  Any result = func("Hello, ", "World!");
  EXPECT_EQ(result.operator std::string(), "Hello, World!");
}

TEST(Func, FunctionReturningObject) {
  Func func([]() { return Ref<StrObj>::New("Test Object"); });
  Any result = func();
  EXPECT_EQ(result.operator Ref<StrObj>()->c_str(), std::string("Test Object"));
}

TEST(Func, FunctionWithAnyArguments) {
  Func func([](const Any &a, const Any &b) { return a.operator int() + b.operator int(); });
  Any result = func(Any(5), Any(3));
  EXPECT_EQ(result.operator int(), 8);
}

TEST(Func, FunctionReturningAny) {
  Func func([]() -> Any { return "Hello, Any!"; });
  Any result = func();
  EXPECT_EQ(result.operator std::string(), "Hello, Any!");
}

TEST(Func, CopyConstructor) {
  Func original([](int x) { return x * 2; });
  Func copy(original);
  Any result = copy(5);
  EXPECT_EQ(result.operator int(), 10);
}

TEST(Func, MoveConstructor) {
  Func original([](int x) { return x * 2; });
  Func moved(std::move(original));
  int result = moved(5);
  EXPECT_EQ(result, 10);
  EXPECT_EQ(original.get(), nullptr);
}

TEST(Func, AssignmentOperator) {
  Func func1([](int x) { return x * 2; });
  Func func2([](int x) { return x + 1; });
  func2 = func1;
  Any result = func2(5);
  EXPECT_EQ(result.operator int(), 10);
  EXPECT_EQ(func1.get(), func2.get());
}

TEST(Func, FunctionReturningRef) {
  Func func([]() -> Ref<StrObj> { return Ref<StrObj>::New("Test Ref"); });
  Any result = func();
  EXPECT_EQ(result.operator Ref<StrObj>()->c_str(), std::string("Test Ref"));
}

template <typename Callable> void CheckSignature(Callable callable, const char *expected) {
  using Traits = FuncTraits<Callable>;
  EXPECT_STREQ(Traits::Sig().c_str(), expected);
  Func func(std::move(callable));
  EXPECT_NE(func.get(), nullptr);
}

TEST(Func, Signature) {
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
      [](Optional<int>, Optional<ObjectRef>, Optional<Str>, Optional<DLDevice>, Optional<DLDataType>) -> void {},
      "(0: Optional<int>, 1: Optional<object.Object>, 2: Optional<object.StrObj>, 3: Optional<Device>, 4: "
      "Optional<dtype>) -> void");
}

TEST(Func, ArgumentObjRawPtr) {
  Func f1([](Object *obj) { EXPECT_EQ(reinterpret_cast<MLCAny *>(obj)->ref_cnt, 1); });
  Func f2([](ObjectRef obj) { EXPECT_EQ(reinterpret_cast<MLCAny *>(obj.get())->ref_cnt, 2); });
  Func f3([](Ref<Object> obj) { EXPECT_EQ(reinterpret_cast<MLCAny *>(obj.get())->ref_cnt, 2); });
  ObjectRef obj;
  f1(obj);
  f2(obj);
  f3(obj);
  // `std::move` always fails because `AnyView` doesn't own the object
  f1(std::move(obj));
  EXPECT_NE(obj.get(), nullptr);
  f2(std::move(obj));
  EXPECT_NE(obj.get(), nullptr);
  f3(std::move(obj));
  EXPECT_NE(obj.get(), nullptr);
}

TEST(Func, ArgumentRawStrToStrObj) {
  Func f1([](StrObj *str) { EXPECT_EQ(reinterpret_cast<MLCStr *>(str)->_mlc_header.ref_cnt, 1); });
  Func f2([](Ref<StrObj> str) { EXPECT_EQ(reinterpret_cast<MLCStr *>(str.get())->_mlc_header.ref_cnt, 1); });
  Func f3([](Str str) { EXPECT_EQ(reinterpret_cast<MLCStr *>(str.get())->_mlc_header.ref_cnt, 1); });
  std::string long_str(1000, 'a');
  const char *c_str = long_str.c_str();
  char c_array[] = "Hello world";
  f1(long_str);
  f1(c_str);
  f1(c_array);
  f2(long_str);
  f2(c_str);
  f2(c_array);
  f3(long_str);
  f3(c_str);
  f3(c_array);
}

TEST(Func, TypeMismatch_0) {
  const char *c_str_raw = "Hello";
  Func func([](int64_t a, double b, const char *, double d) { return a + b + d; });
  try {
    func(1.0, 2, c_str_raw, 4);
    FAIL() << "No execption thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Mismatched type on argument #0 when calling: "
                            "`(0: int, 1: float, 2: char *, 3: float) -> float`. "
                            "Expected `int` but got `float`");
  }
}

TEST(Func, TypeMismatch_1) {
  Func func([](DLDataType, DLDevice, std::string) {});
  try {
    func(DLDataType{kDLInt, 32, 1}, DLDevice{kDLCPU, 0}, 1);
    FAIL() << "No execption thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Mismatched type on argument #2 when calling: "
                            "`(0: dtype, 1: Device, 2: char *) -> void`. "
                            "Expected `char *` but got `int`");
  }
}

TEST(Func, IncorrectArgumentCountError) {
  const char *c_str_raw = "Hello";
  Func func([](int64_t a, double b, const char *, double d) { return a + b + d; });
  try {
    func(1, 2, c_str_raw);
    FAIL() << "No execption thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Mismatched number of arguments when calling: "
                            "`(0: int, 1: float, 2: char *, 3: float) -> float`. "
                            "Expected 4 but got 3 arguments");
  }
}

TEST(Func, ReturnTypeMismatch_0) {
  Func func([](DLDataType, DLDevice, std::string) {});
  try {
    int ret = func(DLDataType{kDLInt, 32, 1}, DLDevice{kDLCPU, 0}, "Hello");
    (void)ret;
    FAIL() << "No execption thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Cannot convert from type `None` to `int`");
  }
}

} // namespace
