#include "./analyzer_impl.h"
#include <algorithm>
#include <memory>

namespace mlc {
namespace sym {

inline Expr ModImpl(Expr a, Expr b, DivMode mode) {
  if (mode == DivMode::kTruncDiv) {
    return truncmod(a, b);
  } else {
    return floormod(a, b);
  }
}

inline Expr DivImpl(Expr a, Expr b, DivMode mode) {
  if (mode == DivMode::kTruncDiv) {
    return truncdiv(a, b);
  } else {
    return floordiv(a, b);
  }
}

bool CastIsSafe(DLDataType dtype, Expr value, AnalyzerObj::Impl *analyzer) {
  if (!IsIndexType(dtype)) {
    return false;
  }
  ConstIntBound bound = analyzer->const_int_bound(value);
  int64_t ubound = max_value(dtype).DynCast<IntImmObj>()->value;
  int64_t lbound = min_value(dtype).DynCast<IntImmObj>()->value;
  if (value->dtype.bits <= dtype.bits || // upcast is safe
      (bound->max_value <= ubound && bound->min_value >= lbound)) {
    return true;
  }
  return false;
}

Expr SplitExprObj::NormalizeWithScale(int64_t sscale) const {
  Expr res = this->index;
  DLDataType dtype = this->dtype;
  if (this->scale == 0) {
    return Expr::Const(dtype, 0);
  }
  if (this->upper_factor != kPosInf) {
    res = ModImpl(res, Expr::Const(dtype, this->upper_factor), div_mode);
  }
  if (this->lower_factor != 1) {
    res = DivImpl(res, Expr::Const(dtype, this->lower_factor), div_mode);
  }
  sscale *= this->scale;
  if (sscale != 1) {
    // TODO: recover this
    // ICHECK(!dtype.is_uint() || sscale > 0);
    res = res * sscale;
  }
  return res;
}

bool SplitExprObj::CanPushCastToChildren(DLDataType dtype, AnalyzerObj::Impl *analyzer) const {
  // cast(dtype, index % upper_factor / lower_factor * scale) ==
  // cast(dtype, index) % upper_factor / lower_factor * scale
  // iff it is an upcast (dtype.bits >= self.dtype.bits) or all of
  // its intermediate results fit in the range of dtype
  if (dtype.bits >= this->dtype.bits) {
    return true; // upcast is safe
  }
  Expr res = this->index;
  if (this->scale == 0) {
    return true;
  }
  if (!CastIsSafe(dtype, res, analyzer)) {
    return false;
  }
  if (this->upper_factor != kPosInf) {
    res = ModImpl(res, Expr::Const(this->dtype, this->upper_factor), div_mode);
    if (!CastIsSafe(dtype, res, analyzer)) {
      return false;
    }
  }
  if (this->lower_factor != 1) {
    res = DivImpl(res, Expr::Const(this->dtype, this->lower_factor), div_mode);
    if (!CastIsSafe(dtype, res, analyzer)) {
      return false;
    }
  }
  if (this->scale != 1) {
    // TODO: recover
    // ICHECK(!this->dtype.is_uint() || this->scale > 0);
    res = res * this->scale;
    if (!CastIsSafe(dtype, res, analyzer)) {
      return false;
    }
  }
  return true;
}

void SplitExprObj::PushCastToChildren(DLDataType dtype) {
  this->index = cast(dtype, this->index);
  this->dtype = dtype;
}

bool SplitExprObj::IndexEqual(const SplitExpr &other) const {
  if (index.same_as(other->index))
    return true;
  return ExprDeepEqual()(index, other->index);
}

bool SplitExprObj::DivModeCompatibleTo(DivMode mode) const {
  if (this->div_mode == mode)
    return true;
  if (lower_factor == 1 && upper_factor == kPosInf)
    return true;
  return false;
}

Expr SumExprObj::Normalize() const {
  // quick path 1.
  if (this->args.size() == 0) {
    return Expr::Const(this->dtype, this->base);
  }
  return Normalize_(this->dtype, SimplifySplitExprs(args), base);
}

bool SumExprObj::DivisibleBy(int64_t scale) {
  if (base % scale != 0)
    return false;
  for (size_t i = 0; i < this->args.size(); ++i) {
    if (args[i]->scale % scale != 0)
      return false;
  }
  return true;
}

void SumExprObj::MulToSelf(int64_t scale) {
  this->base *= scale;
  for (size_t i = 0; i < this->args.size(); ++i) {
    args[i].CopyOnWrite()->scale *= scale;
  }
}

void SumExprObj::DivideBy(int64_t scale) {
  // TODO: recover
  // ICHECK_EQ(this->base % scale, 0);
  this->base /= scale;
  for (size_t i = 0; i < this->args.size(); ++i) {
    // TODO: recover
    // ICHECK_EQ(args[i]->scale % scale, 0);
    args[i].CopyOnWrite()->scale /= scale;
  }
}

void SumExprObj::AddToSelf(SplitExpr other, int64_t scale) {
  if (other->scale == 0)
    return;
  // We need to maintain the segment invariance:
  // Same index are stored close to each other.
  // sorted from big lower_factor to small one.
  size_t start = 0;
  for (; start < args.size(); ++start) {
    if (args[start]->IndexEqual(other))
      break;
  }
  for (size_t j = start; j < args.size(); ++j) {
    if (!args[j]->IndexEqual(other) || other->lower_factor > args[j]->lower_factor) {
      other.CopyOnWrite()->scale *= scale;
      this->args.insert(this->args.begin() + j, other);
      return;
    }
    if (other->lower_factor == args[j]->lower_factor && other->upper_factor == args[j]->upper_factor &&
        other->DivModeCompatibleTo(args[j]->div_mode)) {
      args[j].CopyOnWrite()->scale += other->scale * scale;
      return;
    }
  }
  // Insert other in the end.
  other.CopyOnWrite()->scale *= scale;
  this->args.emplace_back(std::move(other));
}

void SumExprObj::AddToSelf(const SumExpr &other, int64_t scale) {
  // NOTE: it is rare to have a balanced long expression,
  // linear scan is fine for our case.
  for (size_t i = 0; i < other->args.size(); ++i) {
    this->AddToSelf(other->args[i], scale);
  }
  this->AddToSelf(other->base * scale);
}

bool SumExprObj::CanPushCastToChildren(DLDataType dtype, AnalyzerObj::Impl *analyzer) const {
  bool is_min_value = dtype.bits == 64                                     //
                          ? base == std::numeric_limits<int64_t>::lowest() //
                          : base == -(1LL << (dtype.bits - 1));
  // cast(dtype, arg_1 + arg_2 + ... arg_n) ==
  // cast(dtype, arg_1) + ... + cast(dtype, arg_n)
  // iff it is an upcast (dtype.bits >= self.dtype.bits) or all of
  // its intermediate results fit in the range of dtype
  if (dtype.bits >= this->dtype.bits) {
    return true; // upcast is safe
  }
  Expr res = Expr::Const(dtype, 0);
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]->scale > 0) {
      res = res + args[i]->Normalize();
      if (!CastIsSafe(dtype, res, analyzer)) {
        return false;
      }
    }
  }
  if (base > 0 || is_min_value) {
    res = res + base;
    if (!CastIsSafe(dtype, res, analyzer)) {
      return false;
    }
  }
  // negative scales follows using sub.
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]->scale < 0) {
      res = res - args[i]->NormalizeWithScale(-1);
      if (!CastIsSafe(dtype, res, analyzer)) {
        return false;
      }
    }
  }
  if (base < 0 && !is_min_value) {
    res = res - (-base);
    if (!CastIsSafe(dtype, res, analyzer)) {
      return false;
    }
  }
  for (const auto &arg : args) {
    if (!arg->CanPushCastToChildren(dtype, analyzer)) {
      return false;
    }
  }
  return true;
}

void SumExprObj::PushCastToChildren(DLDataType dtype) {
  for (auto &arg : args) {
    arg.CopyOnWrite()->PushCastToChildren(dtype);
  }
  this->dtype = dtype;
}

std::vector<SplitExpr> SumExprObj::SimplifySplitExprs(std::vector<SplitExpr> args) {
  // NOTE: This algorithm relies on the factor that args are divided into segments
  // and each segment is sorted in descending order of lower_factor.
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]->scale == 0)
      continue;
    for (size_t j = i + 1; j < args.size(); ++j) {
      SplitExpr &lhs = args[i];
      SplitExpr &rhs = args[j];
      if (!lhs->IndexEqual(rhs))
        break;
      if (lhs->upper_factor < rhs->lower_factor)
        break;
      if (lhs->upper_factor == rhs->upper_factor && lhs->lower_factor == rhs->lower_factor &&
          lhs->DivModeCompatibleTo(rhs->div_mode)) {
        // folding same co-efficient.
        rhs.CopyOnWrite()->scale += lhs->scale;
        lhs.CopyOnWrite()->scale = 0;
      } else if (lhs->lower_factor == rhs->upper_factor && rhs->scale != 0 && lhs->scale % rhs->scale == 0 &&
                 lhs->lower_factor == (lhs->scale / rhs->scale) * rhs->lower_factor &&
                 lhs->DivModeCompatibleTo(rhs->div_mode)) {
        // Rules used in the proof:
        //
        // Rule 1:  (x % (c * s)) / c  =  (x / c) % s
        // Proof:
        //  x can always be decomposed into p * c * s + q * c + r
        //  where  0 <= q * c + r < c * s  and  0 <= r  <  c.
        //  Then, lhs = ((p * c * s + q * c + r) % (c * s)) / c = (q * c + r) / c = q
        //  rhs = ((p * c * s + q * c + r) / c) % s = (p * s + q) % s = q
        //  Thus, lhs = rhs
        //
        // The above proof is for the floordiv.
        // The same rule also holds for truncdiv(division rule in C).
        // Because both sides only involve mul, div and mod,
        // we can take abs of x, c and s, apply the floordiv proof,
        // and finally add the sign back.
        //
        // Rule 2:  (x / s) * s + x % s = x  (true for both trunc and floor div)
        //
        // General merge condition and proof:
        // - x = lhs->index % lhs->upper_factor
        // - s = lhs->scale / rhs->scale
        // - c = rhs->lower_factor
        //
        //    (x / (c * s)) * s + (x % (c * s)) / c
        // => ((x / c) / s) * s + ((x / c) % s)
        // => (x / c)
        //
        // Examples:
        //
        //    (z / 6) * 6 + ((z % 6) / 3) * 3
        // => ((z / 6) * 2 + (z % 6) / 3) * 3
        // => (z / 3) * 3
        // note: x = z, c = 3, s = 2
        //
        //    ((z % 12) / 6) * 6 + ((z % 6) / 3) * 3
        // => (((z % 12) / 6) * 2 + ((z % 12) % 6) / 3) * 3
        // => ((z % 12) / 3) * 3
        // note: x = z % 12, c = 3, s = 2
        // note also the invariance lhs->upper_factor % lhs->lower_factor == 0
        //
        SplitExprObj *merged = rhs.CopyOnWrite();
        merged->upper_factor = lhs->upper_factor;
        // reset args[i] to be zero.
        lhs.CopyOnWrite()->scale = 0;
        break;
      }
    }
  }
  // sort by the entry
  // Here we simply sort by descending order of scales.
  // For now, we do not compare by index because that comparison
  // can be runtime dependent and create inderminism.
  // we do not sort by index for now because it can be costly
  // to deep compare Exprs, and address of Vars can be runtime dependent.
  //
  auto fcompare = [](const SplitExpr &lhs, const SplitExpr &rhs) {
    // order by scale first
    if (lhs->scale > rhs->scale)
      return true;
    if (lhs->scale < rhs->scale)
      return false;
    // then order by factor
    if (lhs->lower_factor > rhs->lower_factor)
      return true;
    if (lhs->lower_factor < rhs->lower_factor)
      return false;
    // then order by upper factor
    if (lhs->upper_factor > rhs->upper_factor)
      return true;
    if (lhs->upper_factor < rhs->upper_factor)
      return false;
    // then order by div mode
    if (lhs->div_mode > rhs->div_mode)
      return true;
    if (lhs->div_mode < rhs->div_mode)
      return false;
    // tie.
    // TODO(tvm-team) We might consider index as the last comparison point,
    // after we make deep comparator more deterministic.
    // Specifically, we can consider comparing names of vars and break ties with address.
    return false;
  };
  std::stable_sort(args.begin(), args.end(), fcompare);
  return args;
}

Expr SumExprObj::Normalize_(DLDataType dtype, const std::vector<SplitExpr> &args, int64_t base) {
  bool is_min_value = dtype.bits == 64 //
                          ? base == std::numeric_limits<int64_t>::lowest()
                          : base == -(1LL << (dtype.bits - 1));
  // Positive scales first
  Expr res = Expr::Const(dtype, 0);
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]->scale > 0) {
      res = res + args[i]->Normalize();
    }
  }
  if (base > 0 || is_min_value) {
    res = res + base;
  }
  // negative scales follows using sub.
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]->scale < 0) {
      res = res - args[i]->NormalizeWithScale(-1);
    }
  }
  if (base < 0 && !is_min_value) {
    res = res - (-base);
  }
  return res;
}

struct CanonicalSimplifier::Impl : public RewriteSimplifier::Impl {
  using Rewriter = RewriteSimplifier::Impl;
  using Rewriter::VisitExpr_;

  explicit Impl(AnalyzerObj::Impl *parent) : Rewriter(parent) {}

  Expr CanonicalSimplify(Expr expr) {
    expr = operator()(expr);
    return expr;
  }

  // override the original mutate function.
  Expr VisitExpr(const Expr &input_expr) final {
    auto expr = Rewriter::VisitExpr(input_expr);
    return Normalize(expr);
  }

  // Normal mutation without normalization.
  Expr CanonicalMutate(Expr expr) { return Rewriter::VisitExpr(expr); }
  Expr VisitExpr_(const AddObj *op) final;
  Expr VisitExpr_(const SubObj *op) final;
  Expr VisitExpr_(const MulObj *op) final;
  Expr VisitExpr_(const DivObj *op) final;
  Expr VisitExpr_(const ModObj *op) final;
  Expr VisitExpr_(const FloorDivObj *op) final;
  Expr VisitExpr_(const FloorModObj *op) final;
  Expr VisitExpr_(const CastObj *op) final;
  Expr VisitExpr_(const LTObj *op) final;

private:
  SplitExpr SplitDivConst(SplitExpr lhs, int64_t cval, DivMode div_mode);
  SplitExpr SplitModConst(SplitExpr lhs, int64_t cval, DivMode div_mode);
  void SeparateDivisibleParts(const SumExprObj *psum, int64_t coeff, SumExpr *out_divisible,
                              SumExpr *out_non_divisible);
  bool ProdDivSimplify(Expr *lhs, Expr *rhs, Expr *common_scale);
  Expr Normalize(Expr expr) {
    if (const auto *op = expr.as<SplitExprObj>()) {
      return op->Normalize();
    } else if (const auto *op = expr.as<SumExprObj>()) {
      return op->Normalize();
    } else {
      return expr;
    }
  }
  SplitExpr ToSplitExpr(Expr expr) {
    if (auto op = expr.as<SplitExprObj>()) {
      return SplitExpr(op);
    }
    if (const auto *op = expr.as<SumExprObj>()) {
      if (op->base == 0 && op->args.size() == 1) {
        return op->args[0];
      }
    }
    if (const auto *op = expr.as<SplitExprObj>()) {
      expr = op->Normalize();
    } else if (const auto *op = expr.as<SumExprObj>()) {
      expr = op->Normalize();
    }
    return SplitExpr(expr->dtype, expr);
  }
  /*!
   * \brief Convert expr to an equivalent SplitExpr
   *        that has the specified div_mode.
   *
   * This function will return the same expr if its
   * div_mode already satisfies the need.
   *
   * \param expr The input expr.
   * \param div_mode The new div_mode.
   * \return The transformed SplitExpr.
   */
  SplitExpr ConvertDivMode(SplitExpr expr, DivMode div_mode) {
    if (expr->div_mode == div_mode)
      return expr;
    if (expr->DivModeCompatibleTo(div_mode)) {
      expr.CopyOnWrite()->div_mode = div_mode;
      return expr;
    }
    expr = ToSplitExpr(Normalize(expr));
    // TODO: recover
    // ICHECK(expr->DivModeCompatibleTo(div_mode));
    expr.CopyOnWrite()->div_mode = div_mode;
    return expr;
  }
  /*!
   * \brief Create a SumExpr from expr.
   * \param expr The input expr.
   * \return The transformed SumExpr.
   */
  SumExpr ToSumExpr(Expr expr) {
    if (auto op = expr.as<SumExprObj>()) {
      return SumExpr(op);
    }
    SumExpr n(expr->dtype);
    if (const auto *op = expr.as<IntImmObj>()) {
      n->base = op->value;
    } else {
      n->args.emplace_back(ToSplitExpr(expr));
      return SumExpr(n);
    }
    return n;
  }
};

Expr CanonicalSimplifier::Impl::VisitExpr_(const AddObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = Add::TryConstFold(a, b))
    return const_res.value();

  // canonical form simplification.
  SumExpr ret = ToSumExpr(std::move(a));

  if (const auto *op = b.as<IntImmObj>()) {
    ret.CopyOnWrite()->AddToSelf(op->value);
  } else if (auto op = b.as<SumExprObj>()) {
    ret.CopyOnWrite()->AddToSelf(SumExpr(op), 1);
  } else {
    ret.CopyOnWrite()->AddToSelf(ToSplitExpr(b), 1);
  }
  return ret;
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const SubObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = Sub::TryConstFold(a, b))
    return const_res.value();

  // canonical form simplification.
  SumExpr ret = ToSumExpr(std::move(a));

  if (const auto *op = b.as<IntImmObj>()) {
    ret.CopyOnWrite()->AddToSelf(-op->value);
  } else if (auto op = b.as<SumExprObj>()) {
    ret.CopyOnWrite()->AddToSelf(SumExpr(op), -1);
  } else {
    ret.CopyOnWrite()->AddToSelf(ToSplitExpr(b), -1);
  }
  return ret;
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const MulObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = Mul::TryConstFold(a, b))
    return const_res.value();

  // x * c
  if (a.as<IntImmObj>()) {
    std::swap(a, b);
  }
  if (const auto *bconst = b.as<IntImmObj>()) {
    if (a.as<SumExprObj>()) {
      SumExpr ret{a.as<SumExprObj>()};
      ret.CopyOnWrite()->MulToSelf(bconst->value);
      return ret;
    } else {
      SplitExpr ret = ToSplitExpr(std::move(a));
      ret.CopyOnWrite()->MulToSelf(bconst->value);
      return ret;
    }
  }

  // normal path.
  // this only happens when b is symbolic
  a = Normalize(a);
  b = Normalize(b);

  Expr ret = MulAndNormalize(a, b);
  const MulObj *mul = ret.as<MulObj>();

  if (mul && mul->a.same_as(op->a) && mul->b.same_as(op->b)) {
    return Expr(op);
  } else {
    return ret;
  }
}

void CanonicalSimplifier::Impl::SeparateDivisibleParts(const SumExprObj *psum, int64_t coeff, SumExpr *out_divisible,
                                                       SumExpr *out_non_divisible) {
  SumExpr divisible(psum->dtype);
  SumExpr non_divisible(psum->dtype);
  if (psum->base % coeff == 0) {
    divisible->base = psum->base;
  } else {
    non_divisible->base = psum->base;
  }
  for (const auto &e : psum->args) {
    if (e->scale % coeff == 0) {
      divisible->args.push_back(e);
    } else {
      non_divisible->args.push_back(e);
    }
  }
  *out_divisible = divisible;
  *out_non_divisible = non_divisible;
}

SplitExpr CanonicalSimplifier::Impl::SplitDivConst(SplitExpr lhs, int64_t cval, DivMode div_mode) {
  // TODO: recover
  // ICHECK_GT(cval, 0);
  lhs = ConvertDivMode(lhs, div_mode);

  // the following rule works for both floordiv and truncdiv
  if (lhs->scale % cval == 0) {
    lhs.CopyOnWrite()->scale /= cval;
    return lhs;
  }

  if (cval % lhs->scale == 0) {
    int64_t scaled_cval = cval / lhs->scale;
    if (lhs->upper_factor == kPosInf || lhs->upper_factor % (lhs->lower_factor * scaled_cval) == 0) {
      // directly fold division.
      lhs.CopyOnWrite()->scale = 1;
      lhs.CopyOnWrite()->lower_factor *= scaled_cval;
      lhs->Verify();
      return lhs;
    } else if (lhs->upper_factor <= (lhs->lower_factor * scaled_cval)) {
      // (x % c1) / c2  => 0 when c2 >= c1
      return ToSplitExpr(Expr::Const(lhs->dtype, 0));
    } else {
      // move the upper_factor modular into index.
      lhs.CopyOnWrite()->index = ModImpl(lhs->index, Expr::Const(lhs->dtype, lhs->upper_factor), div_mode);
      lhs.CopyOnWrite()->upper_factor = kPosInf;
      lhs.CopyOnWrite()->scale = 1;
      lhs.CopyOnWrite()->lower_factor *= scaled_cval;
      lhs->Verify();
      return lhs;
    }
  }
  // directly return the split with cval == 1
  lhs = ToSplitExpr(Normalize(lhs));
  // TODO: recover
  // ICHECK(lhs->DivModeCompatibleTo(div_mode));
  // ICHECK_EQ(lhs->scale, 1);
  lhs.CopyOnWrite()->lower_factor *= cval;
  lhs.CopyOnWrite()->div_mode = div_mode;
  return lhs;
}

bool CanonicalSimplifier::Impl::ProdDivSimplify(Expr *plhs, Expr *prhs, Expr *common_scale) {
  // the constant rhs case is covered by other simplifier so
  // we just skip to save the time
  if (prhs->as<IntImmObj>())
    return false;
  // collect lhs products and try to eliminate by matching them to prod in rhs
  // TODO: Support List<Optional<>>
  UList lhs_prods;
  Expr new_rhs = Expr::Const((*prhs)->dtype, 1);
  Expr new_common_scale = Expr::Const((*prhs)->dtype, 1);
  int64_t lhs_cscale = 1, rhs_cscale = 1;
  int num_elimination = 0;

  // collect lhs product and constant scale.
  auto fcollect_lhs = [&](Expr value) {
    if (auto *intimm = value.as<IntImmObj>()) {
      lhs_cscale *= intimm->value;
    } else {
      lhs_prods.push_back(value);
    }
  };
  UnpackReduction<MulObj>(*plhs, fcollect_lhs);

  // collect rhs product and try to eliminate when possible
  PEqualChecker<Expr> deep_equal;
  auto fcollect_rhs = [&](Expr value) {
    if (auto *intimm = value.as<IntImmObj>()) {
      rhs_cscale *= intimm->value;
    } else {
      // try eliminate from lhs
      for (int64_t i = 0; i < lhs_prods.size(); ++i) {
        if (ExprObj *lhs_prod = lhs_prods[i].as<ExprObj>()) {
          if (deep_equal(value, Expr(lhs_prod))) {
            lhs_prods[i] = Any();
            ++num_elimination;
            new_common_scale = new_common_scale * value;
            return;
          }
        }
      }
      // if elimination is not possible then construct the expression.
      new_rhs = new_rhs * value;
    }
  };
  UnpackReduction<MulObj>(*prhs, fcollect_rhs);
  // find gcd of const scales.
  int64_t cscale_gcd = ZeroAwareGCD(lhs_cscale, rhs_cscale);
  lhs_cscale /= cscale_gcd;
  rhs_cscale /= cscale_gcd;
  // if no elimination is possible
  if (num_elimination == 0 && cscale_gcd == 1)
    return false;

  // construct prod via canonical form
  Expr new_lhs = Expr::Const((*plhs)->dtype, 1);
  for (Optional<Expr> val : lhs_prods) {
    if (val.defined())
      new_lhs = new_lhs * val.value();
  }
  *plhs = new_lhs * lhs_cscale;
  *prhs = new_rhs * rhs_cscale;
  *common_scale = new_common_scale * cscale_gcd;
  return true;
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const DivObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }

  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = Div::TryConstFold(a, b))
    return const_res.value();
  PVar<IntImm> c1;
  // x / c1
  if (c1.Match(b) && c1.Eval()->value > 0) {
    int64_t cval = c1.Eval()->value;
    if (cval == 1)
      return a;

    if (const auto *psum = a.as<SumExprObj>()) {
      SumExpr lhs{Null}, extra{Null};
      SeparateDivisibleParts(psum, cval, &lhs, &extra);
      // can be divided by cval
      if (extra->IsZero()) {
        lhs.CopyOnWrite()->DivideBy(cval);
        return lhs;
      }
      // both lhs and extra are non-negative
      if (analyzer_->CanProveGreaterEqual(lhs->Normalize(), 0) &&
          analyzer_->CanProveGreaterEqual(extra->Normalize(), 0)) {
        lhs.CopyOnWrite()->DivideBy(cval);
        Expr temp = Normalize(extra);
        if (const auto *pconst = temp.as<IntImmObj>()) {
          lhs.CopyOnWrite()->AddToSelf(pconst->value / cval);
        } else {
          // if 0 <= extra < cval, it means the extra can be eliminated.
          if (TryCompare(temp, cval) != CompareResult::kLT) {
            lhs.CopyOnWrite()->AddToSelf(SplitDivConst(ToSplitExpr(temp), cval, DivMode::kTruncDiv), 1);
          }
        }
        return lhs;
      }
    } else {
      // if a >= 0 && a < cval, then result == 0
      auto cbound = analyzer_->const_int_bound(Normalize(a));
      if (cbound->min_value >= 0 && cbound->max_value < cval) {
        return Expr::Const(a->dtype, 0);
      }
    }
    return SplitDivConst(ToSplitExpr(std::move(a)), cval, DivMode::kTruncDiv);
  }
  // normal path
  a = Normalize(a);
  b = Normalize(b);
  Expr scale{Null};
  // note this is the case where b is not constant
  if (ProdDivSimplify(&a, &b, &scale)) {
    // use operator ver so it can constant fold if b == 1
    return truncdiv(a, b);
  }
  if (op->a.same_as(a) && op->b.same_as(b)) {
    return Expr(op);
  } else {
    return Div(a->dtype, a, b);
  }
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const FloorDivObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = FloorDiv::TryConstFold(a, b))
    return const_res.value();
  PVar<IntImm> c1;
  // x / c1
  if (c1.Match(b) && c1.Eval()->value > 0) {
    int64_t cval = c1.Eval()->value;
    if (cval == 1)
      return a;

    if (const auto *psum = a.as<SumExprObj>()) {
      SumExpr lhs{Null}, extra{Null};
      SeparateDivisibleParts(psum, cval, &lhs, &extra);
      if (extra->IsZero()) {
        lhs.CopyOnWrite()->DivideBy(cval);
        return lhs;
      }
      // continue simplification.
      lhs.CopyOnWrite()->DivideBy(cval);
      Expr temp = Normalize(extra);
      if (const auto *pconst = temp.as<IntImmObj>()) {
        lhs.CopyOnWrite()->AddToSelf(floordiv(pconst->value, cval));
      } else {
        // if 0 <= extra < cval, it means the extra can be eliminated.
        if (!(TryCompare(temp, cval) == CompareResult::kLT && analyzer_->CanProveGreaterEqual(temp, 0))) {
          lhs.CopyOnWrite()->AddToSelf(SplitDivConst(ToSplitExpr(temp), cval, DivMode::kFloorDiv), 1);
        }
      }
      return lhs;
    } else {
      // if a >= 0 && a < cval, then result == 0
      auto cbound = analyzer_->const_int_bound(Normalize(a));
      if (cbound->min_value >= 0 && cbound->max_value < cval) {
        return Expr::Const(a->dtype, 0);
      }
    }
    return SplitDivConst(ToSplitExpr(std::move(a)), cval, DivMode::kFloorDiv);
  }
  // normal path
  a = Normalize(a);
  b = Normalize(b);
  Expr scale{Null};
  if (ProdDivSimplify(&a, &b, &scale)) {
    // use operator ver so it can const fold.
    return floordiv(a, b);
  }
  if (op->a.same_as(a) && op->b.same_as(b)) {
    return Expr(op);
  } else {
    return FloorDiv(a->dtype, a, b);
  }
}

SplitExpr CanonicalSimplifier::Impl::SplitModConst(SplitExpr lhs, int64_t cval, DivMode div_mode) {
  // TODO: recover
  // ICHECK_GT(cval, 0);
  lhs = ConvertDivMode(lhs, div_mode);

  if (lhs->scale % cval == 0) {
    lhs.CopyOnWrite()->scale = 0;
    return lhs;
  }
  if (cval % lhs->scale == 0) {
    // The rationale:
    //   (index % upper) / lower * scale % cval, given cval = scaled_cval * scale
    //   by the rule (x * c1) % (c2 * c1) => (x % c2) * c1,
    // = (index % upper) / lower % scaled_cval * scale
    //   by the rule (x / c1) % c2  =>  (x % (c1 * c2)) / c1,
    // = (index % upper) % (new_upper_factor) / lower * scale
    int64_t scaled_cval = cval / lhs->scale;
    int64_t new_upper_factor = lhs->lower_factor * scaled_cval;
    // try to see if we can reduce the existing upper modular.
    if (lhs->upper_factor == kPosInf || lhs->upper_factor % new_upper_factor == 0) {
      // we gained a new upper factor that is smaller
      // than the original one
      // Perhaps there are more chances in simplifying the index
      // Do a recursive call to simplify the mod with the new factor.
      if (new_upper_factor < lhs->upper_factor && lhs->upper_factor != kPosInf) {
        auto updated =
            ToSplitExpr(this->VisitExpr(ModImpl(lhs->index, Expr::Const(lhs->dtype, new_upper_factor), div_mode)));
        // re-apply the lower_factor
        if (lhs->lower_factor != 1) {
          auto ret = SplitDivConst(updated, lhs->lower_factor, div_mode);
          ret.CopyOnWrite()->MulToSelf(lhs->scale);
          return ret;
        } else {
          updated.CopyOnWrite()->MulToSelf(lhs->scale);
          return updated;
        }
      } else {
        lhs.CopyOnWrite()->upper_factor = new_upper_factor;
        return lhs;
      }
    } else if (new_upper_factor % lhs->upper_factor == 0) {
      // (x % 2) % 4 => x % 2
      return lhs;
    }
  }
  // Normalize the value.
  lhs = ToSplitExpr(Normalize(lhs));
  // TODO: recover
  // ICHECK(lhs->DivModeCompatibleTo(div_mode));
  // ICHECK_EQ(lhs->scale, 1);
  // ICHECK_EQ(lhs->lower_factor, 1);
  lhs.CopyOnWrite()->div_mode = div_mode;
  lhs.CopyOnWrite()->upper_factor = cval;
  return lhs;
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const ModObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = Mod::TryConstFold(a, b))
    return const_res.value();

  PVar<IntImm> c1;
  // x % c1
  if (c1.Match(b) && c1.Eval()->value > 0) {
    int64_t cval = c1.Eval()->value;
    if (const auto *psum = a.as<SumExprObj>()) {
      SumExpr lhs{Null}, extra{Null};
      SeparateDivisibleParts(psum, cval, &lhs, &extra);
      if (extra->IsZero()) {
        return Expr::Const(a->dtype, 0);
      }
      // both lhs and extra are non-negative
      if (analyzer_->CanProveGreaterEqual(lhs->Normalize(), 0) &&
          analyzer_->CanProveGreaterEqual(extra->Normalize(), 0)) {
        Expr temp = Normalize(extra);
        if (temp.as<IntImmObj>()) {
          return truncmod(temp, c1.Eval());
        } else {
          // If temp < cval && temp >=0 then can remove the mod.
          if (TryCompare(temp, cval) == CompareResult::kLT) {
            return temp;
          } else {
            // continue to use logic below.
            a = extra;
            psum = a.as<SumExprObj>();
            // TODO: recover
            // ICHECK(psum != nullptr);
          }
        }
      }
      // Simplify the offset constant if necessary.
      // (x - 5) % 3 => (x - 2) % 3 if x - 5 >= 0
      auto cbound = analyzer_->const_int_bound(Normalize(a));
      int64_t new_base = psum->base % cval;
      if (cbound->min_value >= 0 && cbound->min_value - psum->base + new_base >= 0) {
        SumExpr sum_expr{a.as<SumExprObj>()};
        sum_expr.CopyOnWrite()->base = new_base;
        return SplitModConst(ToSplitExpr(std::move(sum_expr)), cval, DivMode::kTruncDiv);
      }
    } else {
      // if a >= 0 && a < cval, then result == 0
      auto cbound = analyzer_->const_int_bound(Normalize(a));
      if (cbound->min_value >= 0 && cbound->max_value < cval) {
        return a;
      }
    }
    return SplitModConst(ToSplitExpr(std::move(a)), cval, DivMode::kTruncDiv);
  }
  // normal path
  a = Normalize(a);
  b = Normalize(b);

  Expr scale{Null};
  if (ProdDivSimplify(&a, &b, &scale)) {
    // use operator version here so it can const fold b == 1
    return truncmod(a, b) * scale;
  }

  if (op->a.same_as(a) && op->b.same_as(b)) {
    return Expr(op);
  } else {
    return Mod(a->dtype, a, b);
  }
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const FloorModObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr a = this->CanonicalMutate(op->a);
  Expr b = this->CanonicalMutate(op->b);

  // const folding
  if (auto const_res = FloorMod::TryConstFold(a, b))
    return const_res.value();

  PVar<IntImm> c1;
  // x % c1
  if (c1.Match(b) && c1.Eval()->value > 0) {
    int64_t cval = c1.Eval()->value;
    if (const auto *psum = a.as<SumExprObj>()) {
      SumExpr lhs{Null}, extra{Null};
      SeparateDivisibleParts(psum, cval, &lhs, &extra);
      Expr temp = Normalize(extra);
      if (temp.as<IntImmObj>()) {
        return floormod(temp, c1.Eval());
      } else {
        // If temp < cval && temp >=0 then can remove the mod.
        if (TryCompare(temp, cval) == CompareResult::kLT && analyzer_->CanProveGreaterEqual(temp, 0)) {
          return temp;
        } else {
          // continue to use logic below.
          a = extra;
          psum = a.as<SumExprObj>();
          // TODO: recover
          // ICHECK(psum != nullptr);
        }
      }
      // Simplify the offset constant if necessary.
      // floormod(x - 5, 3) => floormod(x + 1, 3)
      int64_t new_base = floormod(psum->base, cval);
      SumExpr sum_expr{a.as<SumExprObj>()};
      sum_expr.CopyOnWrite()->base = new_base;
      return SplitModConst(ToSplitExpr(std::move(sum_expr)), cval, DivMode::kFloorDiv);
    } else {
      // if a >= 0 && a < cval, then result == a
      auto cbound = analyzer_->const_int_bound(Normalize(a));
      if (cbound->min_value >= 0 && cbound->max_value < cval) {
        return a;
      }
    }
    return SplitModConst(ToSplitExpr(std::move(a)), cval, DivMode::kFloorDiv);
  }
  // normal path
  a = Normalize(a);
  b = Normalize(b);

  Expr scale{Null};
  if (ProdDivSimplify(&a, &b, &scale)) {
    // use operator version here so it can const fold b == 1
    return floormod(a, b) * scale;
  }

  if (op->a.same_as(a) && op->b.same_as(b)) {
    return Expr(op);
  } else {
    return FloorMod(a->dtype, a, b);
  }
}

// Simplify reduce expression.

Expr CanonicalSimplifier::Impl::VisitExpr_(const CastObj *op) {
  if (!IsIndexType(op->dtype)) {
    return Rewriter::VisitExpr_(op);
  }
  // normalize
  Expr value = this->CanonicalMutate(op->value);
  // PushCastToChildren
  if (value.as<SumExprObj>()) {
    SumExpr se{value.as<SumExprObj>()};
    if (se->CanPushCastToChildren(op->dtype, analyzer_)) {
      se.CopyOnWrite()->PushCastToChildren(op->dtype);
      return se;
    }
  }
  if (value.as<SplitExprObj>()) {
    SplitExpr se{value.as<SplitExprObj>()};
    if (se->CanPushCastToChildren(op->dtype, analyzer_)) {
      se.CopyOnWrite()->PushCastToChildren(op->dtype);
      return se;
    }
  }
  return Rewriter::VisitExpr_(op);
}

Expr CanonicalSimplifier::Impl::VisitExpr_(const LTObj *op) {
  // First convert a < b into a - b < 0
  Expr expr = this->CanonicalMutate(op->a - op->b);
  // Case: x0 * s0 + x1 * s1 + ... + xn + c < 0, let d = gcd(s0, s1, ..., s{n-1}, c)
  // 1. if can prove -d < xn < d, then we can simplify
  //    the expression to x0 * (s0/d) + x1 * (s1/d) + ... + x{n-1} * (s{n-1}/d) < c/d,
  //    e.g. `x * 8 + y < 16` where `y` \in [0, 8), we can simplify it to `x < 2`
  // 2. if xn is in pattern of yn % m, where m % d == 0, convert it to yn // d % (m/d)
  //    e.g. `x1 * 64 + (x2 * 8 + x3) % 64 < 120`, `x3` \in [0, 8), we can simplify it to
  //    `x1 * 8 + (x2 * 8 + x3) // 8 % 8 < 15` ==> `x1 * 8 + x2 % 8 < 15`

  if (const auto *lhs = expr.as<SumExprObj>()) {
    int64_t gcd = lhs->base;
    bool has_non_one_scale = false;
    for (const SplitExpr &split_expr : lhs->args) {
      if (split_expr->scale > 1 || split_expr->scale < -1) {
        has_non_one_scale = true;
        gcd = ZeroAwareGCD(gcd, std::abs(split_expr->scale));
      }
    }
    // Skip if gcd == 1 or all s_n are 1
    if (!has_non_one_scale || gcd <= 1) {
      return Rewriter::VisitExpr_(op);
    }
    SumExpr divisible{Null}, extra{Null};
    SeparateDivisibleParts(lhs, gcd, &divisible, &extra);
    // TODO: recover this
    // ICHECK(DType::Equal(extra->dtype, divisible->dtype));
    Expr normal_extra = extra->Normalize();
    if (this->analyzer_->CanProve(normal_extra < gcd) && this->analyzer_->CanProve(normal_extra > -gcd)) {
      // Case 1. -d < xn < d
      divisible.CopyOnWrite()->DivideBy(gcd);
      return Rewriter::VisitExpr(divisible->Normalize() < 0);
    } else if (extra->args.size() == 1 && extra->args[0]->upper_factor != kPosInf &&
               extra->args[0]->upper_factor % (gcd * extra->args[0]->lower_factor) == 0) {
      // Case 2. xn == yn % m, where m % d == 0
      divisible.CopyOnWrite()->DivideBy(gcd);
      const auto split_expr = extra->args[0];
      int64_t lower_factor = gcd * extra->args[0]->lower_factor;
      Expr extra_expr = floormod(floordiv(split_expr->index, lower_factor), //
                                 floordiv(split_expr->upper_factor, lower_factor));
      return Rewriter::VisitExpr(divisible->Normalize() + extra_expr < 0);
    }
  }

  return Rewriter::VisitExpr_(op);
}

Expr CanonicalSimplifier::operator()(const Expr &expr) { return impl_->CanonicalSimplify(expr); }

void CanonicalSimplifier::Update(const Var &var, const Expr &info, bool override) {
  impl_->Update(var, info, override);
}

CanonicalSimplifier::CanonicalSimplifier(AnalyzerObj::Impl *parent) : impl_(std::make_unique<Impl>(parent)) {}
CanonicalSimplifier::~CanonicalSimplifier() {}

} // namespace sym
} // namespace mlc
