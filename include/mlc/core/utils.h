#ifndef MLC_CORE_UTILS_H_
#define MLC_CORE_UTILS_H_

#include <mlc/base/all.h>
#include <vector>

#define MLC_SAFE_CALL_BEGIN()                                                                                          \
  try {                                                                                                                \
  (void)0

#define MLC_SAFE_CALL_END(err_ret)                                                                                     \
  return 0;                                                                                                            \
  }                                                                                                                    \
  catch (::mlc::Exception & err) {                                                                                     \
    *(err_ret) = std::move(err.data_);                                                                                 \
    return -2;                                                                                                         \
  }                                                                                                                    \
  catch (std::exception & err) {                                                                                       \
    *(err_ret) = err.what();                                                                                           \
    return -1;                                                                                                         \
  }                                                                                                                    \
  MLC_UNREACHABLE()

namespace mlc {
namespace core {

/********** Section 1. Nested type checking *********/

struct NestedTypeError : public std::runtime_error {
  explicit NestedTypeError(const char *msg) : std::runtime_error(msg) {}

  struct Frame {
    std::string expected_type;
    std::vector<AnyView> indices;
  };

  NestedTypeError &NewFrame(std::string expected_type) {
    frames.push_back(Frame{expected_type, {}});
    return *this;
  }

  NestedTypeError &NewIndex(AnyView index) {
    frames.back().indices.push_back(index);
    return *this;
  }

  void Format(std::ostream &os, std::string overall_expected) const {
    int32_t num_frames = static_cast<int32_t>(frames.size());
    if (num_frames == 1) {
      os << "Let input be `A: " << overall_expected //
         << "`. Type mismatch on `A";
      for (auto rit = frames[0].indices.rbegin(); rit != frames[0].indices.rend(); ++rit) {
        os << "[" << *rit << "]";
      }
      os << "`: " << this->what();
      return;
    }
    int32_t last_var = num_frames;
    os << "Let input be `A_0: " << overall_expected << "`";
    for (int32_t frame_id = num_frames - 1; frame_id >= 0; --frame_id) {
      const Frame &frame = frames[frame_id];
      if (frame_id == 0 && frame.indices.empty()) {
        last_var = num_frames - 1;
        break;
      }
      os << ", `A_" << (num_frames - frame_id) //
         << ": " << frame.expected_type        //
         << (frame_id == 0 ? " := A_" : " in A_") << (num_frames - frame_id - 1);
      for (auto rit = frame.indices.rbegin(); rit != frame.indices.rend(); ++rit) {
        os << "[" << *rit << "]";
      }
      if (frame_id > 0) {
        os << ".keys()";
      }
      os << "`";
    }
    os << ". Type mismatch on `A_" << last_var << "`: " << this->what();
  }

  std::vector<Frame> frames;
};

template <typename T> struct NestedTypeCheck {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};
template <typename T> struct NestedTypeCheck<List<T>> {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};
template <typename K, typename V> struct NestedTypeCheck<Dict<K, V>> {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};

} // namespace core
} // namespace mlc

#endif // MLC_CORE_UTILS_H_
