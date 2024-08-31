#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {

using namespace mlc;
using mlc::base::DataTypeEqual;
using mlc::base::DeviceEqual;

void TestSizeCapacityClear(UListObj *list, int64_t size, int64_t capacity) {
  EXPECT_EQ(list->size(), size);
  EXPECT_EQ(list->capacity(), capacity);
  EXPECT_EQ(list->empty(), size == 0);
  list->clear();
  EXPECT_EQ(list->size(), 0);
  EXPECT_EQ(list->capacity(), capacity);
  EXPECT_EQ(list->empty(), true);
}

TEST(Legacy_UList_Constructor, Default) {
  UList list;
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  ASSERT_NE(list_ptr, nullptr);
  EXPECT_EQ(list_ptr->_mlc_header.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCList));
  EXPECT_EQ(list_ptr->_mlc_header.ref_cnt, 1);
  EXPECT_NE(list_ptr->_mlc_header.deleter, nullptr);
  EXPECT_EQ(list_ptr->capacity, 0);
  EXPECT_EQ(list_ptr->size, 0);
  TestSizeCapacityClear(list.get(), 0, 0);
}

TEST(Legacy_UList_Constructor, InitializerList) {
  UList list1{100, 1.0f, "Hi", DLDataType{kDLInt, 32, 1}, DLDevice{kDLCPU, 0}, Ref<Object>::New(), Ref<Object>()};
  UList list2{100, 1.0f, "Hi", DLDataType{kDLInt, 32, 1}, DLDevice{kDLCPU, 0}, Ref<Object>::New(), Ref<Object>()};
  auto test = [](UListObj *src) {
    auto *list_ptr = reinterpret_cast<const MLCList *>(src);
    ASSERT_NE(list_ptr, nullptr);
    EXPECT_EQ(list_ptr->_mlc_header.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCList));
    EXPECT_EQ(list_ptr->_mlc_header.ref_cnt, 1);
    EXPECT_NE(list_ptr->_mlc_header.deleter, nullptr);
    EXPECT_EQ(list_ptr->capacity, 7);
    EXPECT_EQ(list_ptr->size, 7);
    EXPECT_EQ(src->size(), 7);
    EXPECT_EQ(src->capacity(), 7);
    EXPECT_EQ(src->empty(), false);
    TestSizeCapacityClear(src, 7, 7);
  };
  test(list1.get());
  test(list2.get());
}

TEST(Legacy_UList_PushBack, POD) {
  UList list;
  ASSERT_NE(list.get(), nullptr);
  list->push_back(100);
  list->push_back(1.0f);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  ASSERT_NE(list_ptr, nullptr);
  EXPECT_EQ(list_ptr->_mlc_header.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCList));
  EXPECT_EQ(list_ptr->_mlc_header.ref_cnt, 1);
  EXPECT_NE(list_ptr->_mlc_header.deleter, nullptr);
  EXPECT_EQ(list_ptr->capacity, 2);
  EXPECT_EQ(list_ptr->size, 2);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
  TestSizeCapacityClear(list.get(), 2, 2);
}

TEST(Legacy_UList_PushBack, Obj) {
  UList list;
  Ref<Object> obj1 = Ref<Object>::New();
  Ref<Object> obj2 = Ref<Object>::New();
  list->push_back(obj1);
  list->push_back(obj2);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  ASSERT_NE(list_ptr, nullptr);
  EXPECT_EQ(list_ptr->_mlc_header.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCList));
  EXPECT_EQ(list_ptr->_mlc_header.ref_cnt, 1);
  EXPECT_NE(list_ptr->_mlc_header.deleter, nullptr);
  EXPECT_EQ(list_ptr->capacity, 2);
  EXPECT_EQ(list_ptr->size, 2);
  EXPECT_EQ((Object *)(list)[0], obj1.get());
  EXPECT_EQ((Object *)(list)[1], obj2.get());
  TestSizeCapacityClear(list.get(), 2, 2);
}

TEST(Legacy_UList_PushBack, Heterogeneous) {
  constexpr int n = 128;
  constexpr int k = 8;
  constexpr int expected_size = n * k;
  constexpr int expected_capacity = 1024;
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  std::string long_str(1024, 'a');

  UList list;
  {
    std::string long_str_copy(1024, 'a');
    for (int i = 0; i < n; ++i) {
      list->push_back(integer);
      list->push_back(fp);
      list->push_back(str);
      list->push_back(dtype);
      list->push_back(device);
      list->push_back(obj);
      list->push_back(null_obj);
      list->push_back(long_str_copy);
    }
  }
  for (int i = 0; i < n; ++i) {
    int64_t i_0 = list[i * k];
    double i_1 = list[i * k + 1];
    std::string i_2 = list[i * k + 2];
    DLDataType i_3 = list[i * k + 3];
    DLDevice i_4 = list[i * k + 4];
    Object *i_5 = list[i * k + 5];
    Object *i_6 = list[i * k + 6];
    const char *i_7 = list[i * k + 7];
    EXPECT_EQ(i_0, integer);
    EXPECT_DOUBLE_EQ(i_1, fp);
    EXPECT_EQ(i_2, str);
    EXPECT_PRED2(DataTypeEqual, i_3, dtype);
    EXPECT_PRED2(DeviceEqual, i_4, device);
    EXPECT_EQ(i_5, obj.get());
    EXPECT_EQ(i_6, nullptr);
    EXPECT_STREQ(i_7, long_str.c_str());
  }
  auto *list_ptr = reinterpret_cast<const MLCList *>(list.get());
  EXPECT_EQ(list_ptr->capacity, expected_capacity);
  EXPECT_EQ(list_ptr->size, expected_size);
}

TEST(Legacy_UList_Insert, Once) {
  UList values{100,
               1.0,
               "Hi",
               DLDataType{kDLInt, 32, 1},
               DLDevice{kDLCPU, 0},
               Ref<Object>::New(),
               Ref<Object>(),
               std::string(1024, 'a')};
  int n = static_cast<int32_t>(values->size());
  for (int pos = 0; pos <= n; ++pos) {
    for (AnyView data : *values) {
      // Test: insert at `pos` with value `data`
      UList list(values->begin(), values->end());
      list->insert(pos, data);
      auto test = [](AnyView expected, AnyView actual) {
        EXPECT_EQ(expected.type_index, actual.type_index);
        EXPECT_EQ(expected.v_int64, actual.v_int64);
      };
      for (int i = 0; i < pos; ++i) {
        test(values[i], list[i]);
      }
      for (int i = pos; i < n; ++i) {
        test(values[i], list[i + 1]);
      }
      test(data, list[pos]);
    }
  }
}

TEST(Legacy_UList_Insert, Error_0) {
  UList list{100, 1.0, "Hi"};
  try {
    list->insert(-1, 1.0);
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Indexing `-1` of a list of size 3");
  }
}

TEST(Legacy_UList_Insert, Error_1) {
  UList list{100, 1.0, "Hi"};
  try {
    list->insert(4, 1.0);
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Indexing `4` of a list of size 3");
  }
}

TEST(Legacy_UList_Resize, Shrink) {
  UList list{100, 1.0, "Hi"};
  list->resize(2);
  EXPECT_EQ(list->size(), 2);
  EXPECT_EQ(list->capacity(), 3);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
}

TEST(Legacy_UList_Resize, Expand) {
  UList list{100, 1.0, "Hi"};
  list->resize(5);
  EXPECT_EQ(list->size(), 5);
  EXPECT_EQ(list->capacity(), 8);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
  EXPECT_STREQ(list[2], "Hi");
  EXPECT_EQ(list[3].operator void *(), nullptr);
}

TEST(Legacy_UList_Reserve, Shrink) {
  UList list{100, 1.0, "Hi"};
  list->reserve(2);
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 3);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
  EXPECT_STREQ(list[2], "Hi");
}

TEST(Legacy_UList_Reserve, Expand) {
  UList list{100, 1.0, "Hi"};
  list->reserve(5);
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 8);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
  EXPECT_STREQ(list[2], "Hi");
}

TEST(Legacy_UList_SetItem, PodToPod) {
  UList list{100, 1.0, "Hi"};
  for (int i = 0; i < 16; ++i) {
    list[1] = i;
    EXPECT_EQ(list->size(), 3);
    EXPECT_EQ(list->capacity(), 3);
    EXPECT_EQ(int32_t(list[0]), 100);
    EXPECT_EQ(int32_t(list[1]), i);
    EXPECT_STREQ(list[2], "Hi");
  }
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 3);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  EXPECT_EQ(list_ptr->capacity, 3);
  EXPECT_EQ(list_ptr->size, 3);
}

TEST(Legacy_UList_SetItem, ObjToPod) {
  UList list{100, 1.0, "Hi"};
  for (int i = 0; i < 16; ++i) {
    list[2] = i;
    EXPECT_EQ(list->size(), 3);
    EXPECT_EQ(list->capacity(), 3);
    EXPECT_EQ(int32_t(list[0]), 100);
    EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
    EXPECT_EQ(int32_t(list[2]), i);
  }
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 3);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  EXPECT_EQ(list_ptr->capacity, 3);
  EXPECT_EQ(list_ptr->size, 3);
}

TEST(Legacy_UList_SetItem, PodToObj) {
  UList list{100, 1.0, "Hi"};
  for (int i = 0; i < 1; ++i) {
    Ref<Object> obj = Ref<Object>::New();
    list[0] = obj;
    EXPECT_EQ(list->size(), 3);
    EXPECT_EQ(list->capacity(), 3);
    EXPECT_EQ((Object *)(list[0]), obj.get());
    EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
    EXPECT_STREQ(list[2], "Hi");
  }
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 3);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  EXPECT_EQ(list_ptr->capacity, 3);
  EXPECT_EQ(list_ptr->size, 3);
}

TEST(Legacy_UList_SetItem, ObjToObj) {
  UList list{100, 1.0, "Hi"};
  for (int i = 0; i < 1; ++i) {
    Ref<Object> obj = Ref<Object>::New();
    list[2] = obj;
    EXPECT_EQ(list->size(), 3);
    EXPECT_EQ(list->capacity(), 3);
    EXPECT_EQ(int32_t(list[0]), 100);
    EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
    EXPECT_EQ((Object *)(list[2]), obj.get());
  }
  EXPECT_EQ(list->size(), 3);
  EXPECT_EQ(list->capacity(), 3);
  MLCList *list_ptr = reinterpret_cast<MLCList *>(list.get());
  EXPECT_EQ(list_ptr->capacity, 3);
  EXPECT_EQ(list_ptr->size, 3);
}

TEST(Legacy_UList_PopBack, Heterogeneous) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UList list{integer, fp, str, dtype, device, obj, null_obj};
  int n = static_cast<int32_t>(list->size());
  for (int i = 0; i < n; ++i) {
    list->pop_back();
    EXPECT_EQ(list->size(), n - 1 - i);
    EXPECT_EQ(list->capacity(), n);
    int m = static_cast<int32_t>(list->size());
    if (m > 0) {
      EXPECT_EQ(int32_t(list[0]), integer);
    }
    if (m > 1) {
      EXPECT_DOUBLE_EQ(double(list[1]), fp);
    }
    if (m > 2) {
      EXPECT_STREQ(list[2], str.c_str());
    }
    if (m > 3) {
      EXPECT_PRED2(DataTypeEqual, DLDataType(list[3]), dtype);
    }
    if (m > 4) {
      EXPECT_PRED2(DeviceEqual, DLDevice(list[4]), device);
    }
    if (m > 5) {
      EXPECT_EQ((Object *)(list[5]), obj.get());
    }
    if (m > 6) {
      EXPECT_EQ((Object *)(list[6]), nullptr);
    }
  }
  EXPECT_EQ(list->size(), 0);
  EXPECT_EQ(list->capacity(), n);
  EXPECT_EQ(list->empty(), true);
  EXPECT_EQ(list->begin(), list->end());
  try {
    list->pop_back();
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Indexing `-1` of a list of size 0");
  }
}

TEST(Legacy_UList_Erase, Front) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UList list{integer, fp, str, dtype, device, obj, null_obj};
  list->erase(0);
  EXPECT_EQ(list->size(), 6);
  EXPECT_EQ(list->capacity(), 7);
  EXPECT_EQ(double(list[0]), 1.0);
  EXPECT_STREQ(list[1], "Hi");
  EXPECT_PRED2(DataTypeEqual, DLDataType(list[2]), dtype);
  EXPECT_PRED2(DeviceEqual, DLDevice(list[3]), device);
  EXPECT_EQ((Object *)(list[4]), obj.get());
  EXPECT_EQ((Object *)(list[5]), nullptr);
}

TEST(Legacy_UList_Erase, Back) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UList list{integer, fp, str, dtype, device, obj, null_obj};
  list->erase(0);
  EXPECT_EQ(list->size(), 6);
  EXPECT_EQ(list->capacity(), 7);
  EXPECT_EQ(double(list[0]), 1.0);
  EXPECT_STREQ(list[1], "Hi");
  EXPECT_PRED2(DataTypeEqual, DLDataType(list[2]), dtype);
  EXPECT_PRED2(DeviceEqual, DLDevice(list[3]), device);
  EXPECT_EQ((Object *)(list[4]), obj.get());
  EXPECT_EQ((Object *)(list[5]), nullptr);
}

TEST(Legacy_UList_Erase, Mid) {
  int64_t integer = 100;
  double fp = 1.0;
  std::string str = "Hi";
  DLDataType dtype{kDLInt, 32, 1};
  DLDevice device{kDLCPU, 0};
  Ref<Object> obj = Ref<Object>::New();
  Ref<Object> null_obj{nullptr};
  UList list{integer, fp, str, dtype, device, obj, null_obj};
  list->erase(3);
  EXPECT_EQ(list->size(), 6);
  EXPECT_EQ(list->capacity(), 7);
  EXPECT_EQ(int32_t(list[0]), 100);
  EXPECT_DOUBLE_EQ(double(list[1]), 1.0);
  EXPECT_STREQ(list[2], "Hi");
  EXPECT_PRED2(DeviceEqual, DLDevice(list[3]), device);
  EXPECT_EQ((Object *)(list[4]), obj.get());
  EXPECT_EQ((Object *)(list[5]), nullptr);
}

TEST(Legacy_UList_Iter, Test) {
  UList list;
  for (int i = 0; i < 16; ++i) {
    list->push_back(i * i);
  }
  int i = 0;
  for (int item : *list) {
    EXPECT_EQ(i * i, item);
    ++i;
  }
}

TEST(Legacy_UList_RevIter, Test) {
  UList list;
  for (int i = 0; i < 16; ++i) {
    list->push_back(i * i);
  }
  int i = static_cast<int32_t>(list->size()) - 1;
  for (auto it = list->rbegin(); it != list->rend(); ++it) {
    EXPECT_EQ(i * i, int32_t(*it));
    --i;
  }
}

} // namespace
