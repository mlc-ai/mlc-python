#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {

using namespace mlc;

class TestObj : public Object {
public:
  int value;
  explicit TestObj(int v) : value(v) {}
  MLC_DEF_DYN_TYPE(TestObj, Object, "mlc.testing.test_base_optional.TestObj");
};

class TestObjRef : public ObjectRef {
public:
  MLC_DEF_OBJ_REF(TestObjRef, TestObj, ObjectRef);
};

// Default Constructor Tests
TEST(OptionalDefaultConstructor, IntType) {
  Optional<int> opt_int;
  EXPECT_FALSE(opt_int.defined());
  EXPECT_EQ(opt_int.get(), nullptr);
}

TEST(OptionalDefaultConstructor, ObjectRefType) {
  Optional<TestObjRef> opt_obj;
  EXPECT_FALSE(opt_obj.defined());
  EXPECT_EQ(opt_obj.get(), nullptr);
}

// Null Constructor Tests
TEST(OptionalNullConstructor, IntType) {
  Optional<int> opt_int(Null);
  EXPECT_FALSE(opt_int.defined());
  EXPECT_EQ(opt_int.get(), nullptr);
}

TEST(OptionalNullConstructor, ObjectRefType) {
  Optional<TestObjRef> opt_obj(Null);
  EXPECT_FALSE(opt_obj.defined());
  EXPECT_EQ(opt_obj.get(), nullptr);
}

// Value Constructor Tests
TEST(OptionalValueConstructor, IntType) {
  Optional<int> opt_int(42);
  EXPECT_TRUE(opt_int.defined());
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalValueConstructor, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj(obj);
  EXPECT_TRUE(opt_obj.defined());
  EXPECT_EQ(opt_obj->value, 10);
}

// Copy Constructor Tests
TEST(OptionalCopyConstructor, IntType) {
  Optional<int> opt_int1(42);
  Optional<int> opt_int2(opt_int1);
  EXPECT_TRUE(opt_int2.defined());
  EXPECT_EQ(*opt_int2, 42);
}

TEST(OptionalCopyConstructor, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj1(obj);
  Optional<TestObjRef> opt_obj2(opt_obj1);
  EXPECT_TRUE(opt_obj2.defined());
  EXPECT_EQ(opt_obj2->value, 10);
}

// Move Constructor Tests
TEST(OptionalMoveConstructor, IntType) {
  Optional<int> opt_int1(42);
  Optional<int> opt_int2(std::move(opt_int1));
  EXPECT_TRUE(opt_int2.defined());
  EXPECT_EQ(*opt_int2, 42);
  EXPECT_FALSE(opt_int1.defined());
}

TEST(OptionalMoveConstructor, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj1(obj);
  Optional<TestObjRef> opt_obj2(std::move(opt_obj1));
  EXPECT_TRUE(opt_obj2.defined());
  EXPECT_EQ(opt_obj2->value, 10);
  EXPECT_FALSE(opt_obj1.defined());
}

// Assignment Operator Tests
TEST(OptionalAssignmentOperator, IntType) {
  Optional<int> opt_int1(42);
  Optional<int> opt_int2;
  opt_int2 = opt_int1;
  EXPECT_TRUE(opt_int2.defined());
  EXPECT_EQ(*opt_int2, 42);
}

TEST(OptionalAssignmentOperator, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj1(obj);
  Optional<TestObjRef> opt_obj2;
  opt_obj2 = opt_obj1;
  EXPECT_TRUE(opt_obj2.defined());
  EXPECT_EQ(opt_obj2->value, 10);
}

// Move Assignment Operator Tests
TEST(OptionalMoveAssignmentOperator, IntType) {
  Optional<int> opt_int1(42);
  Optional<int> opt_int2;
  opt_int2 = std::move(opt_int1);
  EXPECT_TRUE(opt_int2.defined());
  EXPECT_EQ(*opt_int2, 42);
  EXPECT_FALSE(opt_int1.defined());
}

TEST(OptionalMoveAssignmentOperator, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj1(obj);
  Optional<TestObjRef> opt_obj2;
  opt_obj2 = std::move(opt_obj1);
  EXPECT_TRUE(opt_obj2.defined());
  EXPECT_EQ(opt_obj2->value, 10);
  EXPECT_FALSE(opt_obj1.defined());
}

// Accessor Tests
TEST(OptionalAccessors, IntType) {
  Optional<int> opt_int(42);
  EXPECT_EQ(opt_int.get(), &(*opt_int));
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalAccessors, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj(obj);
  EXPECT_EQ(opt_obj.get(), opt_obj.get());
  EXPECT_EQ(opt_obj->value, 10);
}

// Boolean Conversion Tests
TEST(OptionalBoolConversion, IntType) {
  Optional<int> opt_int1;
  EXPECT_FALSE(opt_int1);

  Optional<int> opt_int2(42);
  EXPECT_TRUE(opt_int2);
}

TEST(OptionalBoolConversion, ObjectRefType) {
  Optional<TestObjRef> opt_obj1;
  EXPECT_FALSE(opt_obj1);

  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj2(obj);
  EXPECT_TRUE(opt_obj2);
}

// Boolean Conversion in If Statement Tests
TEST(OptionalBoolConversionInIf, IntType) {
  Optional<int> opt_int1;
  if (opt_int1) {
    FAIL() << "Expected false for undefined Optional<int>";
  }

  Optional<int> opt_int2(42);
  if (!opt_int2) {
    FAIL() << "Expected true for defined Optional<int>";
  }
}

TEST(OptionalBoolConversionInIf, ObjectRefType) {
  Optional<TestObjRef> opt_obj1;
  if (opt_obj1) {
    FAIL() << "Expected false for undefined Optional<TestObjRef>";
  }

  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj2(obj);
  if (!opt_obj2) {
    FAIL() << "Expected true for defined Optional<TestObjRef>";
  }
}

// Comparison Tests
TEST(OptionalComparison, IntType) {
  Optional<int> opt_int1;
  Optional<int> opt_int2(42);

  EXPECT_TRUE(opt_int1 == nullptr);
  EXPECT_FALSE(opt_int2 == nullptr);
  EXPECT_FALSE(opt_int1 != nullptr);
  EXPECT_TRUE(opt_int2 != nullptr);
}

TEST(OptionalComparison, ObjectRefType) {
  Optional<TestObjRef> opt_obj1;
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj2(obj);

  EXPECT_TRUE(opt_obj1 == nullptr);
  EXPECT_FALSE(opt_obj2 == nullptr);
  EXPECT_FALSE(opt_obj1 != nullptr);
  EXPECT_TRUE(opt_obj2 != nullptr);
}

// Reset Tests
TEST(OptionalReset, IntType) {
  Optional<int> opt_int(42);
  EXPECT_TRUE(opt_int.defined());
  opt_int.Reset();
  EXPECT_FALSE(opt_int.defined());
}

TEST(OptionalReset, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj(obj);
  EXPECT_TRUE(opt_obj.defined());
  opt_obj.Reset();
  EXPECT_FALSE(opt_obj.defined());
}

// AnyView Conversion Tests
TEST(OptionalAnyViewConversion, IntType) {
  Optional<int> opt_int(42);
  AnyView view = opt_int;
  EXPECT_EQ(view.operator int(), 42);
}

TEST(OptionalAnyViewConversion, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj(obj);
  AnyView obj_view = opt_obj;
  EXPECT_EQ(obj_view.operator TestObjRef()->value, 10);
}

// Any Conversion Tests
TEST(OptionalAnyConversion, IntType) {
  Optional<int> opt_int(42);
  Any any = opt_int;
  EXPECT_EQ(any.operator int(), 42);
}

TEST(OptionalAnyConversion, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Optional<TestObjRef> opt_obj(obj);
  Any obj_any = opt_obj;
  EXPECT_EQ(obj_any.operator TestObjRef()->value, 10);
}

// Construct from AnyView Tests
TEST(OptionalConstructFromAnyView, IntType) {
  AnyView view(42);
  Optional<int> opt_int(view);
  EXPECT_TRUE(opt_int.defined());
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalConstructFromAnyView, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  AnyView obj_view(obj);
  Optional<TestObjRef> opt_obj(obj_view);
  EXPECT_TRUE(opt_obj.defined());
  EXPECT_EQ(opt_obj->value, 10);
}

// Construct from Any Tests
TEST(OptionalConstructFromAny, IntType) {
  Any any(42);
  Optional<int> opt_int(any);
  EXPECT_TRUE(opt_int.defined());
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalConstructFromAny, ObjectRefType) {
  TestObjRef obj(Ref<TestObj>::New(10));
  Any obj_any(obj);
  Optional<TestObjRef> opt_obj(obj_any);
  EXPECT_TRUE(opt_obj.defined());
  EXPECT_EQ(opt_obj->value, 10);
}

// Assign from AnyView Tests
TEST(OptionalAssignFromAnyView, IntType) {
  Optional<int> opt_int;
  AnyView view(42);
  opt_int = view;
  EXPECT_TRUE(opt_int.defined());
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalAssignFromAnyView, ObjectRefType) {
  Optional<TestObjRef> opt_obj;
  TestObjRef obj(Ref<TestObj>::New(10));
  AnyView obj_view(obj);
  opt_obj = obj_view;
  EXPECT_TRUE(opt_obj.defined());
  EXPECT_EQ(opt_obj->value, 10);
}

// Assign from Any Tests
TEST(OptionalAssignFromAny, IntType) {
  Optional<int> opt_int;
  Any any(42);
  opt_int = any;
  EXPECT_TRUE(opt_int.defined());
  EXPECT_EQ(*opt_int, 42);
}

TEST(OptionalAssignFromAny, ObjectRefType) {
  Optional<TestObjRef> opt_obj;
  TestObjRef obj(Ref<TestObj>::New(10));
  Any obj_any(obj);
  opt_obj = obj_any;
  EXPECT_TRUE(opt_obj.defined());
  EXPECT_EQ(opt_obj->value, 10);
}

// Exception Tests
TEST(OptionalExceptions, IntType) {
  Optional<int> opt_int;
  EXPECT_THROW(*opt_int, Exception);
}

TEST(OptionalExceptions, ObjectRefType) {
  Optional<TestObjRef> opt_obj;
  EXPECT_THROW(*opt_obj, Exception);
}

} // namespace
