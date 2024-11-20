#ifndef MLC_CORE_JSON_H_
#define MLC_CORE_JSON_H_
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

Any ParseJSON(const char *json_str, int64_t json_str_len);
mlc::Str Serialize(Any any);
Any Deserialize(const char *json_str, int64_t json_str_len);
MLC_INLINE Any ParseJSON(const char *json_str) { return ParseJSON(json_str, -1); }
MLC_INLINE Any ParseJSON(const Str &json_str) { return ParseJSON(json_str->data(), json_str->size()); }
MLC_INLINE Any ParseJSON(const std::string &json_str) {
  return ParseJSON(json_str.data(), static_cast<int64_t>(json_str.size()));
}
MLC_INLINE Any Deserialize(const char *json_str) { return Deserialize(json_str, -1); }
MLC_INLINE Any Deserialize(const Str &json_str) { return Deserialize(json_str->data(), json_str->size()); }
MLC_INLINE Any Deserialize(const std::string &json_str) {
  return Deserialize(json_str.data(), static_cast<int64_t>(json_str.size()));
}

struct SerializerState {

  struct ObjInfo {
    Object *obj;
    MLCTypeInfo *type_info;
    int32_t json_type_index;
    int32_t num_deps;
    int32_t obj_idx;
    std::vector<ObjInfo *> parents;
  };

  std::ostringstream os;
  // Tracking type keys
  std::unordered_map<const char *, int32_t> type_key2index;
  std::vector<const char *> type_keys;
  // Tracking objects
  std::vector<std::unique_ptr<ObjInfo>> obj_list;
  std::unordered_map<const Object *, int32_t> obj2index;

  int32_t type_int = this->GetJSONTypeIndex(::mlc::base::TypeTraits<int64_t>::type_str);
  int32_t type_device = this->GetJSONTypeIndex(::mlc::base::TypeTraits<DLDevice>::type_str);
  int32_t type_dtype = this->GetJSONTypeIndex(::mlc::base::TypeTraits<DLDataType>::type_str);

  ObjInfo *GetObjInfo(Object *obj) {
    if (auto it = obj2index.find(obj); it != obj2index.end()) {
      return obj_list[it->second].get();
    }
    int32_t obj_index = static_cast<int32_t>(obj_list.size());
    obj2index[obj] = obj_index;
    obj_list.push_back(std::make_unique<ObjInfo>());
    ObjInfo *info = obj_list.back().get();
    info->obj = obj;
    info->type_info = ::mlc::base::TypeIndex2TypeInfo(obj->GetTypeIndex());
    info->json_type_index =
        info->type_info->type_index == kMLCStr ? -1 : this->GetJSONTypeIndex(info->type_info->type_key);
    info->num_deps = 0;
    info->obj_idx = -1;
    return info;
  }

  int32_t GetJSONTypeIndex(const char *type_key) {
    if (auto it = type_key2index.find(type_key); it != type_key2index.end()) {
      return it->second;
    }
    int32_t type_index = static_cast<int32_t>(type_key2index.size());
    type_key2index[type_key] = type_index;
    type_keys.push_back(type_key);
    return type_index;
  }

  void TrackObject(SerializerState::ObjInfo *parent, Object *child) {
    SerializerState::ObjInfo *child_info = this->GetObjInfo(child);
    if (parent) {
      parent->num_deps += 1;
      child_info->parents.push_back(parent);
    }
  }
};

struct SerializerFieldExtractor {
  MLC_INLINE void operator()(int32_t, MLCTypeField *, const Any *any) {
    if (any->type_index >= kMLCStaticObjectBegin) {
      state->TrackObject(current, any->operator Object *());
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, ObjectRef *obj) {
    if (Object *v = obj->get()) {
      state->TrackObject(current, v);
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<Object> *opt) {
    if (Object *v = opt->get()) {
      state->TrackObject(current, v);
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<int64_t> *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<double> *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<DLDevice> *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<DLDataType> *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int8_t *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int16_t *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int32_t *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int64_t *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, float *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, double *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, DLDataType *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, DLDevice *) {}
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<void *> *) {
    MLC_THROW(TypeError) << "Unserializable type: void *";
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, void **) {
    MLC_THROW(TypeError) << "Unserializable type: void *";
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, const char *) {
    MLC_THROW(TypeError) << "Unserializable type: const char *";
  }

  static void Run(SerializerState *state, SerializerState::ObjInfo *current) {
    int32_t type_index = current->type_info->type_index;
    if (type_index == kMLCList) {
      UListObj *list = reinterpret_cast<UListObj *>(current->obj); // TODO: support Downcast
      for (Any any : *list) {
        SerializerFieldExtractor{state, current}(0, nullptr, &any);
      }
    } else if (type_index == kMLCDict) {
      UDictObj *dict = reinterpret_cast<UDictObj *>(current->obj); // TODO: support Downcast
      for (auto &kv : *dict) {
        SerializerFieldExtractor{state, current}(0, nullptr, &kv.first);
        SerializerFieldExtractor{state, current}(0, nullptr, &kv.second);
      }
    } else if (type_index == kMLCStr) {
    } else if (type_index == kMLCError || type_index == kMLCFunc) {
      MLC_THROW(TypeError) << "Unserializable type: " << current->type_info->type_key;
    } else {
      ::mlc::base::VisitTypeField(current->obj, current->type_info, SerializerFieldExtractor{state, current});
    }
  }

  SerializerState *state;
  SerializerState::ObjInfo *current;
};

struct SerializerObjectEmitter {
  MLC_INLINE void operator()(int32_t, MLCTypeField *, const Any *any) { EmitAny(any); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, ObjectRef *obj) {
    if (Object *v = obj->get()) {
      EmitObject(v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<Object> *opt) {
    if (Object *v = opt->get()) {
      EmitObject(v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<int64_t> *opt) {
    if (const int64_t *v = opt->get()) {
      EmitInt(*v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<double> *opt) {
    if (const double *v = opt->get()) {
      EmitFloat(*v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<DLDevice> *opt) {
    if (const DLDevice *v = opt->get()) {
      EmitDevice(*v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<DLDataType> *opt) {
    if (const DLDataType *v = opt->get()) {
      EmitDType(*v);
    } else {
      EmitNil();
    }
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int8_t *v) { EmitInt(static_cast<int64_t>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int16_t *v) { EmitInt(static_cast<int64_t>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int32_t *v) { EmitInt(static_cast<int64_t>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, int64_t *v) { EmitInt(static_cast<int64_t>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, float *v) { EmitFloat(static_cast<double>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, double *v) { EmitFloat(static_cast<double>(*v)); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, DLDataType *v) { EmitDType(*v); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, DLDevice *v) { EmitDevice(*v); }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, Optional<void *> *) {
    MLC_THROW(TypeError) << "Unserializable type: void *";
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, void **) {
    MLC_THROW(TypeError) << "Unserializable type: void *";
  }
  MLC_INLINE void operator()(int32_t, MLCTypeField *, const char *) {
    MLC_THROW(TypeError) << "Unserializable type: const char *";
  }
  inline void EmitNil() { state->os << ", null"; }
  inline void EmitFloat(double v) { state->os << ", " << std::fixed << std::setprecision(19) << v; }
  inline void EmitInt(int64_t v) { state->os << ", [" << state->type_int << ", " << v << "]"; }
  inline void EmitDevice(DLDevice v) {
    state->os << ", [" << state->type_device << ", " << ::mlc::base::TypeTraits<DLDevice>::__str__(v) << "]";
  }
  inline void EmitDType(DLDataType v) {
    state->os << ", [" << state->type_dtype << ", " << ::mlc::base::TypeTraits<DLDataType>::__str__(v) << "]";
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
    if (obj) {
      SerializerState::ObjInfo *info = state->GetObjInfo(obj);
      if (info->obj_idx != -1) {
        state->os << ", " << info->obj_idx;
      } else {
        MLC_THROW(InternalError) << "This should never happen: object not registered before EmitObject";
      }
    } else {
      MLC_THROW(InternalError) << "This should never happen: null object pointer during EmitObject";
    }
  }
  static void Run(SerializerState *state, SerializerState::ObjInfo *current) {
    int32_t type_index = current->type_info->type_index;
    if (type_index == kMLCList) {
      UListObj *list = reinterpret_cast<UListObj *>(current->obj); // TODO: support Downcast
      for (Any any : *list) {
        SerializerObjectEmitter{state}(0, nullptr, &any);
      }
    } else if (type_index == kMLCDict) {
      UDictObj *dict = reinterpret_cast<UDictObj *>(current->obj); // TODO: support Downcast
      for (auto &kv : *dict) {
        SerializerObjectEmitter{state}(0, nullptr, &kv.first);
        SerializerObjectEmitter{state}(0, nullptr, &kv.second);
      }
    } else if (type_index == kMLCStr) {
      state->os << ", ";
      reinterpret_cast<StrObj *>(current->obj)->PrintEscape(state->os);
    } else if (type_index == kMLCError || type_index == kMLCFunc) {
      MLC_THROW(TypeError) << "Unserializable type: " << current->type_info->type_key;
    } else {
      ::mlc::base::VisitTypeField(current->obj, current->type_info, SerializerObjectEmitter{state});
    }
  }
  SerializerState *state;
};

inline mlc::Str Serialize(Any any) {
  using ObjInfo = SerializerState::ObjInfo;
  SerializerState state;
  state.os << "{\"values\": [";
  if (any.type_index >= kMLCStaticObjectBegin) { // Topological the objects according to dependency
    // Step 1. Build dependency graph
    state.TrackObject(nullptr, any.operator Object *());
    for (size_t i = 0; i < state.obj_list.size(); ++i) {
      ObjInfo *current = state.obj_list[i].get();
      SerializerFieldExtractor::Run(&state, current);
    }
    // Step 2. Enqueue nodes with no dependency
    std::vector<ObjInfo *> stack;
    stack.reserve(state.obj_list.size());
    for (size_t i = 0; i < state.obj_list.size(); ++i) {
      if (ObjInfo *current = state.obj_list[i].get(); current->num_deps == 0) {
        stack.push_back(current);
      }
    }
    // Step 3. Traverse the graph by topological order
    size_t num_objects = 0;
    for (; !stack.empty(); ++num_objects) {
      ObjInfo *current = stack.back();
      stack.pop_back();
      // Step 1. Lable object index
      if (current->obj_idx == -1) {
        current->obj_idx = static_cast<int32_t>(num_objects);
      } else {
        MLC_THROW(InternalError) << "This should never happen: object already serialized";
      }
      // Step 2. Serialize object
      if (num_objects > 0) {
        state.os << ", ";
      }
      if (current->type_info->type_index == kMLCStr) { // string
        reinterpret_cast<StrObj *>(current->obj)->PrintEscape(state.os);
      } else {
        state.os << "[" << current->json_type_index;
        SerializerObjectEmitter::Run(&state, current);
        state.os << "]";
      }
      // Step 3. Decrease the dependency count of parents
      for (ObjInfo *parent : current->parents) {
        if (--parent->num_deps == 0) {
          stack.push_back(parent);
        }
      }
    }
    if (num_objects != state.obj_list.size()) {
      MLC_THROW(ValueError) << "Can't serialize objects with circular dependency";
    }
  } else if (any.type_index == kMLCNone) {
    state.os << "null";
  } else if (any.type_index == kMLCInt) {
    int64_t v = any;
    state.os << "[" << state.type_int << ", " << v << "]";
  } else if (any.type_index == kMLCFloat) {
    double v = any;
    state.os << v;
  } else if (any.type_index == kMLCDevice) {
    DLDevice v = any;
    state.os << "[" << state.type_device << ", \"" << ::mlc::base::TypeTraits<DLDevice>::__str__(v) << "\"]";
  } else if (any.type_index == kMLCDataType) {
    DLDataType v = any;
    state.os << "[" << state.type_dtype << ", \"" << ::mlc::base::TypeTraits<DLDataType>::__str__(v) << "\"]";
  } else {
    MLC_THROW(TypeError) << "Cannot serialize type: " << ::mlc::base::TypeIndex2TypeKey(any.type_index);
  }
  state.os << "], \"type_keys\": [\"" << state.type_keys[0] << "\"";
  for (size_t i = 1; i < state.type_keys.size(); ++i) {
    state.os << ", \"" << state.type_keys[i] << "\"";
  }
  state.os << "]}";
  return state.os.str();
}

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

private:
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

  template <size_t N> void ExpectString(const char (&expected)[N]) {
    // len(expected) = N - 1, which includes the null terminator
    int64_t len = static_cast<int64_t>(N) - 1;
    if (i + len <= json_str_len && std::strncmp(json_str + i, expected, len) == 0) {
      i = i + len;
    } else {
      MLC_THROW(ValueError) << "JSON parsing failure at position " << i << ": Expected '" << expected
                            << ". JSON string: " << json_str;
    }
  }

  Any ParseNull() {
    ExpectString("null");
    return Any(nullptr);
  }

  Any ParseBoolean() {
    if (PeekChar() == 't') {
      ExpectString("true");
      return Any(1);
    } else {
      ExpectString("false");
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
  }

public:
  int64_t i;
  int64_t json_str_len;
  const char *json_str;
};

MLC_INLINE Any ParseJSON(const char *json_str, int64_t json_str_len) {
  if (json_str_len < 0) {
    json_str_len = static_cast<int64_t>(std::strlen(json_str));
  }
  return JSONParser{0, json_str_len, json_str}.Parse();
}

inline Any Deserialize(const char *json_str, int64_t json_str_len) {
  // Step 0. Parse JSON string
  UDict json_obj = ParseJSON(json_str, json_str_len).operator UDict(); // TODO: impl "Any -> UDict"
  // Step 1. type_key => constructors
  UList type_keys = json_obj->at(Str("type_keys")).operator UList(); // TODO: impl `UDict::at(Str)`
  std::vector<Func> constructors;
  constructors.reserve(type_keys.size());
  for (Str type_key : type_keys) {
    Any init_func;
    MLCVTableGet(nullptr, ::mlc::base::TypeKey2TypeIndex(type_key->data()), "__init__", &init_func);
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

} // namespace core
} // namespace mlc

#endif // MLC_CORE_JSON_H_
