#include "./analyzer_impl.h"
#include <memory>
#include <unordered_set>

namespace mlc {
namespace sym {

enum class Key : size_t { kNonExist = std::numeric_limits<size_t>::max() };

struct Comparison {
  Comparison(Key lhs, Key rhs, int64_t offset, CompareResult result)
      : lhs_(lhs), rhs_(rhs), offset_(offset), result_(result) {
    // Normalize the comparison to remove LT and GT expressions,
    // reducing the number of operators that must be handled later.  By
    // eliminating LT and GT, instead of eliminating LE or GE, a
    // potential off-by-one error is avoided.
    //
    // For floating-point numbers, (x < y + c1) and (y < z + c2) implies
    // that (x < z + (c1 + c2)).  For integer types, which the
    // TransitiveComparisonAnalyzer is intended for use with integers,
    // LT or GT can give a tighter constraint, though with a less
    // convenient symmetry.
    //
    // i < j + c1, j < k + c2
    // i <= j + c1 - 1, j <= k + c2 - 1
    // i + 1 - c1 <= j, j <= k + c2 - 1
    // i + 1 - c1 <= k + c2 - 1
    // i <= k + c1 + c2 - 2
    // i < k + (c1 + c2 - 1)
    //
    // By always working with LE and GE comparisons, we avoid needing to
    // handle the offset of one that would be introduced by LT and GT at
    // all points of use.  The only point of use for LT and GT is when
    // normalizing comparisons (i.e. this constructor).

    if (result_ == CompareResult::kLT) {
      result_ = CompareResult::kLE;
      offset_ -= 1;
    }
    if (result_ == CompareResult::kGT) {
      result_ = CompareResult::kGE;
      offset_ += 1;
    }
  }
  static Comparison NonExist() { return Comparison(Key::kNonExist, Key::kNonExist, -1, CompareResult::kInconsistent); }
  operator bool() const { return lhs_ != Key::kNonExist; }
  bool IsNormalized() const { return result_ != CompareResult::kLT && result_ != CompareResult::kGT; }
  Comparison WithLHS(Key new_lhs) const {
    if (new_lhs == lhs_) {
      return *this;
    } else if (new_lhs == rhs_) {
      return Comparison(rhs_, lhs_, -offset_, Reverse(result_));
    } else {
      return Comparison::NonExist();
    }
  }
  Comparison Negated() const { return Comparison(lhs_, rhs_, offset_, Negate(result_)); }
  bool Implies(const Comparison &other) const {
    // TODO: recover this
    // ICHECK(lhs_ == other.lhs_);
    // ICHECK(rhs_ == other.rhs_);
    // ICHECK(IsNormalized());
    // ICHECK(other.IsNormalized());
    if (result_ == other.result_ && offset_ == other.offset_) {
      // if c1 == c2, x != y + c1 => x != y + c2
      // if c1 == c2, x == y + c1 => x == y + c2
      return true;
    }

    if (other.result_ == CompareResult::kLE && offset_ <= other.offset_) {
      if (result_ == CompareResult::kEQ || result_ == CompareResult::kLE) {
        // if c1 <= c2, x <= y + c1 => x <= y + c2
        // if c1 <= c2, x == y + c1 => x <= y + c2
        return true;
      }
    }

    if (other.result_ == CompareResult::kGE && offset_ >= other.offset_) {
      if (result_ == CompareResult::kEQ || result_ == CompareResult::kGE) {
        // if c1 >= c2, x == y + c1 => x >= y + c2
        // if c1 >= c2, x >= y + c1 => x >= y + c2
        return true;
      }
    }

    if (other.result_ == CompareResult::kNE) {
      if (result_ == CompareResult::kEQ && offset_ != other.offset_) {
        // if c1 != c2, x == y + c1 => x != y + c2
        return true;
      }

      if (result_ == CompareResult::kLE && offset_ < other.offset_) {
        // if c1 < c2, x <= y + c1 => x < y + c2 => x != y + c2
        return true;
      }

      if (result_ == CompareResult::kGE && offset_ > other.offset_) {
        // if c1 != c2, x >= y + c1 => x > y + c2 => x != y + c2
        return true;
      }
    }
    return false;
  }
  Key lhs_;
  Key rhs_;
  int64_t offset_{0};
  CompareResult result_{CompareResult::kInconsistent};
};

struct TransitiveComparisonAnalyzer::Impl {
  CompareResult TryCompare(const Expr &lhs, const Expr &rhs, bool propagate_inequalities = true) const;
  void Bind(const Var &var, const Expr &expr, bool allow_override = false);
  void Bind(const Var &var, const Range &expr, bool allow_override = false);
  std::function<void()> EnterConstraint(const Expr &expr);

private:
  Key ExprToKey(const Expr &expr);
  bool ExprToPreviousKey(const Expr &expr, Key *ret) const;
  std::unordered_map<Expr, Key, StructuralHash, StructuralEqual<false>> expr_to_key;
  Comparison FromExpr(const Expr &expr);
  void AddKnown(const Expr &expr, std::vector<Comparison> *vec);
  std::vector<Comparison> CollectDirectComparisons(Key lhs_key, Key rhs_key) const;
  std::vector<Comparison> CollectIndirectComparisons(Key lhs_key, Key rhs_key) const;
  std::vector<Comparison> DFSFromLHS(Key lhs_key, Key rhs_key) const;
  CompareResult MergeComparisons(const std::vector<Comparison> &lhs_to_rhs, int64_t offset) const;
  Dict<Var, Range> prev_bindings_;
  std::vector<Comparison> knowns_;
  std::vector<Comparison> scoped_knowns_;
};

std::tuple<Expr, Expr, int64_t> ExtractOffsets(const Expr &lhs, const Expr &rhs) {
  auto extract_offset = [](const Expr &expr) -> std::pair<Expr, int64_t> {
    PVar<Expr> x;
    PVar<IntImm> c;
    if ((x + c).Match(expr)) {
      return {x.Eval(), c.Eval()->value};
    } else if ((x - c).Match(expr)) {
      return {x.Eval(), -c.Eval()->value};
    } else if (c.Match(expr)) {
      IntImm c_val = c.Eval();
      return {Expr::Const(c_val->dtype, 0), c.Eval()->value};
    } else {
      return {expr, 0};
    }
  };
  auto lhs_split = extract_offset(lhs);
  auto rhs_split = extract_offset(rhs);
  return {lhs_split.first, rhs_split.first, rhs_split.second - lhs_split.second};
}

Comparison TransitiveComparisonAnalyzer::Impl::FromExpr(const Expr &expr) {
  CompareResult res;
  PVar<Expr> x, y;
  if ((x <= y).Match(expr)) {
    res = CompareResult::kLE;
  } else if ((x >= y).Match(expr)) {
    res = CompareResult::kGE;
  } else if ((x < y).Match(expr)) {
    res = CompareResult::kLT;
  } else if ((x > y).Match(expr)) {
    res = CompareResult::kGT;
  } else if ((x == y).Match(expr)) {
    res = CompareResult::kEQ;
  } else if ((x != y).Match(expr)) {
    res = CompareResult::kNE;
  } else {
    return Comparison::NonExist();
  }
  Expr lhs_expr = x.Eval();
  Expr rhs_expr = y.Eval();
  if (lhs_expr.as<IntImmObj>() && rhs_expr.as<IntImmObj>()) {
    return Comparison::NonExist();
  }
  auto [lhs, rhs, offset] = ExtractOffsets(lhs_expr, rhs_expr);
  Key lhs_key = ExprToKey(lhs);
  Key rhs_key = ExprToKey(rhs);
  return Comparison(lhs_key, rhs_key, offset, res);
}

bool TransitiveComparisonAnalyzer::Impl::ExprToPreviousKey(const Expr &expr, Key *ret) const {
  auto it = expr_to_key.find(expr);
  if (it != expr_to_key.end()) {
    *ret = it->second;
    return true;
  } else {
    return false;
  }
}

Key TransitiveComparisonAnalyzer::Impl::ExprToKey(const Expr &expr) {
  Key prev;
  if (ExprToPreviousKey(expr, &prev)) {
    return prev;
  }
  Key new_key = Key(expr_to_key.size());
  expr_to_key[expr] = new_key;
  return new_key;
}

TransitiveComparisonAnalyzer::TransitiveComparisonAnalyzer(AnalyzerObj::Impl *) : impl_(std::make_unique<Impl>()) {}
TransitiveComparisonAnalyzer::~TransitiveComparisonAnalyzer() {}

CompareResult TransitiveComparisonAnalyzer::TryCompare(const Expr &lhs, const Expr &rhs, bool propagate_inequalities) {
  return impl_->TryCompare(lhs, rhs, propagate_inequalities);
}

void TransitiveComparisonAnalyzer::Bind(const Var &var, const Expr &expr, bool allow_override) {
  impl_->Bind(var, expr, allow_override);
}
void TransitiveComparisonAnalyzer::Bind(const Var &var, const Range &range, bool allow_override) {
  impl_->Bind(var, range, allow_override);
}

std::function<void()> TransitiveComparisonAnalyzer::EnterConstraint(const Expr &constraint) {
  return impl_->EnterConstraint(constraint);
}

void TransitiveComparisonAnalyzer::Impl::AddKnown(const Expr &expr, std::vector<Comparison> *vec) {
  for (const auto &subexpr : ExtractConstraints(expr, false)) {
    if (Comparison cmp = FromExpr(subexpr)) {
      vec->push_back(cmp);
    }
  }
}

void TransitiveComparisonAnalyzer::Impl::Bind(const Var &var, const Range &range, bool allow_override) {
  auto it = prev_bindings_.find(var);
  if (it != prev_bindings_.end()) {
    ExprDeepEqual expr_equal;
    bool differs_from_previous =
        !expr_equal(range->min, (*it).second->min) || !expr_equal(range->extent, (*it).second->extent);
    if (differs_from_previous) {
      // TODO: recover
      (void)allow_override;
      // ICHECK(allow_override) << "Binding of variable " << var << " as " << range
      //                        << " conflicts with previous binding as " << (*it).second;
      Key key;
      if (ExprToPreviousKey(var, &key)) {
        knowns_.erase(
            std::remove_if(knowns_.begin(), knowns_.end(), [&](const auto &known) { return known.lhs_ == key; }),
            knowns_.end());
      }
    }
  }

  prev_bindings_.Set(var, range);
  if (IsConstInt(range->extent, 1)) {
    AddKnown(var == range->min, &knowns_);
  } else {
    AddKnown(var >= range->min, &knowns_);
    AddKnown(var < range->min + range->extent, &knowns_);
  }
}

void TransitiveComparisonAnalyzer::Impl::Bind(const Var &var, const Expr &expr, bool allow_override) {
  Bind(var, Range(expr, Expr::Const(expr->dtype, 1)), allow_override);
}

std::function<void()> TransitiveComparisonAnalyzer::Impl::EnterConstraint(const Expr &expr) {
  size_t old_literal_size = scoped_knowns_.size();
  AddKnown(expr, &scoped_knowns_);
  size_t new_literal_size = scoped_knowns_.size();

  auto frecover = [old_literal_size, new_literal_size, this]() {
    if (scoped_knowns_.size() != new_literal_size) {
      MLC_THROW(InternalError);
    }
    scoped_knowns_.erase(scoped_knowns_.begin() + old_literal_size, scoped_knowns_.end());
  };
  return frecover;
}

CompareResult TransitiveComparisonAnalyzer::Impl::TryCompare(const Expr &lhs_expr, const Expr &rhs_expr,
                                                             bool propagate_inequalities) const {
  // Currently only supports integer checks
  if (lhs_expr->dtype.code != kDLInt || rhs_expr->dtype.code != kDLInt) {
    return CompareResult::kUnknown;
  }
  // Bail out early if possible.  This int check should have been
  // constant-folded earlier, so this check shouldn't occur.
  auto *x_int = lhs_expr.as<IntImmObj>();
  auto *y_int = rhs_expr.as<IntImmObj>();
  if (x_int && y_int) {
    if (x_int->value < y_int->value) {
      return CompareResult::kLT;
    } else if (x_int->value > y_int->value) {
      return CompareResult::kGT;
    } else {
      return CompareResult::kEQ;
    }
  }
  auto [lhs, rhs, offset] = ExtractOffsets(lhs_expr, rhs_expr);
  Key lhs_key, rhs_key;
  if (!ExprToPreviousKey(lhs, &lhs_key) || !ExprToPreviousKey(rhs, &rhs_key)) {
    return CompareResult::kUnknown;
  }

  auto lhs_to_rhs = [&]() {
    if (propagate_inequalities) {
      return CollectIndirectComparisons(lhs_key, rhs_key);
    } else {
      return CollectDirectComparisons(lhs_key, rhs_key);
    }
  }();
  return MergeComparisons(lhs_to_rhs, offset);
}

std::vector<Comparison> TransitiveComparisonAnalyzer::Impl::CollectDirectComparisons(Key lhs_key, Key rhs_key) const {
  std::vector<Comparison> output;

  auto append_known = [&](Comparison cmp) {
    if (Comparison normalized = cmp.WithLHS(lhs_key)) {
      if (normalized.rhs_ == rhs_key) {
        output.push_back(normalized);
      }
    }
  };

  for (const auto &known : knowns_) {
    append_known(known);
  }
  for (const auto &known : scoped_knowns_) {
    append_known(known);
  }

  return output;
}

std::vector<Comparison> TransitiveComparisonAnalyzer::Impl::CollectIndirectComparisons(Key lhs_key, Key rhs_key) const {
  auto output = DFSFromLHS(lhs_key, rhs_key);
  for (Comparison cmp : DFSFromLHS(rhs_key, lhs_key)) {
    Comparison normalized = cmp.WithLHS(lhs_key);
    // TODO: recover
    // ICHECK(opt_normalized.has_value());
    output.push_back(normalized);
  }
  return output;
}

std::vector<Comparison> TransitiveComparisonAnalyzer::Impl::DFSFromLHS(Key lhs_key, Key rhs_key) const {
  // Everything in `to_visit` has lhs as its lhs.
  std::unordered_set<Key> seen;
  std::unordered_set<Key> to_visit;
  std::unordered_map<Key, std::vector<Comparison>> compared_to_lhs;
  // Utility function to add a new known statement
  auto declare_known = [&](Comparison cmp) {
    std::vector<Comparison> &knowns = compared_to_lhs[cmp.rhs_];

    // The comparison adds no new information, no modification
    // required.
    for (auto &prev_known : knowns) {
      if (prev_known.Implies(cmp)) {
        return;
      }
    }

    // New information may require visiting a new expression.
    if (cmp.rhs_ != rhs_key && !seen.count(cmp.rhs_)) {
      to_visit.insert(cmp.rhs_);
      seen.insert(cmp.rhs_);
    }

    // This comparison is a stronger version of a previous constraint.
    // Therefore, replace the old version entirely.
    for (auto &prev_known : knowns) {
      if (cmp.Implies(prev_known)) {
        prev_known = cmp;
        return;
      }
    }

    // Neither a superset nor a subset of previously known
    // constraints, must be tracked separately.
    knowns.push_back(cmp);
  };

  // Initialize the search based on any known (in)equalities that use
  // the LHS of the comparison.
  for (const auto &known : knowns_) {
    if (Comparison normalized = known.WithLHS(lhs_key)) {
      declare_known(normalized);
    }
  }
  for (const auto &known : scoped_knowns_) {
    if (Comparison normalized = known.WithLHS(lhs_key)) {
      declare_known(normalized);
    }
  }

  // Walk through the space of all comparisons that can be made with
  // LHS.
  while (to_visit.size()) {
    Key middle_key = *to_visit.begin();
    to_visit.erase(to_visit.begin());

    std::vector<Comparison> &prev_knowns_using_middle = compared_to_lhs.at(middle_key);
    // TODO: recover
    // ICHECK(compared_to_lhs.count(middle_key));

    std::vector<Comparison> new_knowns_using_lhs;

    auto attempt_transitive = [&](Comparison cmp) {
      // TODO: recover
      // ICHECK(cmp.IsNormalized());
      Key right_key = cmp.rhs_;
      if (right_key == lhs_key) {
        return;
      }
      for (const auto &prev : prev_knowns_using_middle) {
        CompareResult new_result = CompareResult::kUnknown;
        int64_t new_offset = prev.offset_ + cmp.offset_;

        if (prev.result_ == CompareResult::kEQ) {
          // x == y + c1 && y OP z + c2, x OP z + (c1 + c2)
          new_result = cmp.result_;
        } else if (cmp.result_ == CompareResult::kEQ) {
          // x OP y + c1 && y == z + c2, x OP z + (c1 + c2)
          new_result = prev.result_;
        } else if (prev.result_ == cmp.result_ &&
                   (prev.result_ == CompareResult::kLE || prev.result_ == CompareResult::kGE)) {
          // x <= y + c1 && y <= z + c2, x <= z + (c1 + c2)
          // x >= y + c1 && y >= z + c2, x >= z + (c1 + c2)
          //
          // This condition is much simpler to write than the
          // equivalent handling of < or of >, which is why the
          // inequalities are normalized to <= and to >=.  See
          // `Comparison::Comparison`
          // for further details.
          new_result = prev.result_;
        }

        if (new_result != CompareResult::kUnknown) {
          Comparison new_known(lhs_key, right_key, new_offset, new_result);
          new_knowns_using_lhs.push_back(new_known);
        }
      }
    };

    // Attempt to prove a new comparison using one of the original
    // known comparisons.  We want to find a known such that
    // `(LHS OP1 middle) && (middle OP2 right)` can be simplified
    // into `(LHS OP3 right)`.
    //
    // Note: The right side is this step is not necessarily the RHS of
    // the comparison we're trying to prove, as we may need to find
    // intermediate comparisons first.  For example, if we know that
    // `a<=b`, `b<=c`, and `c<=d`, and we wish to prove that `a<=d`,
    // we must first combine `a<=b` and `b<=c` into `a<=c`.  During
    // this first step, `b` is the "middle" and `c` is the "right".
    // The next step can then combind `a<=c` and `c<=d` into `a<=d`.
    for (const auto &known : knowns_) {
      if (Comparison cmp = known.WithLHS(middle_key)) {
        attempt_transitive(cmp);
      }
    }

    for (const auto &known : scoped_knowns_) {
      if (Comparison cmp = known.WithLHS(middle_key)) {
        attempt_transitive(cmp);
      }
    }

    // Collect together all new knowns, marking new nodes for visiting
    // as needed.
    for (const auto &new_known : new_knowns_using_lhs) {
      declare_known(new_known);
    }
  }

  if (auto it = compared_to_lhs.find(rhs_key); it != compared_to_lhs.end()) {
    return it->second;
  } else {
    // There are known comparisons involving the LHS and the RHS, but
    // no path that connects the two expressions.
    return {};
  }
}

CompareResult TransitiveComparisonAnalyzer::Impl::MergeComparisons(const std::vector<Comparison> &lhs_to_rhs,
                                                                   int64_t offset) const {
  // Just because we found a comparison involving LHS and RHS doesn't
  // mean that it's useful.  e.g. Knowing that `x < y` doesn't let us
  // prove whether `x + 5 < y`.
  CompareResult result = CompareResult::kUnknown;
  for (const auto &cmp : lhs_to_rhs) {
    switch (cmp.result_) {
    case CompareResult::kInconsistent:
      result = CompareResult::kInconsistent;
      break;

    case CompareResult::kEQ:
      if (offset == cmp.offset_) {
        result = result & CompareResult::kEQ;
      } else {
        result = result & CompareResult::kNE;
      }
      break;

    case CompareResult::kLE:
      if (cmp.offset_ < offset) {
        result = result & CompareResult::kLT;
      } else if (cmp.offset_ <= offset) {
        result = result & CompareResult::kLE;
      }
      break;

    case CompareResult::kGE:
      if (cmp.offset_ > offset) {
        result = result & CompareResult::kGT;
      } else if (cmp.offset_ >= offset) {
        result = result & CompareResult::kGE;
      }
      break;

    case CompareResult::kNE:
      if (offset == cmp.offset_) {
        result = result & CompareResult::kNE;
      }
      break;

    case CompareResult::kUnknown:
      break;

    case CompareResult::kGT:
    case CompareResult::kLT:
      MLC_THROW(InternalError) << "Normalized comparisons should only include <= and >=";
      MLC_UNREACHABLE();
    default:
      MLC_THROW(InternalError) << "Invalid CompareResult: " << static_cast<int>(cmp.result_);
      MLC_UNREACHABLE();
    }
  }
  return result;
}

} // namespace sym
} // namespace mlc
