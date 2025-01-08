#include "mlc/core/error.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mlc/core/all.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace mlc {
namespace core {
namespace {

uint64_t StructuralHash(Object *obj);
bool StructuralEqual(Object *lhs, Object *rhs, bool bind_free_vars, bool assert_mode);
void StructuralEqualImpl(Object *lhs, Object *rhs, bool bind_free_vars);

struct SEqualError : public std::runtime_error {
  ObjectPath path;
  SEqualError(const char *msg, ObjectPath path) : std::runtime_error(msg), path(path) {}
};

inline bool StructuralEqual(Object *lhs, Object *rhs, bool bind_free_vars, bool assert_mode) {
  try {
    StructuralEqualImpl(lhs, rhs, bind_free_vars);
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
  using mlc::base::DataTypeEqual;
  using mlc::base::DeviceEqual;
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
    MLC_CORE_EQ_S_OPT(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_OPT(double, DoubleEqual);
    MLC_CORE_EQ_S_OPT(DLDevice, DeviceEqual);
    MLC_CORE_EQ_S_OPT(DLDataType, DataTypeEqual);
    MLC_CORE_EQ_S_OPT(VoidPtr, std::equal_to<const void *>());
    MLC_CORE_EQ_S_POD(int8_t, std::equal_to<int8_t>());
    MLC_CORE_EQ_S_POD(int16_t, std::equal_to<int16_t>());
    MLC_CORE_EQ_S_POD(int32_t, std::equal_to<int32_t>());
    MLC_CORE_EQ_S_POD(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_POD(float, FloatEqual);
    MLC_CORE_EQ_S_POD(double, DoubleEqual);
    MLC_CORE_EQ_S_POD(DLDataType, DataTypeEqual);
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
    static void EnqueueAny(std::vector<Task> *tasks, bool bind_free_vars, const Any *lhs, const Any *rhs,
                           ObjectPath new_path) {
      int32_t type_index = lhs->GetTypeIndex();
      if (type_index != rhs->GetTypeIndex()) {
        MLC_CORE_EQ_S_ERR(lhs->GetTypeKey(), rhs->GetTypeKey(), new_path);
      }
      if (type_index == kMLCNone) {
        return;
      }
      MLC_CORE_EQ_S_ANY(type_index == kMLCInt, int64_t, std::equal_to<int64_t>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCFloat, double, DoubleEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCPtr, VoidPtr, std::equal_to<const void *>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_ANY(type_index == kMLCDataType, DLDataType, DataTypeEqual, lhs, rhs, new_path);
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
        MLC_CORE_EQ_S_ERR(::mlc::base::TypeIndex2TypeKey(lhs_type_index),
                          ::mlc::base::TypeIndex2TypeKey(rhs_type_index), new_path);
      } else if (lhs_type_index == kMLCStr) {
        Str lhs_str(reinterpret_cast<StrObj *>(lhs));
        Str rhs_str(reinterpret_cast<StrObj *>(rhs));
        if (lhs_str != rhs_str) {
          MLC_CORE_EQ_S_ERR(lhs_str, rhs_str, new_path);
        }
      } else if (lhs_type_index == kMLCFunc || lhs_type_index == kMLCError) {
        throw SEqualError("Cannot compare `mlc.Func` or `mlc.Error`", new_path);
      } else {
        bool visited = false;
        MLCTypeInfo *type_info = ::mlc::base::TypeIndex2TypeInfo(lhs_type_index);
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
      ::mlc::base::HashCombine(::mlc::base::TypeIndex2TypeInfo(kMLCNone)->type_key_hash, 0);
  inline static const uint64_t MLC_SYMBOL_HIDE kInt = ::mlc::base::TypeIndex2TypeInfo(kMLCInt)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kFloat = ::mlc::base::TypeIndex2TypeInfo(kMLCFloat)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kPtr = ::mlc::base::TypeIndex2TypeInfo(kMLCPtr)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kDType = ::mlc::base::TypeIndex2TypeInfo(kMLCDataType)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kDevice = ::mlc::base::TypeIndex2TypeInfo(kMLCDevice)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kRawStr = ::mlc::base::TypeIndex2TypeInfo(kMLCRawStr)->type_key_hash;
  inline static const uint64_t MLC_SYMBOL_HIDE kStrObj = ::mlc::base::TypeIndex2TypeInfo(kMLCStr)->type_key_hash;
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

inline uint64_t StructuralHash(Object *obj) {
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
    static uint64_t HashInteger(int64_t a) { return HashTyped<int64_t>(HashCache::kInt, a); }
    static uint64_t HashPtr(VoidPtr a) { return HashTyped<VoidPtr>(HashCache::kPtr, a); }
    static uint64_t HashDevice(DLDevice a) { return HashTyped<DLDevice>(HashCache::kDevice, a); }
    static uint64_t HashDataType(DLDataType a) { return HashTyped<DLDataType>(HashCache::kDType, a); }
    // clang-format off
    static uint64_t HashFloat(float a) { return HashTyped<float>(HashCache::kFloat, std::isnan(a) ? std::numeric_limits<float>::quiet_NaN() : a); }
    static uint64_t HashDouble(double a) { return HashTyped<double>(HashCache::kFloat, std::isnan(a) ? std::numeric_limits<double>::quiet_NaN() : a); }
    static uint64_t HashCharArray(CharArray a) { return HashTyped<uint64_t>(HashCache::kRawStr, ::mlc::base::StrHash(a)); }
    // clang-format on
    MLC_CORE_HASH_S_OPT(int64_t, HashInteger);
    MLC_CORE_HASH_S_OPT(double, HashDouble);
    MLC_CORE_HASH_S_OPT(DLDevice, HashDevice);
    MLC_CORE_HASH_S_OPT(DLDataType, HashDataType);
    MLC_CORE_HASH_S_OPT(VoidPtr, HashPtr);
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
      } else if (type_index == kMLCFunc || type_index == kMLCError) {
        throw SEqualError("Cannot compare `mlc.Func` or `mlc.Error`", ObjectPath::Root());
      } else {
        MLCTypeInfo *type_info = ::mlc::base::TypeIndex2TypeInfo(type_index);
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

inline Any CopyShallow(AnyView source) {
  int32_t type_index = source.type_index;
  if (::mlc::base::IsTypeIndexPOD(type_index)) {
    return source;
  } else if (UListObj *list = source.TryCast<UListObj>()) {
    return UList(list->begin(), list->end());
  } else if (UDictObj *dict = source.TryCast<UDictObj>()) {
    return UDict(dict->begin(), dict->end());
  } else if (source.IsInstance<StrObj>() || source.IsInstance<ErrorObj>() || source.IsInstance<FuncObj>()) {
    return source;
  }
  struct Copier {
    MLC_INLINE void operator()(MLCTypeField *, const Any *any) { fields->push_back(AnyView(*any)); }
    MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { fields->push_back(AnyView(*obj)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { fields->push_back(AnyView(*opt)); }
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
  FuncObj *init_func = ::mlc::base::LibState::VTableGetFunc(::mlc::base::LibState::init, type_index, "__init__");
  MLCTypeInfo *type_info = ::mlc::base::TypeIndex2TypeInfo(type_index);
  std::vector<AnyView> fields;
  VisitFields(source.operator Object *(), type_info, Copier{&fields});
  Any ret;
  ::mlc::base::FuncCall(init_func, static_cast<int32_t>(fields.size()), fields.data(), &ret);
  return ret;
}

inline Any CopyDeep(AnyView source) {
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
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { fields->push_back(AnyView(*opt)); }
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { fields->push_back(AnyView(*opt)); }
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
      if (const Object *obj = any->TryCast<Object>()) {
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
    if (UListObj *list = object->TryCast<UListObj>()) {
      fields.clear();
      fields.reserve(list->size());
      for (Any &e : *list) {
        Copier{&orig2copy, &fields}.HandleAny(&e);
      }
      UList::FromAnyTuple(static_cast<int32_t>(fields.size()), fields.data(), &ret);
    } else if (UDictObj *dict = object->TryCast<UDictObj>()) {
      fields.clear();
      for (auto [key, value] : *dict) {
        Copier{&orig2copy, &fields}.HandleAny(&key);
        Copier{&orig2copy, &fields}.HandleAny(&value);
      }
      UDict::FromAnyTuple(static_cast<int32_t>(fields.size()), fields.data(), &ret);
    } else if (object->IsInstance<StrObj>() || object->IsInstance<ErrorObj>() || object->IsInstance<FuncObj>()) {
      ret = object;
    } else {
      fields.clear();
      VisitFields(object, type_info, Copier{&orig2copy, &fields});
      FuncObj *func =
          ::mlc::base::LibState::VTableGetFunc(::mlc::base::LibState::init, type_info->type_index, "__init__");
      ::mlc::base::FuncCall(func, static_cast<int32_t>(fields.size()), fields.data(), &ret);
    }
    orig2copy[object] = ret.operator ObjectRef();
  });
  return orig2copy.at(source.operator Object *());
}

MLC_REGISTER_FUNC("mlc.core.StructuralEqual").set_body(::mlc::core::StructuralEqual);
MLC_REGISTER_FUNC("mlc.core.StructuralHash").set_body([](::mlc::Object *obj) -> int64_t {
  uint64_t ret = ::mlc::core::StructuralHash(obj);
  return static_cast<int64_t>(ret);
});
MLC_REGISTER_FUNC("mlc.core.CopyShallow").set_body(::mlc::core::CopyShallow);
MLC_REGISTER_FUNC("mlc.core.CopyDeep").set_body(::mlc::core::CopyDeep);
} // namespace
} // namespace core
} // namespace mlc
