#include <algorithm>
#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {

using namespace mlc;

TEST(UDict, DefaultConstructor) {
  UDict dict;
  EXPECT_EQ(dict->size(), 0);
  EXPECT_TRUE(dict->empty());
}

TEST(UDict, InitializerListConstructor) {
  UDict dict{{"key1", 1}, {"key2", "value2"}, {3, 4.0}};
  EXPECT_EQ(dict->size(), 3);
  EXPECT_EQ(int(dict["key1"]), 1);
  EXPECT_EQ(dict["key2"].operator std::string(), "value2");
  EXPECT_DOUBLE_EQ(double(dict[3]), 4.0);
}

TEST(UDict, InsertAndAccess) {
  UDict dict;
  dict["key1"] = 100;
  dict["key2"] = 1.5;
  dict["key3"] = "Hello";

  EXPECT_EQ(dict->size(), 3);
  EXPECT_EQ(int(dict["key1"]), 100);
  EXPECT_DOUBLE_EQ(double(dict["key2"]), 1.5);
  EXPECT_EQ(dict["key3"].operator std::string(), "Hello");
}

TEST(UDict, OverwriteExistingKey) {
  UDict dict{{"key", 1}};
  dict["key"] = "new value";
  EXPECT_EQ(dict->size(), 1);
  EXPECT_EQ(dict["key"].operator std::string(), "new value");
}

TEST(UDict, AtMethod) {
  UDict dict{{"key1", 1}, {"key2", "value2"}};
  EXPECT_EQ(int(dict->at("key1")), 1);
  EXPECT_EQ(dict->at("key2").operator std::string(), "value2");
  EXPECT_THROW(dict->at("non_existent"), mlc::Exception);
}

TEST(UDict, CountMethod) {
  UDict dict{{"key1", 1}, {"key2", 2}};
  EXPECT_EQ(dict->count("key1"), 1);
  EXPECT_EQ(dict->count("non_existent"), 0);
}

TEST(UDict, ClearMethod) {
  UDict dict{{"key1", 1}, {"key2", 2}};
  dict->clear();
  EXPECT_EQ(dict->size(), 0);
  EXPECT_TRUE(dict->empty());
}

TEST(UDict, EraseMethod) {
  UDict dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  dict->erase("key2");
  EXPECT_EQ(dict->size(), 2);
  EXPECT_EQ(dict->count("key2"), 0);
  EXPECT_THROW(dict->at("key2"), mlc::Exception);
}

TEST(UDict, IteratorBasic) {
  UDict dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::unordered_map<std::string, int> expected{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::unordered_map<std::string, int> actual;

  for (const auto &kv : *dict) {
    actual[kv.first.operator std::string()] = kv.second.operator int();
  }

  EXPECT_EQ(actual, expected);
}

TEST(UDict, FindMethod) {
  UDict dict{{"key1", 1}, {"key2", 2}};
  auto it = dict->find("key1");
  EXPECT_NE(it, dict->end());
  EXPECT_EQ(it->first.operator std::string(), "key1");
  EXPECT_EQ(it->second.operator int(), 1);

  it = dict->find("non_existent");
  EXPECT_EQ(it, dict->end());
}

TEST(UDict, Rehash) {
  UDict dict;
  for (int i = 0; i < 1000; ++i) {
    dict[i] = i;
  }
  EXPECT_EQ(dict->size(), 1000);

  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(dict[i].operator int(), i);
  }
}

TEST(UDict, MixedTypes) {
  UDict dict;
  dict["int"] = 42;
  dict["float"] = 3.14;
  dict["string"] = "Hello";
  dict["bool"] = true;
  dict[1] = "One";

  EXPECT_EQ(dict["int"].operator int(), 42);
  EXPECT_DOUBLE_EQ(dict["float"].operator double(), 3.14);
  EXPECT_EQ(dict["string"].operator std::string(), "Hello");
  EXPECT_EQ(dict["bool"].operator bool(), true);
  EXPECT_EQ(dict[1].operator std::string(), "One");
}

TEST(UDict, ComplexKeys) {
  UDict dict;
  DLDevice device{kDLCPU, 0};
  DLDataType dtype{kDLFloat, 32, 1};
  Ref<Object> obj = Ref<Object>::New();

  dict[nullptr] = "Null";
  dict[device] = "CPU";
  dict[dtype] = "Float32";
  dict[obj] = "Object";

  EXPECT_EQ(dict[nullptr].operator std::string(), "Null");
  EXPECT_EQ(dict[device].operator std::string(), "CPU");
  EXPECT_EQ(dict[dtype].operator std::string(), "Float32");
  EXPECT_EQ(dict[obj].operator std::string(), "Object");
}

TEST(UDict, ComplexValues) {
  UDict dict;
  DLDevice device{kDLCPU, 0};
  DLDataType dtype{kDLFloat, 32, 1};
  Ref<Object> obj = Ref<Object>::New();

  dict["Null"] = nullptr;
  dict["device"] = device;
  dict["dtype"] = dtype;
  dict["object"] = obj;

  if (Optional<int> v = dict["Null"]) {
    FAIL() << "Expected to return nullptr, but got: " << *v.get();
  } else {
    EXPECT_EQ(v.get(), nullptr);
  }
  if (Optional<DLDevice> v = dict["device"]) {
    EXPECT_EQ(v->device_type, kDLCPU);
    EXPECT_EQ(v->device_id, 0);
  } else {
    FAIL() << "Expected DLDevice value not found";
  }
  if (Optional<DLDataType> v = dict["dtype"]) {
    EXPECT_EQ(v->code, kDLFloat);
    EXPECT_EQ(v->bits, 32);
    EXPECT_EQ(v->lanes, 1);
  } else {
    FAIL() << "Expected DLDataType value not found";
  }
  if (Optional<ObjectRef> v = dict["object"]) {
    EXPECT_EQ(v.get(), obj.get());
  } else {
    FAIL() << "Expected Object value not found";
  }
}

TEST(UDict, ConstAccess) {
  const UDict dict{{"key1", 1}, {"key2", 2}};
  EXPECT_EQ(dict["key1"].operator int(), 1);
  EXPECT_EQ(dict["key2"].operator int(), 2);
  EXPECT_THROW(dict["non_existent"], mlc::Exception);
}

TEST(UDict, CopyConstructor) {
  UDict dict1{{"key1", 1}, {"key2", 2}};
  UDict dict2(dict1);

  EXPECT_EQ(dict1->size(), dict2->size());
  EXPECT_EQ(dict1["key1"].operator int(), dict2["key1"].operator int());
  EXPECT_EQ(dict1["key2"].operator int(), dict2["key2"].operator int());
}

TEST(UDict, MoveConstructor) {
  UDict dict1{{"key1", 1}, {"key2", 2}};
  UDict dict2(std::move(dict1));

  EXPECT_EQ(dict2->size(), 2);
  EXPECT_EQ(dict2["key1"].operator int(), 1);
  EXPECT_EQ(dict2["key2"].operator int(), 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(UDict, AssignmentOperator) {
  UDict dict1{{"key1", 1}, {"key2", 2}};
  UDict dict2;
  dict2 = dict1;

  EXPECT_EQ(dict1->size(), dict2->size());
  EXPECT_EQ(dict1["key1"].operator int(), dict2["key1"].operator int());
  EXPECT_EQ(dict1["key2"].operator int(), dict2["key2"].operator int());
}

TEST(UDict, MoveAssignmentOperator) {
  UDict dict1{{"key1", 1}, {"key2", 2}};
  UDict dict2;
  dict2 = std::move(dict1);

  EXPECT_EQ(dict2->size(), 2);
  EXPECT_EQ(dict2["key1"].operator int(), 1);
  EXPECT_EQ(dict2["key2"].operator int(), 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(UDict, IteratorInvalidation) {
  UDict dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  auto it = dict->begin();
  dict["key4"] = 4; // This might cause rehashing and iterator invalidation

  // We can't guarantee the exact behavior after potential rehashing,
  // but we can at least check that we don't crash when using the iterator
  EXPECT_NO_THROW({
    while (it != dict->end()) {
      ++it;
    }
  });
}

TEST(UDict, ReverseIteratorBasic) {
  UDict dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::vector<std::pair<std::string, int>> expected{{"key3", 3}, {"key2", 2}, {"key1", 1}};
  std::vector<std::pair<std::string, int>> actual;

  for (auto it = dict->rbegin(); it != dict->rend(); ++it) {
    actual.emplace_back(it->first.operator std::string(), it->second.operator int());
  }

  // Sort both vectors because the order of elements in a UDict is not guaranteed
  auto comparator = [](const auto &a, const auto &b) { return a.first < b.first; };
  std::sort(expected.begin(), expected.end(), comparator);
  std::sort(actual.begin(), actual.end(), comparator);

  EXPECT_EQ(actual, expected);
}

TEST(UDict, ReverseIteratorEmpty) {
  UDict dict;
  EXPECT_EQ(dict->rbegin(), dict->rend());
}

TEST(UDict, ReverseIteratorSingleElement) {
  UDict dict{{"key", "value"}};
  auto it = dict->rbegin();
  EXPECT_EQ(it->first.operator std::string(), "key");
  EXPECT_EQ(it->second.operator std::string(), "value");
  ++it;
  EXPECT_EQ(it, dict->rend());
}

TEST(UDict, ReverseIteratorModification) {
  UDict dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  for (auto it = dict->rbegin(); it != dict->rend(); ++it) {
    it->second = it->second.operator int() * 2;
  }

  EXPECT_EQ(dict["key1"].operator int(), 2);
  EXPECT_EQ(dict["key2"].operator int(), 4);
  EXPECT_EQ(dict["key3"].operator int(), 6);
}

} // namespace
