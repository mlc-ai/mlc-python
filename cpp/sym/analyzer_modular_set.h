#ifndef MLC_SYM_ANALYZER_MODULAR_SET_H_
#define MLC_SYM_ANALYZER_MODULAR_SET_H_

#include <mlc/sym/all.h>

namespace mlc {
namespace sym {

struct ModularSetObj {
  MLCAny _mlc_header;
  int64_t coeff;
  int64_t base;
  explicit ModularSetObj(int64_t coeff, int64_t base) : _mlc_header{}, coeff(coeff), base(base) {}

  std::string __str__() const {
    std::ostringstream oss;
    oss << "ModularSet(coeff=" << coeff << ", base=" << base << ")";
    return oss.str();
  }

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, ModularSetObj, ::mlc::Object, "mlc.sym.ModularSet");
}; // struct ModularSetObj

struct ModularSet : public ::mlc::ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, ModularSet, ModularSetObj, ::mlc::ObjectRef)
      .Field("coeff", &ModularSetObj::coeff)
      .Field("base", &ModularSetObj::base)
      .MemFn("__str__", &ModularSetObj::__str__)
      .StaticFn("__init__", ::mlc::InitOf<ModularSetObj, int64_t, int64_t>);
  MLC_DEF_OBJ_REF_FWD_NEW(ModularSet)
}; // struct ModularSet

class ModularSetAnalyzer {
public:
  ModularSet operator()(const Expr &expr);
  void Update(const Var &var, const ModularSet &info, bool allow_override = false);

  explicit ModularSetAnalyzer(AnalyzerObj::Impl *parent);
  ~ModularSetAnalyzer();
  std::function<void()> EnterConstraint(const Expr &constraint);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sym
} // namespace mlc

#endif
