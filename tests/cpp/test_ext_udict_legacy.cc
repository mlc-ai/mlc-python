#include <gtest/gtest.h>
#include <mlc/all.h>
#include <unordered_set>

namespace {

using namespace mlc;
using mlc::base::DataTypeEqual;
using mlc::core::AnyEqual;

TEST(Legacy_UDict_Construtor, Default) {
  UDict dict;
  ASSERT_EQ(dict->size(), 0);
  MLCDict *dict_ptr = reinterpret_cast<MLCDict *>(dict.get());
  EXPECT_EQ(dict_ptr->_mlc_header.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCDict));
  EXPECT_EQ(dict_ptr->_mlc_header.ref_cnt, 1);
  EXPECT_NE(dict_ptr->_mlc_header.deleter, nullptr);
  EXPECT_EQ(dict_ptr->size, 0);
  EXPECT_EQ(dict_ptr->capacity, 0);
}

TEST(Legacy_UDict_Construtor, InitializerList) {
  UDict dict{{"key1", 1}, {"key2", "value2"}, {3, 4}};
  EXPECT_EQ(dict->size(), 3);
  EXPECT_EQ(int(dict["key1"]), 1);
  EXPECT_EQ(dict["key2"].operator std::string(), "value2");
  EXPECT_EQ(int(dict[3]), 4);

  bool found[3] = {false, false, false};
  for (const auto &kv : *dict) {
    if (AnyEqual()(kv.first, Any("key1"))) {
      found[0] = true;
      EXPECT_EQ(int(kv.second), 1);
    } else if (AnyEqual()(kv.first, Any("key2"))) {
      found[1] = true;
      EXPECT_EQ(kv.second.operator std::string(), "value2");
    } else if (AnyEqual()(kv.first, Any(3))) {
      found[2] = true;
      EXPECT_EQ(int(kv.second), 4);
    } else {
      FAIL() << "Unexpected key: " << kv.first;
    }
  }
  EXPECT_TRUE(found[0]);
  EXPECT_TRUE(found[1]);
  EXPECT_TRUE(found[2]);
}

TEST(Legacy_UDict_Insert, New) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UDict dict{{integer, fp}, {str, dtype}, {null_obj, 0}};
  dict[device] = null_obj;
  EXPECT_EQ(dict->size(), 4);
  EXPECT_DOUBLE_EQ(double(dict[integer]), fp);
  EXPECT_PRED2(DataTypeEqual, DLDataType(dict[str]), dtype);
  EXPECT_EQ(int(dict[null_obj]), 0);
  EXPECT_EQ((Object *)(dict[device]), nullptr);
}

TEST(Legacy_UDict_Insert, Override) {
  UDict dict{{"key1", 1}, {"key2", "value2"}, {3, 4}};
  EXPECT_EQ(dict->size(), 3);
  dict["key1"] = 2;
  dict["key2"] = "new_value";
  dict[3] = 5;
  EXPECT_EQ(dict->size(), 3);
  EXPECT_EQ(int(dict["key1"]), 2);
  EXPECT_EQ(dict["key2"].operator std::string(), "new_value");
  EXPECT_EQ(int(dict[3]), 5);
}

TEST(Legacy_UDict_At, Found) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UDict dict{{integer, fp}, {str, dtype}, {null_obj, 0}};
  EXPECT_DOUBLE_EQ(double(dict->at(integer)), fp);
  EXPECT_PRED2(DataTypeEqual, DLDataType(dict->at(str)), dtype);
  EXPECT_EQ(int(dict->at(null_obj)), 0);
}

TEST(Legacy_UDict_At, NotFound) {
  UDict dict{{"key1", 1}, {"key2", "value2"}, {3, 4}};
  try {
    dict->at("key3");
    FAIL() << "Expected Exception";
  } catch (const Exception &) {
  }
}

TEST(Legacy_UDict_ReHash, POD) {
  UDict dict;
  for (int j = 0; j < 1000; ++j) {
    dict[j] = j;
  }
  EXPECT_EQ(dict->size(), 1000);
  std::unordered_set<int64_t> keys;
  for (auto &kv : *dict) {
    int64_t key = kv.first;
    int64_t value = kv.second;
    EXPECT_EQ(key, value);
    EXPECT_FALSE(keys.count(key));
    EXPECT_EQ(key, value);
    EXPECT_TRUE(0 <= key && key < 1000);
  }
  EXPECT_EQ(dict->size(), 1000);
}

TEST(Legacy_UDict_ReHash, Object) {
  std::vector<Ref<Object>> objs;
  std::unordered_map<Object *, int64_t> obj_map;
  for (int j = 0; j < 1000; ++j) {
    objs.push_back(Ref<Object>::New());
    obj_map[objs[j].get()] = j;
  }
  UDict dict;
  for (int j = 0; j < 1000; ++j) {
    dict[objs[j]] = j;
  }
  EXPECT_EQ(dict->size(), 1000);
  std::unordered_set<Object *> keys;
  for (auto &kv : *dict) {
    Ref<Object> key = kv.first;
    int64_t value = kv.second;
    keys.insert(key.get());
    EXPECT_EQ(value, obj_map[key.get()]);
  }
  EXPECT_EQ(dict->size(), 1000);
}

TEST(Legacy_UDict_Erase, POD) {
  UDict dict;
  for (int j = 0; j < 1000; ++j) {
    dict[j] = j;
  }
  EXPECT_EQ(dict->size(), 1000);
  for (int j = 0; j < 1000; ++j) {
    dict->erase(j);
    EXPECT_EQ(dict->size(), 1000 - j - 1);
  }
  for (int j = 0; j < 1000; ++j) {
    dict[j] = j;
    EXPECT_EQ(dict->size(), j + 1);
  }
}

TEST(Legacy_UDict_Erase, Object) {
  std::vector<Ref<Object>> objs;
  std::unordered_map<Object *, int64_t> obj_map;
  for (int j = 0; j < 1000; ++j) {
    objs.push_back(Ref<Object>::New());
    obj_map[objs[j].get()] = j;
  }
  UDict dict;
  for (int j = 0; j < 1000; ++j) {
    dict[objs[j]] = j;
  }
  EXPECT_EQ(dict->size(), 1000);
  for (int j = 0; j < 1000; ++j) {
    dict->erase(objs[j]);
    EXPECT_EQ(dict->size(), 1000 - j - 1);
  }
  for (int j = 0; j < 1000; ++j) {
    dict[objs[j]] = j;
    EXPECT_EQ(dict->size(), j + 1);
  }
}

} // namespace
