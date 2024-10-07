#include <gtest/gtest.h>
#include <mlc/all.h>
#include <type_traits>

namespace {
using namespace mlc;

// Helper function to get ref count
template <typename T> int32_t GetRefCount(const T &ref) {
  return reinterpret_cast<const MLCObjPtr &>(ref).ptr->ref_cnt;
}

// Test class
class TestObj : public Object {
public:
  int data;
  explicit TestObj(int data) : data(data) {}
  MLC_DEF_DYN_TYPE(TestObj, Object, "mlc.testing.test_core_ref.Test");
};

class DerivedTestObj : public TestObj {
public:
  explicit DerivedTestObj(int data) : TestObj(data) {}
  MLC_DEF_DYN_TYPE(DerivedTestObj, TestObj, "mlc.testing.test_core_ref.DerivedTest");
};

class TestObjRef : public ObjectRef {
  MLC_DEF_OBJ_REF(TestObjRef, TestObj, ObjectRef);
};

// Tests for Ref<>

TEST(Ref, DefaultConstructor) {
  // Testing method Ref<TestObj>::Ref()
  Ref<TestObj> ref;
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(Ref, ConstructorFromNullptr) {
  // Testing method Ref<TestObj>::Ref(nullptr)
  Ref<TestObj> ref(nullptr);
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(Ref, CopyConstructor) {
  // Testing method Ref<TestObj>::Ref(const Ref<TestObj>&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  Ref<TestObj> ref2(ref1);
  EXPECT_EQ(ref1->data, 42);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(Ref, MoveConstructor) {
  // Testing method Ref<TestObj>::Ref(Ref<TestObj>&&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  Ref<TestObj> ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(Ref, CopyConstructorFromDerived) {
  // Testing method Ref<TestObj>::Ref(const Ref<DerivedTestObj>&)
  Ref<DerivedTestObj> derived = Ref<DerivedTestObj>::New(42);
  Ref<TestObj> base(derived);
  EXPECT_EQ(derived->data, 42);
  EXPECT_EQ(base->data, 42);
  EXPECT_EQ(GetRefCount(derived), 2);
  EXPECT_EQ(GetRefCount(base), 2);
}

TEST(Ref, MoveConstructorFromDerived) {
  // Testing method Ref<TestObj>::Ref(Ref<DerivedTestObj>&&)
  Ref<DerivedTestObj> derived = Ref<DerivedTestObj>::New(42);
  Ref<TestObj> base(std::move(derived));
  EXPECT_EQ(derived.get(), nullptr);
  EXPECT_EQ(base->data, 42);
  EXPECT_EQ(GetRefCount(base), 1);
}

TEST(Ref, ConstructorFromRawPointer) {
  // Testing method Ref<TestObj>::Ref(TestObj*)
  TestObj *raw_ptr = ::mlc::base::AllocatorOf<TestObj>::New(42);
  EXPECT_EQ(raw_ptr->_mlc_header.ref_cnt, 0);
  Ref<TestObj> ref(raw_ptr);
  EXPECT_EQ(ref->data, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(Ref, CopyAssignment) {
  // Testing method Ref<TestObj>& Ref<TestObj>::operator=(const Ref<TestObj>&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  Ref<TestObj> ref2;
  ref2 = ref1;
  EXPECT_EQ(ref1->data, 42);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(Ref, MoveAssignment) {
  // Testing method Ref<TestObj>& Ref<TestObj>::operator=(Ref<TestObj>&&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  Ref<TestObj> ref2;
  ref2 = std::move(ref1);
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(Ref, CopyConstructorFromDerivedObjRef) {
  // Testing method Ref<Object>::Ref(const TestObjRef&)
  TestObjRef derived = Ref<DerivedTestObj>::New(42);
  Ref<Object> base(derived);
  EXPECT_EQ(derived->data, 42);
  EXPECT_EQ(static_cast<TestObj *>(base.get())->data, 42);
  EXPECT_EQ(GetRefCount(derived), 2);
  EXPECT_EQ(GetRefCount(base), 2);
}

TEST(Ref, MoveConstructorFromDerivedObjRef) {
  // Testing method Ref<Object>::Ref(TestObjRef&&)
  TestObjRef derived = Ref<DerivedTestObj>::New(42);
  Ref<Object> base(std::move(derived));
  EXPECT_EQ(derived.get(), nullptr);
  EXPECT_EQ(static_cast<TestObj *>(base.get())->data, 42);
  EXPECT_EQ(GetRefCount(base), 1);
}

TEST(Ref, CopyAssignmentFromDerivedObjRef) {
  // Testing method Ref<Object>& Ref<Object>::operator=(const TestObjRef&)
  TestObjRef derived = Ref<DerivedTestObj>::New(42);
  Ref<Object> base;
  base = derived;
  EXPECT_EQ(derived->data, 42);
  EXPECT_EQ(static_cast<TestObj *>(base.get())->data, 42);
  EXPECT_EQ(GetRefCount(derived), 2);
  EXPECT_EQ(GetRefCount(base), 2);
}

TEST(Ref, MoveAssignmentFromDerivedObjRef) {
  // Testing method Ref<Object>& Ref<Object>::operator=(TestObjRef&&)
  TestObjRef derived = Ref<DerivedTestObj>::New(42);
  Ref<Object> base;
  base = std::move(derived);
  EXPECT_EQ(derived.get(), nullptr);
  EXPECT_EQ(static_cast<TestObj *>(base.get())->data, 42);
  EXPECT_EQ(GetRefCount(base), 1);
}

// Tests for ObjectRef

TEST(ObjectRef, ConstructorFromNullptr) {
  // Testing method TestObjRef::TestObjRef(NullType)
  TestObjRef ref(Null);
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(ObjectRef, CopyConstructor) {
  // Testing method TestObjRef::TestObjRef(const TestObjRef&)
  TestObjRef ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(ref1);
  EXPECT_EQ(ref1->data, 42);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(ObjectRef, MoveConstructor) {
  // Testing method TestObjRef::TestObjRef(TestObjRef&&)
  TestObjRef ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(ObjectRef, ConstructorFromRef) {
  // Testing method TestObjRef::TestObjRef(const Ref<TestObj>&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(ref1);
  EXPECT_EQ(ref1->data, 42);
  EXPECT_EQ(ref2->data, 42);       // TODO
  EXPECT_EQ(GetRefCount(ref1), 2); // TODO
  EXPECT_EQ(GetRefCount(ref2), 2); // TODO
}

TEST(ObjectRef, MoveConstructorFromRef) {
  // Testing method TestObjRef::TestObjRef(Ref<TestObj>&&)
  Ref<TestObj> ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(ObjectRef, ConstructorFromRawPointer) {
  // Testing method TestObjRef::TestObjRef(TestObj*)
  TestObj *raw_ptr = ::mlc::base::AllocatorOf<TestObj>::New(42);
  TestObjRef ref(raw_ptr);
  EXPECT_EQ(ref->data, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(ObjectRef, CopyAssignment) {
  // Testing method TestObjRef& TestObjRef::operator=(const TestObjRef&)
  TestObjRef ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(18);
  ref2 = ref1;
  EXPECT_EQ(ref1->data, 42);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(ObjectRef, MoveAssignment) {
  // Testing method TestObjRef& TestObjRef::operator=(TestObjRef&&)
  TestObjRef ref1 = Ref<TestObj>::New(42);
  TestObjRef ref2(12);
  ref2 = std::move(ref1);
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(ref2->data, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, DefaultConstructor) {
  Ref<int32_t> ref;
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, ConstructorFromValue) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  EXPECT_NE(ref.get(), nullptr);
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, CopyConstructor) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2(ref1);
  EXPECT_EQ(*ref1, 42);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefPOD, MoveConstructor) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, CopyAssignment) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2;
  ref2 = ref1;
  EXPECT_EQ(*ref1, 42);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefPOD, MoveAssignment) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2;
  ref2 = std::move(ref1);
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, Dereference) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  EXPECT_EQ(*ref, 42);
  *ref = 24;
  EXPECT_EQ(*ref, 24);
}

TEST(RefPOD, ArrowOperator) {
  Ref<DLDevice> ref = Ref<DLDevice>::New({kDLCPU, 0});
  EXPECT_EQ(ref->device_type, kDLCPU);
  EXPECT_EQ(ref->device_id, 0);
}

TEST(RefPOD, Null) {
  Ref<int32_t> ref(Null);
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, Reset) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  EXPECT_NE(ref.get(), nullptr);
  ref.Reset();
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, ConversionToAny) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  Any any = ref;
  ref.Reset();
  EXPECT_EQ(any.operator int64_t(), 42);
  ref = Ref<int32_t>(any);
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, ConversionFromAny) {
  Any any = Ref<int32_t>::New(42);
  Ref<int32_t> ref = any;
  EXPECT_EQ(*ref, 42);
}

TEST(RefPOD, ConversionToAnyView) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  AnyView any_view = ref;
  ref.Reset();
  EXPECT_EQ(any_view.operator int64_t(), 42);
  ref = Ref<int32_t>(any_view);
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, ConversionFromAnyView) {
  Any any = Ref<int32_t>::New(42);
  AnyView any_view = any;
  Ref<int32_t> ref = any_view;
  EXPECT_EQ(*ref, 42);
}

TEST(RefPOD, NewMethodWithDifferentTypes) {
  Ref<int32_t> ref_int = Ref<int32_t>::New(42);
  EXPECT_EQ(*ref_int, 42);

  Ref<double> ref_double = Ref<double>::New(3.14);
  EXPECT_DOUBLE_EQ(*ref_double, 3.14);

  Ref<DLDevice> ref_device = Ref<DLDevice>::New({kDLCPU, 0});
  EXPECT_EQ(ref_device->device_type, kDLCPU);
  EXPECT_EQ(ref_device->device_id, 0);

  Ref<DLDataType> ref_dtype = Ref<DLDataType>::New({kDLFloat, 32, 1});
  EXPECT_EQ(ref_dtype->code, kDLFloat);
  EXPECT_EQ(ref_dtype->bits, 32);
  EXPECT_EQ(ref_dtype->lanes, 1);
}

TEST(RefPOD, ComparisonOperators) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2 = Ref<int32_t>::New(42);
  Ref<int32_t> ref3 = Ref<int32_t>::New(24);

  EXPECT_TRUE(ref1 == ref1);
  EXPECT_FALSE(ref1 == ref2);
  EXPECT_FALSE(ref1 == ref3);

  EXPECT_FALSE(ref1 != ref1);
  EXPECT_TRUE(ref1 != ref2);
  EXPECT_TRUE(ref1 != ref3);
}

TEST(RefPOD, NullComparison) {
  Ref<int32_t> ref1;
  Ref<int32_t> ref2 = Ref<int32_t>::New(42);

  EXPECT_TRUE(ref1 == nullptr);
  EXPECT_FALSE(ref2 == nullptr);

  EXPECT_FALSE(ref1 != nullptr);
  EXPECT_TRUE(ref2 != nullptr);
}

TEST(RefPOD, Defined) {
  Ref<int32_t> ref1;
  Ref<int32_t> ref2 = Ref<int32_t>::New(42);

  EXPECT_FALSE(ref1.defined());
  EXPECT_TRUE(ref2.defined());
}

TEST(RefPOD, MultipleReferences) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2 = ref1;
  Ref<int32_t> ref3 = ref1;

  EXPECT_EQ(GetRefCount(ref1), 3);
  EXPECT_EQ(GetRefCount(ref2), 3);
  EXPECT_EQ(GetRefCount(ref3), 3);

  ref2.Reset();
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref3), 2);

  ref3.Reset();
  EXPECT_EQ(GetRefCount(ref1), 1);
}

TEST(RefPOD, MoveSemantics) {
  Ref<int32_t> ref1 = Ref<int32_t>::New(42);
  Ref<int32_t> ref2 = std::move(ref1);

  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, DereferenceNull) {
  Ref<int32_t> ref;
  try {
    *ref;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

TEST(RefPOD, ResetAndAccess) {
  Ref<int32_t> ref = Ref<int32_t>::New(42);
  ref.Reset();
  try {
    *ref;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

TEST(RefPOD, ConversionToAnyFromNull) {
  Ref<int32_t> ref;
  Any any = ref;
  EXPECT_EQ(any.operator void *(), nullptr);
}

TEST(RefPOD, ConversionFromAnyContainingInt32) {
  Any any_int = 42;
  Ref<int32_t> ref_int = any_int;
  EXPECT_NE(ref_int.get(), nullptr);
  EXPECT_EQ(*ref_int, 42);
  EXPECT_EQ(GetRefCount(ref_int), 1);
}

TEST(RefPOD, ConversionFromAnyContainingDouble) {
  Any any_double = 3.14;
  Ref<double> ref_double = any_double;
  EXPECT_NE(ref_double.get(), nullptr);
  EXPECT_DOUBLE_EQ(*ref_double, 3.14);
  EXPECT_EQ(GetRefCount(ref_double), 1);
}

TEST(RefPOD, ConversionFromAnyContainingDLDevice) {
  DLDevice device{kDLCPU, 0};
  Any any_device = device;
  Ref<DLDevice> ref_device = any_device;
  EXPECT_NE(ref_device.get(), nullptr);
  EXPECT_EQ(ref_device->device_type, kDLCPU);
  EXPECT_EQ(ref_device->device_id, 0);
  EXPECT_EQ(GetRefCount(ref_device), 1);
}

TEST(RefPOD, ConversionFromAnyContainingDLDataType) {
  DLDataType dtype{kDLFloat, 32, 1};
  Any any_dtype = dtype;
  Ref<DLDataType> ref_dtype = any_dtype;
  EXPECT_NE(ref_dtype.get(), nullptr);
  EXPECT_EQ(ref_dtype->code, kDLFloat);
  EXPECT_EQ(ref_dtype->bits, 32);
  EXPECT_EQ(ref_dtype->lanes, 1);
  EXPECT_EQ(GetRefCount(ref_dtype), 1);
}

TEST(RefPOD, ConversionFromAnyViewContainingInt32) {
  AnyView any_view_int = 42;
  Ref<int32_t> ref_int = any_view_int;
  EXPECT_NE(ref_int.get(), nullptr);
  EXPECT_EQ(*ref_int, 42);
  EXPECT_EQ(GetRefCount(ref_int), 1);
}

TEST(RefPOD, ConversionFromAnyViewContainingDouble) {
  AnyView any_view_double = 3.14;
  Ref<double> ref_double = any_view_double;
  EXPECT_NE(ref_double.get(), nullptr);
  EXPECT_DOUBLE_EQ(*ref_double, 3.14);
  EXPECT_EQ(GetRefCount(ref_double), 1);
}

TEST(RefPOD, ConversionFromAnyViewContainingDLDevice) {
  DLDevice device{kDLCPU, 0};
  AnyView any_view_device = device;
  Ref<DLDevice> ref_device = any_view_device;
  EXPECT_NE(ref_device.get(), nullptr);
  EXPECT_EQ(ref_device->device_type, kDLCPU);
  EXPECT_EQ(ref_device->device_id, 0);
  EXPECT_EQ(GetRefCount(ref_device), 1);
}

TEST(RefPOD, ConversionFromAnyViewContainingDLDataType) {
  DLDataType dtype{kDLFloat, 32, 1};
  AnyView any_view_dtype = dtype;
  Ref<DLDataType> ref_dtype = any_view_dtype;
  EXPECT_NE(ref_dtype.get(), nullptr);
  EXPECT_EQ(ref_dtype->code, kDLFloat);
  EXPECT_EQ(ref_dtype->bits, 32);
  EXPECT_EQ(ref_dtype->lanes, 1);
  EXPECT_EQ(GetRefCount(ref_dtype), 1);
}

TEST(RefPOD, ConversionFromInt32) {
  Ref<int32_t> ref_int = 42;
  EXPECT_NE(ref_int.get(), nullptr);
  EXPECT_EQ(*ref_int, 42);
  EXPECT_EQ(GetRefCount(ref_int), 1);
}

TEST(RefPOD, ConversionFromDouble) {
  Ref<double> ref_double = 3.14;
  EXPECT_NE(ref_double.get(), nullptr);
  EXPECT_DOUBLE_EQ(*ref_double, 3.14);
  EXPECT_EQ(GetRefCount(ref_double), 1);
}

TEST(RefPOD, ConversionFromDLDevice) {
  Ref<DLDevice> ref_device = DLDevice{kDLCPU, 0};
  EXPECT_NE(ref_device.get(), nullptr);
  EXPECT_EQ(ref_device->device_type, kDLCPU);
  EXPECT_EQ(ref_device->device_id, 0);
  EXPECT_EQ(GetRefCount(ref_device), 1);
}

TEST(RefPOD, ConversionFromDLDataType) {
  Ref<DLDataType> ref_dtype = DLDataType{kDLFloat, 32, 1};
  EXPECT_NE(ref_dtype.get(), nullptr);
  EXPECT_EQ(ref_dtype->code, kDLFloat);
  EXPECT_EQ(ref_dtype->bits, 32);
  EXPECT_EQ(ref_dtype->lanes, 1);
  EXPECT_EQ(GetRefCount(ref_dtype), 1);
}

TEST(RefPOD, ConversionFromAnyContainingPODToIncompatibleRef) {
  Any any_int = 42.5;
  try {
    any_int.operator Ref<int32_t>();
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `float` to `int`");
  }
}

TEST(RefPOD, ConversionFromAnyViewContainingPODToIncompatibleRef) {
  AnyView any_view_int = 42.5;
  try {
    any_view_int.operator Ref<int32_t>();
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `float` to `int`");
  }
}

static_assert(!std::is_assignable<Ref<int32_t> &, Ref<double>>::value,
              "Ref<int32_t> should not be assignable from Ref<double>");
static_assert(!std::is_constructible<Ref<int32_t>, Ref<double>>::value,
              "Ref<int32_t> should not be constructible from Ref<double>");
static_assert(!std::is_constructible<Ref<int32_t>, Ref<double> &&>::value,
              "Ref<int32_t> should not be move constructible from Ref<double>");
static_assert(!std::is_convertible<Ref<int32_t>, Ref<double>>::value,
              "Ref<int32_t> should not be convertible to Ref<double>");
static_assert(!std::is_convertible<Ref<double>, Ref<int32_t>>::value,
              "Ref<double> should not be convertible to Ref<int32_t>");
} // namespace
