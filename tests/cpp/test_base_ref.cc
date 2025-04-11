#include "./common.h"
#include <gtest/gtest.h>
#include <mlc/core/all.h>
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
  MLC_DEF_DYN_TYPE(MLC_CPPTESTS_EXPORTS, TestObj, Object, "mlc.testing.test_core_ref.Test");
};

class DerivedTestObj : public TestObj {
public:
  explicit DerivedTestObj(int data) : TestObj(data) {}
  MLC_DEF_DYN_TYPE(MLC_CPPTESTS_EXPORTS, DerivedTestObj, TestObj, "mlc.testing.test_core_ref.DerivedTest");
};

class TestObjRef : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_CPPTESTS_EXPORTS, TestObjRef, TestObj, ObjectRef);
  MLC_DEF_OBJ_REF_FWD_NEW(TestObjRef)
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
  Ref<int64_t> ref;
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, ConstructorFromValue) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
  EXPECT_NE(ref.get(), nullptr);
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, CopyConstructor) {
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2(ref1);
  EXPECT_EQ(*ref1, 42);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefPOD, MoveConstructor) {
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, CopyAssignment) {
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2;
  ref2 = ref1;
  EXPECT_EQ(*ref1, 42);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefPOD, MoveAssignment) {
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2;
  ref2 = std::move(ref1);
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, Dereference) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
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
  Ref<int64_t> ref(Null);
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, Reset) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
  EXPECT_NE(ref.get(), nullptr);
  ref.Reset();
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefPOD, ConversionToAny) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
  Any any = ref;
  ref.Reset();
  EXPECT_EQ(any.operator int64_t(), 42);
  ref = any.operator Ref<int64_t>();
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, ConversionFromAny) {
  Any any = Ref<int64_t>::New(42);
  Ref<int64_t> ref = any;
  EXPECT_EQ(*ref, 42);
}

TEST(RefPOD, ConversionToAnyView) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
  AnyView any_view = ref;
  ref.Reset();
  EXPECT_EQ(any_view.operator int64_t(), 42);
  ref = any_view.operator Ref<int64_t>();
  EXPECT_EQ(*ref, 42);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefPOD, ConversionFromAnyView) {
  Any any = Ref<int64_t>::New(42);
  AnyView any_view = any;
  Ref<int64_t> ref = any_view;
  EXPECT_EQ(*ref, 42);
}

TEST(RefPOD, NewMethodWithDifferentTypes) {
  Ref<int64_t> ref_int = Ref<int64_t>::New(42);
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
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2 = Ref<int64_t>::New(42);
  Ref<int64_t> ref3 = Ref<int64_t>::New(24);

  EXPECT_TRUE(ref1 == ref1);
  EXPECT_FALSE(ref1 == ref2);
  EXPECT_FALSE(ref1 == ref3);

  EXPECT_FALSE(ref1 != ref1);
  EXPECT_TRUE(ref1 != ref2);
  EXPECT_TRUE(ref1 != ref3);
}

TEST(RefPOD, NullComparison) {
  Ref<int64_t> ref1;
  Ref<int64_t> ref2 = Ref<int64_t>::New(42);

  EXPECT_TRUE(ref1 == nullptr);
  EXPECT_FALSE(ref2 == nullptr);

  EXPECT_FALSE(ref1 != nullptr);
  EXPECT_TRUE(ref2 != nullptr);
}

TEST(RefPOD, Defined) {
  Ref<int64_t> ref1;
  Ref<int64_t> ref2 = Ref<int64_t>::New(42);

  EXPECT_FALSE(ref1.defined());
  EXPECT_TRUE(ref2.defined());
}

TEST(RefPOD, MultipleReferences) {
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2 = ref1;
  Ref<int64_t> ref3 = ref1;

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
  Ref<int64_t> ref1 = Ref<int64_t>::New(42);
  Ref<int64_t> ref2 = std::move(ref1);

  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_EQ(*ref2, 42);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefPOD, DereferenceNull) {
  Ref<int64_t> ref;
  try {
    *ref;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

TEST(RefPOD, ResetAndAccess) {
  Ref<int64_t> ref = Ref<int64_t>::New(42);
  ref.Reset();
  try {
    *ref;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

TEST(RefPOD, ConversionToAnyFromNull) {
  Ref<int64_t> ref;
  Any any = ref;
  EXPECT_EQ(any.operator void *(), nullptr);
}

TEST(RefPOD, ConversionFromAnyContainingInt32) {
  Any any_int = 42;
  Ref<int64_t> ref_int = any_int;
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
  Ref<int64_t> ref_int = any_view_int;
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
  Ref<int64_t> ref_int = 42;
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
    any_int.operator Ref<int64_t>();
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `float` to `int`");
  }
}

TEST(RefPOD, ConversionFromAnyViewContainingPODToIncompatibleRef) {
  AnyView any_view_int = 42.5;
  try {
    any_view_int.operator Ref<int64_t>();
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `float` to `int`");
  }
}

TEST(RefPOD, FromAnyViewNone) {
  AnyView any_view_none;
  try {
    any_view_none.operator Ref<int64_t>();
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `None` to `int`");
  }
}

TEST(RefPOD, FromAnyNone) {
  Any any_none;
  try {
    any_none.operator Ref<int64_t>();
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `None` to `int`");
  }
}

TEST(RefBool, DefaultConstructor) {
  // Default-constructed Ref<bool> should be null
  Ref<bool> ref;
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefBool, ConstructorFromValue) {
  // Creating a Ref<bool> from a boolean literal
  Ref<bool> ref_true = Ref<bool>::New(true);
  EXPECT_NE(ref_true.get(), nullptr);
  EXPECT_TRUE(*ref_true);
  EXPECT_EQ(GetRefCount(ref_true), 1);

  // Also test a false case
  Ref<bool> ref_false = Ref<bool>::New(false);
  EXPECT_NE(ref_false.get(), nullptr);
  EXPECT_FALSE(*ref_false);
  EXPECT_EQ(GetRefCount(ref_false), 1);
}

TEST(RefBool, CopyConstructor) {
  // Copy construct from an existing Ref<bool>
  Ref<bool> ref1 = Ref<bool>::New(true);
  Ref<bool> ref2(ref1);
  EXPECT_NE(ref1.get(), nullptr);
  EXPECT_NE(ref2.get(), nullptr);
  EXPECT_TRUE(*ref1);
  EXPECT_TRUE(*ref2);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefBool, MoveConstructor) {
  // Move construct from an existing Ref<bool>
  Ref<bool> ref1 = Ref<bool>::New(true);
  Ref<bool> ref2(std::move(ref1));
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_NE(ref2.get(), nullptr);
  EXPECT_TRUE(*ref2);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefBool, CopyAssignment) {
  // Copy-assign one Ref<bool> to another
  Ref<bool> ref1 = Ref<bool>::New(false);
  Ref<bool> ref2;
  ref2 = ref1;
  EXPECT_NE(ref1.get(), nullptr);
  EXPECT_NE(ref2.get(), nullptr);
  EXPECT_FALSE(*ref1);
  EXPECT_FALSE(*ref2);
  EXPECT_EQ(GetRefCount(ref1), 2);
  EXPECT_EQ(GetRefCount(ref2), 2);
}

TEST(RefBool, MoveAssignment) {
  // Move-assign one Ref<bool> to another
  Ref<bool> ref1 = Ref<bool>::New(false);
  Ref<bool> ref2;
  ref2 = std::move(ref1);
  EXPECT_EQ(ref1.get(), nullptr);
  EXPECT_NE(ref2.get(), nullptr);
  EXPECT_FALSE(*ref2);
  EXPECT_EQ(GetRefCount(ref2), 1);
}

TEST(RefBool, Dereference) {
  Ref<bool> ref_true = Ref<bool>::New(true);
  EXPECT_TRUE(*ref_true);
  *ref_true = false;
  EXPECT_FALSE(*ref_true);
}

TEST(RefBool, Null) {
  // Construct from Null
  Ref<bool> ref(Null);
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefBool, Reset) {
  // Reset a Ref<bool> and ensure it becomes null
  Ref<bool> ref = Ref<bool>::New(true);
  EXPECT_NE(ref.get(), nullptr);
  ref.Reset();
  EXPECT_EQ(ref.get(), nullptr);
}

TEST(RefBool, ConversionToAny) {
  // Convert Ref<bool> to Any
  Ref<bool> ref = Ref<bool>::New(true);
  Any any = ref;
  ref.Reset();
  EXPECT_TRUE(any.operator bool());
  // Convert back from Any to Ref<bool>
  ref = any.operator Ref<bool>();
  EXPECT_TRUE(*ref);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefBool, ConversionFromAny) {
  // Convert a Ref<bool> inside an Any back to Ref<bool>
  Any any = Ref<bool>::New(false);
  Ref<bool> ref = any;
  EXPECT_FALSE(*ref);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefBool, ConversionToAnyView) {
  // Convert Ref<bool> to AnyView
  Ref<bool> ref = Ref<bool>::New(true);
  AnyView any_view = ref;
  ref.Reset();
  EXPECT_TRUE(any_view.operator bool());
  // Convert back from AnyView to Ref<bool>
  ref = any_view.operator Ref<bool>();
  EXPECT_TRUE(*ref);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefBool, ConversionFromAnyView) {
  // Convert a Ref<bool> inside an AnyView back to Ref<bool>
  Any any = Ref<bool>::New(false);
  AnyView any_view = any;
  Ref<bool> ref = any_view;
  EXPECT_FALSE(*ref);
  EXPECT_EQ(GetRefCount(ref), 1);
}

TEST(RefBool, MultipleReferences) {
  // Multiple references to the same Ref<bool>
  Ref<bool> ref1 = Ref<bool>::New(true);
  Ref<bool> ref2 = ref1;
  Ref<bool> ref3 = ref1;
  EXPECT_EQ(GetRefCount(ref1), 3);
  EXPECT_EQ(GetRefCount(ref2), 3);
  EXPECT_EQ(GetRefCount(ref3), 3);

  ref2.Reset();
  EXPECT_EQ(GetRefCount(ref1), 2);
  ref3.Reset();
  EXPECT_EQ(GetRefCount(ref1), 1);
}

TEST(RefBool, ComparisonOperators) {
  // Test == and != for Ref<bool>
  Ref<bool> ref_true1 = Ref<bool>::New(true);
  Ref<bool> ref_true2 = Ref<bool>::New(true);
  Ref<bool> ref_false = Ref<bool>::New(false);

  // They point to different allocations, so operator== checks pointer identity
  EXPECT_TRUE(ref_true1 == ref_true1);
  EXPECT_FALSE(ref_true1 == ref_true2);
  EXPECT_FALSE(ref_true1 == ref_false);
  EXPECT_FALSE(ref_true2 == ref_false);

  EXPECT_FALSE(ref_true1 != ref_true1);
  EXPECT_TRUE(ref_true1 != ref_true2);
  EXPECT_TRUE(ref_true1 != ref_false);
  EXPECT_TRUE(ref_true2 != ref_false);
}

TEST(RefBool, NullComparison) {
  // Comparing Ref<bool> to nullptr
  Ref<bool> ref_null;
  Ref<bool> ref_true = Ref<bool>::New(true);

  EXPECT_TRUE(ref_null == nullptr);
  EXPECT_FALSE(ref_true == nullptr);

  EXPECT_FALSE(ref_null != nullptr);
  EXPECT_TRUE(ref_true != nullptr);
}

TEST(RefBool, Defined) {
  // defined() should return false if null, true otherwise
  Ref<bool> ref_null;
  Ref<bool> ref_true = Ref<bool>::New(true);

  EXPECT_FALSE(ref_null.defined());
  EXPECT_TRUE(ref_true.defined());
}

TEST(RefBool, DereferenceNull) {
  // Attempting to dereference a null Ref<bool> should throw
  Ref<bool> ref_null;
  try {
    *ref_null;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

TEST(RefBool, ResetAndAccess) {
  // Reset and then attempt to dereference
  Ref<bool> ref = Ref<bool>::New(true);
  ref.Reset();
  try {
    *ref;
    FAIL() << "Expected exception not thrown";
  } catch (Exception &e) {
    EXPECT_STREQ(e.what(), "Attempt to dereference a null pointer");
  }
}

static_assert(!std::is_assignable<Ref<int64_t> &, Ref<double>>::value,
              "Ref<int64_t> should not be assignable from Ref<double>");
static_assert(!std::is_constructible<Ref<int64_t>, Ref<double>>::value,
              "Ref<int64_t> should not be constructible from Ref<double>");
static_assert(!std::is_constructible<Ref<int64_t>, Ref<double> &&>::value,
              "Ref<int64_t> should not be move constructible from Ref<double>");
static_assert(!std::is_convertible<Ref<int64_t>, Ref<double>>::value,
              "Ref<int64_t> should not be convertible to Ref<double>");
static_assert(!std::is_convertible<Ref<double>, Ref<int64_t>>::value,
              "Ref<double> should not be convertible to Ref<int64_t>");
} // namespace
