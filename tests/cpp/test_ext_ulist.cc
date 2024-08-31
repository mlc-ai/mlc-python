#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {

using namespace mlc;

TEST(UList, DefaultConstructor) {
  UList list;
  EXPECT_EQ(list.size(), 0);
  EXPECT_EQ(list.capacity(), 0);
  EXPECT_TRUE(list.empty());
}

TEST(UList, InitializerListConstructor) {
  UList list{1, 2.0, "three"};
  EXPECT_EQ(list.size(), 3);
  EXPECT_GE(list.capacity(), 3);
  EXPECT_FALSE(list.empty());
  EXPECT_EQ(list[0].operator int(), 1);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 2.0);
  EXPECT_STREQ(list[2].operator const char *(), "three");
}

TEST(UList, IteratorConstructor) {
  std::vector<Any> vec{1, 2.0, "three"};
  UList list(vec.begin(), vec.end());
  EXPECT_EQ(list.size(), 3);
  EXPECT_GE(list.capacity(), 3);
  EXPECT_FALSE(list.empty());
  EXPECT_EQ(list[0].operator int(), 1);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 2.0);
  EXPECT_STREQ(list[2].operator const char *(), "three");
}

TEST(UList, Insert) {
  UList list{1, 2, 3};
  list.insert(1, 4);
  EXPECT_EQ(list.size(), 4);
  EXPECT_EQ(list[1].operator int(), 4);
}

TEST(UList, InsertRange) {
  UList list{1, 2, 3};
  std::vector<Any> vec{4, 5};
  list.insert(1, vec.begin(), vec.end());
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list[1].operator int(), 4);
  EXPECT_EQ(list[2].operator int(), 5);
}

TEST(UList, Reserve) {
  UList list;
  list.reserve(10);
  EXPECT_GE(list.capacity(), 10);
  EXPECT_EQ(list.size(), 0);
}

TEST(UList, Clear) {
  UList list{1, 2, 3};
  list.clear();
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());
}

TEST(UList, Resize) {
  UList list{1, 2, 3};
  list.resize(5);
  EXPECT_EQ(list.size(), 5);
  EXPECT_EQ(list[3].operator void *(), nullptr);
  EXPECT_EQ(list[4].operator void *(), nullptr);

  list.resize(2);
  EXPECT_EQ(list.size(), 2);
}

TEST(UList, PushBack) {
  UList list;
  list.push_back(1);
  list.push_back(2.0);
  list.push_back("three");
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[0].operator int(), 1);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 2.0);
  EXPECT_STREQ(list[2].operator const char *(), "three");
}

TEST(UList, PopBack) {
  UList list{1, 2, 3};
  list.pop_back();
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list[1].operator int(), 2);
}

TEST(UList, Erase) {
  UList list{1, 2, 3, 4};
  list.erase(1);
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list[1].operator int(), 3);
}

TEST(UList, OperatorSquareBrackets) {
  UList list{1, 2.0, "three"};
  EXPECT_EQ(list[0].operator int(), 1);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 2.0);
  EXPECT_STREQ(list[2].operator const char *(), "three");

  list[1] = 3;
  EXPECT_EQ(list[1].operator int(), 3);
}

TEST(UList, FrontAndBack) {
  UList list{1, 2, 3};
  EXPECT_EQ(list.front().operator int(), 1);
  EXPECT_EQ(list.back().operator int(), 3);
}

TEST(UList, Iterators) {
  UList list{1, 2, 3};
  int sum = 0;
  for (const auto &item : *list) {
    sum += item.operator int();
  }
  EXPECT_EQ(sum, 6);
}

TEST(UList, ReverseIterators) {
  UList list{1, 2, 3};
  std::vector<int> reversed;
  for (auto it = list.rbegin(); it != list.rend(); ++it) {
    reversed.push_back(it->operator int());
  }
  EXPECT_EQ(reversed, std::vector<int>({3, 2, 1}));
}

TEST(UList, CopyConstructor) {
  UList list1{1, 2, 3};
  UList list2(list1);
  EXPECT_EQ(list1.size(), list2.size());
  for (int64_t i = 0; i < list1.size(); ++i) {
    EXPECT_EQ(list1[i].operator int(), list2[i].operator int());
  }
}

TEST(UList, MoveConstructor) {
  UList list1{1, 2, 3};
  UList list2(std::move(list1));
  EXPECT_EQ(list2.size(), 3);
  EXPECT_EQ(list2[0].operator int(), 1);
  EXPECT_EQ(list2[1].operator int(), 2);
  EXPECT_EQ(list2[2].operator int(), 3);
  EXPECT_EQ(list1.get(), nullptr); // Moved-from object should be empty
}

TEST(UList, AssignmentOperator) {
  UList list1{1, 2, 3};
  UList list2;
  list2 = list1;
  EXPECT_EQ(list1.size(), list2.size());
  for (int64_t i = 0; i < list1.size(); ++i) {
    EXPECT_EQ(list1[i].operator int(), list2[i].operator int());
  }
}

TEST(UList, MoveAssignmentOperator) {
  UList list1{1, 2, 3};
  UList list2;
  list2 = std::move(list1);
  EXPECT_EQ(list2.size(), 3);
  EXPECT_EQ(list2[0].operator int(), 1);
  EXPECT_EQ(list2[1].operator int(), 2);
  EXPECT_EQ(list2[2].operator int(), 3);
  EXPECT_EQ(list1.get(), nullptr); // Moved-from object should be empty
}

TEST(UList, HeterogeneousTypes) {
  UList list;
  list.push_back(1);
  list.push_back(2.5);
  list.push_back("three");
  list.push_back(Ref<Object>::New());

  EXPECT_EQ(list.size(), 4);
  EXPECT_EQ(list[0].operator int(), 1);
  EXPECT_DOUBLE_EQ(list[1].operator double(), 2.5);
  EXPECT_STREQ(list[2].operator const char *(), "three");
  EXPECT_NE(list[3].operator Ref<Object>().get(), nullptr);
}

TEST(UList, LargeList) {
  UList list;
  const int size = 10000;
  for (int i = 0; i < size; ++i) {
    list.push_back(i);
  }
  EXPECT_EQ(list.size(), size);
  for (int i = 0; i < size; ++i) {
    EXPECT_EQ(list[i].operator int(), i);
  }
}

} // namespace
