#ifndef MLC_CORE_TENSOR_H_
#define MLC_CORE_TENSOR_H_

#include "./func.h"
#include "./object.h"
#include "./typing.h"
#include <numeric>

namespace mlc {

struct TensorObj : public MLCTensor {
  struct Allocator;

  explicit TensorObj() : MLCTensor{} {}
  explicit TensorObj(DLManagedTensor *ext) : MLCTensor{} {
    this->tensor = ext->dl_tensor;
    this->manager_ctx = ext;
    this->_Init();
  }

  explicit TensorObj(DLManagedTensorVersioned *ext) : MLCTensor{} {
    this->tensor = ext->dl_tensor;
    this->manager_ctx = ext;
    this->_Init();
  }

  ~TensorObj() { delete[] this->tensor.shape; }

  Str ToBytes() const {
    static auto func = ::mlc::base::GetGlobalFuncCall<1>("mlc.core.TensorToBytes");
    return func({this});
  }

  static Ref<TensorObj> FromBytes(const Str &source) {
    static auto func = ::mlc::base::GetGlobalFuncCall<1>("mlc.core.TensorFromBytes");
    return func({source});
  }

  Str ToBase64() const {
    static auto func = ::mlc::base::GetGlobalFuncCall<1>("mlc.core.TensorToBase64");
    return func({this});
  }

  static Ref<TensorObj> FromBase64(const Str &source) {
    static auto func = ::mlc::base::GetGlobalFuncCall<1>("mlc.core.TensorFromBase64");
    return func({source});
  }

  DLManagedTensor *DLPack() {
    ::mlc::base::IncRef(&this->_mlc_header);
    // N.B. This leaks memory if the resulting DLManagedTensor's deleter is not called.
    DLManagedTensor *ret = new DLManagedTensor();
    ret->dl_tensor = this->tensor;
    ret->manager_ctx = this;
    ret->deleter = +[](DLManagedTensor *dl) {
      TensorObj *self = static_cast<TensorObj *>(dl->manager_ctx);
      ::mlc::base::DecRef(&self->_mlc_header);
      delete dl;
    };
    return ret;
  }

  ::mlc::Str __str__() const {
    std::ostringstream oss;
    oss << "<mlc.Tensor";
    oss << " " << mlc::base::DType::Str(tensor.dtype);
    oss << "[";
    for (int i = 0; i < tensor.ndim; ++i) {
      if (i != 0) {
        oss << ", ";
      }
      oss << this->tensor.shape[i];
    }
    oss << "]";
    if (tensor.strides) {
      oss << " strides = [";
      for (int i = 0; i < tensor.ndim; ++i) {
        if (i != 0) {
          oss << ", ";
        }
        oss << this->tensor.strides[i];
      }
      oss << "]";
    }
    if (tensor.byte_offset) {
      oss << " byte_offset = " << tensor.byte_offset;
    }
    oss << " @ " << ::mlc::base::DeviceToStr(tensor.device);
    oss << ">";
    return ::mlc::Str(oss.str());
  }

  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, TensorObj, Object, MLCTypeIndex::kMLCTensor, "mlc.core.Tensor");

private:
  void _Init() {
    int32_t ndim = this->tensor.ndim;
    int64_t *shape = this->tensor.shape;
    int64_t *strides = this->tensor.strides;
    if (IsContiguous(ndim, shape, strides)) {
      strides = nullptr;
    }
    if (strides == nullptr) {
      int64_t *buffer = new int64_t[ndim + 1];
      std::copy(shape, shape + ndim, buffer);
      buffer[ndim] = -1;
      this->tensor.shape = buffer;
      this->tensor.strides = nullptr;
    } else {
      int64_t *buffer = new int64_t[ndim * 2 + 2];
      std::copy(shape, shape + ndim, buffer);
      buffer[ndim] = -1;
      std::copy(strides, strides + ndim, buffer + ndim + 1);
      buffer[ndim * 2 + 1] = -1;
      this->tensor.shape = buffer;
      this->tensor.strides = buffer + ndim + 1;
    }
  }
  static bool IsContiguous(int32_t ndim, int64_t *shape, int64_t *strides) {
    if (strides == nullptr) {
      return true;
    }
    int64_t stride = 1;
    for (int i = ndim - 1; i >= 0; --i) {
      if (shape[i] == 0) {
        return true;
      }
      if (shape[i] > 1 && strides[i] != stride) {
        return false;
      }
      stride *= shape[i];
    }
    return true;
  }
};

struct TensorObj::Allocator {
  MLC_INLINE static TensorObj *New(DLManagedTensor *ext) {
    TensorObj *ret = ::mlc::DefaultObjectAllocator<TensorObj>::New(ext);
    ret->_mlc_header.v.deleter = TensorObj::Allocator::Deleter_DLManagedTensor;
    return ret;
  }
  MLC_INLINE static TensorObj *New(DLManagedTensorVersioned *ext) {
    TensorObj *ret = ::mlc::DefaultObjectAllocator<TensorObj>::New(ext);
    ret->_mlc_header.v.deleter = TensorObj::Allocator::Deleter_DLManagedTensorVersioned;
    return ret;
  }

private:
  static void Deleter_DLManagedTensor(void *_self) {
    TensorObj *self = static_cast<TensorObj *>(_self);
    if (self->manager_ctx != nullptr) {
      DLManagedTensor *ext = static_cast<DLManagedTensor *>(self->manager_ctx);
      if (ext->deleter != nullptr) {
        ext->deleter(ext);
      }
    }
    ::mlc::DefaultObjectAllocator<TensorObj>::Deleter(self);
  }
  static void Deleter_DLManagedTensorVersioned(void *_self) {
    TensorObj *self = static_cast<TensorObj *>(_self);
    if (self->manager_ctx != nullptr) {
      DLManagedTensorVersioned *ext = static_cast<DLManagedTensorVersioned *>(self->manager_ctx);
      if (ext->deleter != nullptr) {
        ext->deleter(ext);
      }
    }
    ::mlc::DefaultObjectAllocator<TensorObj>::Deleter(self);
  }
};

struct Tensor : public ObjectRef {
  explicit Tensor(DLManagedTensorVersioned *ext) : Tensor(Tensor::New(ext)) {}
  explicit Tensor(DLManagedTensor *tensor) : Tensor(Tensor::New(tensor)) {}
  static Tensor FromBytes(const Str &source) { return Tensor(TensorObj::FromBytes(source)); }
  static Tensor FromBase64(const Str &source) { return Tensor(TensorObj::FromBase64(source)); }

  const void *data() const { return this->get()->tensor.data; }
  DLDevice device() const { return this->get()->tensor.device; }
  int32_t ndim() const { return this->get()->tensor.ndim; }
  DLDataType dtype() const { return this->get()->tensor.dtype; }
  const int64_t *shape() const { return this->get()->tensor.shape; }
  const int64_t *strides() const { return this->get()->tensor.strides; }
  uint64_t byte_offset() const { return this->get()->tensor.byte_offset; }
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Tensor, TensorObj, ObjectRef)
      .StaticFn("__init_DLManagedTensor",
                [](void *tensor) { return TensorObj::Allocator::New(static_cast<DLManagedTensor *>(tensor)); })
      .StaticFn("__init_DLManagedTensorVersioned",
                [](void *tensor) { return TensorObj::Allocator::New(static_cast<DLManagedTensorVersioned *>(tensor)); })
      .MemFn("__str__", &TensorObj::__str__);
};

namespace core {
inline int64_t ShapeToNumel(int32_t ndim, const int64_t *shape) {
  return std::accumulate(shape, shape + ndim, static_cast<int64_t>(1), std::multiplies<int64_t>());
}
} // namespace core
} // namespace mlc

#endif // MLC_CORE_TENSOR_H_
