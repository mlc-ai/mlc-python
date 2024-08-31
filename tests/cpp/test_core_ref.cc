#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {
using namespace mlc;

// Helper function to get ref count
template <typename T> int32_t GetRefCount(const T &ref) { return reinterpret_cast<const MLCAny *>(ref.get())->ref_cnt; }

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

} // namespace
