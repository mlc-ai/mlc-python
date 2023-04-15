#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {

using namespace mlc;

// Custom object for testing object reference types
class TestTypeObj : public Object {
public:
  int value;
  explicit TestTypeObj(int v) : value(v) {}
  MLC_DEF_DYN_TYPE(TestTypeObj, Object, "TestType");
};

class TestType : public ObjectRef {
public:
  MLC_DEF_OBJ_REF(TestType, TestTypeObj, ObjectRef);
};

// Tests for List<int>
TEST(ListIntTest, DefaultConstructor) {
  List<int> list;
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());
}

TEST(ListIntTest, InitializerListConstructor) {
  List<int> list{1, 2, 3};
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
  EXPECT_EQ(list[2], 3);
}

TEST(ListIntTest, PushBackAndAccess) {
  List<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
  EXPECT_EQ(list[2], 3);
}

TEST(ListIntTest, PopBack) {
  List<int> list{1, 2, 3};
  list.pop_back();
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
}

TEST(ListIntTest, Clear) {
  List<int> list{1, 2, 3};
  list.clear();
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());
}

TEST(ListIntTest, Resize) {
  List<int> list{1, 2, 3};
  list.resize(5);
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
  EXPECT_EQ(list[2], 3);
  EXPECT_EQ(list[3], 0);
  EXPECT_EQ(list[4], 0);
  list.resize(2);
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 2);
}

TEST(ListIntTest, Insert) {
  List<int> list{1, 2, 3};
  list.insert(1, 4);
  EXPECT_EQ(list.size(), 4);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 4);
  EXPECT_EQ(list[2], 2);
  EXPECT_EQ(list[3], 3);
}

TEST(ListIntTest, Erase) {
  List<int> list{1, 2, 3, 4};
  list.erase(1);
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[0], 1);
  EXPECT_EQ(list[1], 3);
  EXPECT_EQ(list[2], 4);
}

TEST(ListIntTest, FrontAndBack) {
  List<int> list{1, 2, 3};
  EXPECT_EQ(list.front(), 1);
  EXPECT_EQ(list.back(), 3);
}

// Tests for List<double>
TEST(ListDoubleTest, BasicOperations) {
  List<double> list;
  list.push_back(1.1);
  list.push_back(2.2);
  list.push_back(3.3);

  EXPECT_EQ(list.size(), 3);
  EXPECT_DOUBLE_EQ(list[0], 1.1);
  EXPECT_DOUBLE_EQ(list[1], 2.2);
  EXPECT_DOUBLE_EQ(list[2], 3.3);

  list.pop_back();
  EXPECT_EQ(list.size(), 2);
  EXPECT_DOUBLE_EQ(list.back(), 2.2);
}

// Tests for List<TestRef>
TEST(ListRefTest, BasicOperations) {
  List<TestType> list;
  list.push_back(TestType(1));
  list.push_back(TestType(2));
  list.push_back(TestType(3));

  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[0]->value, 1);
  EXPECT_EQ(list[1]->value, 2);
  EXPECT_EQ(list[2]->value, 3);

  list.pop_back();
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.back()->value, 2);
}

TEST(ListRefTest, NullObjectHandling) {
  List<TestType> list;
  list.push_back(TestType{Null}); // Null reference
  EXPECT_EQ(list.size(), 1);
  try {
    list[0];
    FAIL() << "Accessing null object should throw an exception";
  } catch (const std::exception &e) {
    EXPECT_STREQ(e.what(), "Cannot convert from type `None` to non-nullable `TestType`");
  }
}

TEST(ListRefTest, ObjectLifetime) {
  List<TestType> list;
  void *ptr = nullptr;
  {
    TestType obj = TestType(42);
    list.push_back(obj);
    ptr = obj.get();
  }
  EXPECT_EQ(list.size(), 1);
  EXPECT_NE(list[0].get(), nullptr);
  EXPECT_EQ(list[0]->value, 42);
  EXPECT_EQ(reinterpret_cast<MLCAny *>(ptr)->ref_cnt, 1);
}

// Iterator tests
TEST(ListIteratorTest, ForwardIteration) {
  List<int> list{1, 2, 3, 4, 5};
  int sum = 0;
  for (int item : list) {
    sum += item;
  }
  EXPECT_EQ(sum, 15);
}

TEST(ListIteratorTest, ReverseIteration) {
  List<int> list{1, 2, 3, 4, 5};
  std::vector<int> reversed;
  for (auto it = list.rbegin(); it != list.rend(); ++it) {
    reversed.push_back(*it);
  }
  EXPECT_EQ(reversed, std::vector<int>({5, 4, 3, 2, 1}));
}

TEST(ListAnyTest, HeterogeneousTypes) {
  List<Any> list;
  list.push_back(Any(42));
  list.push_back(Any(3.14));
  list.push_back(Any("Hello"));
  list.push_back(Any(Ref<TestTypeObj>::New(100)));

  EXPECT_EQ(list.size(), 4);
  EXPECT_EQ(list[0].operator int(), 42);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 3.14);
  EXPECT_STREQ(list[2].operator const char *(), "Hello");
  EXPECT_EQ(list[3].operator Ref<TestTypeObj>()->value, 100);
}

TEST(ListAnyTest, ModifyingElements) {
  List<Any> list{Any(1), Any(2.0), Any("three")};

  list.Set(1, Any(4));
  EXPECT_EQ(list[1].operator int(), 4);

  list.Set(2, Any(5.5));
  EXPECT_DOUBLE_EQ(list[2].operator double(), 5.5);
}

TEST(ListAnyTest, InsertAndErase) {
  List<Any> list{Any(1), Any(2), Any(3)};

  list.insert(1, Any("inserted"));
  EXPECT_EQ(list.size(), 4);
  EXPECT_STREQ(list[1].operator const char *(), "inserted");

  list.erase(0);
  EXPECT_EQ(list.size(), 3);
  EXPECT_STREQ(list[0].operator const char *(), "inserted");
}

TEST(ListAnyTest, ClearAndResize) {
  List<Any> list{Any(1), Any(2.0), Any("three")};

  list.clear();
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());

  list.resize(2);
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list[0].type_index, static_cast<int32_t>(MLCTypeIndex::kMLCNone));
  EXPECT_EQ(list[1].type_index, static_cast<int32_t>(MLCTypeIndex::kMLCNone));
}

TEST(ListAnyTest, IterationWithTypeChecking) {
  List<Any> list{Any(1), Any(2.0), Any("three"), Any(Ref<TestTypeObj>::New(4))};

  std::vector<int> type_checks;
  for (const auto &item : *list) {
    if (item.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCInt)) {
      type_checks.push_back(1);
    } else if (item.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFloat)) {
      type_checks.push_back(2);
    } else if (item.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
      type_checks.push_back(3);
    } else if (item.type_index == TestTypeObj::_type_index) {
      type_checks.push_back(4);
    }
  }

  EXPECT_EQ(type_checks, std::vector<int>({1, 2, 3, 4}));
}

TEST(ListAnyTest, ComplexOperations) {
  List<Any> list;

  // Push different types
  list.push_back(Any(10));
  list.push_back(Any(20.5));
  list.push_back(Any("Hello"));
  list.push_back(Any(Ref<TestTypeObj>::New(30)));

  // Modify and check
  list.Set(1, Any("World"));
  EXPECT_STREQ(list[1].operator const char *(), "World");

  // Insert and erase
  list.insert(2, Any(40));
  list.erase(0);

  // Check final state
  EXPECT_EQ(list.size(), 4);
  EXPECT_STREQ(list[0].operator const char *(), "World");
  EXPECT_EQ(list[1].operator int(), 40);
  EXPECT_STREQ(list[2].operator const char *(), "Hello");
  EXPECT_EQ(list[3].operator Ref<TestTypeObj>()->value, 30);

  // Iterate and sum numeric values
  double sum = 0;
  for (const auto &item : *list) {
    if (item.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCInt)) {
      sum += item.operator int();
    } else if (item.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFloat)) {
      sum += item.operator double();
    }
  }
  EXPECT_DOUBLE_EQ(sum, 40);
}

} // namespace
