#ifndef MLC_SYM_UTILS_H_
#define MLC_SYM_UTILS_H_

#include <limits>
#include <mlc/sym/all.h>
#include <utility>

namespace mlc {
namespace sym {

static constexpr int64_t kPosInf = std::numeric_limits<int64_t>::max();
static constexpr int64_t kNegInf = -kPosInf;

struct SymbolicLimits {
  inline static Expr pos_inf_ = Var("pos_inf", DType::Int(64));
  inline static Expr neg_inf_ = Var("neg_inf", DType::Int(64));
};

inline Expr pos_inf() { return SymbolicLimits::pos_inf_; }
inline Expr neg_inf() { return SymbolicLimits::neg_inf_; }
inline bool is_pos_inf(const Expr &value) { return value.get() == SymbolicLimits::pos_inf_.get(); }
inline bool is_neg_inf(const Expr &value) { return value.get() == SymbolicLimits::neg_inf_.get(); }

/*! \brief Structure for representing result of known
 * Values are assigned to allow these flags to be used in bitwise
 * operations.
 */
enum class CompareResult : int {
  kInconsistent = 0,
  kEQ = 1,
  kLT = 2,
  kLE = 3,
  kGT = 4,
  kGE = 5,
  kNE = 6,
  kUnknown = 7
};

inline constexpr CompareResult operator&(CompareResult lhs, CompareResult rhs) {
  return CompareResult(static_cast<int>(lhs) & static_cast<int>(rhs));
}
inline constexpr CompareResult operator|(CompareResult lhs, CompareResult rhs) {
  return CompareResult(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline CompareResult Reverse(CompareResult res) {
  switch (res) {
  case CompareResult::kInconsistent:
    return CompareResult::kInconsistent;
  case CompareResult::kEQ:
    return CompareResult::kEQ;
  case CompareResult::kLT:
    return CompareResult::kGT;
  case CompareResult::kLE:
    return CompareResult::kGE;
  case CompareResult::kGT:
    return CompareResult::kLT;
  case CompareResult::kGE:
    return CompareResult::kLE;
  case CompareResult::kNE:
    return CompareResult::kNE;
  case CompareResult::kUnknown:
    return CompareResult::kUnknown;
  }
  return CompareResult::kUnknown;
}

inline CompareResult Negate(CompareResult res) {
  switch (res) {
  case CompareResult::kInconsistent:
    return CompareResult::kInconsistent;
  case CompareResult::kUnknown:
    return CompareResult::kUnknown;
  default:
    return CompareResult(~static_cast<int>(res) & static_cast<int>(CompareResult::kUnknown));
  }
  return CompareResult::kUnknown;
}
struct ConstraintContext {
  explicit ConstraintContext(AnalyzerObj::Impl *analyzer, Expr constraint);
  ~ConstraintContext();

  AnalyzerObj::Impl *analyzer_;
  Expr constraint_;
  std::vector<std::function<void()>> recovery_functions_;
};

std::vector<Expr> ExtractConstraints(const Expr &expr, bool keep_composite_constraints = true);

std::vector<Expr> ExtractComponents(const Expr &expr);

Expr SimplifyAsAndOfOrs(const Expr &expr, AnalyzerObj::Impl *analyzer);

inline const int64_t *AsConstInt(const Expr &x) {
  if (const IntImmObj *op = x.as<IntImmObj>()) {
    return &(op->value);
  }
  return nullptr;
}

inline bool IsConstInt(const Expr &x, int64_t value) {
  if (const int64_t *v = AsConstInt(x)) {
    return *v == value;
  }
  return false;
}

inline bool IsIndexType(const DLDataType &type) {
  return type.code == kDLInt && type.lanes == 1 && (type.bits == 32 || type.bits == 64);
}

/*!
 * \brief Use Extended Euclidean algorithm to solve ax + by = gcd(a, b)
 * \param a The first coefficient.
 * \param b The second coefficient.
 * \param x The solution of x.
 * \param y The solution of y.
 * \return The GCD of a and b.
 */
inline int64_t ExtendedEuclidean(int64_t a, int64_t b, int64_t *x, int64_t *y) {
  // Extended Euclidean algorithm
  // if a < 0, the problem can be convert into
  // |a|* (-x) + b * y = gcd(|a|, b)
  //
  // initial condition:
  // a * 0 + b * 1 = b
  // a * 1 + b * 0 = a
  int64_t s = 0, old_s = 1;
  int64_t r = b, old_r = a >= 0 ? a : -a;
  // Iteration (r2 < r1):
  // a * x1 + b * y1 = r1
  // a * x2 + b * y2 = r2
  // The above two eqs can derive the following eq (q = r1 / r2)
  // a * (x1 - x2 * q) + b * (y1 - y2 * q) = r1 - r2 * q = r3
  // Because r3 < r2, the iteration can eventually terminate
  while (r != 0) {
    int64_t q = old_r / r;
    int64_t tmp = old_r;
    old_r = r;
    r = tmp - q * r;
    tmp = old_s;
    old_s = s;
    s = tmp - q * s;
  }

  *x = a >= 0 ? old_s : -old_s;
  if (b != 0) {
    *y = (old_r - (*x) * a) / b;
  } else {
    *y = 1;
  }

  return old_r;
}

/*!
 * \brief Take GCD of a and b.
 * \param a The first operand.
 * \param b The second operand.
 * \return The result.
 */
inline int64_t ZeroAwareGCD(int64_t a, int64_t b) {
  if (a < 0)
    a = -a;
  if (b < 0)
    b = -b;
  if (a < b)
    std::swap(a, b);
  if (b == 0)
    return a;
  // perform GCD (greatest common divisor)
  // ax + by = gcd(a, b) z if a != 0, b != 0
  while (a % b != 0) {
    a = a % b;
    std::swap(a, b);
  }
  return b;
}

/*!
 * \brief Calculate the least common multiple for two values.
 * \param a an integer number
 * \param b an integer number
 * \return the least common multiple.
 */
inline int64_t LeastCommonMultiple(int64_t a, int64_t b) {
  int64_t x, y;
  return (a * b) / ExtendedEuclidean(a, b, &x, &y);
}

template <typename TNode, typename FLeaf> inline void UnpackReduction(const Expr &value, FLeaf fleaf) {
  if (const TNode *node = value.as<TNode>()) {
    UnpackReduction<TNode, FLeaf>(node->a, fleaf);
    UnpackReduction<TNode, FLeaf>(node->b, fleaf);
  } else {
    fleaf(value);
  }
}

template <typename FLeaf> inline void UnpackSum(const Expr &value, FLeaf fleaf, int sign = 1) {
  if (const auto *node = value.as<AddObj>()) {
    UnpackSum(node->a, fleaf, sign);
    UnpackSum(node->b, fleaf, sign);
  } else if (const auto *node = value.as<SubObj>()) {
    UnpackSum(node->a, fleaf, sign);
    UnpackSum(node->b, fleaf, -sign);
  } else {
    fleaf(value, sign);
  }
}

inline Expr MulAndNormalize(const Expr &lhs, const Expr &rhs) {
  int64_t cscale = 1;
  Expr res = Expr::Const(lhs->dtype, 1);
  auto fcollect = [&](Expr val) {
    if (const auto *intimm = val.as<IntImmObj>()) {
      cscale *= intimm->value;
    } else {
      res = res * val;
    }
  };
  UnpackReduction<MulObj>(lhs, fcollect);
  UnpackReduction<MulObj>(rhs, fcollect);
  if (cscale != 1) {
    res = res * cscale;
  }
  return res;
}

inline int32_t CheckPowOfTwo(uint64_t x) {
  if (x == 0) {
    return -1;
  }
  int32_t leading_zero = ::mlc::base::CountLeadingZeros(x);
  if (x == static_cast<uint64_t>(1) << (63 - leading_zero)) {
    return 63 - leading_zero;
  }
  return -1;
}

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_UTILS_H_
