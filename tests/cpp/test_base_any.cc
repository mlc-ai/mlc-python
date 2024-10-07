#include <gtest/gtest.h>
#include <mlc/all.h>

namespace {
using namespace mlc;
using mlc::base::DataTypeEqual;
using mlc::base::DeviceEqual;

template <typename T> inline void CheckObjPtr(const MLCAny &any, MLCTypeIndex type_index, T *ptr, int32_t ref_cnt) {
  EXPECT_EQ(any.type_index, static_cast<int32_t>(type_index));
  EXPECT_EQ(any.small_len, 0);
  MLCAny *v_obj = any.v_obj;
  EXPECT_EQ(static_cast<void *>(v_obj), static_cast<void *>(ptr));
  EXPECT_EQ(v_obj->type_index, static_cast<int32_t>(type_index));
  EXPECT_EQ(v_obj->ref_cnt, ref_cnt);
}

template <typename T> inline void CheckAnyPOD(const MLCAny &any, MLCTypeIndex type_index, T v) {
  EXPECT_EQ(any.type_index, static_cast<int32_t>(type_index));
  EXPECT_EQ(any.small_len, 0);
  if constexpr (std::is_integral_v<T>) {
    EXPECT_EQ(any.v_int64, v);
  } else if constexpr (std::is_floating_point_v<T>) {
    EXPECT_EQ(any.v_float64, v);
  } else if constexpr (std::is_same_v<T, void *>) {
    EXPECT_EQ(any.v_ptr, v);
  } else if constexpr (std::is_same_v<T, DLDevice>) {
    EXPECT_PRED2(DeviceEqual, any.v_device, v);
  } else if constexpr (std::is_same_v<T, DLDataType>) {
    EXPECT_PRED2(DataTypeEqual, any.v_dtype, v);
  } else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>) {
    EXPECT_STREQ(any.v_str, v);
  } else {
    static_assert(std::is_same_v<T, void>, "Unsupported type");
  }
}

template <typename T> inline void CheckConvert(std::function<T()> convert, T expected) {
  if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>) {
    EXPECT_STREQ(convert(), expected);
  } else if constexpr (std::is_same_v<T, DLDevice>) {
    EXPECT_PRED2(DeviceEqual, convert(), expected);
  } else if constexpr (std::is_same_v<T, DLDataType>) {
    EXPECT_PRED2(DataTypeEqual, convert(), expected);
  } else {
    EXPECT_EQ(convert(), expected);
  }
}

template <typename Callable>
inline void CheckConvertFail(Callable convert, int32_t type_index, const char *expected_type) {
  try {
    convert();
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    std::ostringstream os;
    os << "Cannot convert from type `" << base::TypeIndex2TypeKey(type_index) << "` to `" << expected_type << "`";
    EXPECT_EQ(ex.what(), os.str());
  }
}

template <typename Callable>
inline void CheckConvertFailStr(Callable convert, const char *expected_type, const char *str) {
  try {
    convert();
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    std::ostringstream os;
    os << "Cannot convert to `" << expected_type << "` from string: " << str;
    EXPECT_EQ(ex.what(), os.str());
  }
}

template <typename T, typename Callable>
inline void CheckConvertFailNullability(Callable convert, const char *type_key) {
  try {
    convert();
    FAIL() << "No exception thrown";
  } catch (Exception &ex) {
    std::ostringstream os;
    os << "Cannot convert from type `None` to non-nullable `" << type_key << "`";
    EXPECT_EQ(ex.what(), os.str());
  }
}

template <typename Callable>
inline void CheckConvertToRef(Callable convert, MLCTypeIndex type_index, int ref_cnt, void *ptr) {
  auto obj = convert();
  MLCAny *obj_ptr = reinterpret_cast<MLCAny *>(obj.get());
  EXPECT_EQ(obj_ptr->type_index, static_cast<int32_t>(type_index));
  EXPECT_EQ(obj_ptr->ref_cnt, ref_cnt);
  if (ptr != nullptr) {
    EXPECT_EQ(static_cast<void *>(obj_ptr), ptr);
  }
}

template <typename AnyType> struct Checker_Constructor_Default {
  static void Check(const AnyType &v) {
    CheckAnyPOD(v, MLCTypeIndex::kMLCNone, 0);
    EXPECT_STREQ(v.str()->c_str(), "None");
    EXPECT_EQ(v.operator void *(), nullptr);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvert<void *>([&]() { return v.operator void *(); }, nullptr);
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvert<Ref<Object>>([&]() { return v.operator Ref<Object>(); }, Ref<Object>());
    CheckConvertFailNullability<Object>([&]() { return v.operator ObjectRef(); }, "object.ObjectRef");
  }
};

TEST(Any, Constructor_Default) {
  {
    AnyView v;
    Checker_Constructor_Default<AnyView>::Check(v);
  }
  {
    Any v;
    Checker_Constructor_Default<Any>::Check(v);
  }
}

template <typename AnyType> struct Checker_Constructor_Integer {
  static void Check(const AnyType &v) {
    CheckAnyPOD<int>(v, MLCTypeIndex::kMLCInt, 1);
    EXPECT_STREQ(v.str()->c_str(), "1");
    EXPECT_EQ(v.operator int(), 1);
    CheckConvert<int>([&]() { return v.operator int(); }, 1);
    CheckConvert<double>([&]() { return v.operator double(); }, 1.0);
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  }
};

TEST(Any, Constructor_Integer) {
  {
    AnyView v(1);
    Checker_Constructor_Integer<AnyView>::Check(v);
  }
  {
    Any v(1);
    Checker_Constructor_Integer<Any>::Check(v);
  }
}

template <typename AnyType> struct Checker_Constructor_Float {
  static void Check(const AnyType &v) {
    CheckAnyPOD<double>(v, MLCTypeIndex::kMLCFloat, 3.14);
    double result = std::stod(v.str()->c_str());
    EXPECT_NEAR(result, 3.14, 1e-4);
    EXPECT_EQ(v.operator double(), 3.14);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvert<double>([&]() { return v.operator double(); }, 3.14);
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  }
};

TEST(Any, Constructor_Float) {
  {
    AnyView v(3.14);
    Checker_Constructor_Float<AnyView>::Check(v);
  }
  {
    Any v(3.14);
    Checker_Constructor_Float<Any>::Check(v);
  }
}

template <typename AnyType> struct Checker_Constructor_Ptr_NotNull {
  static void Check(const AnyType &v) {
    CheckAnyPOD<void *>(v, MLCTypeIndex::kMLCPtr, reinterpret_cast<void *>(0x1234));
#ifndef _MSC_VER
    EXPECT_STREQ(v.str()->c_str(), "0x1234");
#else
    EXPECT_STREQ(v.str()->c_str(), "0000000000001234");
#endif
    EXPECT_EQ(v.operator void *(), reinterpret_cast<void *>(0x1234));
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvert<void *>([&]() { return v.operator void *(); }, reinterpret_cast<void *>(0x1234));
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  }
};

TEST(Any, Constructor_Ptr_NotNull) {
  {
    AnyView v(reinterpret_cast<void *>(0x1234));
    Checker_Constructor_Ptr_NotNull<AnyView>::Check(v);
  }
  {
    Any v(reinterpret_cast<void *>(0x1234));
    Checker_Constructor_Ptr_NotNull<Any>::Check(v);
  }
}

template <typename AnyType> struct Checker_Constructor_Ptr_Null {
  static void Check(const AnyType &v) {
    CheckAnyPOD<void *>(v, MLCTypeIndex::kMLCNone, nullptr);
    EXPECT_STREQ(v.str()->c_str(), "None");
    EXPECT_EQ(v.operator void *(), nullptr);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvert<void *>([&]() { return v.operator void *(); }, nullptr);
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvert<Ref<Object>>([&]() { return v.operator Ref<Object>(); }, Ref<Object>());
    CheckConvertFailNullability<Object>([&]() { return v.operator ObjectRef(); }, "object.ObjectRef");
  }
};

TEST(Any, Constructor_Ptr_Null) {
  void *my_ptr = nullptr;
  {
    AnyView v(my_ptr);
    Checker_Constructor_Ptr_Null<AnyView>::Check(v);
  }
  {
    Any v(my_ptr);
    Checker_Constructor_Ptr_Null<Any>::Check(v);
  }
}

template <typename AnyType> struct Checker_Constructor_Device {
  static void Check(const AnyView &v, DLDevice dev) {
    CheckAnyPOD<DLDevice>(v, MLCTypeIndex::kMLCDevice, dev);
    EXPECT_STREQ(v.str()->c_str(), "cpu:0");
    EXPECT_PRED2(DeviceEqual, v.operator DLDevice(), dev);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvert<DLDevice>([&]() { return v.operator DLDevice(); }, dev);
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  }
};

TEST(Any, Constructor_Device) {
  DLDevice dev = {kDLCPU, 0};
  {
    AnyView v(dev);
    Checker_Constructor_Device<AnyView>::Check(v, dev);
  }
  {
    Any v(dev);
    Checker_Constructor_Device<Any>::Check(v, dev);
  }
}

template <typename AnyType> struct Checker_Constructor_DataType {
  static void Check(const AnyView &v, DLDataType dtype) {
    CheckAnyPOD<DLDataType>(v, MLCTypeIndex::kMLCDataType, dtype);
    EXPECT_STREQ(v.str()->c_str(), "float32x4");
    EXPECT_PRED2(DataTypeEqual, v.operator DLDataType(), dtype);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvert<DLDataType>([&]() { return v.operator DLDataType(); }, dtype);
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  }
};

TEST(Any, Constructor_DataType) {
  DLDataType dtype = {kDLFloat, 32, 4};
  {
    AnyView v(dtype);
    Checker_Constructor_DataType<AnyView>::Check(v, dtype);
  }
  {
    Any v(dtype);
    Checker_Constructor_DataType<Any>::Check(v, dtype);
  }
}

template <typename AnyType> struct Checker_Constructor_RawStr {
  static void Check(const AnyType &v, char *str, StrObj *str_ptr, int ref_cnt) {
    bool owned = std::is_same_v<AnyType, Any> || (str_ptr != nullptr);
    MLCAny *v_obj = owned ? reinterpret_cast<MLCAny *>(v.v_obj) : nullptr;
    if (owned) {
      EXPECT_EQ(v_obj->ref_cnt, ref_cnt);
      EXPECT_EQ(v_obj->type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
    } else {
      CheckAnyPOD<const char *>(v, MLCTypeIndex::kMLCRawStr, str);
    }
    EXPECT_STREQ(v.str()->c_str(), ('"' + std::string(str) + '"').c_str());
    EXPECT_STREQ(v.operator const char *(), str);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    if (owned) {
      CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    } else {
      CheckConvert<void *>([&]() { return v.operator void *(); }, static_cast<void *>(const_cast<char *>(str)));
    }
    CheckConvertFailStr([&]() { return v.operator DLDevice(); }, "Device", str);
    CheckConvertFailStr([&]() { return v.operator DLDataType(); }, "dtype", str);
    CheckConvert<const char *>([&]() { return v.operator const char *(); }, str);
    CheckConvert<std::string>([&]() { return v.operator std::string(); }, std::string(str));
    if (owned) {
      using RefObj = Ref<Object>;
      CheckConvertToRef([&]() { return v.operator Ref<Object>(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, str_ptr);
      CheckConvertToRef([&]() { return v.operator ObjectRef(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, str_ptr);
      CheckConvertToRef([&]() { return v.operator Ref<StrObj>(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, str_ptr);
      CheckConvertToRef([&]() { return v.operator Str(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, str_ptr);
      CheckConvertFail([&]() { return v.operator Ref<UListObj>(); }, v.type_index, "object.ListObj[Any] *");
      CheckConvert<RefObj>([&]() { return v.operator RefObj(); }, RefObj(reinterpret_cast<StrObj *>(v_obj)));
      CheckConvert<Object *>([&]() { return v.operator Object *(); }, reinterpret_cast<Object *>(v_obj));
      CheckConvert<StrObj *>([&]() { return v.operator StrObj *(); }, reinterpret_cast<StrObj *>(v_obj));
      EXPECT_EQ(v.operator ObjectRef().get(), reinterpret_cast<Object *>(v_obj));
    } else {
      CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
      CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
      CheckConvertFail([&]() { return v.operator StrObj *(); }, v.type_index, "object.StrObj *");
    }
    CheckConvert<Str>([&]() { return v.operator Str(); }, Str(str));
  }
};

struct Checker_Constructor_StrObj_Ref {
  template <typename RefCls> static void Check(RefCls str, const char *c_str) {
    {
      using CheckerAnyView = Checker_Constructor_RawStr<AnyView>;
      CheckerAnyView::Check(AnyView(str), const_cast<char *>(str->c_str()), str.get(), 1);       // Copy
      CheckerAnyView::Check(AnyView(str.get()), const_cast<char *>(str->c_str()), str.get(), 1); // Ptr
      // Move
      CheckerAnyView::Check(AnyView(std::move(str)), const_cast<char *>(str->c_str()), str.get(), 1);
      MLCAny *ptr = reinterpret_cast<MLCAny *>(str.get());
      EXPECT_NE(ptr, nullptr);
      EXPECT_EQ(ptr->ref_cnt, 1);
      EXPECT_EQ(ptr->type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
    }
    {
      using CheckerAny = Checker_Constructor_RawStr<Any>;
      CheckerAny::Check(Any(str), const_cast<char *>(str->c_str()), str.get(), 2);       // Copy
      CheckerAny::Check(Any(str.get()), const_cast<char *>(str->c_str()), str.get(), 2); // Ptr
      // move
      Any v(std::move(str));
      str.Reset();
      EXPECT_EQ(v.type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
      EXPECT_EQ(v.small_len, 0);
      MLCAny *v_obj = v.v_obj;
      EXPECT_EQ(v_obj->type_index, static_cast<int32_t>(MLCTypeIndex::kMLCStr));
      EXPECT_EQ(v_obj->ref_cnt, 1);
      EXPECT_STREQ(reinterpret_cast<StrObj *>(v_obj)->data(), c_str);
      EXPECT_EQ(str.get(), nullptr);
    }
  }
};

TEST(Any, Constructor_C_CharPtr) {
  const char *str = "hello";
  {
    AnyView v(str);
    Checker_Constructor_RawStr<AnyView>::Check(v, const_cast<char *>(str), nullptr, 0);
  }
  {
    Any v(str);
    Checker_Constructor_RawStr<Any>::Check(v, const_cast<char *>(str), nullptr, 1);
  }
}

TEST(Any, Constructor_C_CharArray) {
  char str[] = "world";
  {
    AnyView v(str);
    Checker_Constructor_RawStr<AnyView>::Check(v, str, nullptr, 0);
  }
  {
    Any v(str);
    Checker_Constructor_RawStr<Any>::Check(v, str, nullptr, 1);
  }
}

TEST(Any, Constructor_StdString) {
  std::string str = "world";
  Checker_Constructor_RawStr<AnyView>::Check(AnyView(str), str.data(), nullptr, 0);
  Checker_Constructor_RawStr<AnyView>::Check(AnyView(std::move(str)), str.data(), nullptr, 0);
  Checker_Constructor_RawStr<Any>::Check(Any(str), str.data(), nullptr, 1);
  Checker_Constructor_RawStr<Any>::Check(Any(std::move(str)), str.data(), nullptr, 1);
}

TEST(Any, Constructor_StrObj_Ref) {
  Checker_Constructor_StrObj_Ref::Check(Ref<StrObj>::New("hello"), "hello");
  Checker_Constructor_StrObj_Ref::Check(Str("world"), "world");
}

template <typename AnyType> struct Checker_Constructor_NullObj_Ref {
  static void Check(const AnyType &v) {
    CheckAnyPOD(v, MLCTypeIndex::kMLCNone, 0);
    EXPECT_STREQ(v.str()->c_str(), "None");
    EXPECT_EQ(v.operator void *(), nullptr);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvert<void *>([&]() { return v.operator void *(); }, nullptr);
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvert<Ref<Object>>([&]() { return v.operator Ref<Object>(); }, Ref<Object>());
    CheckConvertFailNullability<Object>([&]() { return v.operator ObjectRef(); }, "object.ObjectRef");
  }
};

TEST(Any, Constructor_NullObj_Ref) {
  {
    ObjectRef obj{Null};
    EXPECT_EQ(obj.get(), nullptr);
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(obj));
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(obj.get()));
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(std::move(obj)));
    EXPECT_EQ(obj.get(), nullptr);
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(obj));
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(obj.get()));
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(std::move(obj)));
    EXPECT_EQ(obj.get(), nullptr);
  }
  {
    Ref<Object> obj;
    EXPECT_EQ(obj.get(), nullptr);
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(obj));
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(obj.get()));
    Checker_Constructor_NullObj_Ref<AnyView>::Check(AnyView(std::move(obj)));
    EXPECT_EQ(obj.get(), nullptr);
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(obj));
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(obj.get()));
    Checker_Constructor_NullObj_Ref<Any>::Check(Any(std::move(obj)));
    EXPECT_EQ(obj.get(), nullptr);
  }
}

TEST(Any, Constructor_Any_POD) {
  auto check = [](const AnyView &v) {
    CheckConvert<int>([&]() { return v.operator int(); }, 1);
    CheckConvert<double>([&]() { return v.operator double(); }, 1.0);
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvertFail([&]() { return v.operator DLDevice(); }, v.type_index, "Device");
    CheckConvertFail([&]() { return v.operator DLDataType(); }, v.type_index, "dtype");
    CheckConvertFail([&]() { return v.operator const char *(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator std::string(); }, v.type_index, "char *");
    CheckConvertFail([&]() { return v.operator Ref<Object>(); }, v.type_index, "object.Object *");
    CheckConvertFail([&]() { return v.operator ObjectRef(); }, v.type_index, "object.Object *");
  };
  Any src(1);
  check(AnyView(src));            // Copy
  check(AnyView(std::move(src))); // Move
  CheckAnyPOD(src, MLCTypeIndex::kMLCInt, 1);
  check(Any(src));            // Copy
  check(Any(std::move(src))); // Move
  CheckAnyPOD(src, MLCTypeIndex::kMLCNone, 0);
}

TEST(Any, Constructor_Any_ObjPtr) {
  auto check = [&](const AnyView &v, Any &src, int ref_cnt) {
    MLCAny *v_obj = reinterpret_cast<MLCAny *>(src.v_obj);
    const char *c_str = reinterpret_cast<StrObj *>(src.v_obj)->c_str();
    CheckObjPtr(v, MLCTypeIndex::kMLCStr, v_obj, ref_cnt);
    EXPECT_STREQ(v.str()->c_str(), "\"hello\"");
    EXPECT_STREQ(v.operator const char *(), c_str);
    CheckConvertFail([&]() { return v.operator int(); }, v.type_index, "int");
    CheckConvertFail([&]() { return v.operator double(); }, v.type_index, "float");
    CheckConvertFail([&]() { return v.operator void *(); }, v.type_index, "Ptr");
    CheckConvertFailStr([&]() { return v.operator DLDevice(); }, "Device", c_str);
    CheckConvertFailStr([&]() { return v.operator DLDataType(); }, "dtype", c_str);
    CheckConvert<const char *>([&]() { return v.operator const char *(); }, c_str);
    CheckConvert<std::string>([&]() { return v.operator std::string(); }, std::string(c_str));
    CheckConvertToRef([&]() { return v.operator Ref<Object>(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, v_obj);
    CheckConvertToRef([&]() { return v.operator ObjectRef(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, v_obj);
    CheckConvertToRef([&]() { return v.operator Ref<StrObj>(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, v_obj);
    CheckConvertToRef([&]() { return v.operator Str(); }, MLCTypeIndex::kMLCStr, ref_cnt + 1, v_obj);
    CheckConvertFail([&]() { return v.operator Ref<UListObj>(); }, v.type_index, "object.ListObj[Any] *");
  };
  Any src(Ref<StrObj>::New("hello"));
  check(AnyView(src), src, 1);            // Copy
  check(AnyView(std::move(src)), src, 1); // Move
  EXPECT_EQ(reinterpret_cast<MLCAny *>(src.v_obj)->ref_cnt, 1);
  check(Any(src), src, 2); // Copy
  {
    // Move
    MLCAny *v_obj = reinterpret_cast<MLCAny *>(src.v_obj);
    Any v(std::move(src));
    CheckObjPtr(v, MLCTypeIndex::kMLCStr, v_obj, 1);
    CheckAnyPOD(src, MLCTypeIndex::kMLCNone, 0);
  }
}

} // namespace
