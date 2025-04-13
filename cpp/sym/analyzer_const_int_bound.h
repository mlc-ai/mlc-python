#ifndef MLC_SYM_ANALYZER_CONST_INT_BOUND_H_
#define MLC_SYM_ANALYZER_CONST_INT_BOUND_H_

#include <mlc/sym/all.h>
#include <sstream>

namespace mlc {
namespace sym {

struct ConstIntBoundObj {
  MLCAny _mlc_header;
  int64_t min_value;
  int64_t max_value;
  explicit ConstIntBoundObj(int64_t min_value, int64_t max_value)
      : _mlc_header{}, min_value(min_value), max_value(max_value) {}

  std::string __str__() const {
    std::ostringstream oss;
    oss << "ConstIntBound[" << min_value << ", " << max_value << "]";
    return oss.str();
  }

  static constexpr int64_t kPosInf = std::numeric_limits<int64_t>::max();
  static constexpr int64_t kNegInf = -kPosInf;

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, ConstIntBoundObj, ::mlc::Object, "mlc.sym.ConstIntBound");
}; // struct ConstIntBoundObj

struct ConstIntBound : public ::mlc::ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, ConstIntBound, ConstIntBoundObj, ::mlc::ObjectRef)
      .Field("min_value", &ConstIntBoundObj::min_value)
      .Field("max_value", &ConstIntBoundObj::max_value)
      .MemFn("__str__", &ConstIntBoundObj::__str__)
      .StaticFn("__init__", ::mlc::InitOf<ConstIntBoundObj, int64_t, int64_t>);
  MLC_DEF_OBJ_REF_FWD_NEW(ConstIntBound)
}; // struct ConstIntBound

struct ConstIntBoundAnalyzer {
  using BoundMapType = Dict<Expr, ConstIntBound>;
  ConstIntBound operator()(const Expr &expr) const;
  ConstIntBound operator()(const Expr &expr, BoundMapType *bound);
  void Update(const Var &var, const ConstIntBound &info, bool allow_override = false);
  void Bind(const Var &var, const Range &range, bool allow_override = false);

  explicit ConstIntBoundAnalyzer(AnalyzerObj::Impl *parent);
  ~ConstIntBoundAnalyzer();
  std::function<void()> EnterConstraint(const Expr &constraint);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_CONST_INT_BOUND_H_
