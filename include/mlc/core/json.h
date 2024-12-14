#ifndef MLC_CORE_JSON_H_
#define MLC_CORE_JSON_H_
#include "./field_visitor.h"
#include "./func.h"
#include "./str.h"
#include "./udict.h"
#include "./ulist.h"
#include <cstdint>
#include <iomanip>
#include <mlc/base/all.h>
#include <sstream>

namespace mlc {
namespace core {

mlc::Str Serialize(Any any);
Any Deserialize(const char *json_str, int64_t json_str_len);
Any JSONLoads(const char *json_str, int64_t json_str_len);
MLC_INLINE Any Deserialize(const char *json_str) { return Deserialize(json_str, -1); }
MLC_INLINE Any Deserialize(const Str &json_str) { return Deserialize(json_str->data(), json_str->size()); }
MLC_INLINE Any Deserialize(const std::string &json_str) {
  return Deserialize(json_str.data(), static_cast<int64_t>(json_str.size()));
}
MLC_INLINE Any JSONLoads(const char *json_str) { return JSONLoads(json_str, -1); }
MLC_INLINE Any JSONLoads(const Str &json_str) { return JSONLoads(json_str->data(), json_str->size()); }
MLC_INLINE Any JSONLoads(const std::string &json_str) {
  return JSONLoads(json_str.data(), static_cast<int64_t>(json_str.size()));
}

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
    // clang-format off
      MLC_INLINE void operator()(MLCTypeField *, const Any *any) { EmitAny(any); }
      MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { if (Object *v = obj->get()) EmitObject(v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<Object> *opt) { if (Object *v = opt->get()) EmitObject(v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { if (const int64_t *v = opt->get()) EmitInt(*v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { if (const double *v = opt->get())  EmitFloat(*v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { if (const DLDevice *v = opt->get()) EmitDevice(*v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { if (const DLDataType *v = opt->get()) EmitDType(*v); else EmitNil(); }
      MLC_INLINE void operator()(MLCTypeField *, int8_t *v) { EmitInt(static_cast<int64_t>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, int16_t *v) { EmitInt(static_cast<int64_t>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, int32_t *v) { EmitInt(static_cast<int64_t>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, int64_t *v) { EmitInt(static_cast<int64_t>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, float *v) { EmitFloat(static_cast<double>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, double *v) { EmitFloat(static_cast<double>(*v)); }
      MLC_INLINE void operator()(MLCTypeField *, DLDataType *v) { EmitDType(*v); }
      MLC_INLINE void operator()(MLCTypeField *, DLDevice *v) { EmitDevice(*v); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *) { MLC_THROW(TypeError) << "Unserializable type: void *"; }
      MLC_INLINE void operator()(MLCTypeField *, void **) { MLC_THROW(TypeError) << "Unserializable type: void *"; }
      MLC_INLINE void operator()(MLCTypeField *, const char **) { MLC_THROW(TypeError) << "Unserializable type: const char *"; }
    // clang-format on
    inline void EmitNil() { (*os) << ", null"; }
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
      (*os) << ", [" << type_dtype << ", " << ::mlc::base::TypeTraits<DLDataType>::__str__(v) << "]";
    }
    inline void EmitAny(const Any *any) {
      int32_t type_index = any->type_index;
      if (type_index == kMLCNone) {
        EmitNil();
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
        MLC_THROW(TypeError) << "Cannot serialize type: " << ::mlc::base::TypeIndex2TypeKey(type_index);
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

  std::ostringstream os;
  auto on_visit = [get_json_type_index = &get_json_type_index, os = &os, is_first_object = true](
                      Object *object, MLCTypeInfo *type_info, const TObj2Idx &obj2index) mutable -> void {
    Emitter emitter{os, get_json_type_index, &obj2index};
    if (is_first_object) {
      is_first_object = false;
    } else {
      os->put(',');
    }
    int32_t type_index = type_info->type_index;
    if (type_index == kMLCStr) {
      StrObj *str = reinterpret_cast<StrObj *>(object);
      str->PrintEscape(*os);
      return;
    }
    (*os) << '[' << (*get_json_type_index)(type_info->type_key);
    if (type_index == kMLCList) {
      UListObj *list = reinterpret_cast<UListObj *>(object); // TODO: support Downcast
      for (Any &any : *list) {
        emitter(nullptr, &any);
      }
    } else if (type_index == kMLCDict) {
      UDictObj *dict = reinterpret_cast<UDictObj *>(object); // TODO: support Downcast
      for (auto &kv : *dict) {
        emitter(nullptr, &kv.first);
        emitter(nullptr, &kv.second);
      }
    } else if (type_index == kMLCFunc || type_index == kMLCError) {
      MLC_THROW(TypeError) << "Unserializable type: " << object->GetTypeKey();
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
    os << "[" << type_dtype << ", \"" << TypeTraits<DLDataType>::__str__(v) << "\"]";
  } else {
    MLC_THROW(TypeError) << "Cannot serialize type: " << mlc::base::TypeIndex2TypeKey(any.type_index);
  }
  os << "], \"type_keys\": [";
  for (size_t i = 0; i < type_keys.size(); ++i) {
    if (i > 0) {
      os << ", ";
    }
    os << '"' << type_keys[i] << '\"';
  }
  os << "]}";
  return os.str();
}

inline Any Deserialize(const char *json_str, int64_t json_str_len) {
  MLCVTableHandle init_vtable;
  MLCVTableGetGlobal(nullptr, "__init__", &init_vtable);
  // Step 0. Parse JSON string
  UDict json_obj = JSONLoads(json_str, json_str_len).operator UDict(); // TODO: impl "Any -> UDict"
  // Step 1. type_key => constructors
  UList type_keys = json_obj->at(Str("type_keys")).operator UList(); // TODO: impl `UDict::at(Str)`
  std::vector<Func> constructors;
  constructors.reserve(type_keys.size());
  for (Str type_key : type_keys) {
    Any init_func;
    int32_t type_index = ::mlc::base::TypeKey2TypeIndex(type_key->data());
    MLCVTableGetFunc(init_vtable, type_index, false, &init_func);
    if (!::mlc::base::IsTypeIndexNone(init_func.type_index)) {
      constructors.push_back(init_func.operator Func());
    } else {
      MLC_THROW(InternalError) << "Method `__init__` is not defined for type " << type_key;
    }
  }
  auto invoke_init = [&constructors](UList args) {
    int32_t json_type_index = args[0];
    Any ret;
    ::mlc::base::FuncCall(constructors.at(json_type_index).get(), static_cast<int32_t>(args.size()) - 1,
                          args->data() + 1, &ret);
    return ret;
  };
  // Step 2. Translate JSON object to objects
  UList values = json_obj->at(Str("values")).operator UList(); // TODO: impl `UDict::at(Str)`
  for (int64_t i = 0; i < values->size(); ++i) {
    Any obj = values[i];
    if (obj.type_index == kMLCList) {
      UList list = obj.operator UList();
      int32_t json_type_index = list[0];
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
        } else if (arg.type_index == kMLCStr || arg.type_index == kMLCFloat || arg.type_index == kMLCNone) {
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
    } else {
      MLC_THROW(ValueError) << "Unexpected value: " << obj;
    }
  }
  return values->back();
}

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
        return Any(1);
      } else {
        ExpectString("false", 5);
        return Any(0);
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

} // namespace core
} // namespace mlc

#endif // MLC_CORE_JSON_H_
