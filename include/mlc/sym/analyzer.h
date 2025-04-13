#ifndef MLC_SYM_ANALYZER_H_
#define MLC_SYM_ANALYZER_H_

#include <mlc/sym/expr.h>
#include <mlc/sym/expr_functor.h>
#include <mlc/sym/op.h>

namespace mlc {
namespace sym {

struct AnalyzerObj {
  MLCAny _mlc_header;
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, AnalyzerObj, ::mlc::Object, "mlc.sym.Analyzer");

  struct Impl;
  struct Testing;
  enum class ProofStrength : int {
    kDefault = 0,
    kSymbolicBound = 1,
  };
  MLC_API AnalyzerObj();
  MLC_API ~AnalyzerObj();
  MLC_API void MarkGlobalNonNegValue(const Expr &value);
  MLC_API void Bind(const Var &var, const Expr &expr, bool allow_override = false);
  MLC_API void Bind(const Var &var, const Range &range, bool allow_override = false);
  MLC_API void Bind(const Dict<Var, Range> &variables, bool allow_override = false);
  MLC_API bool CanProveGreaterEqual(const Expr &expr, int64_t lower_bound);
  MLC_API bool CanProveLess(const Expr &expr, int64_t upper_bound);
  MLC_API bool CanProveEqual(const Expr &lhs, const Expr &rhs);
  MLC_API bool CanProveLessEqualThanSymbolicShapeValue(const Expr &lhs, const Expr &shape);
  MLC_API bool CanProve(const Expr &cond, ProofStrength strength = ProofStrength::kDefault);
  MLC_API Expr Simplify(const Expr &expr, int steps = 2);

private:
  friend struct IRMutatorWithAnalyzer;
  std::unique_ptr<Impl> impl_;
};

struct Analyzer : public ObjectRef {
  Analyzer() : ObjectRef(Analyzer::New()) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Analyzer, AnalyzerObj, ObjectRef)
      .StaticFn("__init__", InitOf<AnalyzerObj>)
      .MemFn("mark_global_non_neg_value", &AnalyzerObj::MarkGlobalNonNegValue)
      .MemFn("_bind_range", [](AnalyzerObj *self, const Var &var, Range range,
                               bool allow_override) { self->Bind(var, range, allow_override); })
      .MemFn("_bind_expr", [](AnalyzerObj *self, const Var &var, Expr expr,
                              bool allow_override) { self->Bind(var, expr, allow_override); })
      .MemFn("can_prove_greater_equal", &AnalyzerObj::CanProveGreaterEqual)
      .MemFn("can_prove_less", &AnalyzerObj::CanProveLess)
      .MemFn("can_prove_equal", &AnalyzerObj::CanProveEqual)
      .MemFn("can_prove_less_equal_than_symbolic_shape_value", &AnalyzerObj::CanProveLessEqualThanSymbolicShapeValue)
      .MemFn("can_prove",
             [](AnalyzerObj *self, const Expr &cond, int32_t strength) {
               return self->CanProve(cond, static_cast<AnalyzerObj::ProofStrength>(strength));
             })
      .MemFn("simplify", &AnalyzerObj::Simplify);
};

struct IRMutatorWithAnalyzer : public ExprMutator {
  explicit IRMutatorWithAnalyzer(AnalyzerObj *analyzer) : analyzer_(analyzer->impl_.get()) {}
  explicit IRMutatorWithAnalyzer(AnalyzerObj::Impl *analyzer) : analyzer_(analyzer) {}
  using ExprMutator::VisitExpr_;
  MLC_API Expr VisitExpr_(const LetObj *op) override;
  MLC_API Expr VisitExpr_(const SelectObj *op) override;
  MLC_API Expr VisitExpr_(const CallObj *op) override;

protected:
  AnalyzerObj::Impl *analyzer_;
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_H_
