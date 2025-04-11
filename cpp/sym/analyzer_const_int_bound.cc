#include "./analyzer_impl.h"
#include "./utils.h"
#include <memory>

namespace mlc {
namespace sym {

struct ConstIntBoundAnalyzerEntry {
  int64_t min_value;
  int64_t max_value;
  bool is_const(int64_t value) const { return min_value == max_value && min_value == value; }
  bool operator==(const ConstIntBoundAnalyzerEntry &other) const {
    return min_value == other.min_value && max_value == other.max_value;
  }
};

/*!
 * \brief Check if an integer op with operand x, y will overflow.
 * \param x The left operand.
 * \param y The left operand.
 * \param min_value The minimum value of the domain.
 * \param max_value The maximum value of the domain.
 * \return Whether overflow can happen.
 * \tparam Op The integer operator.
 */
template <typename Op>
inline bool WillOverflow([[maybe_unused]] int64_t x, [[maybe_unused]] int64_t y, [[maybe_unused]] int64_t min_value,
                         [[maybe_unused]] int64_t max_value) {
  return false;
}

template <> inline bool WillOverflow<AddObj>(int64_t x, int64_t y, int64_t min_value, int64_t max_value) {
  if ((y > 0) && (x > max_value - y))
    return true;
  if ((y < 0) && (x < min_value - y))
    return true;
  return false;
}

template <> inline bool WillOverflow<SubObj>(int64_t x, int64_t y, int64_t min_value, int64_t max_value) {
  if ((y > 0) && (x < min_value + y))
    return true;
  if ((y < 0) && (x > max_value + y))
    return true;
  return false;
}

template <> inline bool WillOverflow<MulObj>(int64_t x, int64_t y, int64_t min_value, int64_t max_value) {
  if (y == 0)
    return false;
  if (y > 0) {
    if (x < min_value / y)
      return true;
    if (x > max_value / y)
      return true;
  } else {
    if (y == -1 && x == std::numeric_limits<int64_t>::min())
      return true;
    if (x > min_value / y)
      return true;
    if (x < max_value / y)
      return true;
  }
  return false;
}

template <>
inline bool WillOverflow<ModObj>([[maybe_unused]] int64_t x, int64_t y, [[maybe_unused]] int64_t min_value,
                                 [[maybe_unused]] int64_t max_value) {
  return y == 0;
}

struct ConstIntBoundAnalyzer::Impl : public ExprFunctor<ConstIntBoundAnalyzerEntry(const Expr &)> {
public:
  /*! \brief additional bound info about expr in bound */
  struct BoundInfo {
    Expr expr;
    ConstIntBoundAnalyzerEntry bound;

    BoundInfo(Expr expr, ConstIntBoundAnalyzerEntry bound) : expr(expr), bound(bound) {}
  };

  void Bind(const Var &var, const Range &range, bool allow_override) {
    ConstIntBoundAnalyzerEntry a = VisitExpr(range->min);
    ConstIntBoundAnalyzerEntry b = VisitExpr(range->extent);
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = a.min_value;
    ret.max_value = InfAwareAdd(a.max_value, InfAwareAdd(b.max_value, -1));
    Update(var, ret, allow_override);
  }

  void Update(const Var &var, const ConstIntBoundAnalyzerEntry &info, bool allow_override) {
    if (!allow_override) {
      auto it = var_map_.find(var);
      if (it != var_map_.end()) {
        // TODO: recover
        // ICHECK(it->second == info) << "Trying to update var \'" << var << "\'"
        //                            << " with a different const bound: "
        //                            << "original=" << ConstIntBound(it->second.min_value, it->second.max_value)
        //                            << ", new=" << ConstIntBound(info.min_value, info.max_value);
      }
    }
    var_map_[var] = info;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const LetObj *op) final {
    auto it = var_map_.find(op->var);
    // if the var has not been binded, update the info.
    if (it == var_map_.end()) {
      var_map_[op->var] = this->VisitExpr(op->value);
      ConstIntBoundAnalyzerEntry ret = VisitExpr(op->body);
      var_map_.erase(op->var);
      return ret;
    } else {
      return VisitExpr(op->body);
    }
  }

  void Update(const Var &var, const ConstIntBound &info, bool allow_override) {
    Update(var, MakeBound(info->min_value, info->max_value), allow_override);
  }

  // Override visitor behaviors
  ConstIntBoundAnalyzerEntry VisitExprDefault_(const Object *op) final {
    return Everything(reinterpret_cast<const ExprObj *>(op)->dtype);
  }

  ConstIntBoundAnalyzerEntry VisitExpr(const Expr &expr) final {
    ConstIntBoundAnalyzerEntry res = ExprFunctor::VisitExpr(expr);
    ExprDeepEqual equal;
    // a linear search over additional info
    // assume we won't have a lot of conditions
    for (const BoundInfo &info : additional_info_) {
      if (equal(expr, info.expr)) {
        res = Intersect(res, info.bound);
      }
    }
    if (bound_) {
      auto it = bound_->find(expr);
      if (it != bound_->end()) {
        ConstIntBoundAnalyzerEntry everything = Everything(expr->dtype);
        ConstIntBound v = (*it).second;
        // TODO: recover this
        (void)everything;
        // ICHECK((v->min_value == res.min_value && v->max_value == res.max_value) ||
        //        (v->min_value == everything.min_value && v->max_value == everything.max_value))
        //     << "Detected bound for " << expr << "conflicts with memorization";
      }
      bound_->Set(expr, ConstIntBound(res.min_value, res.max_value));
    }
    return res;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const RampObj *op) final {
    // op = {base + i * stride | 0 <= i < lanes}
    // ConstIntBoundAnalyzerEntry(op) = Union(ConstIntBoundAnalyzerEntry(base + i * stride) | 0 <= i < lanes)
    // Note that `base + i * stride` is linear w.r.t. `i`
    // ConstIntBoundAnalyzerEntry(op) = Union(ConstIntBoundAnalyzerEntry(base + i * stride) | i = 0, i = lanes-1)
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->base);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->base + (op->lanes - 1) * op->stride);
    return Union(a, b);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const BroadcastObj *op) final { return VisitExpr(op->value); }

  ConstIntBoundAnalyzerEntry VisitExpr_(const CastObj *op) final {
    // TODO: recover ceil(log2(n))
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->value);
    ConstIntBoundAnalyzerEntry b = Everything(op->dtype);
    return Intersect(a, b);
  }

  /*!
   * \brief Process the divisor by making assumption that divide by zero
   * won't happen in a valid program.
   *
   * This is important for us to get a lot of symbolic shape bound right
   * now that the shape n >= 0, but in cases
   * when mod or divide of n occur, the intention is actually n > 0
   *
   * \param divisor The input divsor entry
   * \return The processed entry
   */
  ConstIntBoundAnalyzerEntry AssumeNoZeroDivisor(ConstIntBoundAnalyzerEntry divisor) {
    // TODO: recover this
    // ICHECK(!divisor.is_const(0)) << "Find divide by zero";
    // NOTE: here we make the assumption that
    // divide by zero won't happen in a valid program
    // this is important for us to get a lot of symbolic shape bound right
    // where most conditions know that the shape n >= 0, but in cases
    // when mod or divide of n occur, the intention is actually n > 0
    if (divisor.min_value == 0) {
      divisor.min_value = 1;
      // TODO: recover this
      // ICHECK_GE(divisor.max_value, 1);
    }
    return divisor;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const IntImmObj *op) final { return MakeBound(op->value, op->value); }

  ConstIntBoundAnalyzerEntry VisitExpr_(const AddObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->b);
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = InfAwareAdd(a.min_value, b.min_value);
    ret.max_value = InfAwareAdd(a.max_value, b.max_value);

    return ret;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const SubObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->b);
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = InfAwareAdd(a.min_value, -b.max_value);
    ret.max_value = InfAwareAdd(a.max_value, -b.min_value);

    return ret;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const MulObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->b);
    return BinaryOpBoundary(a, b, InfAwareMul);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const DivObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = AssumeNoZeroDivisor(VisitExpr(op->b));
    return HandleDivision(a, b, op->dtype, InfAwareDiv);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const ModObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = AssumeNoZeroDivisor(VisitExpr(op->b));

    if (b.min_value > 0) {
      int64_t b_max_cap = InfAwareAdd(b.max_value, -1);
      if (a.min_value >= 0) {
        // 0 <= [a_min, a_max] < b_min
        if (a.max_value < b.min_value)
          return a;
        // other case, we can get close to 0
        return MakeBound(0, std::min(a.max_value, b_max_cap));
      } else {
        return MakeBound(std::max(a.min_value, -b_max_cap), std::min(std::max(a.max_value, (int64_t)0), b_max_cap));
      }
    } else {
      // TODO: recover this
      //   ICHECK(!b.is_const(0)) << "mod by zero";
      // mod by negative value is rare,
      // and we just use the simpliest rule.
      return Everything(op->dtype);
    }
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const FloorDivObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = AssumeNoZeroDivisor(VisitExpr(op->b));
    return HandleDivision(a, b, op->dtype, InfAwareFloorDiv);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const FloorModObj *op) final {
    /* let a / b = x + y, where x is integer, y \in [0, 1)
     * floormod(a, b) = a - floordiv(a, b) * b
     * floordiv(a, b) = x
     * floormod(a, b) = a - floordiv(a, b) * b
     *                = a - x * b
     *                = a - (a / b - y) * b
     *                = a - a + y * b
     *                = y * b
     * note that 0 <= y < 1
     * when b > 0, 0 <= b * y < b
     *             0 <= b * y <= b - 1
     * when b < 0, b < b * y <= 0
     *             b + 1 <= b * y <= 0
     * In all cases, min(0, b + 1) <= b * y <= max(0, b - 1)
     *               min(0, b_min + 1) <= b * y <= max(0, b_max - 1)
     * That is, min(0, b_min + 1) <= floormod(a, b) <= max(0, b_max - 1)
     */
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = AssumeNoZeroDivisor(VisitExpr(op->b));

    if (b.min_value > 0) {
      int64_t b_max_cap = InfAwareAdd(b.max_value, -1);
      if (a.min_value >= 0) {
        // 0 <= [a_min, a_max] < b_min
        if (a.max_value < b.min_value)
          return a;
        // other case, we can get close to 0
        return MakeBound(0, std::min(a.max_value, b_max_cap));
      } else {
        return MakeBound(0, b_max_cap);
      }
    } else {
      // TODO: recover this
      //   ICHECK(!b.is_const(0)) << "floormod by zero";
      int64_t b_min_cap = InfAwareAdd(b.min_value, 1);
      int64_t b_max_cap = InfAwareAdd(b.max_value, -1);
      return Intersect(
          MakeBound(std::min(static_cast<int64_t>(0), b_min_cap), std::max(static_cast<int64_t>(0), b_max_cap)),
          Everything(op->dtype));
    }
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const MinObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->b);
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = std::min(a.min_value, b.min_value);
    ret.max_value = std::min(a.max_value, b.max_value);
    return ret;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const MaxObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->a);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->b);
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = std::max(a.min_value, b.min_value);
    ret.max_value = std::max(a.max_value, b.max_value);
    return ret;
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const SelectObj *op) final {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->true_value);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->false_value);
    return Union(a, b);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const CallObj *op) final {
    if (const OpObj *_op = op->op.as<OpObj>()) {
      if (Op_::right_shift.get() == _op) {
        return VisitRightShift(op);
      } else if (Op_::left_shift.get() == _op) {
        return VisitLeftShift(op);
      } else if (Op_::bitwise_and.get() == _op) {
        return VisitBitwiseAnd(op);
      }
    }
    return Everything(op->dtype);
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const VarObj *op) final {
    Var v(op);
    auto it = var_map_.find(v);
    if (it != var_map_.end()) {
      return it->second;
    } else {
      return Everything(op->dtype);
    }
  }

  ConstIntBoundAnalyzerEntry VisitExpr_(const ShapeVarObj *op) final {
    ShapeVar v(op);
    auto it = var_map_.find(v);
    if (it != var_map_.end()) {
      return it->second;
    } else {
      return MakeBound(0, kPosInf);
    }
  }

  ConstIntBoundAnalyzerEntry VisitLeftShift(const CallObj *op) {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->args[0]);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->args[1]);

    if (a.min_value < 0 || b.min_value < 0) {
      // If either operand can negative, we may run into undefined
      // behavior for some targets.  In these cases, avoid making any
      // assumptions about the result.
      return Everything(op->dtype);
    }

    return BinaryOpBoundary(a, b, InfAwareLeftShift);
  }

  ConstIntBoundAnalyzerEntry VisitRightShift(const CallObj *op) {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->args[0]);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->args[1]);
    return BinaryOpBoundary(a, b, InfAwareRightShift);
  }

  ConstIntBoundAnalyzerEntry VisitBitwiseAnd(const CallObj *op) {
    ConstIntBoundAnalyzerEntry a = VisitExpr(op->args[0]);
    ConstIntBoundAnalyzerEntry b = VisitExpr(op->args[1]);
    // handle positive index case.
    if (a.min_value >= 0 && b.min_value >= 0) {
      return MakeBound(0, std::min(a.max_value, b.max_value));
    } else {
      if (b.min_value >= 0) {
        return MakeBound(0, b.max_value);
      }
      if (a.min_value >= 0) {
        return MakeBound(0, a.max_value);
      }
      return Everything(op->dtype);
    }
  }

  std::function<void()> EnterConstraint(const Expr &constraint) {
    std::vector<BoundInfo> info = DetectBoundInfo(constraint);
    if (info.size() == 0)
      return nullptr;
    size_t old_size = additional_info_.size();
    additional_info_.insert(additional_info_.end(), info.begin(), info.end());
    size_t new_size = old_size + info.size();
    auto frecover = [old_size, new_size, this]() {
      // TODO: recover this
      (void)new_size;
      //   ICHECK_EQ(additional_info_.size(), new_size);
      additional_info_.erase(additional_info_.begin() + old_size, additional_info_.end());
    };
    return frecover;
  }

  // internal variable map
  std::unordered_map<Var, ConstIntBoundAnalyzerEntry, ObjRefHash, ObjRefEqual> var_map_;
  // additional bound info
  std::vector<BoundInfo> additional_info_;
  // look up table for memorization
  BoundMapType *bound_{nullptr};
  // internal helper functions
  /*!
   * \brief Get boundary of binary op who are monotonic wrt to one argument.
   * \param a The entry of the left operand.
   * \param b The entry of the right operand.
   * \param op The operator.
   * \tparam F the operator function type.
   * \return The result.
   */
  template <typename F>
  static ConstIntBoundAnalyzerEntry BinaryOpBoundary(ConstIntBoundAnalyzerEntry a, ConstIntBoundAnalyzerEntry b,
                                                     const F &op) {
    ConstIntBoundAnalyzerEntry ret;
    // The boundary point must be shihft of the original boundary.
    int64_t v1 = op(a.min_value, b.min_value);
    int64_t v2 = op(a.max_value, b.max_value);
    int64_t v3 = op(a.min_value, b.max_value);
    int64_t v4 = op(a.max_value, b.min_value);
    ret.min_value = std::min(std::min(std::min(v1, v2), v3), v4);
    ret.max_value = std::max(std::max(std::max(v1, v2), v3), v4);
    return ret;
  }
  /*!
   * \brief Get value boundaries of division (e.g. Div or FloorDiv).
   * \param a The entry of the left operand.
   * \param b The entry of the right operand.
   * \param dt The data type of the division operator.
   * \param op The division operator.
   * \tparam F the operator function type.
   * \return The result.
   */
  template <typename F>
  static ConstIntBoundAnalyzerEntry HandleDivision(ConstIntBoundAnalyzerEntry a, ConstIntBoundAnalyzerEntry b,
                                                   DLDataType dt, const F &op) {
    // Here we have a / b.
    // The largest value of the division will be for the smallest (with
    // respect to the absolute value) value of b. If the range of b starts
    // at a negative value and ends at a positive one, narrow it down to
    // be closer to 0, because BinaryOpBoundary only checks end-points of
    // the domain ranges.
    // If the range of b contains 0, then some infinity will be involved
    if (b.min_value <= 0 && 0 <= b.max_value && dt.code == kDLInt) {
      ConstIntBoundAnalyzerEntry b_neg = b.min_value < 0 ? MakeBound(b.min_value, -1) : Everything(dt);
      ConstIntBoundAnalyzerEntry b_pos = b.max_value > 0 ? MakeBound(1, b.max_value) : Everything(dt);

      ConstIntBoundAnalyzerEntry e_neg = BinaryOpBoundary(a, b_neg, op);
      ConstIntBoundAnalyzerEntry e_pos = BinaryOpBoundary(a, b_pos, op);

      return MakeBound(std::min(e_neg.min_value, e_pos.min_value), std::max(e_neg.max_value, e_pos.max_value));
    } else if (b.min_value == 0 && dt.code == kDLUInt) {
      // uints only have one sided bounds
      ConstIntBoundAnalyzerEntry assumed_b = MakeBound(1, b.max_value);
      return BinaryOpBoundary(a, assumed_b, op);
    }
    // If the range of b does not have 0, use BinaryOpBoundary.
    return BinaryOpBoundary(a, b, op);
  }
  /*!
   * \brief Compute x + y, aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareAdd(int64_t x, int64_t y) {
    if (x == kPosInf) {
      // TODO: recover this
      // ICHECK(y != kNegInf);
      return kPosInf;
    }
    if (x == kNegInf) {
      // TODO: recover this
      // ICHECK(y != kPosInf);
      return kNegInf;
    }
    if (y == kPosInf || y == kNegInf)
      return y;
    if (WillOverflow<AddObj>(x, y, kNegInf, kPosInf)) {
      if (x > 0)
        return kPosInf;
      return kNegInf;
    }
    return x + y;
  }
  /*!
   * \brief Compute x * y, aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareMul(int64_t x, int64_t y) {
    if (!WillOverflow<MulObj>(x, y, kNegInf, kPosInf))
      return x * y;
    if ((x > 0 && y > 0) || (x < 0 && y < 0))
      return kPosInf;
    return kNegInf;
  }
  /*!
   * \brief Compute x / y, aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareDiv(int64_t x, int64_t y) {
    if (y == 0) {
      MLC_THROW(ValueError) << "Division by zero";
    }
    if (x == kPosInf || x == kNegInf) {
      if (y > 0)
        return x;
      return -x;
    }
    return x / y;
  }
  /*!
   * \brief Compute floodiv(x, y), aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareFloorDiv(int64_t x, int64_t y) {
    if (y == 0) {
      MLC_THROW(ValueError) << "Division by zero";
    }
    if (x == kPosInf || x == kNegInf) {
      if (y > 0)
        return x;
      return -x;
    }
    return floordiv(x, y);
  }
  /*!
   * \brief Compute x << y, aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareLeftShift(int64_t x, int64_t y) {
    if (x == kPosInf || x == kNegInf)
      return x;

    // Can be replaced with std::bit_width in C++20
    auto bit_width = [](int64_t as_signed) {
      uint64_t val = std::abs(as_signed);
      int num_bits = 0;
      while (val) {
        ++num_bits;
        val >>= 1;
      }
      return num_bits;
    };
    int x_bits = bit_width(x);
    if (x_bits + y < 64) {
      return x << y;
    } else {
      return kPosInf;
    }
  }
  /*!
   * \brief Compute x >> y, aware of inf.
   * \param x The left operand.
   * \param y The right operand.
   * \return the result.
   */
  static int64_t InfAwareRightShift(int64_t x, int64_t y) {
    if (x == kPosInf || x == kNegInf)
      return x;
    return x >> y;
  }
  /*!
   * \brief Make a new bound entry.
   */
  static ConstIntBoundAnalyzerEntry MakeBound(int64_t min_value, int64_t max_value) {
    ConstIntBoundAnalyzerEntry e;
    e.min_value = (min_value == kPosInf) ? min_value - 1 : min_value;
    e.max_value = (max_value == kNegInf) ? max_value + 1 : max_value;
    return e;
  }
  /*!
   * \brief Create union of two sets.
   * \param a The left operand.
   * \param b the right operand.
   */
  static ConstIntBoundAnalyzerEntry Union(ConstIntBoundAnalyzerEntry a, ConstIntBoundAnalyzerEntry b) {
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = std::min(a.min_value, b.min_value);
    ret.max_value = std::max(a.max_value, b.max_value);
    return ret;
  }
  /*!
   * \brief Create intersect of two sets.
   * \param a The left operand.
   * \param b the right operand.
   */
  static ConstIntBoundAnalyzerEntry Intersect(ConstIntBoundAnalyzerEntry a, ConstIntBoundAnalyzerEntry b) {
    ConstIntBoundAnalyzerEntry ret;
    ret.min_value = std::max(a.min_value, b.min_value);
    ret.max_value = std::min(a.max_value, b.max_value);
    return ret;
  }
  /*!
   * \brief Flip the sign of a set.
   * \param entry The set of values
   */
  static ConstIntBoundAnalyzerEntry Negative(ConstIntBoundAnalyzerEntry entry) {
    ConstIntBoundAnalyzerEntry ret;
    if (entry.max_value == kPosInf) {
      ret.min_value = kNegInf;
    } else {
      ret.min_value = -entry.max_value;
    }
    if (entry.min_value == kNegInf) {
      ret.max_value = kPosInf;
    } else {
      ret.max_value = -entry.min_value;
    }

    return ret;
  }
  /*!
   * \brief return everything dtype can represent.
   * \param dtype The data type.
   * \return Bound that represent everything dtype can represent.
   */
  static ConstIntBoundAnalyzerEntry Everything(DLDataType dtype) {
    if (dtype.code != kDLInt && dtype.code != kDLUInt) {
      return MakeBound(kNegInf, kPosInf);
    }
    ConstIntBoundAnalyzerEntry ret;
    int64_t vbits = static_cast<int64_t>(dtype.bits) - static_cast<int64_t>(dtype.code == kDLInt);
    if (dtype.code == kDLUInt) {
      ret.min_value = 0;
    } else {
      if (vbits >= 63) {
        ret.min_value = kNegInf;
      } else {
        ret.min_value = -(static_cast<int64_t>(1) << vbits);
      }
    }
    if (vbits >= 63) {
      ret.max_value = kPosInf;
    } else {
      ret.max_value = (static_cast<int64_t>(1) << vbits) - 1;
    }
    return ret;
  }

  /*!
   * \brief Detect additional constant bound from cond, if any
   * \param cond The constraint condition.
   * \return List of detected bounds.
   */
  static std::vector<BoundInfo> DetectBoundInfo(const Expr &cond) {
    PVar<Expr> x, y;
    PVar<IntImm> c;

    std::vector<BoundInfo> info;
    auto add_info = [&](const Expr &expr, int64_t min_value, int64_t max_value) {
      // If the conditional is comparing two integers, do not assign a
      // value to them.
      if (!expr->IsInstance<IntImmObj>()) {
        info.push_back(BoundInfo(expr, MakeBound(min_value, max_value)));
      }
    };

    for (const auto &subexpr : ExtractConstraints(cond)) {
      // NOTE: The canonical form always uses <= or <, but a
      // user-supplied constraint from the python API might not be
      // canonicalized.
      if ((c <= x).Match(subexpr) || (x >= c).Match(subexpr)) {
        add_info(x.Eval(), c.Eval()->value, kPosInf);
      } else if ((c < x).Match(subexpr) || (x > c).Match(subexpr)) {
        add_info(x.Eval(), c.Eval()->value + 1, kPosInf);
      } else if ((x <= c).Match(subexpr) || (x >= c).Match(subexpr)) {
        add_info(x.Eval(), kNegInf, c.Eval()->value);
      } else if ((x < c).Match(subexpr) || (c > x).Match(subexpr)) {
        add_info(x.Eval(), kNegInf, c.Eval()->value - 1);
      } else if ((x == c).Match(subexpr) || (c == x).Match(subexpr)) {
        add_info(x.Eval(), c.Eval()->value, c.Eval()->value);
      }
    }

    return info;
  }

  /*!
   * TODO: recover this
   * \brief Extract the argument from int(ceil(log2(arg)))
   *
   * This expression is used as the implementation of
   * topi.math.ceil_log2, and can appear in iteration bounds.
   */
  //   static Optional<Expr> FindCeilLog2Arg(const CastObj *op) {
  //     if (op->dtype.code == kDLInt) {
  //       if (const auto as_call = op->value.as<CallObj>()) {
  //         if (as_call->op.same_as(Op::Get("tir.ceil"))) {
  //           Expr ceil_arg = as_call->args[0];
  //           if (auto arg_call = ceil_arg.as<CallObj>()) {
  //             if (arg_call->op.same_as(Op::Get("tir.log2"))) {
  //               Expr log_arg = arg_call->args[0];
  //               return log_arg;
  //             }
  //           }
  //         }
  //       }
  //     }
  //     return NullOpt;
  //   }

  /*! \brief Propagate constraints through ceil(log2(arg))
   *
   * Helper function for CastNode visitor
   */
  ConstIntBoundAnalyzerEntry CeilLog2Bounds(Expr arg) {
    if (auto as_float = arg.as<FloatImmObj>()) {
      // A cast from int to float may have already been simplified
      // out.  Normally we don't inspect floating-point arguments, but here we can
      int64_t val = std::ceil(std::log2(as_float->value));
      return MakeBound(val, val);
    } else {
      ConstIntBoundAnalyzerEntry arg_bounds = VisitExpr(arg);
      return MakeBound(std::ceil(std::log2(arg_bounds.min_value)), std::ceil(std::log2(arg_bounds.max_value)));
    }
  }
};

ConstIntBound ConstIntBoundAnalyzer::operator()(const Expr &expr) const {
  ConstIntBoundAnalyzerEntry ret = impl_->VisitExpr(expr);
  return ConstIntBound(ret.min_value, ret.max_value);
}

ConstIntBound ConstIntBoundAnalyzer::operator()(const Expr &expr, BoundMapType *bound) {
  impl_->bound_ = bound;
  ConstIntBoundAnalyzerEntry ret = impl_->VisitExpr(expr);
  impl_->bound_ = nullptr;
  return ConstIntBound(ret.min_value, ret.max_value);
}

void ConstIntBoundAnalyzer::Update(const Var &var, const ConstIntBound &info, bool allow_override) {
  impl_->Update(var, info, allow_override);
}

void ConstIntBoundAnalyzer::Bind(const Var &var, const Range &range, bool allow_override) {
  impl_->Bind(var, range, allow_override);
}

std::function<void()> ConstIntBoundAnalyzer::EnterConstraint(const Expr &constraint) {
  return impl_->EnterConstraint(constraint);
}

ConstIntBoundAnalyzer::ConstIntBoundAnalyzer(AnalyzerObj::Impl *) : impl_(std::make_unique<Impl>()) {}
ConstIntBoundAnalyzer::~ConstIntBoundAnalyzer() {}

} // namespace sym
} // namespace mlc
