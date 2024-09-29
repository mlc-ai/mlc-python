#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {
using namespace mlc;

static_assert(base::IsObj<Object>, "IsObj<Object> == true");
static_assert(base::IsObj<FuncObj>, "IsObj<Func> == true");
static_assert(base::IsObj<StrObj>, "IsObj<Str> == true");

struct SubType : public Object {
  int data;
  explicit SubType(int data) : data(data) {
    if (data == 1) {
      throw std::runtime_error("New Error");
    }
  }
};

struct TestObj : public Object {
  int x;
  explicit TestObj(int x) : x(x) {}
  MLC_DEF_DYN_TYPE(TestObj, Object, "test.TestObj");
};

struct SubTestObj : public TestObj {
  int y;
  explicit SubTestObj(int x, int y) : TestObj(x), y(y) {}
  MLC_DEF_DYN_TYPE(SubTestObj, TestObj, "test.SubTestObj");
};

void CheckAncestor(int32_t num, const int32_t *ancestors, std::vector<int32_t> expected) {
  EXPECT_EQ(num, expected.size());
  for (int i = 0; i < num; ++i) {
    EXPECT_EQ(ancestors[i], expected[i]);
  }
}

TEST(StaticTypeInfo, Object) {
  EXPECT_EQ(Object::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCObject));
  EXPECT_STRCASEEQ(Object::_type_key, "object.Object");
  EXPECT_EQ(Object::_type_depth, 0);
  CheckAncestor(Object::_type_depth, Object::_type_ancestors, {});
}

TEST(StaticTypeInfo, FuncObj) {
  EXPECT_EQ(FuncObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCFunc));
  EXPECT_STRCASEEQ(FuncObj::_type_key, "object.Func");
  EXPECT_EQ(FuncObj::_type_depth, 1);
  CheckAncestor(FuncObj::_type_depth, FuncObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(StaticTypeInfo, StrObj) {
  EXPECT_EQ(StrObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
  EXPECT_STRCASEEQ(StrObj::_type_key, "object.Str");
  EXPECT_EQ(StrObj::_type_depth, 1);
  CheckAncestor(StrObj::_type_depth, StrObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(StaticTypeInheritance, None) {
  Ref<Object> obj;
  // FIXME: The lines below are going to segfault
  // EXPECT_STREQ(obj->GetTypeKey(), "None");
  // EXPECT_FALSE(obj->IsInstance<Object>());
  // EXPECT_FALSE(obj->IsInstance<Func>());
  // EXPECT_FALSE(obj->IsInstance<Str>());
}

TEST(StaticTypeInheritance, Object) {
  Ref<Object> obj = Ref<Object>::New();
  EXPECT_STREQ(obj->GetTypeKey(), "object.Object");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Func_0) {
  Ref<FuncObj> obj = Ref<FuncObj>::New([](int64_t x) { return x + 1; });
  EXPECT_STREQ(obj->GetTypeKey(), "object.Func");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Func_1) {
  Ref<FuncObj> obj = Ref<FuncObj>::New([](int64_t x) { return x + 1; });
  EXPECT_STREQ(obj->GetTypeKey(), "object.Func");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Str_0) {
  Ref<StrObj> obj = Ref<StrObj>::New("Hello, World!");
  EXPECT_STREQ(obj->GetTypeKey(), "object.Str");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_TRUE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeInheritance, Str_1) {
  Ref<Object> obj = Ref<StrObj>::New("Hello, World!");
  EXPECT_STREQ(obj->GetTypeKey(), "object.Str");
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_TRUE(obj->IsInstance<StrObj>());
}

TEST(StaticTypeSubclass, NoException) {
  Ref<SubType> obj = Ref<SubType>::New(0);
  EXPECT_EQ(obj->data, 0);
}

TEST(StaticTypeSubclass, Exception) {
  try {
    Ref<SubType>::New(1);
    FAIL() << "No exception thrown";
  } catch (std::runtime_error &ex) {
    EXPECT_STREQ(ex.what(), "New Error");
  }
}

TEST(DynTypeInfo, TestObj) {
  EXPECT_GE(TestObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin));
  EXPECT_STRCASEEQ(TestObj::_type_key, "test.TestObj");
  EXPECT_EQ(TestObj::_type_depth, 1);
  CheckAncestor(TestObj::_type_depth, TestObj::_type_ancestors, {static_cast<int32_t>(MLCTypeIndex::kMLCObject)});
}

TEST(DynTypeInfo, SubTestObj) {
  EXPECT_GE(SubTestObj::_type_index, static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin));
  EXPECT_NE(SubTestObj::_type_index, TestObj::_type_index);
  EXPECT_STRCASEEQ(SubTestObj::_type_key, "test.SubTestObj");
  EXPECT_EQ(SubTestObj::_type_depth, 2);
  CheckAncestor(SubTestObj::_type_depth, SubTestObj::_type_ancestors,
                {static_cast<int32_t>(MLCTypeIndex::kMLCObject), TestObj::_type_index});
}

TEST(DynTypeInheritance, TestObj) {
  Ref<TestObj> obj = Ref<TestObj>::New(10);
  EXPECT_EQ(obj->x, 10);
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<TestObj>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

TEST(DynTypeInheritance, SubTestObj) {
  Ref<SubTestObj> obj = Ref<SubTestObj>::New(10, 20);
  EXPECT_EQ(obj->x, 10);
  EXPECT_EQ(obj->y, 20);
  EXPECT_TRUE(obj->IsInstance<Object>());
  EXPECT_TRUE(obj->IsInstance<TestObj>());
  EXPECT_TRUE(obj->IsInstance<SubTestObj>());
  EXPECT_FALSE(obj->IsInstance<FuncObj>());
  EXPECT_FALSE(obj->IsInstance<StrObj>());
}

} // namespace
