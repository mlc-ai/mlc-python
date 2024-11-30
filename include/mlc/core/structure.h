#ifndef MLC_CORE_STRUCTURE_H_
#define MLC_CORE_STRUCTURE_H_
#include "./field_visitor.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mlc/base/all.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace mlc {
namespace core {

void StructuralEqualImpl(Object *lhs, Object *rhs, bool bind_free_vars);

struct SEqualPath {
  std::shared_ptr<const SEqualPath> prev;
  int32_t kind;
  const char *field_name; // kind = 0
  int64_t list_index;     // kind = 1
  AnyView dict_key;       // kind = 2

  void Print(std::ostream &) const;
};

struct SEqualError : public std::runtime_error {
  std::shared_ptr<SEqualPath> path;
  SEqualError(const char *msg, std::shared_ptr<SEqualPath> path) : std::runtime_error(msg), path(path) {}
};

inline bool StructuralEqual(Object *lhs, Object *rhs, bool bind_free_vars, bool assert_mode) {
  try {
    StructuralEqualImpl(lhs, rhs, bind_free_vars);
    return true;
  } catch (SEqualError &e) {
    if (assert_mode) {
      std::ostringstream err;
      if (e.path) {
        e.path->Print(err);
      }
      MLC_THROW(ValueError) << "Structural equality check failed at {root}" << err.str() << ": " << e.what();
    }
  }
  return false;
}

inline void SEqualPath::Print(std::ostream &os) const {
  std::vector<const SEqualPath *> items;
  for (const SEqualPath *p = this; p; p = p->prev.get()) {
    items.push_back(p);
  }
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    const SEqualPath *p = *it;
    if (p->kind == 0) {
      os << "." << p->field_name;
    } else if (p->kind == 1) {
      os << "[" << p->list_index << "]";
    } else {
      int32_t type_index = p->dict_key.type_index;
      if (::mlc::base::IsTypeIndexPOD(type_index)) {
        os << "[" << p->dict_key << "]";
      } else if (type_index == kMLCStr) {
        StrObj *str_obj = p->dict_key;
        os << "[";
        str_obj->PrintEscape(os);
        os << "]";
      } else {
        const char *type_key = ::mlc::base::TypeIndex2TypeKey(type_index);
        os << "[" << type_key << "@" << static_cast<const void *>(p->dict_key.v.v_obj) << "]";
      }
    }
  }
}

inline std::ostream &operator<<(std::ostream &os, const std::shared_ptr<SEqualPath> &path) {
  if (path) {
    path->Print(os);
  }
  return os;
}

template <typename T> MLC_INLINE T *WithOffset(Object *obj, MLCTypeField *field) {
  return reinterpret_cast<T *>(reinterpret_cast<char *>(obj) + field->offset);
}

#define MLC_CORE_EQ_S_ERR_OUT(LHS, RHS, PATH)                                                                          \
  {                                                                                                                    \
    std::ostringstream err;                                                                                            \
    err << (LHS) << " vs " << (RHS);                                                                                   \
    throw SEqualError(err.str().c_str(), (PATH));                                                                      \
  }
#define MLC_CORE_EQ_S_CMP_ANY(Cond, Type, EQ, LHS, RHS, PATH)                                                          \
  if (Cond) {                                                                                                          \
    Type lhs_value = LHS->operator Type();                                                                             \
    Type rhs_value = RHS->operator Type();                                                                             \
    if (EQ(lhs_value, rhs_value)) {                                                                                    \
      return;                                                                                                          \
    } else {                                                                                                           \
      MLC_CORE_EQ_S_ERR_OUT(*lhs, *rhs, PATH);                                                                         \
    }                                                                                                                  \
  }
#define MLC_CORE_EQ_S_COMPARE_OPT(Type, EQ)                                                                            \
  MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind, Optional<Type> *_lhs) {                          \
    const Type *lhs = _lhs->get();                                                                                     \
    const Type *rhs = WithOffset<Optional<Type>>(obj_rhs, field)->get();                                               \
    if ((lhs != nullptr || rhs != nullptr) && (lhs == nullptr || rhs == nullptr || !EQ(*lhs, *rhs))) {                 \
      AnyView LHS = lhs ? AnyView(*lhs) : AnyView(nullptr);                                                            \
      AnyView RHS = rhs ? AnyView(*rhs) : AnyView(nullptr);                                                            \
      MLC_CORE_EQ_S_ERR_OUT(LHS, RHS, Append::Field(path, field->name));                                               \
    }                                                                                                                  \
  }
#define MLC_CORE_EQ_S_COMPARE(Type, EQ)                                                                                \
  MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind, Type *lhs) {                                     \
    const Type *rhs = WithOffset<Type>(obj_rhs, field);                                                                \
    if (!EQ(*lhs, *rhs)) {                                                                                             \
      MLC_CORE_EQ_S_ERR_OUT(AnyView(*lhs), AnyView(*rhs), Append::Field(path, field->name));                           \
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
    std::shared_ptr<SEqualPath> path;
    std::unique_ptr<std::ostringstream> err;
  };
  struct Append {
    static std::shared_ptr<SEqualPath> Field(const std::shared_ptr<SEqualPath> &self, const char *new_field_name) {
      return std::make_shared<SEqualPath>(SEqualPath{self, 0, new_field_name, 0, nullptr});
    }
    static std::shared_ptr<SEqualPath> ListIndex(const std::shared_ptr<SEqualPath> &self, int64_t new_list_index) {
      return std::make_shared<SEqualPath>(SEqualPath{self, 1, nullptr, new_list_index, nullptr});
    }
    static std::shared_ptr<SEqualPath> DictKey(const std::shared_ptr<SEqualPath> &self, AnyView new_dict_key) {
      return std::make_shared<SEqualPath>(SEqualPath{self, 2, nullptr, 0, new_dict_key});
    }
  };
  struct FieldComparator {
    static bool CharArrayEqual(CharArray lhs, CharArray rhs) { return std::strcmp(lhs, rhs) == 0; }
    static bool FloatEqual(float lhs, float rhs) { return std::abs(lhs - rhs) < 1e-6; }
    static bool DoubleEqual(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-8; }
    MLC_CORE_EQ_S_COMPARE_OPT(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_COMPARE_OPT(double, DoubleEqual);
    MLC_CORE_EQ_S_COMPARE_OPT(DLDevice, DeviceEqual);
    MLC_CORE_EQ_S_COMPARE_OPT(DLDataType, DataTypeEqual);
    MLC_CORE_EQ_S_COMPARE_OPT(VoidPtr, std::equal_to<const void *>());
    MLC_CORE_EQ_S_COMPARE(int8_t, std::equal_to<int8_t>());
    MLC_CORE_EQ_S_COMPARE(int16_t, std::equal_to<int16_t>());
    MLC_CORE_EQ_S_COMPARE(int32_t, std::equal_to<int32_t>());
    MLC_CORE_EQ_S_COMPARE(int64_t, std::equal_to<int64_t>());
    MLC_CORE_EQ_S_COMPARE(float, FloatEqual);
    MLC_CORE_EQ_S_COMPARE(double, DoubleEqual);
    MLC_CORE_EQ_S_COMPARE(DLDataType, DataTypeEqual);
    MLC_CORE_EQ_S_COMPARE(DLDevice, DeviceEqual);
    MLC_CORE_EQ_S_COMPARE(VoidPtr, std::equal_to<const void *>());
    MLC_CORE_EQ_S_COMPARE(CharArray, CharArrayEqual);
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, const Any *lhs) {
      const Any *rhs = WithOffset<Any>(obj_rhs, field);
      bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
      EnqueueAny(tasks, bind_free_vars, lhs, rhs, Append::Field(path, field->name));
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, ObjectRef *_lhs) {
      HandleObject(field, field_kind, _lhs->get(), WithOffset<ObjectRef>(obj_rhs, field)->get());
    }
    MLC_INLINE void operator()(MLCTypeField *field, StructureFieldKind field_kind, Optional<Object> *_lhs) {
      HandleObject(field, field_kind, _lhs->get(), WithOffset<Optional<Object>>(obj_rhs, field)->get());
    }
    inline void HandleObject(MLCTypeField *field, StructureFieldKind field_kind, Object *lhs, Object *rhs) {
      if (lhs || rhs) {
        bool bind_free_vars = this->obj_bind_free_vars || field_kind == StructureFieldKind::kBind;
        EnqueueTask(tasks, bind_free_vars, lhs, rhs, Append::Field(path, field->name));
      }
    }
    static void EnqueueAny(std::vector<Task> *tasks, bool bind_free_vars, const Any *lhs, const Any *rhs,
                           std::shared_ptr<SEqualPath> new_path) {
      int32_t type_index = lhs->GetTypeIndex();
      if (type_index != rhs->GetTypeIndex()) {
        MLC_CORE_EQ_S_ERR_OUT(lhs->GetTypeKey(), rhs->GetTypeKey(), new_path);
      }
      if (type_index == kMLCNone) {
        return;
      }
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCInt, int64_t, std::equal_to<int64_t>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCFloat, double, DoubleEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCPtr, VoidPtr, std::equal_to<const void *>(), lhs, rhs, new_path);
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCDataType, DLDataType, DataTypeEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCDevice, DLDevice, DeviceEqual, lhs, rhs, new_path);
      MLC_CORE_EQ_S_CMP_ANY(type_index == kMLCRawStr, CharArray, CharArrayEqual, lhs, rhs, new_path);
      if (type_index < kMLCStaticObjectBegin) {
        MLC_THROW(InternalError) << "Unknown type key: " << lhs->GetTypeKey();
      }
      EnqueueTask(tasks, bind_free_vars, lhs->operator Object *(), rhs->operator Object *(), new_path);
    }
    static void EnqueueTask(std::vector<Task> *tasks, bool bind_free_vars, Object *lhs, Object *rhs,
                            std::shared_ptr<SEqualPath> new_path) {
      int32_t lhs_type_index = lhs ? lhs->GetTypeIndex() : kMLCNone;
      int32_t rhs_type_index = rhs ? rhs->GetTypeIndex() : kMLCNone;
      if (lhs_type_index != rhs_type_index) {
        MLC_CORE_EQ_S_ERR_OUT(::mlc::base::TypeIndex2TypeKey(lhs_type_index),
                              ::mlc::base::TypeIndex2TypeKey(rhs_type_index), new_path);
      } else if (lhs_type_index == kMLCStr) {
        Str lhs_str(reinterpret_cast<StrObj *>(lhs));
        Str rhs_str(reinterpret_cast<StrObj *>(rhs));
        if (lhs_str != rhs_str) {
          MLC_CORE_EQ_S_ERR_OUT(lhs_str, rhs_str, new_path);
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
    const std::shared_ptr<SEqualPath> &path;
  };
  std::vector<Task> tasks;
  std::unordered_map<Object *, Object *> eq_lhs_to_rhs;
  std::unordered_map<Object *, Object *> eq_rhs_to_lhs;
  FieldComparator::EnqueueTask(&tasks, bind_free_vars, lhs, rhs, nullptr);
  while (!tasks.empty()) {
    MLCTypeInfo *type_info;
    std::shared_ptr<SEqualPath> path;
    {
      Task &task = tasks.back();
      type_info = task.type_info;
      path = task.path;
      lhs = task.lhs;
      rhs = task.rhs;
      bind_free_vars = task.bind_free_vars;
      if (task.err) {
        throw SEqualError(task.err->str().c_str(), path);
      }
      // check binding consistency: lhs -> rhs, rhs -> lhs
      auto it_lhs_to_rhs = eq_lhs_to_rhs.find(lhs);
      auto it_rhs_to_lhs = eq_rhs_to_lhs.find(rhs);
      bool exist_lhs_to_rhs = it_lhs_to_rhs != eq_lhs_to_rhs.end();
      bool exist_rhs_to_lhs = it_rhs_to_lhs != eq_rhs_to_lhs.end();
      // already proven equal
      if (exist_lhs_to_rhs && exist_rhs_to_lhs) {
        if (it_lhs_to_rhs->second == rhs && it_rhs_to_lhs->second == lhs) {
          tasks.pop_back();
          continue;
        } else {
          throw SEqualError("Inconsistent binding: LHS and RHS are both bound, but to different nodes", path);
        }
      }
      // inconsistent binding
      if (exist_lhs_to_rhs) {
        throw SEqualError("Inconsistent binding. LHS has been bound to a different node while RHS is not bound", path);
      }
      if (exist_rhs_to_lhs) {
        throw SEqualError("Inconsistent binding. RHS has been bound to a different node while LHS is not bound", path);
      }
      if (!task.visited) {
        task.visited = true;
      } else {
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
    }
    int64_t task_index = static_cast<int64_t>(tasks.size()) - 1;
    if (type_info->type_index == kMLCList) {
      UListObj *lhs_list = reinterpret_cast<UListObj *>(lhs);
      UListObj *rhs_list = reinterpret_cast<UListObj *>(rhs);
      int64_t lhs_size = lhs_list->size();
      int64_t rhs_size = rhs_list->size();
      for (int64_t i = (lhs_size < rhs_size ? lhs_size : rhs_size) - 1; i >= 0; --i) {
        FieldComparator::EnqueueAny(&tasks, bind_free_vars, &lhs_list->at(i), &rhs_list->at(i),
                                    Append::ListIndex(path, i));
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
        UDictObj::Iterator rhs_it;
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
        FieldComparator::EnqueueAny(&tasks, bind_free_vars, &kv.second, &rhs_it->second,
                                    Append::DictKey(path, lhs_key));
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
      VisitStructure(lhs, type_info, FieldComparator{rhs, &tasks, bind_free_vars, path});
    }
  }
}

#undef MLC_CORE_EQ_S_COMPARE_OPT
#undef MLC_CORE_EQ_S_COMPARE
#undef MLC_CORE_EQ_S_ERR_OUT
#undef MLC_CORE_EQ_S_CMP_ANY

} // namespace core
} // namespace mlc

#endif // MLC_CORE_STRUCTURE_H_
