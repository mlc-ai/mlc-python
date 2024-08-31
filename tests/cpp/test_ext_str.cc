#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {
using namespace mlc;

TEST(StrObj, DefaultConstructor) {
  Ref<StrObj> str = Ref<StrObj>::New("");
  EXPECT_EQ(str->size(), 0);
  EXPECT_STREQ(str->c_str(), "");
}

TEST(StrObj, ConstructorFromNullptr) {
  try {
    Ref<StrObj>::New(static_cast<const char *>(nullptr));
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Cannot create StrObj from nullptr");
  }
}

TEST(StrObj, ConstructorFromCStyleCharPointer) {
  const char *c_str = "Hello, World!";
  Ref<StrObj> str = Ref<StrObj>::New(c_str);
  EXPECT_EQ(str->size(), strlen(c_str));
  EXPECT_STREQ(str->c_str(), c_str);
}

TEST(StrObj, ConstructorFromCStyleCharArray) {
  constexpr int64_t len = 23;
  char c_str[len] = "Hello, World!";
  Ref<StrObj> str = Ref<StrObj>::New(c_str);
#ifndef _MSC_VER
  EXPECT_EQ(str->size(), len - 1);
#endif
  EXPECT_STREQ(str->c_str(), c_str);
}

TEST(StrObj, ConstructorFromStdString) {
  std::string std_str = "Hello, World!";
  Ref<StrObj> str = Ref<StrObj>::New(std_str);
  EXPECT_EQ(str->size(), std_str.size());
  EXPECT_STREQ(str->c_str(), std_str.c_str());
}

TEST(StrObj, ConstructorFromStdStringRValue) {
  std::string std_str = "Hello, World!";
  Ref<StrObj> str = Ref<StrObj>::New(std::move(std_str));
  EXPECT_EQ(str->size(), 13);
  EXPECT_STREQ(str->c_str(), "Hello, World!");
}

TEST(Str, DefaultConstructor) {
  Str str(Null);
  EXPECT_EQ(str.get(), nullptr);
}

TEST(Str, ConstructorFromNullptr) {
  const char *c_str = nullptr;
  try {
    Str str(c_str);
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    EXPECT_STREQ(ex.what(), "Cannot create StrObj from nullptr");
  }
}

TEST(Str, ConstructorFromCStyleCharPointer) {
  const char *c_str = "Hello, World!";
  Str str(c_str);
  EXPECT_EQ(str.size(), strlen(c_str));
  EXPECT_STREQ(str.c_str(), c_str);
}

TEST(Str, ConstructorFromCStyleCharArray) {
  constexpr int64_t len = 23;
  char c_str[len] = "Hello, World!";
  Str str(c_str);
#ifndef _MSC_VER
  EXPECT_EQ(str.size(), len - 1);
#endif
  EXPECT_STREQ(str.c_str(), c_str);
}

TEST(Str, ConstructorFromStdString) {
  std::string std_str = "Hello, World!";
  Str str(std_str);
  EXPECT_EQ(str.size(), std_str.size());
  EXPECT_STREQ(str.c_str(), std_str.c_str());
}

TEST(Str, ConstructorFromStdStringRValue) {
  std::string std_str = "Hello, World!";
  Str str(std::move(std_str));
  EXPECT_EQ(str.size(), 13);
  EXPECT_STREQ(str.c_str(), "Hello, World!");
}

TEST(Str, CopyConstructor) {
  Str str1("Hello, World!");
  Str str2(str1);
  EXPECT_EQ(str1.get(), str2.get());
}

TEST(Str, MoveConstructor) {
  Str str1("Hello, World!");
  Str str2(std::move(str1));
  EXPECT_EQ(str1.get(), nullptr);
  EXPECT_EQ(str2.size(), 13);
  EXPECT_STREQ(str2.c_str(), "Hello, World!");
}

TEST(Str, CopyAssignment) {
  Str str1("Hello, World!");
  Str str2("Test");
  str2 = str1;
  EXPECT_EQ(str1.get(), str2.get());
}

TEST(Str, MoveAssignment) {
  Str str1("Hello, World!");
  Str str2("Test");
  StrObj *str_ptr = str1.get();
  str2 = std::move(str1);
  EXPECT_EQ(str1.get(), nullptr);
  EXPECT_EQ(str2.get(), str_ptr);
}

TEST(Str, Comparison) {
  Str str1("Hello");
  Str str2("World");
  Str str3("Hello");
  std::string std_str1 = "Hello";
  std::string std_str2 = "World";

  EXPECT_TRUE(str1 < str2);
  EXPECT_FALSE(str1 > str2);
  EXPECT_TRUE(str1 <= str3);
  EXPECT_TRUE(str1 >= str3);
  EXPECT_TRUE(str1 == str3);
  EXPECT_TRUE(str1 != str2);

  EXPECT_TRUE(str1 < "World");
  EXPECT_TRUE("World" > str1);
  EXPECT_TRUE(str1 <= "Hello");
  EXPECT_TRUE("Hello" >= str1);
  EXPECT_TRUE(str1 == "Hello");
  EXPECT_TRUE(str1 != "World");

  EXPECT_TRUE(str1 < std_str2);
  EXPECT_TRUE(std_str2 > str1);
  EXPECT_TRUE(str1 <= std_str1);
  EXPECT_TRUE(std_str1 >= str1);
  EXPECT_TRUE(str1 == std_str1);
  EXPECT_TRUE(str1 != std_str2);
}

TEST(Str, StreamOperator) {
  Str str("Hello, World!");
  std::ostringstream oss;
  oss << str;
  EXPECT_EQ(oss.str(), "Hello, World!");
}

} // namespace
