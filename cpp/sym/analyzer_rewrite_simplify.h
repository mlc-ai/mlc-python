#ifndef MLC_SYM_ANALYZER_REWRITE_SIMPLIFY_H_
#define MLC_SYM_ANALYZER_REWRITE_SIMPLIFY_H_

#include "./utils.h"
#include <mlc/sym/all.h>

namespace mlc {
namespace sym {

struct RewriteSimplifier {
  Expr operator()(const Expr &expr);
  void Update(const Var &var, const Expr &new_expr, bool allow_override = false);
  std::function<void()> EnterConstraint(const Expr &constraint);
  enum Extension {
    // No extensions enabled
    kNone = 0,
    /* When simplifying an inequality, attempt to use scope-based knowns.
     *
     * Example:
     * if_then_else(i<j && j<k, i<k, false) => if_then_else(i<j && j<k, true, false)
     */
    kTransitivelyProveInequalities = (1 << 0),
    /* When simplifying a boolean expression, convert to an AND of ORs
     * (conjunctive normal form).
     *
     * Example:
     *   (a && b) || c => (a || c) && (b || c)
     */
    kConvertBooleanToAndOfOrs = (1 << 1),
    /* When simplifying a boolean AND or a boolean OR, simplify each
     * branch under the assumption that the other branch does not
     * already dominate the result.  That is, simplify each branch of
     * (A && B) under the assumption that the other branch is true,
     * and simplify each branch of (A || B) under the assumption that
     * the other branch is false.
     *
     * Example:
     *   (n < 10) && (n < 5) => (n < 10)
     *   (n < 10) || (n < 5) => (n < 5)
     */
    kApplyConstraintsToBooleanBranches = (1 << 2),
    /* Special handling for expressions `(A+B)*C < (A*B)*D`
     *
     * Expressions of the form `(A+B)*C < (A*B)*D` can occur occur
     * when comparing the number of operations required for two
     * different orderings in which matrix multiplications can be
     * performed.  Proving or disproving this conditional allows an
     * optimal order of execution to be selected, even for dynamic
     * argument shapes.
     *
     * The default behavior of `ConstIntBounds` assumes that each term
     * in an expression is independent, and is insufficient to prove
     * these inequalities.  For example, the maximum value of `(A+B)*C
     * - (A*B)*D` is determined by taking the maximum value of
     * `(A+B)*C` and subtracting the minimum value of `(A*B)*D`.
     * While this algorithm can be applied in all cases, the bound it
     * provides is looser than strictly required.
     *
     * This extension adds a check for this case.  When `A`, `B`, `C`,
     * and `D` are all positive values, as is the case for tensor
     * shapes, the inequality can be written as `1/A + 1/B < D/C`.  If
     * this inequality holds for the minimum values of `A`, `B`, and
     * `D`, along with the maximum value of `C`, then the inequality
     * holds for all values.
     *
     * This extension requires little to no performance overhead, and
     * may be enabled by default in future releases.
     */
    kComparisonOfProductAndSum = (1 << 3),
  };
  void SetEnabledExtensions(Extension flags);
  Extension GetEnabledExtensions() const;
  void ResetStatsCounters();
  void SetMaximumRewriteSteps(int64_t maximum);

  explicit RewriteSimplifier(AnalyzerObj::Impl *parent);
  ~RewriteSimplifier();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/* \brief Usage counters for RewriteSimplifier
 *
 * These are intended for debug and testing purposes, to ensure that
 * Expr simplifications and TIR passes do not require an excessive
 */
struct RewriteSimplifierStats {
  int64_t nodes_visited{0};
  int64_t constraints_entered{0};
  int64_t rewrites_attempted{0};
  int64_t rewrites_performed{0};
  int64_t max_recursive_depth{0};
  int64_t num_recursive_rewrites{0};
};

/*!
 * \brief Rewrite-based simplifier.
 *
 * This class can be inheritated for other simplifiers.
 */
struct RewriteSimplifier::Impl : public IRMutatorWithAnalyzer {
public:
  using IRMutatorWithAnalyzer::VisitExpr_;

  explicit Impl(AnalyzerObj::Impl *parent) : IRMutatorWithAnalyzer(parent) {}

  Expr VisitExpr(const Expr &e) override;

  void Update(const Var &var, const Expr &info, bool override_info);
  Expr VisitExpr_(const AddObj *op) override;
  Expr VisitExpr_(const SubObj *op) override;
  Expr VisitExpr_(const MulObj *op) override;
  Expr VisitExpr_(const DivObj *op) override;
  Expr VisitExpr_(const ModObj *op) override;
  Expr VisitExpr_(const FloorDivObj *op) override;
  Expr VisitExpr_(const FloorModObj *op) override;
  Expr VisitExpr_(const MinObj *op) override;
  Expr VisitExpr_(const MaxObj *op) override;
  Expr VisitExpr_(const EQObj *op) override;
  Expr VisitExpr_(const NEObj *op) override;
  Expr VisitExpr_(const LTObj *op) override;
  Expr VisitExpr_(const LEObj *op) override;
  Expr VisitExpr_(const GTObj *op) override;
  Expr VisitExpr_(const GEObj *op) override;
  Expr VisitExpr_(const AndObj *op) override;
  Expr VisitExpr_(const OrObj *op) override;
  Expr VisitExpr_(const NotObj *op) override;
  Expr VisitExpr_(const SelectObj *op) override;
  Expr VisitExpr_(const CallObj *op) override;
  Expr VisitExpr_(const VarObj *op) override;
  Expr VisitExpr_(const CastObj *op) override;
  Expr VisitExpr_(const LetObj *op) override;

  std::function<void()> EnterConstraint(const Expr &constraint);

  /*! \brief Enable an optional extension or extensions
   *
   * \param flags A bitwise OR of all optional extensions that should
   * be enabled.
   */
  void SetEnabledExtensions(Extension flags);

  /*! \brief Return the currently enabled extensions */
  Extension GetEnabledExtensions() const;

  RewriteSimplifierStats GetStatsCounters() const { return RewriteSimplifierStats(stats_); }

  void ResetStatsCounters() { stats_ = {}; }

  void SetMaximumRewriteSteps(int64_t maximum) { maximum_rewrite_steps_ = maximum; }

protected:
  int64_t maximum_rewrite_steps_{0};
  RewriteSimplifierStats stats_;

  void RecordAttemptedRewrite() { stats_.rewrites_attempted++; }
  void RecordRewrite() {
    stats_.rewrites_performed++;
    // TODO: recover
    // ICHECK(maximum_rewrite_steps_ <= 0 || stats_.rewrites_performed <= maximum_rewrite_steps_)
    //     << "RewriteSimplifier exceeded maximum number of rewrites allowed (" << maximum_rewrite_steps_ << ")";
  }

  // counter to record recursive rewrite depth.
  int64_t recur_depth_{0};
  // internal variable map
  Dict<Var, Expr> var_map_;

  std::vector<Expr> literal_constraints_;

  // Optionally enabled extensions
  Extension enabled_extensions_{kNone};

  /*! Whether the simplifier is current
   */
  bool recursively_visiting_boolean_{false};

  // maximum number of recursion allowed during a single pass.
  static const constexpr int64_t kMaxRecurDepth = 5;
  /*!
   * \brief try to compare x against val.
   * \param x The expression to be evaluated.
   * \param val The constant value.
   * \return comparison result.
   */
  CompareResult TryCompare(const Expr &x, int64_t val);

  /*! Try to compare x against y
   *
   * \param x The lhs of the comparison
   * \param y The rhs of the comparison
   * \return comparison result.
   */
  CompareResult TryCompare(const Expr &x, const Expr &y);

  /*!
   * \brief Internal function to check whether or not to inline let.
   * \param op The let expr.
   * \return The inline decision.
   */
  bool CanInlineLet(const LetObj *op);

  /*! \brief Internal function to apply constraints
   *
   * Tests whether the expression is known to be true or false based
   * on existing constraints.  If the expression or its negation
   * matches a constraint, return the boolean it should be replaced
   * with.  Otherwise, return false.
   */
  Optional<Expr> TryMatchLiteralConstraint(const Expr &expr) const;

  /*! \brief Rewrite rules for Less Than comparisons
   *
   * These are separate from the VisitExpr_(const LTNode*) method, as
   * they may required from rewrites of LT or LE.
   */
  Expr ApplyRewriteRules(LT node);

  /*! \brief Rewrite rules for Equal comparisons
   *
   * These are separate from the VisitExpr_(const EQNode*) method, as
   * they may required from rewrites of LE or NE.
   */
  Expr ApplyRewriteRules(EQ node);

  /*! \brief Rewrite rules for Equal comparisons
   *
   * These are separate from the VisitExpr_(const EQNode*) method, as
   * they may required from rewrites of LT, LE, or NE.
   */
  Expr ApplyRewriteRules(Not node);

private:
  CompareResult TryCompareUsingKnownInequalities(const Expr &x, const Expr &y);
  CompareResult TryCompareUsingConstIntBounds(const Expr &x, const Expr y);
  CompareResult TryComparisonOfProductAndSum(const Expr &x, const Expr &y);

  // TODO: recover
  // Whether x >= val
  bool CanProveGreaterEqual(const Expr &x, int64_t val);
  // Whether x < val
  bool CanProveLess(const Expr &x, int64_t val);
  // Whether x == val. TODO(tqchen) refer back to super-analyzer.
  bool CanProveEqual(const Expr &x, int64_t val);
  // Whether x is true
  bool CanProve(const Expr &x);

  // Recursive rewrite x
  // we limit maximum depth of recursive rewrite allowed to
  // avoid infinite loop
  Expr RecursiveRewrite(const Expr &x) {
    stats_.num_recursive_rewrites++;
    if (recur_depth_ >= kMaxRecurDepth)
      return x;
    ++recur_depth_;
    stats_.max_recursive_depth = std::max(recur_depth_, stats_.max_recursive_depth);
    Expr res = this->VisitExpr(x);
    --recur_depth_;
    return res;
  }

  template <typename TA> PConstWithTypeLike<TA> ZeroWithTypeLike(const Pattern<TA> &pattern) {
    return PConstWithTypeLike<TA>(pattern.derived(), 0);
  }

  template <typename TA> PConstWithTypeLike<TA> OneWithTypeLike(const Pattern<TA> &pattern) {
    return PConstWithTypeLike<TA>(pattern.derived(), 1);
  }
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_REWRITE_SIMPLIFY_H_
