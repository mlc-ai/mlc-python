#include "./analyzer_impl.h"
#include "./utils.h"
#include <memory>

namespace mlc {
namespace sym {

// internal entry for const int bound
struct ModularSetAnalyzerEntry {
  int64_t coeff{1};
  int64_t base{0};

  ModularSetAnalyzerEntry() = default;

  ModularSetAnalyzerEntry(int64_t coeff, int64_t base) {
    if (coeff < 0) {
      // `analyzer->canonical_simplify()` can generate expressions with
      // negative coefficients (e.g. simplifying `floormod(-i, 2)`
      // into `floormod(i, -2) * -1`).  When this happens, the
      // ModularSet may enter a constraint based on this expression.
      //
      // Handling a negative coeff uses the same sign convention as
      // canonical_simplify, requiring that
      // `floormod(var, coeff) == -floormod(var, -coeff)`.
      coeff *= -1;
      base *= -1;
    }
    this->coeff = coeff;
    if (coeff != 0) {
      base = base % coeff;
      if (base < 0)
        base += coeff;
    }
    this->base = base;
  }

  bool is_const() const { return coeff == 0; }

  bool operator==(const ModularSetAnalyzerEntry &other) const { return coeff == other.coeff && base == other.base; }

  bool operator==(const ModularSet &other) const {
    return other.defined() && coeff == other->coeff && base == other->base;
  }
};

struct ModularSetAnalyzer::Impl : public ExprFunctor<ModularSetAnalyzerEntry(const Expr &)> {
public:
  explicit Impl(AnalyzerObj::Impl *parent) : parent_(parent) {}

  void Update(const Var &var, const ModularSet &info, bool allow_override) {
    if (!allow_override) {
      auto it = var_map_.find(var);
      if (it != var_map_.end()) {
        // TODO: recover
        // ICHECK(it->second == info) << "Trying to update var \'" << var << "\'"
        //                            << " with a different const bound: "
        //                            << "original=" << ModularSet(it->second.coeff, it->second.base) << ", new=" <<
        //                            info;
      }
    }
    var_map_[var] = ModularSetAnalyzerEntry(info->coeff, info->base);
  }

  // Detect useful constraints and use them in the analysis scope.
  std::function<void()> EnterConstraint(const Expr &constraint) {
    PVar<Var> var;
    PVar<IntImm> coeff, base;
    // pattern match interesting constraints
    if ((truncmod(var, coeff) == base).Match(constraint) || (floormod(var, coeff) == base).Match(constraint)) {
      ModularSetAnalyzerEntry entry(coeff.Eval()->value, base.Eval()->value);
      return UpdateByIntersect(var.Eval(), entry);
    }
    if ((var == base).Match(constraint) || (base == var).Match(constraint)) {
      ModularSetAnalyzerEntry entry(1, base.Eval()->value);
      return UpdateByIntersect(var.Eval(), entry);
    }
    return nullptr;
  }

  // Override visitor behaviors
  ModularSetAnalyzerEntry VisitExprDefault_(const Object *) final { return Everything(); }

  ModularSetAnalyzerEntry VisitExpr_(const LetObj *op) final {
    auto it = var_map_.find(op->var);
    // if the var has not been binded, update the info.
    if (it == var_map_.end()) {
      var_map_[op->var] = this->VisitExpr(op->value);
      ModularSetAnalyzerEntry ret = VisitExpr(op->body);
      var_map_.erase(op->var);
      return ret;
    } else {
      return VisitExpr(op->body);
    }
  }

  ModularSetAnalyzerEntry VisitExpr_(const CastObj *op) final { return VisitExpr(op->value); }

  ModularSetAnalyzerEntry VisitExpr_(const IntImmObj *op) final { return ModularSetAnalyzerEntry(0, op->value); }

  ModularSetAnalyzerEntry VisitExpr_(const AddObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->a);
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    int64_t coeff = ZeroAwareGCD(a.coeff, b.coeff);
    return ModularSetAnalyzerEntry(coeff, a.base + b.base);
  }

  ModularSetAnalyzerEntry VisitExpr_(const SubObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->a);
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    int64_t coeff = ZeroAwareGCD(a.coeff, b.coeff);
    return ModularSetAnalyzerEntry(coeff, a.base - b.base);
  }

  ModularSetAnalyzerEntry VisitExpr_(const MulObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->a);
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    // Simplification rule, x, y, z are in Z
    // (p x + n) (q y + m)
    // -> pq xy + pm x + qn y + mn
    // -> pq z + pm x + qn y + mn
    int64_t pq = a.coeff * b.coeff;
    int64_t pm = a.coeff * b.base;
    int64_t qn = a.base * b.coeff;
    int64_t coeff = ZeroAwareGCD(pq, ZeroAwareGCD(pm, qn));
    return ModularSetAnalyzerEntry(coeff, a.base * b.base);
  }

  ModularSetAnalyzerEntry DivByConst(const Expr &lhs, int64_t val, bool round_down) {
    ModularSetAnalyzerEntry a = VisitExpr(lhs);
    // TODO: recover
    // ICHECK_NE(val, 0);
    if (a.coeff % val == 0) {
      if (a.base == 0) {
        // a c x  / c -> a x
        return ModularSetAnalyzerEntry(std::abs(a.coeff / val), 0);
      }
      // positive division have a clear rounding mode.
      // Only handle case where we clearly know we need to round down.
      if (a.base > 0 && val > 0 && (round_down || parent_->CanProveGreaterEqual(lhs, 0))) {
        return ModularSetAnalyzerEntry(a.coeff / val, a.base / val);
      }
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const DivObj *op) final {
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    if (b.is_const()) {
      return DivByConst(op->a, b.base, false);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const FloorDivObj *op) final {
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    if (b.is_const()) {
      return DivByConst(op->a, b.base, true);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const MinObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->a);
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    return Union(a, b);
  }

  ModularSetAnalyzerEntry VisitExpr_(const MaxObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->a);
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    return Union(a, b);
  }

  ModularSetAnalyzerEntry VisitExpr_(const SelectObj *op) final {
    ModularSetAnalyzerEntry a = VisitExpr(op->true_value);
    ModularSetAnalyzerEntry b = VisitExpr(op->false_value);
    return Union(a, b);
  }

  ModularSetAnalyzerEntry ModByConst(const Expr &lhs, int64_t val, bool round_down) {
    ModularSetAnalyzerEntry a = VisitExpr(lhs);
    // TODO: recover
    // ICHECK_NE(val, 0);
    int64_t coeff = ZeroAwareGCD(a.coeff, val);
    if (a.base % coeff == 0 || (a.base > 0 && (round_down || parent_->CanProveGreaterEqual(lhs, 0)))) {
      return ModularSetAnalyzerEntry(coeff, a.base % coeff);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const FloorModObj *op) final {
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    if (b.is_const()) {
      return ModByConst(op->a, b.base, true);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const ModObj *op) final {
    ModularSetAnalyzerEntry b = VisitExpr(op->b);
    if (b.is_const()) {
      return ModByConst(op->a, b.base, false);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const CallObj *op) final {
    // only special handle >> which can be used for index calculation.
    if (Op_::right_shift->Same(op->op)) {
      return VisitRightShift(op);
    } else if (Op_::bitwise_and->Same(op->op)) {
      return VisitBitwiseAnd(op);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitExpr_(const VarObj *op) final {
    Var v(op);
    auto it = var_map_.find(v);
    if (it != var_map_.end()) {
      return it->second;
    } else {
      return Everything();
    }
  }

  ModularSetAnalyzerEntry VisitRightShift(const CallObj *op) {
    ModularSetAnalyzerEntry b = VisitExpr(op->args[1]);
    // a c x  / c -> a x
    if (b.is_const()) {
      return DivByConst(op->args[0], static_cast<int64_t>(1) << b.base, true);
    }
    return Everything();
  }

  ModularSetAnalyzerEntry VisitBitwiseAnd(const CallObj *op) {
    ModularSetAnalyzerEntry b = VisitExpr(op->args[1]);
    if (b.is_const()) {
      if (int32_t shift = CheckPowOfTwo(b.base + 1); shift != -1) {
        return ModByConst(op->args[0], static_cast<int64_t>(1) << shift, true);
      }
    }
    return Everything();
  }

private:
  /*! \brief pointer to parent. */
  AnalyzerObj::Impl *parent_{nullptr};
  // internal variable map
  std::unordered_map<Var, ModularSetAnalyzerEntry, ObjRefHash, ObjRefEqual> var_map_;
  /*!
   * \brief Update var by intersecting entry with var's current set.
   * \param var The variable.
   * \param entry The entry to be updated.
   * \return The recovery function of the scope.
   */
  std::function<void()> UpdateByIntersect(const Var &var, ModularSetAnalyzerEntry entry) {
    ModularSetAnalyzerEntry old = Everything();
    auto it = var_map_.find(var);
    if (it != var_map_.end()) {
      old = it->second;
    }
    var_map_[var] = Intersect(old, entry);
    // reover function.
    return [this, old, var]() { var_map_[var] = old; };
  }
  /*!
   * \brief Create union of two sets.
   * \param a The left operand.
   * \param b the right operand.
   */
  static ModularSetAnalyzerEntry Union(ModularSetAnalyzerEntry a, ModularSetAnalyzerEntry b) {
    // {ax + y} \cup {bz + h} => {gcd(a, b) x + {y or h}}
    int64_t coeff = ZeroAwareGCD(a.coeff, b.coeff);
    if (coeff == 0) {
      if (a.base == b.base)
        return a;
      return Everything();
    }
    int64_t base0 = a.base % coeff;
    int64_t base1 = b.base % coeff;
    if (base0 == base1) {
      return ModularSetAnalyzerEntry(coeff, base0);
    } else {
      return ModularSetAnalyzerEntry(ZeroAwareGCD(ZeroAwareGCD(base0, base1), coeff), base0);
    }
  }

  /*!
   * \brief Create interect of two sets.
   * \param a The left operand.
   * \param b the right operand.
   */
  static ModularSetAnalyzerEntry Intersect(ModularSetAnalyzerEntry a, ModularSetAnalyzerEntry b) {
    int64_t x, y;
    int64_t c1 = a.coeff, b1 = a.base, c2 = b.coeff, b2 = b.base;
    // z = c1 * p + b1
    // z = c2 * q + b2
    // c1 * x + c2 * y = gcd(c1, c2)
    // -> c1 * p - c2 * q = b2 - b1
    // -> p = (b2 - b1) / gcd * x
    // -> q = (b2 - b1) / gcd * (-y)
    // -> z = LCM(x, y) * k + (c1 * p + b1)
    int64_t gcd = ExtendedEuclidean(c1, c2, &x, &y);
    int64_t v = b2 - b1;
    if (v % gcd == 0) {
      x = v / gcd * x;
      y = v / gcd * (-y);
      int64_t coeff = c1 / gcd * c2;
      return ModularSetAnalyzerEntry(coeff, x * c1 + b1);
    } else {
      return Nothing();
    }
  }
  /*!
   * \brief return everything dtype can represent.
   * \return Bound that represent everything dtype can represent.
   */
  static ModularSetAnalyzerEntry Everything() { return ModularSetAnalyzerEntry(1, 0); }
  /*!
   * \brief return an empty set
   * \return Bound that represent everything dtype can represent.
   */
  static ModularSetAnalyzerEntry Nothing() { return ModularSetAnalyzerEntry(0, 1); }
};

ModularSet ModularSetAnalyzer::operator()(const Expr &expr) {
  ModularSetAnalyzerEntry ret = impl_->VisitExpr(expr);
  return ModularSet(ret.coeff, ret.base);
}

void ModularSetAnalyzer::Update(const Var &var, const ModularSet &info, bool allow_override) {
  impl_->Update(var, info, allow_override);
}

std::function<void()> ModularSetAnalyzer::EnterConstraint(const Expr &constraint) {
  return impl_->EnterConstraint(constraint);
}

ModularSetAnalyzer::ModularSetAnalyzer(AnalyzerObj::Impl *parent) : impl_(std::make_unique<Impl>(parent)) {}

ModularSetAnalyzer::~ModularSetAnalyzer() {}

} // namespace sym
} // namespace mlc
