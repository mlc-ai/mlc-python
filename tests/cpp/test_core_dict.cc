#include <algorithm>
#include <gtest/gtest.h>
#include <mlc/all.h>
#include <string>
#include <vector>

namespace {

using namespace mlc;

TEST(DictKV, DefaultConstructor) {
  Dict<int, Str> dict;
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictKV, InitializerListConstructor) {
  Dict<Str, int> dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict["key1"], 1);
  EXPECT_EQ(dict["key2"], 2);
  EXPECT_EQ(dict["key3"], 3);
}

TEST(DictKV, InsertAndAccess) {
  Dict<int, double> dict;
  dict.Set(1, 1.5);
  dict.Set(2, 2.7);
  dict.Set(3, 3.14);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_DOUBLE_EQ(dict[1], 1.5);
  EXPECT_DOUBLE_EQ(dict[2], 2.7);
  EXPECT_DOUBLE_EQ(dict[3], 3.14);
}

TEST(DictKV, OverwriteExistingKey) {
  Dict<Str, Str> dict{{"key", "old value"}};
  dict.Set("key", "new value");
  EXPECT_EQ(dict.size(), 1);
  EXPECT_EQ(dict["key"], "new value");
}

TEST(DictKV, AtMethod) {
  Dict<int, Str> dict{{1, "one"}, {2, "two"}};
  EXPECT_EQ(dict.at(1), "one");
  EXPECT_EQ(dict.at(2), "two");
  EXPECT_THROW(dict.at(3), mlc::Exception);
}

TEST(DictKV, CountMethod) {
  Dict<Str, int> dict{{"key1", 1}, {"key2", 2}};
  EXPECT_EQ(dict.count("key1"), 1);
  EXPECT_EQ(dict.count("non_existent"), 0);
}

TEST(DictKV, ClearMethod) {
  Dict<int, int> dict{{1, 1}, {2, 2}};
  dict.clear();
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictKV, EraseMethod) {
  Dict<Str, int> dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  dict.erase("key2");
  EXPECT_EQ(dict.size(), 2);
  EXPECT_EQ(dict.count("key2"), 0);
  EXPECT_THROW(dict.at("key2"), mlc::Exception);
}

TEST(DictKV, IteratorBasic) {
  Dict<Str, int> dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::vector<std::pair<Str, int>> expected{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::vector<std::pair<Str, int>> actual;

  for (const auto &kv : *dict) {
    actual.emplace_back(kv.first, kv.second);
  }

  // Sort both vectors because the order of elements in a Dict is not guaranteed
  auto comparator = [](const auto &a, const auto &b) { return a.first < b.first; };
  std::sort(expected.begin(), expected.end(), comparator);
  std::sort(actual.begin(), actual.end(), comparator);

  EXPECT_EQ(actual, expected);
}

TEST(DictKV, FindMethod) {
  Dict<int, Str> dict{{1, "one"}, {2, "two"}};
  auto it = dict.find(1);
  EXPECT_NE(it, dict.end());
  EXPECT_EQ((*it).first, 1);
  EXPECT_EQ((*it).second, "one");

  it = dict.find(3);
  EXPECT_EQ(it, dict.end());
}

TEST(DictKV, Rehash) {
  Dict<int, int> dict;
  for (int i = 0; i < 1000; ++i) {
    dict.Set(i, i * 2);
  }
  EXPECT_EQ(dict.size(), 1000);

  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(dict[i], i * 2);
  }
}

TEST(DictKV, CopyConstructor) {
  Dict<Str, int> dict1{{"key1", 1}, {"key2", 2}};
  Dict<Str, int> dict2(dict1.begin(), dict1.end());

  EXPECT_EQ(dict1.size(), dict2.size());
  EXPECT_EQ(dict1["key1"], dict2["key1"]);
  EXPECT_EQ(dict1["key2"], dict2["key2"]);
}

TEST(DictKV, MoveConstructor) {
  Dict<Str, int> dict1{{"key1", 1}, {"key2", 2}};
  Dict<Str, int> dict2(std::move(dict1));

  EXPECT_EQ(dict2.size(), 2);
  EXPECT_EQ(dict2["key1"], 1);
  EXPECT_EQ(dict2["key2"], 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(DictKV, AssignmentOperator) {
  Dict<Str, int> dict1{{"key1", 1}, {"key2", 2}};
  Dict<Str, int> dict2;
  dict2 = dict1;

  EXPECT_EQ(dict1.size(), dict2.size());
  EXPECT_EQ(dict1["key1"], dict2["key1"]);
  EXPECT_EQ(dict1["key2"], dict2["key2"]);
}

TEST(DictKV, MoveAssignmentOperator) {
  Dict<Str, int> dict1{{"key1", 1}, {"key2", 2}};
  Dict<Str, int> dict2;
  dict2 = std::move(dict1);

  EXPECT_EQ(dict2.size(), 2);
  EXPECT_EQ(dict2["key1"], 1);
  EXPECT_EQ(dict2["key2"], 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(DictKV, ReverseIteratorBasic) {
  Dict<Str, int> dict{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::vector<std::pair<Str, int>> expected{{"key3", 3}, {"key2", 2}, {"key1", 1}};
  std::vector<std::pair<Str, int>> actual;

  for (auto it = dict.rbegin(); it != dict.rend(); ++it) {
    actual.emplace_back((*it).first, (*it).second);
  }

  // Sort both vectors because the order of elements in a Dict is not guaranteed
  auto comparator = [](const auto &a, const auto &b) { return a.first < b.first; };
  std::sort(expected.begin(), expected.end(), comparator);
  std::sort(actual.begin(), actual.end(), comparator);

  EXPECT_EQ(actual, expected);
}

TEST(DictKV, IteratorInvalidation) {
  Dict<int, int> dict{{1, 1}, {2, 2}, {3, 3}};
  auto it = dict.begin();
  dict.Set(4, 4); // This might cause rehashing and iterator invalidation

  // We can't guarantee the exact behavior after potential rehashing,
  // but we can at least check that we don't crash when using the iterator
  EXPECT_NO_THROW({
    while (it != dict.end()) {
      ++it;
    }
  });
}

TEST(DictAnyTest, DefaultConstructor) {
  Dict<Any, Any> dict;
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictAnyTest, InitializerListConstructor) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any("value2")}, {Any(3), Any(4.0)}};
  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(int(dict["key1"]), 1);
  EXPECT_EQ(dict["key2"].operator std::string(), "value2");
  EXPECT_DOUBLE_EQ(double(dict[3]), 4.0);
}

TEST(DictAnyTest, InsertAndAccess) {
  Dict<Any, Any> dict;
  dict.Set(Any("key1"), Any(100));
  dict.Set(Any("key2"), Any(1.5));
  dict.Set(Any("key3"), Any("Hello"));

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(int(dict["key1"]), 100);
  EXPECT_DOUBLE_EQ(double(dict["key2"]), 1.5);
  EXPECT_EQ(dict["key3"].operator std::string(), "Hello");
}

TEST(DictAnyTest, OverwriteExistingKey) {
  Dict<Any, Any> dict{{Any("key"), Any(1)}};
  dict.Set(Any("key"), Any("new value"));
  EXPECT_EQ(dict.size(), 1);
  EXPECT_EQ(dict["key"].operator std::string(), "new value");
}

TEST(DictAnyTest, AtMethod) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any("value2")}};
  EXPECT_EQ(int(dict.at("key1")), 1);
  EXPECT_EQ(dict.at("key2").operator std::string(), "value2");
  EXPECT_THROW(dict.at("non_existent"), mlc::Exception);
}

TEST(DictAnyTest, CountMethod) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  EXPECT_EQ(dict.count("key1"), 1);
  EXPECT_EQ(dict.count("non_existent"), 0);
}

TEST(DictAnyTest, ClearMethod) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  dict.clear();
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictAnyTest, EraseMethod) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}, {Any("key3"), Any(3)}};
  dict.erase("key2");
  EXPECT_EQ(dict.size(), 2);
  EXPECT_EQ(dict.count("key2"), 0);
  EXPECT_THROW(dict.at("key2"), mlc::Exception);
}

TEST(DictAnyTest, IteratorBasic) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}, {Any("key3"), Any(3)}};
  std::unordered_map<std::string, int> expected{{"key1", 1}, {"key2", 2}, {"key3", 3}};
  std::unordered_map<std::string, int> actual;

  for (const auto &kv : *dict) {
    actual[kv.first.operator std::string()] = kv.second.operator int();
  }

  EXPECT_EQ(actual, expected);
}

TEST(DictAnyTest, FindMethod) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  auto it = dict.find("key1");
  EXPECT_NE(it, dict.end());
  EXPECT_EQ((*it).first.operator std::string(), "key1");
  EXPECT_EQ((*it).second.operator int(), 1);

  it = dict.find("non_existent");
  EXPECT_EQ(it, dict.end());
}

TEST(DictAnyTest, Rehash) {
  Dict<Any, Any> dict;
  for (int i = 0; i < 1000; ++i) {
    dict.Set(i, i * 2);
  }
  EXPECT_EQ(dict.size(), 1000);

  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(dict[i].operator int(), i * 2);
  }
}

TEST(DictAnyTest, MixedTypes) {
  Dict<Any, Any> dict;
  dict.Set(Any("int"), Any(42));
  dict.Set(Any("float"), Any(3.14));
  dict.Set(Any("string"), Any("Hello"));
  dict.Set(Any("bool"), Any(true));
  dict.Set(Any(1), Any("One"));

  EXPECT_EQ(dict["int"].operator int(), 42);
  EXPECT_DOUBLE_EQ(dict["float"].operator double(), 3.14);
  EXPECT_EQ(dict["string"].operator std::string(), "Hello");
  EXPECT_EQ(dict["bool"].operator bool(), true);
  EXPECT_EQ(dict[1].operator std::string(), "One");
}

TEST(DictAnyTest, ComplexKeys) {
  Dict<Any, Any> dict;
  DLDevice device{kDLCPU, 0};
  DLDataType dtype{kDLFloat, 32, 1};
  Ref<Object> obj = Ref<Object>::New();

  dict.Set(Any(device), Any("CPU"));
  dict.Set(Any(dtype), Any("Float32"));
  dict.Set(Any(obj), Any("Object"));

  EXPECT_EQ(dict[device].operator std::string(), "CPU");
  EXPECT_EQ(dict[dtype].operator std::string(), "Float32");
  EXPECT_EQ(dict[obj].operator std::string(), "Object");
}

TEST(DictAnyTest, CopyConstructor) {
  Dict<Any, Any> dict1{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  Dict<Any, Any> dict2(dict1);

  EXPECT_EQ(dict1.size(), dict2.size());
  EXPECT_EQ(dict1["key1"].operator int(), dict2["key1"].operator int());
  EXPECT_EQ(dict1["key2"].operator int(), dict2["key2"].operator int());
}

TEST(DictAnyTest, MoveConstructor) {
  Dict<Any, Any> dict1{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  Dict<Any, Any> dict2(std::move(dict1));

  EXPECT_EQ(dict2.size(), 2);
  EXPECT_EQ(dict2["key1"].operator int(), 1);
  EXPECT_EQ(dict2["key2"].operator int(), 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(DictAnyTest, AssignmentOperator) {
  Dict<Any, Any> dict1{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  Dict<Any, Any> dict2;
  dict2 = dict1;

  EXPECT_EQ(dict1.size(), dict2.size());
  EXPECT_EQ(dict1["key1"].operator int(), dict2["key1"].operator int());
  EXPECT_EQ(dict1["key2"].operator int(), dict2["key2"].operator int());
}

TEST(DictAnyTest, MoveAssignmentOperator) {
  Dict<Any, Any> dict1{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}};
  Dict<Any, Any> dict2;
  dict2 = std::move(dict1);

  EXPECT_EQ(dict2.size(), 2);
  EXPECT_EQ(dict2["key1"].operator int(), 1);
  EXPECT_EQ(dict2["key2"].operator int(), 2);
  EXPECT_EQ(dict1.get(), nullptr);
}

TEST(DictAnyTest, IteratorInvalidation) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}, {Any("key3"), Any(3)}};
  auto it = dict.begin();
  dict.Set(Any("key4"), Any(4)); // This might cause rehashing and iterator invalidation

  // We can't guarantee the exact behavior after potential rehashing,
  // but we can at least check that we don't crash when using the iterator
  EXPECT_NO_THROW({
    while (it != dict.end()) {
      ++it;
    }
  });
}

TEST(DictAnyTest, ReverseIteratorBasic) {
  Dict<Any, Any> dict{{Any("key1"), Any(1)}, {Any("key2"), Any(2)}, {Any("key3"), Any(3)}};
  std::vector<std::pair<std::string, int>> expected{{"key3", 3}, {"key2", 2}, {"key1", 1}};
  std::vector<std::pair<std::string, int>> actual;
  for (auto it = dict.rbegin(); it != dict.rend(); ++it) {
    actual.emplace_back((*it).first.operator std::string(), (*it).second.operator int());
  }
  // Sort both vectors because the order of elements in a Dict is not guaranteed
  auto comparator = [](const auto &a, const auto &b) { return a.first < b.first; };
  std::sort(expected.begin(), expected.end(), comparator);
  std::sort(actual.begin(), actual.end(), comparator);

  EXPECT_EQ(actual, expected);
}

// Tests for Dict<Any, int>
TEST(DictAnyIntTest, DefaultConstructor) {
  Dict<Any, int> dict;
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictAnyIntTest, InitializerListConstructor) {
  Dict<Any, int> dict{{Any("key1"), 1}, {Any("key2"), 2}, {Any(3), 3}};
  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict["key1"], 1);
  EXPECT_EQ(dict["key2"], 2);
  EXPECT_EQ(dict[3], 3);
}

TEST(DictAnyIntTest, InsertAndAccess) {
  Dict<Any, int> dict;
  dict.Set(Any("key1"), 100);
  dict.Set(Any(2), 200);
  dict.Set(Any(3.14), 314);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict["key1"], 100);
  EXPECT_EQ(dict[2], 200);
  EXPECT_EQ(dict[3.14], 314);
}

TEST(DictAnyIntTest, OverwriteExistingKey) {
  Dict<Any, int> dict{{Any("key"), 1}};
  dict.Set(Any("key"), 2);
  EXPECT_EQ(dict.size(), 1);
  EXPECT_EQ(dict["key"], 2);
}

TEST(DictAnyIntTest, EraseMethod) {
  Dict<Any, int> dict{{Any("key1"), 1}, {Any("key2"), 2}, {Any("key3"), 3}};
  dict.erase("key2");
  EXPECT_EQ(dict.size(), 2);
  EXPECT_EQ(dict.count("key2"), 0);
  EXPECT_THROW(dict.at("key2"), mlc::Exception);
}

TEST(DictAnyIntTest, MixedKeyTypes) {
  Dict<Any, int> dict;
  dict.Set(Any("string"), 1);
  dict.Set(Any(2), 2);
  dict.Set(Any(3.14), 3);
  dict.Set(Any(true), 4);

  EXPECT_EQ(dict["string"], 1);
  EXPECT_EQ(dict[2], 2);
  EXPECT_EQ(dict[3.14], 3);
  EXPECT_EQ(dict[true], 4);
}

// Tests for Dict<int, Any>
TEST(DictIntAnyTest, DefaultConstructor) {
  Dict<int, Any> dict;
  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.empty());
}

TEST(DictIntAnyTest, InitializerListConstructor) {
  Dict<int, Any> dict{{1, Any("one")}, {2, Any(2.0)}, {3, Any(true)}};
  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict[1].operator std::string(), "one");
  EXPECT_DOUBLE_EQ(dict[2].operator double(), 2.0);
  EXPECT_EQ(dict[3].operator bool(), true);
}

TEST(DictIntAnyTest, InsertAndAccess) {
  Dict<int, Any> dict;
  dict.Set(1, Any("one"));
  dict.Set(2, Any(2.0));
  dict.Set(3, Any(true));

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict[1].operator std::string(), "one");
  EXPECT_DOUBLE_EQ(dict[2].operator double(), 2.0);
  EXPECT_EQ(dict[3].operator bool(), true);
}

TEST(DictIntAnyTest, OverwriteExistingKey) {
  Dict<int, Any> dict{{1, Any("one")}};
  dict.Set(1, Any(1.0));
  EXPECT_EQ(dict.size(), 1);
  EXPECT_DOUBLE_EQ(dict[1].operator double(), 1.0);
}

TEST(DictIntAnyTest, EraseMethod) {
  Dict<int, Any> dict{{1, Any("one")}, {2, Any("two")}, {3, Any("three")}};
  dict.erase(2);
  EXPECT_EQ(dict.size(), 2);
  EXPECT_EQ(dict.count(2), 0);
  EXPECT_THROW(dict.at(2), mlc::Exception);
}

TEST(DictIntAnyTest, MixedValueTypes) {
  Dict<int, Any> dict;
  dict.Set(1, Any("string"));
  dict.Set(2, Any(2));
  dict.Set(3, Any(3.14));
  dict.Set(4, Any(true));

  EXPECT_EQ(dict[1].operator std::string(), "string");
  EXPECT_EQ(dict[2].operator int(), 2);
  EXPECT_DOUBLE_EQ(dict[3].operator double(), 3.14);
  EXPECT_EQ(dict[4].operator bool(), true);
}

TEST(DictIntAnyTest, ComplexValues) {
  Dict<int, Any> dict;
  DLDevice device{kDLCPU, 0};
  DLDataType dtype{kDLFloat, 32, 1};
  Ref<Object> obj = Ref<Object>::New();

  dict.Set(1, Any(device));
  dict.Set(2, Any(dtype));
  dict.Set(3, Any(obj));

  EXPECT_EQ(dict[1].operator DLDevice().device_type, kDLCPU);
  EXPECT_EQ(dict[1].operator DLDevice().device_id, 0);
  EXPECT_EQ(dict[2].operator DLDataType().code, kDLFloat);
  EXPECT_EQ(dict[2].operator DLDataType().bits, 32);
  EXPECT_EQ(dict[2].operator DLDataType().lanes, 1);
  EXPECT_EQ(dict[3].operator Ref<Object>(), obj);
}

TEST(DictAnyIntTest, IteratorTest) {
  Dict<Any, int> dict{{Any("one"), 1}, {Any("two"), 2}, {Any("three"), 3}};
  std::unordered_map<std::string, int> expected{{"one", 1}, {"two", 2}, {"three", 3}};
  std::unordered_map<std::string, int> actual;

  for (const auto &kv : *dict) {
    actual[kv.first.operator std::string()] = kv.second;
  }

  EXPECT_EQ(actual, expected);
}

TEST(DictIntAnyTest, IteratorTest) {
  Dict<int, Any> dict{{1, Any("one")}, {2, Any("two")}, {3, Any("three")}};
  std::unordered_map<int, std::string> expected{{1, "one"}, {2, "two"}, {3, "three"}};
  std::unordered_map<int, std::string> actual;

  for (const auto &kv : *dict) {
    actual[kv.first] = kv.second.operator std::string();
  }

  EXPECT_EQ(actual, expected);
}
} // namespace
