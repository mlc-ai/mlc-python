#include "./analyzer_impl.h"
#include <memory>

namespace mlc {
namespace sym {

IntervalSet IntervalSetObj::Intersect(IntervalSetObj *b, AnalyzerObj::Impl *analyzer) const {
  Expr max_value_ = min(this->max_value, b->max_value);
  Expr min_value_ = max(this->min_value, b->min_value);
  auto int_or_uint = [](DLDataType dtype) { return dtype.code == kDLInt || dtype.code == kDLUInt; };
  if (int_or_uint(max_value_->dtype) && //
      int_or_uint(min_value_->dtype) && //
      analyzer->CanProve(max_value_ < min_value_)) {
    return IntervalSet::Empty();
  } else {
    return IntervalSet(min_value_, max_value_);
  }
}

IntervalSet IntervalSetObj::Union(IntervalSetObj *b, AnalyzerObj::Impl *) const {
  if (this->IsEmpty())
    return IntervalSet(b);
  if (b->IsEmpty())
    return IntervalSet(this);
  Expr max_value_ = max(this->max_value, b->max_value);
  Expr min_value_ = min(this->min_value, b->min_value);
  return IntervalSet(min_value_, max_value_);
}

bool IntervalSetObj::HasUpperBound() const { return !is_pos_inf(max_value) && !IsEmpty(); }
bool IntervalSetObj::HasLowerBound() const { return !is_neg_inf(min_value) && !IsEmpty(); }
bool IntervalSetObj::IsSinglePoint() const { return min_value.same_as(max_value); }
bool IntervalSetObj::IsEmpty() const { return is_pos_inf(min_value) || is_neg_inf(max_value); }
bool IntervalSetObj::IsEverything() const { return is_neg_inf(min_value) && is_pos_inf(max_value); }

IntervalSet IntervalSet::Everything() { return IntervalSet(neg_inf(), pos_inf()); }
IntervalSet IntervalSet::Empty() { return IntervalSet(pos_inf(), neg_inf()); }
IntervalSet IntervalSet::FromRange(const Range &range) {
  if (IsConstInt(range->extent, 1)) {
    return SinglePoint(range->min);
  }
  Expr range_max = range->extent + range->min;
  range_max = range_max - 1;
  return IntervalSet(range->min, range_max);
}
IntervalSet IntervalSet::Interval(Expr min, Expr max) {
  if (min.same_as(max)) {
    return IntervalSet::SinglePoint(min);
  }
  return IntervalSet(min, max);
}
IntervalSet IntervalSet::Intersect(const List<IntervalSet> &sets, AnalyzerObj::Impl *analyzer) {
  if (sets.size() == 0)
    return IntervalSet::Nothing();
  if (sets.size() == 1)
    return sets[0];
  IntervalSet x = sets[0];
  for (int64_t i = 1; i < sets.size(); ++i) {
    IntervalSet y = sets[i];
    x = x->Intersect(y.get(), analyzer);
  }
  Expr min_value = analyzer->Simplify(x->min_value);
  Expr max_value = analyzer->Simplify(x->max_value);
  return IntervalSet(min_value, max_value);
}

template <typename Op> inline IntervalSet Combine(AnalyzerObj::Impl *, IntervalSet a, IntervalSet b, DLDataType dtype) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    Expr expr{Null};
    if (auto res = Op::TryConstFold(a->min_value, b->min_value)) {
      expr = res.value();
    } else {
      expr = Op(a->min_value, b->min_value);
    }
    return IntervalSet::SinglePoint(expr);
  }
  if (Op::is_logical) {
    return IntervalSet(Expr::Const(dtype, 0), Expr::Const(dtype, 1));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  if (a->IsEverything())
    return a;
  if (b->IsEverything())
    return b;
  return IntervalSet::Everything();
}

template <> inline IntervalSet Combine<Add>(AnalyzerObj::Impl *, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(a->min_value + b->min_value);
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  Expr min_value = a->HasLowerBound() && b->HasLowerBound() ? a->min_value + b->min_value : neg_inf();
  Expr max_value = a->HasUpperBound() && b->HasUpperBound() ? a->max_value + b->max_value : pos_inf();
  return IntervalSet(min_value, max_value);
}

template <> inline IntervalSet Combine<Sub>(AnalyzerObj::Impl *, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(a->min_value - b->min_value);
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  Expr min_value = a->HasLowerBound() && b->HasUpperBound() ? a->min_value - b->max_value : neg_inf();
  Expr max_value = a->HasUpperBound() && b->HasLowerBound() ? a->max_value - b->min_value : pos_inf();
  return IntervalSet(min_value, max_value);
}

template <> inline IntervalSet Combine<Mul>(AnalyzerObj::Impl *analyzer, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(a->min_value * b->min_value);
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  if (a->IsSinglePoint()) {
    std::swap(a, b);
  }
  if (b->IsSinglePoint()) {
    if (IsConstInt(b->min_value, 0))
      return b;
    if (IsConstInt(b->min_value, 1))
      return a;
    if (analyzer->CanProveGreaterEqual(b->min_value, 0)) {
      Expr min_value = a->HasLowerBound() ? a->min_value * b->min_value : neg_inf();
      Expr max_value = a->HasUpperBound() ? a->max_value * b->min_value : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (analyzer->CanProveGreaterEqual(-b->min_value, 1)) {
      Expr min_value = a->HasUpperBound() ? a->max_value * b->min_value : neg_inf();
      Expr max_value = a->HasLowerBound() ? a->min_value * b->min_value : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (a->HasUpperBound() && a->HasLowerBound()) {
      Expr sign = b->min_value >= 0;
      Expr e1 = a->min_value * b->min_value;
      Expr e2 = a->max_value * b->min_value;
      return IntervalSet(Select(sign, e1, e2), Select(sign, e2, e1));
    }
  }
  return IntervalSet::Everything();
}

template <> inline IntervalSet Combine<Div>(AnalyzerObj::Impl *analyzer, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(truncdiv(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  if (b->IsSinglePoint()) {
    if (IsConstInt(b->min_value, 0)) {
      MLC_THROW(ValueError) << "Divide by zero in CombineInterval Div";
    }
    if (IsConstInt(b->min_value, 1))
      return a;
    // no relaxation is needed in here due to set is inclusive
    if (analyzer->CanProveGreaterEqual(b->min_value, 0)) {
      Expr min_value = a->HasLowerBound() ? truncdiv(a->min_value, b->min_value) : neg_inf();
      Expr max_value = a->HasUpperBound() ? truncdiv(a->max_value, b->min_value) : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (analyzer->CanProveGreaterEqual(-b->min_value, 1)) {
      Expr min_value = a->HasUpperBound() ? truncdiv(a->max_value, b->min_value) : neg_inf();
      Expr max_value = a->HasLowerBound() ? truncdiv(a->min_value, b->min_value) : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (a->HasUpperBound() && a->HasLowerBound()) {
      Expr sign = b->min_value >= 0;
      Expr e1 = truncdiv(a->min_value, b->min_value);
      Expr e2 = truncdiv(a->max_value, b->min_value);
      return IntervalSet(Select(sign, e1, e2), Select(sign, e2, e1));
    }
  }
  return IntervalSet::Everything();
}

template <> inline IntervalSet Combine<Mod>(AnalyzerObj::Impl *analyzer, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(truncmod(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;

  if (b->IsSinglePoint()) {
    const Expr &divisor = b->min_value;
    if (IsConstInt(divisor, 0)) {
      MLC_THROW(ValueError) << "Modular by zero in CombineInterval Mod";
    }
    // We need to add more bound constraints throughout the code.
    // The logic below assumes a is non-negative, which usually
    // is the case of our application.
    // TODO(tqchen): add bound constraints for a.
    if (analyzer->CanProveGreaterEqual(divisor, 0)) {
      return IntervalSet(Expr::Const(divisor->dtype, 0), divisor - 1);
    } else {
      Expr bound = abs(divisor) - 1;
      return IntervalSet(-bound, bound);
    }
  }
  return IntervalSet::Everything();
}

template <>
inline IntervalSet Combine<FloorDiv>(AnalyzerObj::Impl *analyzer, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(floordiv(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  if (b->IsSinglePoint()) {
    if (IsConstInt(b->min_value, 0)) {
      MLC_THROW(ValueError) << "Divide by zero in CombineInterval Div";
    }
    if (IsConstInt(b->min_value, 1))
      return a;
    // no relaxation is needed in here due to set is inclusive
    if (analyzer->CanProveGreaterEqual(b->min_value, 0)) {
      Expr min_value = a->HasLowerBound() ? floordiv(a->min_value, b->min_value) : neg_inf();
      Expr max_value = a->HasUpperBound() ? floordiv(a->max_value, b->min_value) : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (analyzer->CanProveGreaterEqual(-b->min_value, 1)) {
      Expr min_value = a->HasUpperBound() ? floordiv(a->max_value, b->min_value) : neg_inf();
      Expr max_value = a->HasLowerBound() ? floordiv(a->min_value, b->min_value) : pos_inf();
      return IntervalSet(min_value, max_value);
    } else if (a->HasUpperBound() && a->HasLowerBound()) {
      Expr sign = b->min_value >= 0;
      Expr e1 = floordiv(a->min_value, b->min_value);
      Expr e2 = floordiv(a->max_value, b->min_value);
      return IntervalSet(Select(sign, e1, e2), Select(sign, e2, e1));
    }
  }
  return IntervalSet::Everything();
}

template <>
inline IntervalSet Combine<FloorMod>(AnalyzerObj::Impl *analyzer, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(floormod(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;

  if (b->IsSinglePoint()) {
    const Expr &divisor = b->min_value;
    if (IsConstInt(divisor, 0)) {
      MLC_THROW(ValueError) << "Modular by zero in CombineInterval Mod";
    }
    if (analyzer->CanProveGreaterEqual(divisor, 0)) {
      if (divisor->IsInstance<IntImmObj>()) {
        // a mod b = a - (a / b) * b if a_max / b == a_min / b
        auto qmax = a->HasUpperBound() ? floordiv(a->max_value, divisor) : pos_inf();
        auto qmin = a->HasLowerBound() ? floordiv(a->min_value, divisor) : neg_inf();
        // We can compare +/- inf against each other, but cannot use
        // operator== between the symbolic limits and an integer.
        bool compatible_dtypes = !((qmin->dtype.code == kDLOpaqueHandle) ^ (qmax->dtype.code == kDLOpaqueHandle));
        if (compatible_dtypes && analyzer->CanProve(qmax == qmin)) {
          auto tmax = a->max_value - divisor * qmin;
          auto tmin = a->min_value - divisor * qmin;
          return IntervalSet(tmin, tmax);
        }
      }
      return IntervalSet(Expr::Const(divisor->dtype, 0), divisor - 1);
    } else {
      Expr bound = abs(divisor) - 1;
      return IntervalSet(-bound, bound);
    }
  }
  return IntervalSet::Everything();
}

template <> inline IntervalSet Combine<Max>(AnalyzerObj::Impl *, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(max(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  return IntervalSet(max(a->min_value, b->min_value), max(a->max_value, b->max_value));
}

template <> inline IntervalSet Combine<Min>(AnalyzerObj::Impl *, IntervalSet a, IntervalSet b, DLDataType) {
  if (a->IsSinglePoint() && b->IsSinglePoint()) {
    return IntervalSet::SinglePoint(min(a->min_value, b->min_value));
  }
  if (a->IsEmpty())
    return a;
  if (b->IsEmpty())
    return b;
  return IntervalSet(min(a->min_value, b->min_value), min(a->max_value, b->max_value));
}

struct IntervalSetEvaluator : public ExprFunctor<IntervalSet(const Expr &)> {
  IntervalSetEvaluator(AnalyzerObj::Impl *analyzer, const Dict<Var, IntervalSet> &dom_map,
                       const std::vector<std::pair<Var, IntervalSet>> *dom_constraints = nullptr, bool eval_vec = false)
      : analyzer_(analyzer), dom_map_(dom_map), dom_constraints_(dom_constraints), eval_vec_(eval_vec) {}

  IntervalSet Eval(const Expr &val) { return this->VisitExpr(val); }
  // evaluate and relax the set
  IntervalSet Eval(IntervalSet val) {
    // avoid recursive indefinite recursive expansion.
    if (recur_depth_ >= dom_map_.size())
      return val;
    ++recur_depth_;
    IntervalSet min_set = this->Eval(val->min_value);
    IntervalSet max_set = this->Eval(val->max_value);
    --recur_depth_;
    return IntervalSet(min_set->min_value, max_set->max_value);
  }

  IntervalSet VisitExpr_(const IntImmObj *op) final { return IntervalSet::SinglePoint(Expr(op)); }

  IntervalSet VisitExpr_(const VarObj *op) final {
    Var var(op);
    List<IntervalSet> values;
    if (dom_constraints_) {
      for (const auto &constraint : *dom_constraints_) {
        if (var.same_as(constraint.first)) {
          values.push_back(constraint.second);
        }
      }
    }
    auto it = dom_map_.find(var);
    if (it != dom_map_.end()) {
      values.push_back((*it).second);
    }
    if (values.empty()) {
      return IntervalSet::SinglePoint(var);
    }

    IntervalSet res = [&]() {
      if (values.size() == 1) {
        return values.front();
      } else {
        return IntervalSet::Intersect(values, this->analyzer_);
      }
    }();
    if (res->min_value.same_as(var) && res->max_value.same_as(var)) {
      return res;
    }
    // recursively evaluate mapped result
    // in case the domain contains variables to be relaxed.
    return Eval(res);
  }

  IntervalSet VisitExpr_(const AddObj *op) final { return VisitBinaryExpr_<Add>(op); }
  IntervalSet VisitExpr_(const SubObj *op) final { return VisitBinaryExpr_<Sub>(op); }
  IntervalSet VisitExpr_(const MulObj *op) final { return VisitBinaryExpr_<Mul>(op); }

  IntervalSet VisitExpr_(const DivObj *op) final { return VisitBinaryExpr_<Div>(op); }

  IntervalSet VisitExpr_(const ModObj *op) final { return VisitBinaryExpr_<Mod>(op); }

  IntervalSet VisitExpr_(const FloorDivObj *op) final { return VisitBinaryExpr_<FloorDiv>(op); }

  IntervalSet VisitExpr_(const FloorModObj *op) final { return VisitBinaryExpr_<FloorMod>(op); }

  IntervalSet VisitExpr_(const MinObj *op) final { return VisitBinaryExpr_<Min>(op); }

  IntervalSet VisitExpr_(const MaxObj *op) final { return VisitBinaryExpr_<Max>(op); }

  IntervalSet VisitExpr_(const EQObj *op) final { return VisitBinaryExpr_<EQ>(op); }

  IntervalSet VisitExpr_(const NEObj *op) final { return VisitBinaryExpr_<NE>(op); }

  IntervalSet VisitExpr_(const LTObj *op) final { return VisitBinaryExpr_<LT>(op); }

  IntervalSet VisitExpr_(const LEObj *op) final { return VisitBinaryExpr_<LE>(op); }

  IntervalSet VisitExpr_(const GTObj *op) final { return VisitBinaryExpr_<GT>(op); }

  IntervalSet VisitExpr_(const GEObj *op) final { return VisitBinaryExpr_<GE>(op); }

  IntervalSet VisitExpr_(const AndObj *op) final { return VisitBinaryExpr_<And>(op); }

  IntervalSet VisitExpr_(const OrObj *op) final { return VisitBinaryExpr_<Or>(op); }

  IntervalSet VisitExpr_(const RampObj *op) final {
    // TODO: recover
    // ICHECK(eval_vec_);
    (void)eval_vec_;
    IntervalSet base = Eval(op->base);
    PVar<IntImm> stride;
    if (stride.Match(op->stride)) {
      DLDataType t = op->base->dtype;
      int64_t vstride = stride.Eval()->value;
      int32_t lanes = static_cast<int32_t>(op->lanes);
      if (vstride > 0) {
        Expr min = Expr::Const(t, 0);
        Expr max = Expr::Const(t, vstride * (lanes - 1));
        return Combine<Add>(analyzer_, base, IntervalSet(min, max), op->dtype);
      } else {
        Expr min = Expr::Const(t, vstride * (lanes - 1));
        Expr max = Expr::Const(t, 0);
        return Combine<Add>(analyzer_, base, IntervalSet(min, max), op->dtype);
      }
    }
    return IntervalSet::Everything();
  }

  IntervalSet VisitExpr_(const BroadcastObj *op) final {
    // TODO: recover
    // ICHECK(eval_vec_);
    return VisitExpr(op->value);
  }

  IntervalSet VisitExpr_(const SelectObj *op) final {
    IntervalSet true_set = this->Eval(op->true_value);
    IntervalSet false_set = this->Eval(op->false_value);
    return false_set->Union(true_set.get(), analyzer_);
  }

  IntervalSet VisitExpr_(const CastObj *op) final {
    IntervalSet value_set = this->Eval(op->value);
    // short cut for the int set.
    if (value_set->min_value.same_as(value_set->max_value)) {
      if (value_set->IsEmpty())
        return value_set;
      return IntervalSet::SinglePoint(cast(op->dtype, value_set->min_value));
    }
    Expr min_value = value_set->HasLowerBound() ? cast(op->dtype, value_set->min_value) : neg_inf();
    Expr max_value = value_set->HasUpperBound() ? cast(op->dtype, value_set->max_value) : pos_inf();
    return IntervalSet(min_value, max_value);
  }

  IntervalSet VisitExpr_(const CallObj *) final { return IntervalSet::Everything(); }
  IntervalSet VisitExprDefault_(const Object *) final { return IntervalSet::Everything(); }

private:
  // whether set is exactly single point that equals value.
  bool MatchPoint(const IntervalSet &set, const Expr &value) const {
    return set->min_value.same_as(value) && set->max_value.same_as(value);
  }

  template <typename TOp, typename T> inline IntervalSet VisitBinaryExpr_(const T *op) {
    static_assert(std::is_same<typename TOp::TObj, T>::value, "constraint");
    IntervalSet a = this->Eval(op->a);
    IntervalSet b = this->Eval(op->b);
    if (MatchPoint(a, op->a) && MatchPoint(b, op->b)) {
      return IntervalSet::SinglePoint(Expr(op));
    }
    return Combine<TOp>(analyzer_, a, b, op->dtype);
  }

  // recursive depth
  int recur_depth_{0};
  // analyzer
  AnalyzerObj::Impl *analyzer_;
  const Dict<Var, IntervalSet> &dom_map_;
  const std::vector<std::pair<Var, IntervalSet>> *dom_constraints_;
  bool eval_vec_{false};
};

struct IntervalSetAnalyzer::Impl {
  explicit Impl(AnalyzerObj::Impl *analyzer) : analyzer_(analyzer) {}

  IntervalSet Eval(const Expr &expr, const Dict<Var, IntervalSet> &dom_map) const {
    return IntervalSetEvaluator(analyzer_, dom_map).Eval(expr);
  }

  IntervalSet Eval(const Expr &expr) const {
    return IntervalSetEvaluator(analyzer_, dom_map_, &dom_constraints_, true).Eval(expr);
  }

  void Bind(const Var &var, const Range &range, bool allow_override) {
    Update(var, IntervalSet::FromRange(range), allow_override);
  }
  void Bind(const Var &var, const Expr &expr, bool override_info) { //
    Update(var, Eval(expr), override_info);
  }
  void Update(const Var &var, const IntervalSet &info, bool override_info);
  std::function<void()> EnterConstraint(const Expr &constraint);

private:
  // Utility function to split a boolean condition into the domain
  // bounds implied by that condition.
  static std::vector<std::pair<Var, IntervalSet>> DetectBoundInfo(const Expr &cond);
  // The parent arith::Analyzer
  AnalyzerObj::Impl *analyzer_;
  // Map of variables to global variable bounds (e.g. loop iterator ranges)
  Dict<Var, IntervalSet> dom_map_;
  // List of implicit scope-dependent bounds (e.g. inside the body of
  // an if-statement).  Maintained as a list of constraints, rather
  // than as a `Map<Var,IntervalSet>`, to avoid computing an Intersection
  // until required.
  std::vector<std::pair<Var, IntervalSet>> dom_constraints_;
};

void IntervalSetAnalyzer::Impl::Update(const Var &var, const IntervalSet &info, bool can_override) {
  if (!can_override) {
    if (auto it = dom_map_.find(var); it != dom_map_.end()) {
      // TODO: recover
      // const IntervalSet &old_info = (*it).second;
      // ICHECK(ExprDeepEqual()(old_info.min(), info.min())) << "Trying to update var \'" << var << "\'"
      //                                                     << " with a different minimum value: "
      //                                                     << "original=" << old_info.min() << ", new=" << info.min();

      // ICHECK(ExprDeepEqual()(old_info.max(), info.max())) << "Trying to update var \'" << var << "\'"
      //                                                     << " with a different maximum value: "
      //                                                     << "original=" << old_info.max() << ", new=" << info.max();
    }
  }
  dom_map_.Set(var, info);
}

std::function<void()> IntervalSetAnalyzer::Impl::EnterConstraint(const Expr &constraint) {
  auto bounds = DetectBoundInfo(constraint);
  if (bounds.size() == 0)
    return nullptr;
  size_t old_size = dom_constraints_.size();
  dom_constraints_.insert(dom_constraints_.end(), bounds.begin(), bounds.end());
  size_t new_size = dom_constraints_.size();
  auto frecover = [old_size, new_size, this]() {
    if (dom_constraints_.size() != new_size) {
      MLC_THROW(InternalError);
    }
    dom_constraints_.erase(dom_constraints_.begin() + old_size, dom_constraints_.end());
  };
  return frecover;
}

std::vector<std::pair<Var, IntervalSet>> IntervalSetAnalyzer::Impl::DetectBoundInfo(const Expr &constraint) {
  PVar<Var> x;
  PVar<Expr> limit;

  std::vector<std::pair<Var, IntervalSet>> bounds;
  for (const Expr &subconstraint : ExtractConstraints(constraint)) {
    if ((x <= limit).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(SymbolicLimits::neg_inf_, limit.Eval())});
    } else if ((x < limit).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(SymbolicLimits::neg_inf_, limit.Eval() - 1)});
    } else if ((x >= limit).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(limit.Eval(), SymbolicLimits::pos_inf_)});
    } else if ((x > limit).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(limit.Eval() + 1, SymbolicLimits::pos_inf_)});
    } else if ((x == limit).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::SinglePoint(limit.Eval())});
    }

    if ((limit >= x).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(SymbolicLimits::neg_inf_, limit.Eval())});
    } else if ((limit > x).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(SymbolicLimits::neg_inf_, limit.Eval() - 1)});
    } else if ((limit <= x).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(limit.Eval(), SymbolicLimits::pos_inf_)});
    } else if ((limit < x).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::Interval(limit.Eval() + 1, SymbolicLimits::pos_inf_)});
    } else if ((limit == x).Match(subconstraint)) {
      bounds.push_back({x.Eval(), IntervalSet::SinglePoint(limit.Eval())});
    }
  }
  return bounds;
}

IntervalSetAnalyzer::IntervalSetAnalyzer(AnalyzerObj::Impl *parent) : impl_(std::make_unique<Impl>(parent)) {}
IntervalSetAnalyzer::~IntervalSetAnalyzer() {}
IntervalSet IntervalSetAnalyzer::operator()(const Expr &expr, const Dict<Var, IntervalSet> &dom_map) {
  return impl_->Eval(expr, dom_map);
}
IntervalSet IntervalSetAnalyzer::operator()(const Expr &expr) { return impl_->Eval(expr); }
void IntervalSetAnalyzer::Update(const Var &var, const IntervalSet &info, bool allow_override) {
  impl_->Update(var, info, allow_override);
}
void IntervalSetAnalyzer::Bind(const Var &var, const Range &range, bool allow_override) {
  impl_->Bind(var, range, allow_override);
}
std::function<void()> IntervalSetAnalyzer::EnterConstraint(const Expr &constraint) {
  return impl_->EnterConstraint(constraint);
}

} // namespace sym
} // namespace mlc
