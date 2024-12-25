#ifndef MLC_CORE_FIELD_VISITOR_H_
#define MLC_CORE_FIELD_VISITOR_H_

#include "./dict.h"
#include "./list.h"
#include "./object.h"
#include <functional>
#include <mlc/base/all.h>
#include <unordered_map>
#include <vector>

namespace mlc {
namespace core {

void ReportTypeFieldError(const char *type_key, MLCTypeField *field);

template <typename Visitor> inline void VisitFields(Object *root, MLCTypeInfo *info, Visitor &&visitor) {
  MLCTypeField *fields = info->fields;
  MLCTypeField *field = fields;
  for (; field->name != nullptr; ++field) {
    void *field_addr = reinterpret_cast<char *>(root) + field->offset;
    int32_t num_bytes = field->num_bytes;
    if (field->ty->type_index == kMLCTypingAny && num_bytes == sizeof(MLCAny)) {
      visitor(field, static_cast<Any *>(field_addr));
    } else if (field->ty->type_index == kMLCTypingAtomic) {
      int32_t type_index = reinterpret_cast<MLCTypingAtomic *>(field->ty)->type_index;
      if (type_index >= MLCTypeIndex::kMLCStaticObjectBegin && num_bytes == sizeof(MLCObjPtr)) {
        visitor(field, static_cast<ObjectRef *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 1) {
        visitor(field, static_cast<int8_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 2) {
        visitor(field, static_cast<int16_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 4) {
        visitor(field, static_cast<int32_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 8) {
        visitor(field, static_cast<int64_t *>(field_addr));
      } else if (type_index == kMLCFloat && num_bytes == 4) {
        visitor(field, static_cast<float *>(field_addr));
      } else if (type_index == kMLCFloat && num_bytes == 8) {
        visitor(field, static_cast<double *>(field_addr));
      } else if (type_index == kMLCPtr && num_bytes == sizeof(void *)) {
        visitor(field, static_cast<void **>(field_addr));
      } else if (type_index == kMLCDataType && num_bytes == sizeof(DLDataType)) {
        visitor(field, static_cast<DLDataType *>(field_addr));
      } else if (type_index == kMLCDevice && num_bytes == sizeof(DLDevice)) {
        visitor(field, static_cast<DLDevice *>(field_addr));
      } else if (type_index == kMLCRawStr) {
        visitor(field, static_cast<const char **>(field_addr));
      } else {
        ReportTypeFieldError(info->type_key, field);
      }
    } else if (field->ty->type_index == kMLCTypingPtr) { // TODO: support pointer type
      MLC_THROW(InternalError) << "Pointer type is not supported yet";
    } else if (field->ty->type_index == kMLCTypingOptional && num_bytes == sizeof(MLCObjPtr)) {
      MLCAny *ty = reinterpret_cast<MLCTypingOptional *>(field->ty)->ty.ptr;
      MLCTypingAtomic *ty_atomic = reinterpret_cast<MLCTypingAtomic *>(ty);
      if (ty->type_index == kMLCTypingAtomic) {
        if (ty_atomic->type_index >= kMLCStaticObjectBegin) {
          visitor(field, static_cast<Optional<ObjectRef> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCInt) {
          visitor(field, static_cast<Optional<int64_t> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCFloat) {
          visitor(field, static_cast<Optional<double> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCPtr) {
          visitor(field, static_cast<Optional<void *> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCDataType) {
          visitor(field, static_cast<Optional<DLDataType> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCDevice) {
          visitor(field, static_cast<Optional<DLDevice> *>(field_addr));
        } else {
          ReportTypeFieldError(info->type_key, field);
        }
      } else if (ty->type_index == kMLCTypingList || ty->type_index == kMLCTypingDict) {
        visitor(field, static_cast<Optional<ObjectRef> *>(field_addr));
      } else {
        ReportTypeFieldError(info->type_key, field);
      }
    } else if (field->ty->type_index == kMLCTypingList && num_bytes == sizeof(MLCObjPtr)) {
      visitor(field, static_cast<::mlc::ObjectRef *>(field_addr));
    } else if (field->ty->type_index == kMLCTypingDict && num_bytes == sizeof(MLCObjPtr)) {
      visitor(field, static_cast<::mlc::ObjectRef *>(field_addr));
    } else {
      ReportTypeFieldError(info->type_key, field);
    }
  }
}

template <typename Visitor> inline void VisitStructure(Object *root, MLCTypeInfo *info, Visitor &&visitor) {
  if (info->structure_kind == 0) {
    MLC_THROW(TypeError) << "Structure is not defined for type: " << info->type_key;
  }
  if (info->sub_structure_indices == nullptr) {
    return;
  }
  for (int32_t i_structure = 0; info->sub_structure_indices[i_structure] != -1; ++i_structure) {
    int32_t field_index = info->sub_structure_indices[i_structure];
    StructureFieldKind field_kind = static_cast<StructureFieldKind>(info->sub_structure_kinds[i_structure]);
    MLCTypeField *field = &info->fields[field_index];
    void *field_addr = reinterpret_cast<char *>(root) + field->offset;
    int32_t num_bytes = field->num_bytes;
    if (field->ty->type_index == kMLCTypingAny && num_bytes == sizeof(MLCAny)) {
      visitor(field, field_kind, static_cast<Any *>(field_addr));
    } else if (field->ty->type_index == kMLCTypingAtomic) {
      int32_t type_index = reinterpret_cast<MLCTypingAtomic *>(field->ty)->type_index;
      if (type_index >= MLCTypeIndex::kMLCStaticObjectBegin && num_bytes == sizeof(MLCObjPtr)) {
        visitor(field, field_kind, static_cast<ObjectRef *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 1) {
        visitor(field, field_kind, static_cast<int8_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 2) {
        visitor(field, field_kind, static_cast<int16_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 4) {
        visitor(field, field_kind, static_cast<int32_t *>(field_addr));
      } else if (type_index == kMLCInt && num_bytes == 8) {
        visitor(field, field_kind, static_cast<int64_t *>(field_addr));
      } else if (type_index == kMLCFloat && num_bytes == 4) {
        visitor(field, field_kind, static_cast<float *>(field_addr));
      } else if (type_index == kMLCFloat && num_bytes == 8) {
        visitor(field, field_kind, static_cast<double *>(field_addr));
      } else if (type_index == kMLCPtr && num_bytes == sizeof(void *)) {
        visitor(field, field_kind, static_cast<void **>(field_addr));
      } else if (type_index == kMLCDataType && num_bytes == sizeof(DLDataType)) {
        visitor(field, field_kind, static_cast<DLDataType *>(field_addr));
      } else if (type_index == kMLCDevice && num_bytes == sizeof(DLDevice)) {
        visitor(field, field_kind, static_cast<DLDevice *>(field_addr));
      } else if (type_index == kMLCRawStr) {
        visitor(field, field_kind, static_cast<const char **>(field_addr));
      } else {
        ReportTypeFieldError(info->type_key, field);
      }
    } else if (field->ty->type_index == kMLCTypingPtr) { // TODO: support pointer type
      MLC_THROW(InternalError) << "Pointer type is not supported yet";
    } else if (field->ty->type_index == kMLCTypingOptional && num_bytes == sizeof(MLCObjPtr)) {
      MLCAny *ty = reinterpret_cast<MLCTypingOptional *>(field->ty)->ty.ptr;
      MLCTypingAtomic *ty_atomic = reinterpret_cast<MLCTypingAtomic *>(ty);
      if (ty->type_index == kMLCTypingAtomic) {
        if (ty_atomic->type_index >= kMLCStaticObjectBegin) {
          visitor(field, field_kind, static_cast<Optional<ObjectRef> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCInt) {
          visitor(field, field_kind, static_cast<Optional<int64_t> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCFloat) {
          visitor(field, field_kind, static_cast<Optional<double> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCPtr) {
          visitor(field, field_kind, static_cast<Optional<void *> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCDataType) {
          visitor(field, field_kind, static_cast<Optional<DLDataType> *>(field_addr));
        } else if (ty_atomic->type_index == kMLCDevice) {
          visitor(field, field_kind, static_cast<Optional<DLDevice> *>(field_addr));
        } else {
          ReportTypeFieldError(info->type_key, field);
        }
      } else if (ty->type_index == kMLCTypingList || ty->type_index == kMLCTypingDict) {
        visitor(field, field_kind, static_cast<Optional<ObjectRef> *>(field_addr));
      } else {
        ReportTypeFieldError(info->type_key, field);
      }
    } else if (field->ty->type_index == kMLCTypingList && num_bytes == sizeof(MLCObjPtr)) {
      visitor(field, field_kind, static_cast<::mlc::ObjectRef *>(field_addr));
    } else if (field->ty->type_index == kMLCTypingDict && num_bytes == sizeof(MLCObjPtr)) {
      visitor(field, field_kind, static_cast<::mlc::ObjectRef *>(field_addr));
    } else {
      ReportTypeFieldError(info->type_key, field);
    }
  }
}

inline void TopoVisit(Object *root, std::function<void(Object *object, MLCTypeInfo *type_info)> pre_visit,
                      std::function<void(Object *object, MLCTypeInfo *type_info,
                                         const std::unordered_map<Object *, int32_t> &topo_indices)>
                          on_visit) {
  struct TopoInfo {
    Object *obj;
    MLCTypeInfo *type_info;
    int32_t topo_deps;
    std::vector<TopoInfo *> topo_parents;
  };
  struct TopoState {
    std::vector<std::unique_ptr<TopoInfo>> obj_list;
    std::unordered_map<const Object *, int32_t> obj2index;

    void TrackObject(TopoInfo *parent, Object *child) {
      TopoInfo *child_info = [this](Object *obj) -> TopoInfo * {
        if (auto it = obj2index.find(obj); it != obj2index.end()) {
          return obj_list[it->second].get();
        }
        int32_t obj_index = static_cast<int32_t>(obj_list.size());
        obj2index[obj] = obj_index;
        obj_list.push_back(std::make_unique<TopoInfo>());
        TopoInfo *info = obj_list.back().get();
        info->obj = obj;
        info->type_info = ::mlc::base::TypeIndex2TypeInfo(obj->GetTypeIndex());
        info->topo_deps = 0;
        return info;
      }(child);
      if (parent) {
        parent->topo_deps += 1;
        child_info->topo_parents.push_back(parent);
      }
    }
  };

  struct FieldExtractor {
    MLC_INLINE void operator()(MLCTypeField *, const Any *any) {
      if (any->type_index >= kMLCStaticObjectBegin) {
        state->TrackObject(current, any->operator Object *());
      }
    }
    MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) {
      if (Object *v = obj->get()) {
        state->TrackObject(current, v);
      }
    }
    MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) {
      if (Object *v = opt->get()) {
        state->TrackObject(current, v);
      }
    }
    MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *) {}
    MLC_INLINE void operator()(MLCTypeField *, Optional<double> *) {}
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *) {}
    MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *) {}
    MLC_INLINE void operator()(MLCTypeField *, int8_t *) {}
    MLC_INLINE void operator()(MLCTypeField *, int16_t *) {}
    MLC_INLINE void operator()(MLCTypeField *, int32_t *) {}
    MLC_INLINE void operator()(MLCTypeField *, int64_t *) {}
    MLC_INLINE void operator()(MLCTypeField *, float *) {}
    MLC_INLINE void operator()(MLCTypeField *, double *) {}
    MLC_INLINE void operator()(MLCTypeField *, DLDataType *) {}
    MLC_INLINE void operator()(MLCTypeField *, DLDevice *) {}
    MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *) {}
    MLC_INLINE void operator()(MLCTypeField *, void **) {}
    MLC_INLINE void operator()(MLCTypeField *, const char **) {}

    TopoState *state;
    TopoInfo *current;
  };

  TopoState state;
  // Step 1. Build dependency graph
  state.TrackObject(nullptr, root);
  for (size_t i = 0; i < state.obj_list.size(); ++i) {
    TopoInfo *current = state.obj_list[i].get();
    if (pre_visit) {
      pre_visit(current->obj, current->type_info);
    }
    if (UListObj *list = current->obj->TryCast<UListObj>()) {
      for (Any any : *list) {
        FieldExtractor{&state, current}(nullptr, &any);
      }
    } else if (UDictObj *dict = current->obj->TryCast<UDictObj>()) {
      for (auto &kv : *dict) {
        FieldExtractor{&state, current}(nullptr, &kv.first);
        FieldExtractor{&state, current}(nullptr, &kv.second);
      }
    } else if (int32_t type_index = current->type_info->type_index;
               type_index == kMLCStr || type_index == kMLCFunc || type_index == kMLCError) {
    } else {
      VisitFields(current->obj, current->type_info, FieldExtractor{&state, current});
    }
  }
  if (on_visit == nullptr) {
    // No need to topo-visit because `on_visit` is not provided
    return;
  }
  // Step 2. Enqueue nodes with no dependency
  std::vector<TopoInfo *> stack;
  stack.reserve(state.obj_list.size());
  for (size_t i = 0; i < state.obj_list.size(); ++i) {
    TopoInfo *current = state.obj_list[i].get();
    if (current->topo_deps == 0) {
      stack.push_back(current);
    }
  }
  // Step 3. Traverse the graph by topological order
  std::unordered_map<Object *, int32_t> topo_indices;
  size_t num_objects = 0;
  for (; !stack.empty(); ++num_objects) {
    TopoInfo *current = stack.back();
    stack.pop_back();
    // Step 3.1. Lable object index
    int32_t &topo_index = topo_indices[current->obj];
    if (topo_index != 0) {
      MLC_THROW(InternalError) << "This should never happen: object already visited";
    }
    topo_index = static_cast<int32_t>(num_objects);
    // Step 3.2. Visit object
    on_visit(current->obj, current->type_info, topo_indices);
    // Step 3.3. Decrease the dependency count of topo_parents
    for (TopoInfo *parent : current->topo_parents) {
      if (--parent->topo_deps == 0) {
        stack.push_back(parent);
      }
    }
  }
  if (num_objects != state.obj_list.size()) {
    MLC_THROW(ValueError) << "Can't topo-visit objects with circular dependency";
  }
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_FIELD_VISITOR_H_
