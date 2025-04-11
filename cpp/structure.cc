#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mlc/core/all.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace mlc {
namespace {

using mlc::core::ObjectPath;
using mlc::core::TopoVisit;
using mlc::core::VisitFields;
using mlc::core::VisitStructure;

/****************** JSON ******************/

inline Any JSONLoads(const char *json_str, int64_t json_str_len) {
  struct JSONParser {
    Any Parse() {
      SkipWhitespace();
      Any result = ParseValue();
      SkipWhitespace();
      if (i != json_str_len) {
        MLC_THROW(ValueError) << "JSON parsing failure at position " << i
                              << ": Extra data after valid JSON. JSON string: " << json_str;
      }
      return result;
    }

    void ExpectChar(char c) {
      if (json_str[i] == c) {
        ++i;
      } else {
        MLC_THROW(ValueError) << "JSON parsing failure at position " << i << ": Expected '" << c << "' but got '"
                              << json_str[i] << "'. JSON string: " << json_str;
      }
    }

    char PeekChar() { return i < json_str_len ? json_str[i] : '\0'; }

    void SkipWhitespace() {
      while (i < json_str_len && std::isspace(json_str[i])) {
        ++i;
      }
    }

    void ExpectString(const char *expected, int64_t len) {
      if (i + len <= json_str_len && std::strncmp(json_str + i, expected, len) == 0) {
        i = i + len;
      } else {
        MLC_THROW(ValueError) << "JSON parsing failure at position " << i << ": Expected '" << expected
                              << ". JSON string: " << json_str;
      }
    }

    Any ParseNull() {
      ExpectString("null", 4);
      return Any(nullptr);
    }

    Any ParseBoolean() {
      if (PeekChar() == 't') {
        ExpectString("true", 4);
        return Any(true);
      } else {
        ExpectString("false", 5);
        return Any(false);
      }
    }

    Any ParseNumber() {
      int64_t start = i;
      // Identify the end of the numeric sequence
      while (i < json_str_len) {
        char c = json_str[i];
        if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-' || std::isdigit(c)) {
          ++i;
        } else {
          break;
        }
      }
      std::string num_str(json_str + start, i - start);
      std::size_t pos = 0;
      try {
        // Attempt to parse as integer
        int64_t int_value = std::stoll(num_str, &pos);
        if (pos == num_str.size()) {
          i = start + static_cast<int64_t>(pos); // Update the main index
          return Any(int_value);
        }
      } catch (const std::invalid_argument &) { // Not an integer, proceed to parse as double
      } catch (const std::out_of_range &) {     // Integer out of range, proceed to parse as double
      }
      try {
        // Attempt to parse as double
        double double_value = std::stod(num_str, &pos);
        if (pos == num_str.size()) {
          i = start + pos; // Update the main index
          return Any(double_value);
        }
      } catch (const std::invalid_argument &) {
      } catch (const std::out_of_range &) {
      }
      MLC_THROW(ValueError) << "JSON parsing failure at position " << i
                            << ": Invalid number format. JSON string: " << json_str;
      MLC_UNREACHABLE();
    }

    Any ParseStr() {
      ExpectChar('"');
      std::ostringstream oss;
      while (true) {
        if (i >= json_str_len) {
          MLC_THROW(ValueError) << "JSON parsing failure at position " << i
                                << ": Unterminated string. JSON string: " << json_str;
        }
        char c = json_str[i++];
        if (c == '"') {
          // End of string
          return Any(Str(oss.str()));
        } else if (c == '\\') {
          // Handle escape sequences
          if (i >= json_str_len) {
            MLC_THROW(ValueError) << "JSON parsing failure at position " << i
                                  << ": Incomplete escape sequence. JSON string: " << json_str;
          }
          char next = json_str[i++];
          switch (next) {
          case 'n':
            oss << '\n';
            break;
          case 't':
            oss << '\t';
            break;
          case 'r':
            oss << '\r';
            break;
          case '\\':
            oss << '\\';
            break;
          case '"':
            oss << '\"';
            break;
          case 'x': {
            if (i + 1 < json_str_len && std::isxdigit(json_str[i]) && std::isxdigit(json_str[i + 1])) {
              int32_t value = std::stoi(std::string(json_str + i, 2), nullptr, 16);
              oss << static_cast<char>(value);
              i += 2;
            } else {
              MLC_THROW(ValueError) << "Invalid hexadecimal escape sequence at position " << i - 2
                                    << " in string: " << json_str;
            }
            break;
          }
          case 'u': {
            if (i + 3 < json_str_len && std::isxdigit(json_str[i]) && std::isxdigit(json_str[i + 1]) &&
                std::isxdigit(json_str[i + 2]) && std::isxdigit(json_str[i + 3])) {
              int32_t codepoint = std::stoi(std::string(json_str + i, 4), nullptr, 16);
              if (codepoint <= 0x7F) {
                // 1-byte UTF-8
                oss << static_cast<char>(codepoint);
              } else if (codepoint <= 0x7FF) {
                // 2-byte UTF-8
                oss << static_cast<char>(0xC0 | (codepoint >> 6));
                oss << static_cast<char>(0x80 | (codepoint & 0x3F));
              } else {
                // 3-byte UTF-8
                oss << static_cast<char>(0xE0 | (codepoint >> 12));
                oss << static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                oss << static_cast<char>(0x80 | (codepoint & 0x3F));
              }
              i += 4;
            } else {
              MLC_THROW(ValueError) << "Invalid Unicode escape sequence at position " << i - 2
                                    << " in string: " << json_str;
            }
            break;
          }
          default:
            // Unrecognized escape sequence, interpret literally
            oss << next;
            break;
          }
        } else {
          // Regular character, copy as-is
          oss << c;
        }
      }
    }

    UList ParseArray() {
      UList arr;
      ExpectChar('[');
      SkipWhitespace();
      if (PeekChar() == ']') {
        ExpectChar(']');
        return arr;
      }
      while (true) {
        SkipWhitespace();
        arr.push_back(ParseValue());
        SkipWhitespace();
        if (PeekChar() == ']') {
          ExpectChar(']');
          return arr;
        }
        ExpectChar(',');
      }
    }

    Any ParseObject() {
      UDict obj;
      ExpectChar('{');
      SkipWhitespace();
      if (PeekChar() == '}') {
        ExpectChar('}');
        return Any(obj);
      }
      while (true) {
        SkipWhitespace();
        Any key = ParseStr();
        SkipWhitespace();
        ExpectChar(':');
        SkipWhitespace();
        Any value = ParseValue();
        obj[key] = value;
        SkipWhitespace();
        if (PeekChar() == '}') {
          ExpectChar('}');
          return Any(obj);
        }
        ExpectChar(',');
      }
    }

    Any ParseValue() {
      SkipWhitespace();
      char c = PeekChar();
      if (c == '"') {
        return ParseStr();
      } else if (c == '{') {
        return ParseObject();
      } else if (c == '[') {
        return ParseArray();
      } else if (c == 'n') {
        return ParseNull();
      } else if (c == 't' || c == 'f') {
        return ParseBoolean();
      } else if (std::isdigit(c) || c == '-') {
        return ParseNumber();
      } else {
        MLC_THROW(ValueError) << "JSON parsing failure at position " << i << ": Unexpected character: " << c
                              << ". JSON string: " << json_str;
      }
      MLC_UNREACHABLE();
    }
    int64_t i;
    int64_t json_str_len;
    const char *json_str;
  };
  if (json_str_len < 0) {
    json_str_len = static_cast<int64_t>(std::strlen(json_str));
  }
  return JSONParser{0, json_str_len, json_str}.Parse();
}

/****************** Base64 Encoding/Decoding ******************/

static const char kBase64EncTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const std::array<uint8_t, 256> kBase64DecTable = []() {
  std::array<uint8_t, 256> ret;
  for (int i = 0; i < 256; ++i) {
    ret[i] = 0xFF;
  }
  // Fill valid mappings: 'A'-'Z', 'a'-'z', '0'-'9', '+' and '/'
  for (int i = 0; i < 64; ++i) {
    ret[kBase64EncTable[i]] = static_cast<uint8_t>(i);
  }
  return ret;
}();

Str Base64Encode(const uint8_t *data, int64_t len) {
  constexpr int BITS_PER_CHAR = 6;
  Str ret(::mlc::core::StrPad::Allocator::NewWithPad<uint8_t>(((len + 2) / 3) * 4 + 1, 0));
  uint8_t *out = reinterpret_cast<uint8_t *>(ret.get()->::MLCStr::data);
  int64_t &out_len = ret.get()->::MLCStr::length;
  for (int64_t i = 0; i < len; i += 3) {
    // Collect up to 3 bytes into a 24-bit chunk
    uint32_t chunk = 0;
    int bytes_in_chunk = 0;
    for (int j = 0; j < 3; ++j) {
      if (i + j < len) {
        chunk <<= 8;
        chunk |= data[i + j];
        bytes_in_chunk++;
      } else {
        chunk <<= 8; // pad with zero
      }
    }
    // chunk now has up to 24 bits of actual data, left-aligned
    // We emit 4 Base64 chars, but some might become '=' padding
    // Based on how many raw bytes we actually had.
    for (int k = 0; k < 4; ++k) {
      // For each 6-bit group: (from left to right in chunk)
      int shift = 18 - (k * BITS_PER_CHAR);
      uint32_t index = (chunk >> shift) & 0x3F;
      // For 3 raw bytes, k goes 0..3 => all real
      // For 2 raw bytes, k goes 0..2 => 3rd is '='
      // For 1 raw byte, k goes 0..1 => 2nd/3rd are '='
      if (k <= bytes_in_chunk) {
        out[out_len++] = kBase64EncTable[index];
      } else {
        out[out_len++] = '=';
      }
    }
  }
  out[out_len] = '\0';
  return ret;
}

Str Base64Decode(const uint8_t *data, int64_t len) {
  if (len % 4 != 0) {
    MLC_THROW(ValueError) << "Base64Decode: Input length not multiple of 4: length = " << len
                          << ", data = " << reinterpret_cast<const char *>(data);
  }
  Str ret(::mlc::core::StrPad::Allocator::NewWithPad<uint8_t>((len / 4) * 3 + 1, 0));
  uint8_t *out = reinterpret_cast<uint8_t *>(ret.get()->::MLCStr::data);
  int64_t &result_len = ret.get()->::MLCStr::length;
  for (int64_t i = 0; i < len; i += 4) {
    // Each block of 4 chars -> up to 3 bytes
    uint32_t accum = 0;
    int valid_chars = 0;
    // Read 4 base64 characters
    for (int j = 0; j < 4; ++j) {
      if (uint8_t c = static_cast<uint8_t>(data[i + j]); c != '=') {
        // '=' indicates padding, do not shift in any bits
        // we still do the loop, but no accumulation
        if (uint8_t v = kBase64DecTable[c]; v != 0xFF) {
          accum = (accum << 6) | v;
          valid_chars++;
        } else {
          MLC_THROW(ValueError) << "Base64Decode: Invalid character in input.";
          MLC_UNREACHABLE();
        }
      }
    }
    int total_bits = valid_chars * 6;
    accum <<= (24 - total_bits);
    int total_bytes = total_bits / 8;
    for (int b = 0; b < total_bytes; ++b) {
      uint8_t byte_val = static_cast<uint8_t>((accum >> (16 - 8 * b)) & 0xFF);
      out[result_len++] = byte_val;
    }
  }
  out[result_len] = '\0';
  return ret;
}

/****************** Structural Equal ******************/

struct SEqualError : public std::runtime_error {
  ObjectPath path;
  SEqualError(const char *msg, ObjectPath path) : std::runtime_error(msg), path(path) {}
};

template <typename T> MLC_INLINE T *WithOffset(Object *obj, MLCTypeField *field) {
  return reinterpret_cast<T *>(reinterpret_cast<char *>(obj) + field->offset);
}

#define MLC_CORE_EQ_S_ERR(LHS, RHS, PATH)                                                                              \
  {                                                                                                                    \
    std::ostringstream err;                                                                                            \
    err << (LHS) << " vs " << (RHS);                                                                                   \
    throw SEqualError(err.str().c_str(), (PATH));                                                                      \
  }
#define MLC_CORE_EQ_S_ANY(Cond, Type, EQ, LHS, RHS, PATH)                                                              \
  if (Cond) {                                                                                                          \
    Type lhs_value = LHS->operator Type();                                                                             \
    Type rhs_value = RHS->operator Type();                                                                             \
    if (EQ(lhs_value, rhs_value)) {                                                                                    \
      return;                                                                                                          \
    } else {                                                                                                           \
      MLC_CORE_EQ_S_ERR(*lhs, *rhs, PATH);                                                                             \
    }                                                                                                                  \
  }
#define MLC_CORE_EQ_S_OPT(Type, EQ)                                                                                    \
  MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind, Optional<Type> *_lhs) {                          \
    const Type *lhs = _lhs->get();                                                                                     \
    const Type *rhs = WithOffset<Optional<Type>>(obj_rhs, field)->get();                                               \
    if ((lhs != nullptr || rhs != nullptr) && (lhs == nullptr || rhs == nullptr || !EQ(*lhs, *rhs))) {                 \
      AnyView LHS = lhs ? AnyView(*lhs) : AnyView(nullptr);                                                            \
      AnyView RHS = rhs ? AnyView(*rhs) : AnyView(nullptr);                                                            \
      MLC_CORE_EQ_S_ERR(LHS, RHS, path->WithField(field->name));                                                       \
    }                                                                                                                  \
  }
#define MLC_CORE_EQ_S_POD(Type, EQ)                                                                                    \
  MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind, Type *lhs) {                                     \
    const Type *rhs = WithOffset<Type>(obj_rhs, field);                                                                \
    if (!EQ(*lhs, *rhs)) {                                                                                             \
      MLC_CORE_EQ_S_ERR(AnyView(*lhs), AnyView(*rhs), path->WithField(field->name));                                   \
    }                                                                                                                  \
  }

inline void StructuralEqualImpl(Object *lhs, Object *rhs, bool bind_free_vars) {
  using CharArray = const char *;
  using VoidPtr = ::mlc::base::VoidPtr;
  using mlc::base::DeviceEqual;
  using mlc::base::DType;
  struct Task {
    Object *lhs;
    Object *rhs;
    MLCTypeInfo *type_info;
    bool visited;
    bool bind_free_vars; // `map_free_vars` in TVM
    ObjectPath path;
    std::unique_ptr<std::ostringstream> err;
  };
  struct Visitor {
    static bool CharArrayEqual(CharArray lhs, CharArray rhs) { return std::strcmp(lhs, rhs) == 0; }
    static bool FloatEqual(float lhs, float rhs) { return std::abs(lhs - rhs) < 1e-6; }
    static bool DoubleEqual(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-8; }
    MLC_CORE_EQ_S_OPT(bool, std::equal_to<bool>());
    MLC_CORE_EQ_S_OPT(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_OPT(double, DoubleEqual);
    MLC_CORE_EQ_S_OPT(DLDevice, DeviceEqual);
    MLC_CORE_EQ_S_OPT(DLDataType, DType::Equal);
    MLC_CORE_EQ_S_OPT(VoidPtr, std::equal_to<const void *>());
    MLC_CORE_EQ_S_POD(bool, std::equal_to<bool>());
    MLC_CORE_EQ_S_POD(int8_t, std::equal_to<int8_t>());
    MLC_CORE_EQ_S_POD(int16_t, std::equal_to<int16_t>());
    MLC_CORE_EQ_S_POD(int32_t, std::equal_to<int32_t>());
    MLC_CORE_EQ_S_POD(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_POD(float, FloatEqual);
    MLC_CORE_EQ_S_POD(double, DoubleEqual);
    MLC_CORE_EQ_S_POD(DLDataType, DType::Equal);
    MLC_CORE_EQ_S_POD(DLDevice, DeviceEqual);
    MLC_CORE_EQ_S_POD(VoidPtr, std::equal_to<const void *>());
    MLC_CORE_EQ_S_POD(CharArray, CharArrayEqual);
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, const Any *lhs) {
      const Any *rhs = WithOffset<Any>(obj_rhs, field);
      bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
      EnqueueAny(tasks, bind_free_vars, lhs, rhs, path->WithField(field->name));
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, ObjectRef *_lhs) {
      HandleObject(field, field_kind, _lhs->get(), WithOffset<ObjectRef>(obj_rhs, field)->get());
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, Optional<ObjectRef> *_lhs) {
      HandleObject(field, field_kind, _lhs->get(), WithOffset<Optional<ObjectRef>>(obj_rhs, field)->get());
    }
    inline void HandleObject(MLCTypeField *field, StructureFieldKind field_kind, Object *lhs, Object *rhs) {
      if (lhs || rhs) {
        bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
        EnqueueTask(tasks, bind_free_vars, lhs, rhs, path->WithField(field->name));
      }
    }
    static void CheckShapeEqual(const int64_t *lhs, const int64_t *rhs, int32_t ndim, const ObjectPath &path) {
      for (int32_t i = 0; i < ndim; ++i) {
        if (lhs[i] != rhs[i]) {
          UList lhs_list{lhs, lhs + ndim};
          UList rhs_list{rhs, rhs + ndim};
          MLC_CORE_EQ_S_ERR(lhs_list, rhs_list, path->WithField("shape"));
        }
      }
    }
    static void CheckStridesEqual(const int64_t *lhs, const int64_t *rhs, int32_t ndim, const ObjectPath &path) {
      if ((lhs == nullptr) != (rhs == nullptr)) {
        Any lhs_list = lhs ? Any(UList(lhs, lhs + ndim)) : Any();
        Any rhs_list = rhs ? Any(UList(rhs, rhs + ndim)) : Any();
        MLC_CORE_EQ_S_ERR(lhs_list, rhs_list, path->WithField("strides"));
      }
      for (int32_t i = 0; i < ndim; ++i) {
        if (lhs[i] != rhs[i]) {
          UList lhs_list{lhs, lhs + ndim};
          UList rhs_list{rhs, rhs + ndim};
          MLC_CORE_EQ_S_ERR(lhs_list, rhs_list, path->WithField("strides"));
        }
      }
    }
    static void EnqueueAny(std::vector<Task> *tasks, bool bind_free_vars, const Any *lhs, const Any *rhs,
                           ObjectPath new_path) {
      int32_t type_index = lhs->GetTypeIndex();
      if (type_index != rhs->GetTypeIndex()) {
        MLC_CORE_EQ_S_ERR(lhs->GetTypeKey(), rhs->GetTypeKey(), new_path);
      }
      if (type_index == kMLCNone) {
        return;
      }
      MLC_CORE_EQ_S_ANY(type_index == kMLCBool, bool, std::equal_to<bool>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCInt, int64_t, std::equal_to<int64_t>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCFloat, double, DoubleEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCPtr, VoidPtr, std::equal_to<const void *>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCDataType, DLDataType, DType::Equal, lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCDevice, DLDevice, DeviceEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCRawStr, CharArray, CharArrayEqual, lhs, rhs, new_path);
      if (type_index < kMLCStaticObjectBegin) {
        MLC_THROW(InternalError) << "Unknown type key: " << lhs->GetTypeKey();
      }
      EnqueueTask(tasks, bind_free_vars, lhs->operator Object *(), rhs->operator Object *(), new_path);
    }
    static void EnqueueTask(std::vector<Task> *tasks, bool bind_free_vars, Object *lhs, Object *rhs,
                            ObjectPath new_path) {
      int32_t lhs_type_index = lhs ? lhs->GetTypeIndex() : kMLCNone;
      int32_t rhs_type_index = rhs ? rhs->GetTypeIndex() : kMLCNone;
      if (lhs_type_index != rhs_type_index) {
        MLC_CORE_EQ_S_ERR(Lib::GetTypeKey(lhs_type_index), Lib::GetTypeKey(rhs_type_index), new_path);
      } else if (lhs_type_index == kMLCStr) {
        Str lhs_str(reinterpret_cast<StrObj *>(lhs));
        Str rhs_str(reinterpret_cast<StrObj *>(rhs));
        if (lhs_str != rhs_str) {
          MLC_CORE_EQ_S_ERR(lhs_str, rhs_str, new_path);
        }
      } else if (lhs_type_index == kMLCTensor) {
        DLTensor *lhs_tensor = &lhs->DynCast<TensorObj>()->tensor;
        DLTensor *rhs_tensor = &rhs->DynCast<TensorObj>()->tensor;
        int32_t ndim = lhs_tensor->ndim;
        if (ndim != rhs_tensor->ndim) {
          MLC_CORE_EQ_S_ERR(lhs_tensor->ndim, rhs_tensor->ndim, new_path->WithField("ndim"));
        }
        if (lhs_tensor->byte_offset != rhs_tensor->byte_offset) {
          MLC_CORE_EQ_S_ERR(lhs_tensor->byte_offset, rhs_tensor->byte_offset, new_path->WithField("byte_offset"));
        }
        if (!::mlc::base::DType::Equal(lhs_tensor->dtype, rhs_tensor->dtype)) {
          MLC_CORE_EQ_S_ERR(AnyView(lhs_tensor->dtype), AnyView(rhs_tensor->dtype), new_path->WithField("dtype"));
        }
        if (!::mlc::base::DeviceEqual(lhs_tensor->device, rhs_tensor->device)) {
          MLC_CORE_EQ_S_ERR(AnyView(lhs_tensor->device), AnyView(rhs_tensor->device), new_path->WithField("device"));
        }
        CheckShapeEqual(lhs_tensor->shape, rhs_tensor->shape, ndim, new_path);
        CheckStridesEqual(lhs_tensor->strides, rhs_tensor->strides, ndim, new_path);
      } else if (lhs_type_index == kMLCFunc || lhs_type_index == kMLCError) {
        throw SEqualError("Cannot compare `mlc.Func` or `mlc.Error`", new_path);
      } else if (lhs_type_index == kMLCOpaque) {
        std::ostringstream err;
        err << "Cannot compare `mlc.Opaque` of type: " << lhs->DynCast<OpaqueObj>()->opaque_type_name;
        throw SEqualError(err.str().c_str(), new_path);
      } else {
        bool visited = false;
        MLCTypeInfo *type_info = Lib::GetTypeInfo(lhs_type_index);
        tasks->push_back(Task{lhs, rhs, type_info, visited, bind_free_vars, new_path, nullptr});
      }
    }
    Object *obj_rhs;
    std::vector<Task> *tasks;
    bool obj_bind_free_vars;
    ObjectPath path;
  };
  std::vector<Task> tasks;
  std::unordered_map<Object *, Object *> eq_lhs_to_rhs;
  std::unordered_map<Object *, Object *> eq_rhs_to_lhs;

  auto check_bind = [&eq_lhs_to_rhs, &eq_rhs_to_lhs](Object *lhs, Object *rhs, const ObjectPath &path) -> bool {
    // check binding consistency: lhs -> rhs, rhs -> lhs
    auto it_lhs_to_rhs = eq_lhs_to_rhs.find(lhs);
    auto it_rhs_to_lhs = eq_rhs_to_lhs.find(rhs);
    bool exist_lhs_to_rhs = it_lhs_to_rhs != eq_lhs_to_rhs.end();
    bool exist_rhs_to_lhs = it_rhs_to_lhs != eq_rhs_to_lhs.end();
    // already proven equal
    if (exist_lhs_to_rhs && exist_rhs_to_lhs) {
      if (it_lhs_to_rhs->second == rhs && it_rhs_to_lhs->second == lhs) {
        return true;
      }
      throw SEqualError("Inconsistent binding: LHS and RHS are both bound, but to different nodes", path);
    }
    // inconsistent binding
    if (exist_lhs_to_rhs) {
      throw SEqualError("Inconsistent binding. LHS has been bound to a different node while RHS is not bound", path);
    }
    if (exist_rhs_to_lhs) {
      throw SEqualError("Inconsistent binding. RHS has been bound to a different node while LHS is not bound", path);
    }
    return false;
  };

  Visitor::EnqueueTask(&tasks, bind_free_vars, lhs, rhs, ObjectPath::Root());
  while (!tasks.empty()) {
    MLCTypeInfo *type_info;
    ObjectPath path{::mlc::Null};
    {
      Task &task = tasks.back();
      type_info = task.type_info;
      path = task.path;
      lhs = task.lhs;
      rhs = task.rhs;
      bind_free_vars = task.bind_free_vars;
      if (task.err) {
        throw SEqualError(task.err->str().c_str(), path);
      } else if (check_bind(lhs, rhs, path)) {
        tasks.pop_back();
        continue;
      } else if (task.visited) {
        StructureKind kind = static_cast<StructureKind>(type_info->structure_kind);
        if (kind == StructureKind::kBind || (kind == StructureKind::kVar && bind_free_vars)) {
          // bind lhs <-> rhs
          eq_lhs_to_rhs[lhs] = rhs;
          eq_rhs_to_lhs[rhs] = lhs;
        } else if (kind == StructureKind::kVar && !bind_free_vars) {
          throw SEqualError("Unbound variable", path);
        }
        tasks.pop_back();
        continue;
      }
      task.visited = true;
    }
    // `task.visited` was `False`
    int64_t task_index = static_cast<int64_t>(tasks.size()) - 1;
    if (type_info->type_index == kMLCList) {
      UListObj *lhs_list = reinterpret_cast<UListObj *>(lhs);
      UListObj *rhs_list = reinterpret_cast<UListObj *>(rhs);
      int64_t lhs_size = lhs_list->size();
      int64_t rhs_size = rhs_list->size();
      for (int64_t i = (lhs_size < rhs_size ? lhs_size : rhs_size) - 1; i >= 0; --i) {
        Visitor::EnqueueAny(&tasks, bind_free_vars, &lhs_list->at(i), &rhs_list->at(i), path->WithListIndex(i));
      }
      if (lhs_size != rhs_size) {
        auto &err = tasks[task_index].err = std::make_unique<std::ostringstream>();
        (*err) << "List length mismatch: " << lhs_size << " vs " << rhs_size;
      }
    } else if (type_info->type_index == kMLCDict) {
      UDictObj *lhs_dict = reinterpret_cast<UDictObj *>(lhs);
      UDictObj *rhs_dict = reinterpret_cast<UDictObj *>(rhs);
      std::vector<AnyView> not_found_lhs_keys;
      for (auto &kv : *lhs_dict) {
        AnyView lhs_key = kv.first;
        int32_t type_index = lhs_key.type_index;
        UDictObj::iterator rhs_it;
        if (type_index < kMLCStaticObjectBegin || type_index == kMLCStr) {
          rhs_it = rhs_dict->find(lhs_key);
        } else if (auto it = eq_lhs_to_rhs.find(lhs_key.operator Object *()); it != eq_lhs_to_rhs.end()) {
          rhs_it = rhs_dict->find(it->second);
        } else {
          not_found_lhs_keys.push_back(lhs_key);
          continue;
        }
        if (rhs_it == rhs_dict->end()) {
          not_found_lhs_keys.push_back(lhs_key);
          continue;
        }
        Visitor::EnqueueAny(&tasks, bind_free_vars, &kv.second, &rhs_it->second, path->WithDictKey(lhs_key));
      }
      auto &err = tasks[task_index].err;
      if (!not_found_lhs_keys.empty()) {
        err = std::make_unique<std::ostringstream>();
        (*err) << "Dict key(s) not found in rhs: " << not_found_lhs_keys[0];
        for (size_t i = 1; i < not_found_lhs_keys.size(); ++i) {
          (*err) << ", " << not_found_lhs_keys[i];
        }
      } else if (lhs_dict->size() != rhs_dict->size()) {
        err = std::make_unique<std::ostringstream>();
        (*err) << "Dict size mismatch: " << lhs_dict->size() << " vs " << rhs_dict->size();
      }
    } else {
      VisitStructure(lhs, type_info, Visitor{rhs, &tasks, bind_free_vars, path});
    }
  }
}

/****************** Structural Hash ******************/

#define MLC_CORE_HASH_S_OPT(Type, Hasher)                                                                              \
  MLC_INLINE void operator()(MLCTypeField *, StructureFieldKind, Optional<Type> *_v) {                                 \
    if (const Type *v = _v->get()) {                                                                                   \
      EnqueuePOD(tasks, Hasher(*v));                                                                                   \
    } else {                                                                                                           \
      EnqueuePOD(tasks, HashCache::kNoneCombined);                                                                     \
    }                                                                                                                  \
  }
#define MLC_CORE_HASH_S_POD(Type, Hasher)                                                                              \
  MLC_INLINE void operator()(MLCTypeField *, StructureFieldKind, Type *v) { EnqueuePOD(tasks, Hasher(*v)); }
#define MLC_CORE_HASH_S_ANY(Cond, Type, Hasher)                                                                        \
  if (Cond) {                                                                                                          \
    EnqueuePOD(tasks, Hasher(v->operator Type()));                                                                     \
    return;                                                                                                            \
  }

struct HashCache {
  inline static const uint64_t MLC_SYMBOL_HIDE kNoneCombined =
      ::mlc::base::HashCombine(Lib::GetTypeInfo(kMLCNone)->type_key_hash, 0);
  inline static const uint64_t MLC_SYMBOL_HIDE kBool = Lib::GetTypeInfo(kMLCBool)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kInt = Lib::GetTypeInfo(kMLCInt)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kFloat = Lib::GetTypeInfo(kMLCFloat)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kPtr = Lib::GetTypeInfo(kMLCPtr)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kDType = Lib::GetTypeInfo(kMLCDataType)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kDevice = Lib::GetTypeInfo(kMLCDevice)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kRawStr = Lib::GetTypeInfo(kMLCRawStr)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kStrObj = Lib::GetTypeInfo(kMLCStr)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kTensorObj = Lib::GetTypeInfo(kMLCTensor)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kBound = ::mlc::base::StrHash("$$Bounds$$");
  inline static const uint64_t MLC_SYMBOL_HIDE kUnbound = ::mlc::base::StrHash("$$Unbound$$");
};

template <typename T> MLC_INLINE uint64_t HashTyped(uint64_t type_hash, T value) {
  union {
    T src;
    uint64_t tgt;
  } u;
  u.tgt = 0;
  u.src = value;
  return ::mlc::base::HashCombine(type_hash, u.tgt);
}

inline uint64_t StructuralHashImpl(Object *obj) {
  using CharArray = const char *;
  using VoidPtr = ::mlc::base::VoidPtr;
  using mlc::base::HashCombine;
  struct Task {
    Object *obj;
    MLCTypeInfo *type_info;
    bool visited;
    bool bind_free_vars;
    uint64_t hash_value;
    size_t index_in_result_hashes{0xffffffffffffffff};
  };
  struct Visitor {
    static uint64_t HashBool(bool a) { return HashTyped<int64_t>(HashCache::kBool, static_cast<int64_t>(a)); }
    static uint64_t HashInteger(int64_t a) { return HashTyped<int64_t>(HashCache::kInt, a); }
    static uint64_t HashPtr(VoidPtr a) { return HashTyped<VoidPtr>(HashCache::kPtr, a); }
    static uint64_t HashDevice(DLDevice a) { return HashTyped<DLDevice>(HashCache::kDevice, a); }
    static uint64_t HashDataType(DLDataType a) { return HashTyped<DLDataType>(HashCache::kDType, a); }
    // clang-format off
    static uint64_t HashFloat(float a) { return HashTyped<float>(HashCache::kFloat, std::isnan(a) ? std::numeric_limits<float>::quiet_NaN() : a); }
    static uint64_t HashDouble(double a) { return HashTyped<double>(HashCache::kFloat, std::isnan(a) ? std::numeric_limits<double>::quiet_NaN() : a); }
    static uint64_t HashCharArray(CharArray a) { return HashTyped<uint64_t>(HashCache::kRawStr, ::mlc::base::StrHash(a)); }
    // clang-format on
    MLC_CORE_HASH_S_OPT(bool, HashBool);
    MLC_CORE_HASH_S_OPT(int64_t, HashInteger);
    MLC_CORE_HASH_S_OPT(double, HashDouble);
    MLC_CORE_HASH_S_OPT(DLDevice, HashDevice);
    MLC_CORE_HASH_S_OPT(DLDataType, HashDataType);
    MLC_CORE_HASH_S_OPT(VoidPtr, HashPtr);
    MLC_CORE_HASH_S_POD(bool, HashBool);
    MLC_CORE_HASH_S_POD(int8_t, HashInteger);
    MLC_CORE_HASH_S_POD(int16_t, HashInteger);
    MLC_CORE_HASH_S_POD(int32_t, HashInteger);
    MLC_CORE_HASH_S_POD(int64_t, HashInteger);
    MLC_CORE_HASH_S_POD(float, HashFloat);
    MLC_CORE_HASH_S_POD(double, HashDouble);
    MLC_CORE_HASH_S_POD(DLDataType, HashDataType);
    MLC_CORE_HASH_S_POD(DLDevice, HashDevice);
    MLC_CORE_HASH_S_POD(VoidPtr, HashPtr);
    MLC_CORE_HASH_S_POD(CharArray, HashCharArray);
    MLC_INLINE void operator()(MLCTypeField *, StructureFieldKind field_kind, const Any *v) {
      bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
      EnqueueAny(tasks, bind_free_vars, v);
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, ObjectRef *_v) {
      HandleObject(field, field_kind, _v->get());
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, Optional<ObjectRef> *_v) {
      HandleObject(field, field_kind, _v->get());
    }
    inline void HandleObject(MLCTypeField *, StructureFieldKind field_kind, Object *v) {
      bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
      EnqueueTask(tasks, bind_free_vars, v);
    }
    static void EnqueuePOD(std::vector<Task> *tasks, uint64_t hash_value) {
      tasks->emplace_back(Task{nullptr, nullptr, false, false, hash_value});
    }
    static void EnqueueAny(std::vector<Task> *tasks, bool bind_free_vars, const Any *v) {
      int32_t type_index = v->GetTypeIndex();
      MLC_CORE_HASH_S_ANY(type_index == kMLCBool, bool, HashBool);
      MLC_CORE_HASH_S_ANY(type_index == kMLCInt, int64_t, HashInteger);
      MLC_CORE_HASH_S_ANY(type_index == kMLCFloat, double, HashDouble);
      MLC_CORE_HASH_S_ANY(type_index == kMLCPtr, VoidPtr, HashPtr);
      MLC_CORE_HASH_S_ANY(type_index == kMLCDataType, DLDataType, HashDataType);
      MLC_CORE_HASH_S_ANY(type_index == kMLCDevice, DLDevice, HashDevice);
      MLC_CORE_HASH_S_ANY(type_index == kMLCRawStr, CharArray, HashCharArray);
      EnqueueTask(tasks, bind_free_vars, v->operator Object *());
    }
    static void EnqueueTask(std::vector<Task> *tasks, bool bind_free_vars, Object *obj) {
      int32_t type_index = obj ? obj->GetTypeIndex() : kMLCNone;
      if (type_index == kMLCNone) {
        EnqueuePOD(tasks, HashCache::kNoneCombined);
      } else if (type_index == kMLCStr) {
        const MLCStr *str = reinterpret_cast<const MLCStr *>(obj);
        uint64_t hash_value = ::mlc::base::StrHash(str->data, str->length);
        hash_value = HashTyped(HashCache::kStrObj, hash_value);
        EnqueuePOD(tasks, hash_value);
      } else if (type_index == kMLCTensor) {
        const DLTensor *tensor = &reinterpret_cast<const MLCTensor *>(obj)->tensor;
        uint64_t hash_value = HashInteger(tensor->ndim);
        hash_value = HashCombine(hash_value, HashInteger(tensor->byte_offset));
        hash_value = HashCombine(hash_value, HashDataType(tensor->dtype));
        hash_value = HashCombine(hash_value, HashDevice(tensor->device));
        for (int32_t i = 0; i < tensor->ndim; ++i) {
          hash_value = HashCombine(hash_value, HashInteger(tensor->shape[i]));
        }
        if (tensor->strides) {
          for (int32_t i = 0; i < tensor->ndim; ++i) {
            hash_value = HashCombine(hash_value, HashInteger(tensor->strides[i]));
          }
        }
        hash_value = HashTyped(HashCache::kTensorObj, hash_value);
        EnqueuePOD(tasks, hash_value);
      } else if (type_index == kMLCFunc || type_index == kMLCError) {
        throw SEqualError("Cannot compare `mlc.Func` or `mlc.Error`", ObjectPath::Root());
      } else if (type_index == kMLCOpaque) {
        std::ostringstream err;
        err << "Cannot compare `mlc.Opaque` of type: " << obj->DynCast<OpaqueObj>()->opaque_type_name;
        throw SEqualError(err.str().c_str(), ObjectPath::Root());
      } else {
        MLCTypeInfo *type_info = Lib::GetTypeInfo(type_index);
        tasks->emplace_back(Task{obj, type_info, false, bind_free_vars, type_info->type_key_hash});
      }
    }

    std::vector<Task> *tasks;
    bool obj_bind_free_vars;
  };
  std::vector<Task> tasks;
  std::vector<uint64_t> result_hashes;
  std::unordered_map<Object *, uint64_t> obj2hash;
  int64_t num_bound_nodes = 0;
  int64_t num_unbound_vars = 0;
  Visitor::EnqueueTask(&tasks, false, obj);
  while (!tasks.empty()) {
    MLCTypeInfo *type_info;
    bool bind_free_vars;
    uint64_t hash_value;
    {
      Task &task = tasks.back();
      hash_value = task.hash_value;
      obj = task.obj;
      type_info = task.type_info;
      bind_free_vars = task.bind_free_vars;
      if (task.visited) {
        if (result_hashes.size() < task.index_in_result_hashes) {
          MLC_THROW(InternalError)
              << "Internal invariant violated: `result_hashes.size() < task.index_in_result_hashes` ("
              << result_hashes.size() << " vs " << task.index_in_result_hashes << ")";
        }
        for (; result_hashes.size() > task.index_in_result_hashes; result_hashes.pop_back()) {
          hash_value = HashCombine(hash_value, result_hashes.back());
        }
        StructureKind kind = static_cast<StructureKind>(type_info->structure_kind);
        if (kind == StructureKind::kBind || (kind == StructureKind::kVar && bind_free_vars)) {
          hash_value = HashCombine(hash_value, HashCache::kBound);
          hash_value = HashCombine(hash_value, num_bound_nodes++);
        } else if (kind == StructureKind::kVar && !bind_free_vars) {
          hash_value = HashCombine(hash_value, HashCache::kUnbound);
          hash_value = HashCombine(hash_value, num_unbound_vars++);
        }
        obj2hash[obj] = hash_value;
        result_hashes.push_back(hash_value);
        tasks.pop_back();
        continue;
      } else if (auto it = obj2hash.find(obj); it != obj2hash.end()) {
        result_hashes.push_back(it->second);
        tasks.pop_back();
        continue;
      } else if (obj == nullptr) {
        result_hashes.push_back(hash_value);
        tasks.pop_back();
        continue;
      }
      task.visited = true;
      task.index_in_result_hashes = result_hashes.size();
    }
    // `task.visited` was `False`
    if (type_info->type_index == kMLCList) {
      UListObj *list = reinterpret_cast<UListObj *>(obj);
      hash_value = HashCombine(hash_value, list->size());
      for (int64_t i = list->size() - 1; i >= 0; --i) {
        Visitor::EnqueueAny(&tasks, bind_free_vars, &list->at(i));
      }
    } else if (type_info->type_index == kMLCDict) {
      UDictObj *dict = reinterpret_cast<UDictObj *>(obj);
      hash_value = HashCombine(hash_value, dict->size());
      struct KVPair {
        uint64_t hash;
        AnyView key;
        AnyView value;
      };
      std::vector<KVPair> kv_pairs;
      for (auto &[k, v] : *dict) {
        uint64_t hash = 0;
        if (k.type_index == kMLCNone) {
          hash = HashCache::kNoneCombined;
        } else if (k.type_index == kMLCBool) {
          hash = Visitor::HashInteger(k.v.v_bool);
        } else if (k.type_index == kMLCInt) {
          hash = Visitor::HashInteger(k.v.v_int64);
        } else if (k.type_index == kMLCFloat) {
          hash = Visitor::HashDouble(k.v.v_float64);
        } else if (k.type_index == kMLCPtr) {
          hash = Visitor::HashPtr(k.v.v_ptr);
        } else if (k.type_index == kMLCDataType) {
          hash = Visitor::HashDataType(k.v.v_dtype);
        } else if (k.type_index == kMLCDevice) {
          hash = Visitor::HashDevice(k.v.v_device);
        } else if (k.type_index == kMLCStr) {
          const StrObj *str = k;
          hash = ::mlc::base::StrHash(str->data(), str->length());
          hash = HashTyped(HashCache::kStrObj, hash);
        } else if (k.type_index >= kMLCStaticObjectBegin) {
          obj = k;
          if (auto it = obj2hash.find(obj); it != obj2hash.end()) {
            hash = it->second;
          } else {
            continue; // Skip unbound nodes
          }
        }
        kv_pairs.push_back(KVPair{hash, k, v});
      }
      std::sort(kv_pairs.begin(), kv_pairs.end(), [](const KVPair &a, const KVPair &b) { return a.hash < b.hash; });
      for (size_t i = 0; i < kv_pairs.size();) {
        // [i, j) are of the same hash
        size_t j = i + 1;
        for (; j < kv_pairs.size() && kv_pairs[i].hash == kv_pairs[j].hash; ++j) {
        }
        // skip cases where multiple keys have the same hash
        if (i + 1 == j) {
          Any k = kv_pairs[i].key;
          Any v = kv_pairs[i].value;
          Visitor::EnqueueAny(&tasks, bind_free_vars, &k);
          Visitor::EnqueueAny(&tasks, bind_free_vars, &v);
        }
      }
    } else {
      VisitStructure(obj, type_info, Visitor{&tasks, bind_free_vars});
    }
  }
  if (result_hashes.size() != 1) {
    MLC_THROW(InternalError) << "Internal invariant violated: `result_hashes.size() != 1` (" << result_hashes.size()
                             << ")";
  }
  return result_hashes[0];
}

#undef MLC_CORE_EQ_S_OPT
#undef MLC_CORE_EQ_S_POD
#undef MLC_CORE_EQ_S_ANY
#undef MLC_CORE_EQ_S_ERR
#undef MLC_CORE_HASH_S_OPT
#undef MLC_CORE_HASH_S_POD
#undef MLC_CORE_HASH_S_ANY

/****************** Copy ******************/

inline Any CopyShallowImpl(AnyView source) {
  int32_t type_index = source.type_index;
  if (::mlc::base::IsTypeIndexPOD(type_index)) {
    return source;
  } else if (UListObj *list = source.as<UListObj>()) {
    return UList(list->begin(), list->end());
  } else if (UDictObj *dict = source.as<UDictObj>()) {
    return UDict(dict->begin(), dict->end());
  } else if (source.IsInstance<StrObj>() || source.IsInstance<ErrorObj>() || source.IsInstance<FuncObj>() ||
             source.IsInstance<TensorObj>()) {
    // TODO: do we want to shallow copy these types at all?
    return source;
  }
  struct Copier {
    MLC_INLINE void operator()(MLCTypeField *, const Any *any) { fields->push_back(AnyView(*any)); }
    MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { fields->push_back(AnyView(*obj)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<bool> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, bool *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int8_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int16_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int32_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int64_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, float *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, double *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, DLDataType *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, DLDevice *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, void **v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, const char **v) { fields->push_back(AnyView(*v)); }
    std::vector<AnyView> *fields;
  };
  FuncObj *init_func = Lib::_init(type_index);
  MLCTypeInfo *type_info = Lib::GetTypeInfo(type_index);
  std::vector<AnyView> fields;
  VisitFields(source.operator Object *(), type_info, Copier{&fields});
  Any ret;
  ::mlc::base::FuncCall(init_func, static_cast<int32_t>(fields.size()), fields.data(), &ret);
  return ret;
}

inline void CopyReplaceImpl(int32_t num_args, const AnyView *args, Any *ret) {
  if (num_args <= 0) {
    MLC_THROW(InternalError) << "InternalError: `CopyReplace` requires at least one argument";
  }
  AnyView source = args[0];
  int32_t type_index = source.type_index;
  if (::mlc::base::IsTypeIndexPOD(type_index)) {
    MLC_THROW(TypeError) << "TypeError: `__replace__` doesn't work on a POD type: " << source;
  } else if (source.IsInstance<StrObj>() || source.IsInstance<ErrorObj>() || source.IsInstance<FuncObj>() ||
             source.IsInstance<UListObj>() || source.IsInstance<UDictObj>() || source.IsInstance<TensorObj>()) {
    MLC_THROW(TypeError) << "TypeError: `__replace__` doesn't work on type: " << source.GetTypeKey();
  }
  struct Copier {
    MLC_INLINE void operator()(MLCTypeField *f, const Any *any) { AddField(f->name, AnyView(*any)); }
    MLC_INLINE void operator()(MLCTypeField *f, ObjectRef *obj) { AddField(f->name, AnyView(*obj)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<ObjectRef> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<bool> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<int64_t> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<double> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<DLDevice> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<DLDataType> *opt) { AddField(f->name, AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *f, bool *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, int8_t *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, int16_t *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, int32_t *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, int64_t *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, float *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, double *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, DLDataType *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, DLDevice *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, Optional<void *> *v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, void **v) { AddField(f->name, AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *f, const char **v) { AddField(f->name, AnyView(*v)); }

    void AddField(std::string_view name, AnyView v) {
      if (auto it = replacements->find(name); it != replacements->end()) {
        fields->push_back(it->second);
      } else {
        fields->push_back(v);
      }
    }
    std::vector<AnyView> *fields;
    std::unordered_map<std::string_view, AnyView> *replacements;
  };
  std::unordered_map<std::string_view, AnyView> replacements;
  for (int32_t i = 1; i < num_args; i += 2) {
    const char *name = args[i];
    replacements[name] = args[i + 1];
  }
  FuncObj *init_func = Lib::_init(type_index);
  MLCTypeInfo *type_info = Lib::GetTypeInfo(type_index);
  std::vector<AnyView> fields;
  VisitFields(source.operator Object *(), type_info, Copier{&fields, &replacements});
  ::mlc::base::FuncCall(init_func, static_cast<int32_t>(fields.size()), fields.data(), ret);
}

inline Any CopyDeepImpl(AnyView source) {
  if (::mlc::base::IsTypeIndexPOD(source.type_index)) {
    return source;
  }
  struct Copier {
    MLC_INLINE void operator()(MLCTypeField *, const Any *any) { HandleAny(any); }
    MLC_INLINE void operator()(MLCTypeField *, ObjectRef *ref) {
      if (const Object *obj = ref->get()) {
        HandleObject(obj);
      } else {
        fields->push_back(AnyView());
      }
    }
    MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) {
      if (const Object *obj = opt->get()) {
        HandleObject(obj);
      } else {
        fields->push_back(AnyView());
      }
    }
    MLC_INLINE void operator()(MLCTypeField *, Optional<bool> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, bool *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int8_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int16_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int32_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int64_t *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, float *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, double *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, DLDataType *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, DLDevice *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, void **v) { fields->push_back(AnyView(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, const char **v) { fields->push_back(AnyView(*v)); }

    void HandleObject(const Object *obj) {
      if (auto it = orig2copy->find(obj); it != orig2copy->end()) {
        fields->push_back(AnyView(it->second));
      } else {
        MLC_THROW(InternalError) << "InternalError: object doesn't exist in the memo: " << AnyView(obj);
      }
    }

    void HandleAny(const Any *any) {
      if (const Object *obj = any->as<Object>()) {
        HandleObject(obj);
      } else {
        fields->push_back(AnyView(*any));
      }
    }

    std::unordered_map<const Object *, ObjectRef> *orig2copy;
    std::vector<AnyView> *fields;
  };
  std::unordered_map<const Object *, ObjectRef> orig2copy;
  std::vector<AnyView> fields;
  TopoVisit(source.operator Object *(), nullptr, [&](Object *object, MLCTypeInfo *type_info) mutable -> void {
    Any ret;
    if (UListObj *list = object->as<UListObj>()) {
      fields.clear();
      fields.reserve(list->size());
      for (Any &e : *list) {
        Copier{&orig2copy, &fields}.HandleAny(&e);
      }
      UList::FromAnyTuple(static_cast<int32_t>(fields.size()), fields.data(), &ret);
    } else if (UDictObj *dict = object->as<UDictObj>()) {
      fields.clear();
      for (auto [key, value] : *dict) {
        Copier{&orig2copy, &fields}.HandleAny(&key);
        Copier{&orig2copy, &fields}.HandleAny(&value);
      }
      UDict::FromAnyTuple(static_cast<int32_t>(fields.size()), fields.data(), &ret);
    } else if (object->IsInstance<StrObj>() || object->IsInstance<ErrorObj>() || object->IsInstance<FuncObj>() ||
               object->IsInstance<TensorObj>()) {
      ret = object;
    } else if (object->IsInstance<OpaqueObj>()) {
      MLC_THROW(TypeError) << "Cannot copy `mlc.Opaque` of type: " << object->DynCast<OpaqueObj>()->opaque_type_name;
    } else {
      fields.clear();
      VisitFields(object, type_info, Copier{&orig2copy, &fields});
      FuncObj *init_func = Lib::_init(type_info->type_index);
      ::mlc::base::FuncCall(init_func, static_cast<int32_t>(fields.size()), fields.data(), &ret);
    }
    orig2copy[object] = ret.operator ObjectRef();
  });
  return orig2copy.at(source.operator Object *());
}

/****************** Tensor <=> Bytes ******************/

template <int N, typename T> union BytesUnion {
  T v;
  uint8_t b[N];
};

#if MLC_IS_BIG_ENDIAN == 1
constexpr bool kIsBigEndian = true;
#else
constexpr bool kIsBigEndian = false;
#endif

template <int N, typename T> void WriteElem(uint8_t *data, int64_t *tail, T val) {
  static_assert(sizeof(BytesUnion<N, T>) == N);
  BytesUnion<N, T> v;
  v.v = val;
  if constexpr (N > 1 && kIsBigEndian) {
    std::reverse(v.b, v.b + N);
  }
  std::memcpy(data + *tail, v.b, N);
  *tail += N;
}

template <int N, typename T> T ReadElem(const uint8_t *data, int64_t *head, int64_t max_size) {
  static_assert(sizeof(BytesUnion<N, T>) == N);
  BytesUnion<N, T> v;
  if (*head + N > max_size) {
    MLC_THROW(ValueError) << "ReadElem: Unexpected EOF in buffer.";
  }
  std::memcpy(v.b, data + *head, N);
  if constexpr (N > 1 && kIsBigEndian) {
    std::reverse(v.b, v.b + N);
  }
  *head += N;
  return v.v;
}

void WriteElemMany(uint8_t *data, int64_t *tail, uint8_t *ptr, int32_t elem_size, int64_t numel) {
  uint8_t *start = data + *tail;
  std::memcpy(start, ptr, elem_size * numel);
  if constexpr (kIsBigEndian) {
    if (elem_size > 1) { // we need to swap each element
      for (int64_t i = 0; i < numel; ++i, start += elem_size) {
        std::reverse(start, start + elem_size);
      }
    }
  }
  *tail += static_cast<int64_t>(numel * elem_size);
}

void ReadElemMany(const uint8_t *data, int64_t *head, int64_t max_size, uint8_t *ptr, int32_t elem_size,
                  int64_t numel) {
  int64_t next_head = *head + numel * elem_size;
  if (next_head > max_size) {
    MLC_THROW(ValueError) << "ReadElemMany: Unexpected EOF in buffer.";
  }
  std::memcpy(ptr, data + *head, elem_size * numel);
  *head = next_head;
  if constexpr (kIsBigEndian) {
    if (elem_size > 1) { // we need to swap each element
      uint8_t *start = ptr;
      for (int64_t i = 0; i < numel; ++i, start += elem_size) {
        std::reverse(start, start + elem_size);
      }
    }
  }
}

static const uint64_t kMLCTensorMagic = 0xDD5E40F096B4A13F;

Str TensorToBytes(const DLTensor *src) {
  if (src->device.device_type != kDLCPU || src->strides != nullptr) {
    MLC_THROW(ValueError) << "SaveDLPack: Only CPU tensor without strides is supported.";
  }
  int32_t ndim = src->ndim;
  int64_t numel = ::mlc::core::ShapeToNumel(ndim, src->shape);
  int32_t elem_size = ::mlc::base::DType::Size(src->dtype);
  int64_t total_bytes = 8 + 4 + 4 + 8 * ndim + numel * elem_size;
  Str ret(::mlc::core::StrPad::Allocator::NewWithPad<uint8_t>(total_bytes + 1, total_bytes));
  uint8_t *data_ptr = reinterpret_cast<uint8_t *>(ret->data());
  int64_t tail = 0;
  WriteElem<8>(data_ptr, &tail, static_cast<uint64_t>(kMLCTensorMagic));
  WriteElem<4>(data_ptr, &tail, static_cast<uint32_t>(ndim));
  WriteElem<4>(data_ptr, &tail, src->dtype);
  for (int i = 0; i < ndim; ++i) {
    WriteElem<8>(data_ptr, &tail, src->shape[i]);
  }
  WriteElemMany(data_ptr, &tail, static_cast<uint8_t *>(src->data), elem_size, numel);
  data_ptr[tail] = '\0';
  if (tail != total_bytes) {
    MLC_THROW(InternalError) << "SaveDLPack: Internal error in serialization.";
  }
  return ret;
}

Tensor TensorFromBytes(const uint8_t *data_ptr, int64_t max_size) {
  int64_t head = 0;
  uint64_t header = ReadElem<8, uint64_t>(data_ptr, &head, max_size);
  if (header != kMLCTensorMagic) {
    MLC_THROW(ValueError) << "LoadDLPack: Magic number mismatch.";
  }
  int32_t ndim = ReadElem<4, int32_t>(data_ptr, &head, max_size);
  Tensor ret = [ndim]() {
    TensorObj *ret = ::mlc::DefaultObjectAllocator<TensorObj>::New();
    ret->tensor.data = nullptr;
    ret->tensor.device = DLDevice{kDLCPU, 0};
    ret->tensor.ndim = ndim;
    ret->tensor.dtype = DLDataType{kDLFloat, 32, 1};
    ret->tensor.shape = new int64_t[ndim + 1];
    ret->tensor.strides = nullptr;
    ret->tensor.byte_offset = 0;
    ret->manager_ctx = nullptr;
    ret->_mlc_header.v.deleter = +[](void *_self) {
      TensorObj *self = static_cast<TensorObj *>(_self);
      uint8_t *data = static_cast<uint8_t *>(self->tensor.data);
      delete[] data;
      ::mlc::DefaultObjectAllocator<TensorObj>::Deleter(self);
    };
    return Tensor(ret);
  }();
  DLTensor *tensor = &ret->tensor;
  tensor->dtype = ReadElem<4, DLDataType>(data_ptr, &head, max_size);
  for (int32_t i = 0; i < ndim; ++i) {
    tensor->shape[i] = ReadElem<8, int64_t>(data_ptr, &head, max_size);
  }
  tensor->shape[ndim] = -1;
  int32_t elem_size = ::mlc::base::DType::Size(tensor->dtype);
  int64_t numel = ::mlc::core::ShapeToNumel(ndim, tensor->shape);
  uint8_t *content = new uint8_t[numel * elem_size];
  ReadElemMany(data_ptr, &head, max_size, content, elem_size, numel);
  tensor->data = content;
  return ret;
}

/****************** Serialize / Deserialize ******************/

inline mlc::Str Serialize(Any any) {
  using mlc::base::TypeTraits;
  std::vector<const char *> type_keys;
  auto get_json_type_index = [type_key2index = std::unordered_map<const char *, int32_t>(),
                              &type_keys](const char *type_key) mutable -> int32_t {
    if (auto it = type_key2index.find(type_key); it != type_key2index.end()) {
      return it->second;
    }
    int32_t type_index = static_cast<int32_t>(type_key2index.size());
    type_key2index[type_key] = type_index;
    type_keys.push_back(type_key);
    return type_index;
  };
  using TObj2Idx = std::unordered_map<Object *, int32_t>;
  using TJsonTypeIndex = decltype(get_json_type_index);
  struct Emitter {
    MLC_INLINE void operator()(MLCTypeField *, const Any *any) { EmitAny(any); }
    // clang-format off
    MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { if (Object *v = obj->get()) EmitObject(v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) { if (Object *v = opt->get()) EmitObject(v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<bool> *opt) { if (const bool *v = opt->get()) EmitBool(*v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { if (const int64_t *v = opt->get()) EmitInt(*v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { if (const double *v = opt->get())  EmitFloat(*v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { if (const DLDevice *v = opt->get()) EmitDevice(*v); else EmitNil(); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { if (const DLDataType *v = opt->get()) EmitDType(*v); else EmitNil(); }
    // clang-format on
    MLC_INLINE void operator()(MLCTypeField *, bool *v) { EmitBool(*v); }
    MLC_INLINE void operator()(MLCTypeField *, int8_t *v) { EmitInt(static_cast<int64_t>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int16_t *v) { EmitInt(static_cast<int64_t>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int32_t *v) { EmitInt(static_cast<int64_t>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, int64_t *v) { EmitInt(static_cast<int64_t>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, float *v) { EmitFloat(static_cast<double>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, double *v) { EmitFloat(static_cast<double>(*v)); }
    MLC_INLINE void operator()(MLCTypeField *, DLDataType *v) { EmitDType(*v); }
    MLC_INLINE void operator()(MLCTypeField *, DLDevice *v) { EmitDevice(*v); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *) {
      MLC_THROW(TypeError) << "Unserializable type: void *";
    }
    MLC_INLINE void operator()(MLCTypeField *, void **) { MLC_THROW(TypeError) << "Unserializable type: void *"; }
    MLC_INLINE void operator()(MLCTypeField *, const char **) {
      MLC_THROW(TypeError) << "Unserializable type: const char *";
    }
    inline void EmitNil() { (*os) << ", null"; }
    inline void EmitBool(bool v) { (*os) << ", " << (v ? "true" : "false"); }
    inline void EmitFloat(double v) { (*os) << ", " << std::fixed << std::setprecision(19) << v; }
    inline void EmitInt(int64_t v) {
      int32_t type_int = (*get_json_type_index)(TypeTraits<int64_t>::type_str);
      (*os) << ", [" << type_int << ", " << v << "]";
    }
    inline void EmitDevice(DLDevice v) {
      int32_t type_device = (*get_json_type_index)(TypeTraits<DLDevice>::type_str);
      (*os) << ", [" << type_device << ", " << ::mlc::base::TypeTraits<DLDevice>::__str__(v) << "]";
    }
    inline void EmitDType(DLDataType v) {
      int32_t type_dtype = (*get_json_type_index)(TypeTraits<DLDataType>::type_str);
      (*os) << ", [" << type_dtype << ", " << ::mlc::base::DType::Str(v) << "]";
    }
    inline void EmitAny(const Any *any) {
      int32_t type_index = any->type_index;
      if (type_index == kMLCNone) {
        EmitNil();
      } else if (type_index == kMLCBool) {
        EmitBool(any->operator bool());
      } else if (type_index == kMLCInt) {
        EmitInt(any->operator int64_t());
      } else if (type_index == kMLCFloat) {
        EmitFloat(any->operator double());
      } else if (type_index == kMLCDevice) {
        EmitDevice(any->operator DLDevice());
      } else if (type_index == kMLCDataType) {
        EmitDType(any->operator DLDataType());
      } else if (type_index >= kMLCStaticObjectBegin) {
        EmitObject(any->operator Object *());
      } else {
        MLC_THROW(TypeError) << "Cannot serialize type: " << Lib::GetTypeKey(type_index);
      }
    }
    inline void EmitObject(Object *obj) {
      if (!obj) {
        MLC_THROW(InternalError) << "This should never happen: null object pointer during EmitObject";
      }
      int32_t obj_idx = obj2index->at(obj);
      if (obj_idx == -1) {
        MLC_THROW(InternalError) << "This should never happen: topological ordering violated";
      }
      (*os) << ", " << obj_idx;
    }
    std::ostringstream *os;
    TJsonTypeIndex *get_json_type_index;
    const TObj2Idx *obj2index;
  };

  std::unordered_map<Object *, int32_t> topo_indices;
  std::vector<TensorObj *> tensors;
  std::ostringstream os;
  auto on_visit = [&topo_indices, get_json_type_index = &get_json_type_index, os = &os, &tensors,
                   is_first_object = true](Object *object, MLCTypeInfo *type_info) mutable -> void {
    int32_t &topo_index = topo_indices[object];
    if (topo_index == 0) {
      topo_index = static_cast<int32_t>(topo_indices.size()) - 1;
    } else {
      MLC_THROW(InternalError) << "This should never happen: object already visited";
    }
    Emitter emitter{os, get_json_type_index, &topo_indices};
    if (is_first_object) {
      is_first_object = false;
    } else {
      os->put(',');
    }
    if (StrObj *str = object->as<StrObj>()) {
      str->PrintEscape(*os);
      return;
    }
    (*os) << '[' << (*get_json_type_index)(type_info->type_key);
    if (UListObj *list = object->as<UListObj>()) {
      for (Any &any : *list) {
        emitter(nullptr, &any);
      }
    } else if (UDictObj *dict = object->as<UDictObj>()) {
      for (auto &kv : *dict) {
        emitter(nullptr, &kv.first);
        emitter(nullptr, &kv.second);
      }
    } else if (TensorObj *tensor = object->as<TensorObj>()) {
      (*os) << ", " << tensors.size();
      tensors.push_back(tensor);
    } else if (object->IsInstance<FuncObj>() || object->IsInstance<ErrorObj>()) {
      MLC_THROW(TypeError) << "Unserializable type: " << object->GetTypeKey();
    } else if (object->IsInstance<OpaqueObj>()) {
      MLC_THROW(TypeError) << "Cannot serialize `mlc.Opaque` of type: "
                           << object->DynCast<OpaqueObj>()->opaque_type_name;
    } else {
      VisitFields(object, type_info, emitter);
    }
    os->put(']');
  };
  os << "{\"values\": [";
  if (any.type_index >= kMLCStaticObjectBegin) { // Topological the objects according to dependency
    TopoVisit(any.operator Object *(), nullptr, on_visit);
  } else if (any.type_index == kMLCNone) {
    os << "null";
  } else if (any.type_index == kMLCBool) {
    bool v = any.operator bool();
    os << (v ? "true" : "false");
  } else if (any.type_index == kMLCInt) {
    int32_t type_int = get_json_type_index(TypeTraits<int64_t>::type_str);
    int64_t v = any;
    os << "[" << type_int << ", " << v << "]";
  } else if (any.type_index == kMLCFloat) {
    double v = any;
    os << v;
  } else if (any.type_index == kMLCDevice) {
    int32_t type_device = get_json_type_index(TypeTraits<DLDevice>::type_str);
    DLDevice v = any;
    os << "[" << type_device << ", \"" << TypeTraits<DLDevice>::__str__(v) << "\"]";
  } else if (any.type_index == kMLCDataType) {
    int32_t type_dtype = get_json_type_index(TypeTraits<DLDataType>::type_str);
    DLDataType v = any;
    os << "[" << type_dtype << ", \"" << ::mlc::base::DType::Str(v) << "\"]";
  } else {
    MLC_THROW(TypeError) << "Cannot serialize type: " << Lib::GetTypeKey(any.type_index);
  }
  os << "], \"type_keys\": [";
  for (size_t i = 0; i < type_keys.size(); ++i) {
    if (i > 0) {
      os << ", ";
    }
    os << '"' << type_keys[i] << '\"';
  }
  os << "]";
  if (!tensors.empty()) {
    os << ", \"tensors\": [";
    for (size_t i = 0; i < tensors.size(); ++i) {
      if (i > 0) {
        os << ", ";
      }
      Str b64 = tensors[i]->ToBase64();
      os << '"' << b64->data() << '"';
    }
    os << "]";
  }
  os << "}";
  return os.str();
}

inline Any Deserialize(const char *json_str, int64_t json_str_len) {
  int32_t json_type_index_tensor = -1;
  // Step 0. Parse JSON string
  UDict json_obj = JSONLoads(json_str, json_str_len);
  // Step 1. type_key => constructors
  UList type_keys = json_obj->at("type_keys");
  std::vector<FuncObj *> constructors;
  constructors.reserve(type_keys.size());
  for (Str type_key : type_keys) {
    int32_t type_index = Lib::GetTypeIndex(type_key->data());
    FuncObj *func = nullptr;
    if (type_index != kMLCTensor) {
      func = Lib::_init(type_index);
    } else {
      json_type_index_tensor = static_cast<int32_t>(constructors.size());
    }
    constructors.push_back(func);
  }
  auto invoke_init = [&constructors](UList args) {
    int32_t json_type_index = args[0];
    Any ret;
    ::mlc::base::FuncCall(constructors.at(json_type_index), static_cast<int32_t>(args.size()) - 1, args->data() + 1,
                          &ret);
    return ret;
  };
  // Step 2. Handle tensors
  std::vector<Tensor> tensors;
  if (json_obj->count("tensors")) {
    UList tensors_b64 = json_obj->at("tensors");
    while (!tensors_b64->empty()) {
      Tensor tensor = Tensor::FromBase64(tensors_b64->back());
      tensors.push_back(tensor);
      tensors_b64->pop_back();
    }
    json_obj->erase("tensors");
    std::reverse(tensors.begin(), tensors.end());
  }
  // Step 3. Translate JSON object to objects
  UList values = json_obj->at("values");
  for (int64_t i = 0; i < values->size(); ++i) {
    Any obj = values[i];
    if (obj.type_index == kMLCList) {
      UList list = obj.operator UList();
      int32_t json_type_index = list[0];
      if (json_type_index == json_type_index_tensor) {
        values[i] = tensors[list[1].operator int32_t()];
        continue;
      }
      for (int64_t j = 1; j < list.size(); ++j) {
        Any arg = list[j];
        if (arg.type_index == kMLCInt) {
          int64_t k = arg;
          if (k < i) {
            list[j] = values[k];
          } else {
            MLC_THROW(ValueError) << "Invalid reference when parsing type `" << type_keys[json_type_index]
                                  << "`: referring #" << k << " at #" << i << ". v = " << obj;
          }
        } else if (arg.type_index == kMLCList) {
          list[j] = invoke_init(arg.operator UList());
        } else if (arg.type_index == kMLCStr || arg.type_index == kMLCBool || arg.type_index == kMLCFloat ||
                   arg.type_index == kMLCNone) {
          // Do nothing
        } else {
          MLC_THROW(ValueError) << "Unexpected value: " << arg;
        }
      }
      values[i] = invoke_init(list);
    } else if (obj.type_index == kMLCInt) {
      int32_t k = obj;
      values[i] = values[k];
    } else if (obj.type_index == kMLCStr) {
      // Do nothing
      // TODO: how about kMLCBool, kMLCFloat, kMLCNone?
    } else {
      MLC_THROW(ValueError) << "Unexpected value: " << obj;
    }
  }
  return values->back();
}

} // namespace
} // namespace mlc

namespace mlc {
namespace registry {

bool StructuralEqual(AnyView lhs, AnyView rhs, bool bind_free_vars, bool assert_mode) {
  try {
    // TODO: support non objects
    ::mlc::StructuralEqualImpl(lhs.operator Object *(), rhs.operator Object *(), bind_free_vars);
    return true;
  } catch (SEqualError &e) {
    if (assert_mode) {
      std::ostringstream os;
      os << "Structural equality check failed at " << e.path << ": " << e.what();
      MLC_THROW(ValueError) << os.str();
    }
  }
  return false;
}

Optional<Str> StructuralEqualFailReason(AnyView lhs, AnyView rhs, bool bind_free_vars) {
  try {
    // TODO: support non objects
    ::mlc::StructuralEqualImpl(lhs.operator Object *(), rhs.operator Object *(), bind_free_vars);
  } catch (SEqualError &e) {
    std::ostringstream os;
    os << "Structural equality check failed at " << e.path << ": " << e.what();
    return Str(os.str());
  }
  return Null;
}

int64_t StructuralHash(AnyView root) {
  // TODO: support non objects
  return static_cast<int64_t>(::mlc::StructuralHashImpl(root.operator Object *()));
}

Any CopyShallow(AnyView source) { return CopyShallowImpl(source); }
Any CopyDeep(AnyView source) { return CopyDeepImpl(source); }
void CopyReplace(int32_t num_args, const AnyView *args, Any *ret) { CopyReplaceImpl(num_args, args, ret); }

Any JSONLoads(AnyView json_str) {
  if (json_str.type_index == kMLCRawStr) {
    return ::mlc::JSONLoads(json_str.operator const char *(), -1);
  } else {
    StrObj *js = json_str.operator StrObj *();
    return ::mlc::JSONLoads(js->data(), js->size());
  }
}

Any JSONDeserialize(AnyView json_str) {
  if (json_str.type_index == kMLCRawStr) {
    return ::mlc::Deserialize(json_str.operator const char *(), -1);
  } else {
    StrObj *js = json_str.operator StrObj *();
    return ::mlc::Deserialize(js->data(), js->size());
  }
}

Str JSONSerialize(AnyView source) { return ::mlc::Serialize(source); }

Str TensorToBytes(const TensorObj *src) {
  return ::mlc::TensorToBytes(&src->tensor); //
}

Str TensorToBase64(const TensorObj *src) {
  Str bytes = ::mlc::TensorToBytes(&src->tensor);
  return ::mlc::Base64Encode(reinterpret_cast<uint8_t *>(bytes->data()), bytes->size());
}

Tensor TensorFromBytes(AnyView any) {
  if (any.type_index == kMLCRawStr) {
    const char *src = any;
    int64_t len = std::strlen(src);
    return ::mlc::TensorFromBytes(reinterpret_cast<const uint8_t *>(src), len);
  } else {
    Str src = any;
    return ::mlc::TensorFromBytes(reinterpret_cast<const uint8_t *>(src->::MLCStr::data), src->length());
  }
}

Tensor TensorFromBase64(AnyView any) {
  if (any.type_index == kMLCRawStr) {
    const char *src = any;
    int64_t len = std::strlen(src);
    Str bytes = ::mlc::Base64Decode(reinterpret_cast<const uint8_t *>(src), len);
    return ::mlc::TensorFromBytes(reinterpret_cast<const uint8_t *>(bytes->data()), bytes->size());
  } else {
    Str src = any;
    Str bytes = ::mlc::Base64Decode(reinterpret_cast<const uint8_t *>(src->::MLCStr::data), src->length());
    return ::mlc::TensorFromBytes(reinterpret_cast<const uint8_t *>(bytes->data()), bytes->size());
  }
}

} // namespace registry
} // namespace mlc
