#ifndef MLC_SYM_ANALYZER_TRANSITIVE_COMPARISONS_H_
#define MLC_SYM_ANALYZER_TRANSITIVE_COMPARISONS_H_

#include "./utils.h"
#include <mlc/sym/all.h>

namespace mlc {
namespace sym {

class TransitiveComparisonAnalyzer {
public:
  CompareResult TryCompare(const Expr &lhs, const Expr &rhs, bool propagate_inequalities = true);
  void Bind(const Var &var, const Expr &expr, bool allow_override = false);
  void Bind(const Var &var, const Range &range, bool allow_override = false);
  std::function<void()> EnterConstraint(const Expr &constraint);

  explicit TransitiveComparisonAnalyzer(AnalyzerObj::Impl *analyzer);
  ~TransitiveComparisonAnalyzer();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_TRANSITIVE_COMPARISONS_H_
