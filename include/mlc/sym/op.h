#ifndef MLC_SYM_OP_H_
#define MLC_SYM_OP_H_

#include <mlc/sym/expr.h>

namespace mlc {
namespace sym {

struct Op_ {
  static inline Op left_shift = Op::Get("mlc.sym.left_shift");
  static inline Op right_shift = Op::Get("mlc.sym.shift_right");
  static inline Op bitwise_and = Op::Get("mlc.sym.bitwise_and");
  static inline Op bitwise_or = Op::Get("mlc.sym.bitwise_or");
  static inline Op bitwise_xor = Op::Get("mlc.sym.bitwise_xor");
  static inline Op bitwise_not = Op::Get("mlc.sym.bitwise_not");
  static inline Op if_then_else = Op::Get("mlc.sym.if_then_else");
  static inline Op fabs = Op::Get("mlc.sym.fabs");
};

MLC_API Expr cast(DLDataType dtype, Expr a);
MLC_API Expr add(Expr a, Expr b);
MLC_API Expr sub(Expr a, Expr b);
MLC_API Expr mul(Expr a, Expr b);
MLC_API Expr neg(Expr a);
MLC_API Expr truncdiv(Expr a, Expr b);
MLC_API Expr truncmod(Expr a, Expr b);
MLC_API Expr floordiv(Expr a, Expr b);
MLC_API Expr floormod(Expr a, Expr b);
MLC_API Expr min(Expr a, Expr b);
MLC_API Expr max(Expr a, Expr b);
MLC_API Expr max_value(DLDataType dtype);
MLC_API Expr min_value(DLDataType dtype);
MLC_API Expr if_then_else(Expr cond, Expr true_value, Expr false_value);
MLC_API Expr select(Expr cond, Expr true_value, Expr false_value);
MLC_API Expr greater(Expr a, Expr b);
MLC_API Expr greater_equal(Expr a, Expr b);
MLC_API Expr less(Expr a, Expr b);
MLC_API Expr less_equal(Expr a, Expr b);
MLC_API Expr equal(Expr a, Expr b);
MLC_API Expr not_equal(Expr a, Expr b);
MLC_API Expr logical_and(Expr a, Expr b);
MLC_API Expr logical_or(Expr a, Expr b);
MLC_API Expr logical_not(Expr a);
MLC_API Expr right_shift(Expr a, Expr b);
MLC_API Expr left_shift(Expr a, Expr b);
MLC_API Expr bitwise_and(Expr a, Expr b);
MLC_API Expr bitwise_or(Expr a, Expr b);
MLC_API Expr bitwise_xor(Expr a, Expr b);
MLC_API Expr bitwise_neg(Expr a);
MLC_API Expr abs(Expr x);

#define MLC_SYM_DEF_BIN_OP_OVERLOAD(OP, Func)                                                                          \
  inline Expr operator OP(Expr a, Expr b) { return Func(a, b); }                                                       \
  template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>                                 \
  inline Expr operator OP(Expr a, T b) {                                                                               \
    return Func(a, Expr::Const<T>(a->dtype, b));                                                                       \
  }                                                                                                                    \
  template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>                                 \
  inline Expr operator OP(T a, Expr b) {                                                                               \
    return Func(Expr::Const<T>(b->dtype, a), b);                                                                       \
  }

inline Expr operator-(Expr a) { return neg(a); }
inline Expr operator!(Expr a) { return logical_not(a); }
inline Expr operator~(Expr a) { return bitwise_neg(a); }

MLC_SYM_DEF_BIN_OP_OVERLOAD(+, add)
MLC_SYM_DEF_BIN_OP_OVERLOAD(-, sub)
MLC_SYM_DEF_BIN_OP_OVERLOAD(*, mul)
MLC_SYM_DEF_BIN_OP_OVERLOAD(<<, left_shift)
MLC_SYM_DEF_BIN_OP_OVERLOAD(>>, right_shift)
MLC_SYM_DEF_BIN_OP_OVERLOAD(>, greater)
MLC_SYM_DEF_BIN_OP_OVERLOAD(<, less)
MLC_SYM_DEF_BIN_OP_OVERLOAD(>=, greater_equal)
MLC_SYM_DEF_BIN_OP_OVERLOAD(<=, less_equal)
MLC_SYM_DEF_BIN_OP_OVERLOAD(==, equal)
MLC_SYM_DEF_BIN_OP_OVERLOAD(!=, not_equal)
MLC_SYM_DEF_BIN_OP_OVERLOAD(&&, logical_and)
MLC_SYM_DEF_BIN_OP_OVERLOAD(||, logical_or)
MLC_SYM_DEF_BIN_OP_OVERLOAD(&, bitwise_and)
MLC_SYM_DEF_BIN_OP_OVERLOAD(|, bitwise_or)
MLC_SYM_DEF_BIN_OP_OVERLOAD(^, bitwise_xor)
// MLC_SYM_DEF_BIN_OP_OVERLOAD(/, floordiv)
// MLC_SYM_DEF_BIN_OP_OVERLOAD(%, floormod)

#undef MLC_SYM_DEF_BIN_OP_OVERLOAD

inline Expr floordiv(Expr a, int64_t b) { return floordiv(a, Expr::Const<int64_t>(a->dtype, b)); }
inline Expr floordiv(int64_t a, Expr b) { return floordiv(Expr::Const<int64_t>(b->dtype, a), b); }
inline int64_t floordiv(int64_t x, int64_t y) {
  int64_t rdiv = x / y;
  int64_t rmod = x % y;
  bool is_floor_div = (y >= 0 && rmod >= 0) || (y < 0 && rmod <= 0);
  return is_floor_div ? rdiv : (rdiv - 1);
}

inline Expr floormod(Expr a, int64_t b) { return floormod(a, Expr::Const<int64_t>(a->dtype, b)); }
inline Expr floormod(int64_t a, Expr b) { return floormod(Expr::Const<int64_t>(b->dtype, a), b); }
inline int64_t floormod(int64_t x, int64_t y) {
  int64_t rmod = x % y;
  bool is_floor_div = (y >= 0 && rmod >= 0) || (y < 0 && rmod <= 0);
  return is_floor_div ? rmod : rmod + y;
}

inline Expr truncdiv(Expr a, int64_t b) { return truncdiv(a, Expr::Const<int64_t>(a->dtype, b)); }
inline Expr truncdiv(int64_t a, Expr b) { return truncdiv(Expr::Const<int64_t>(b->dtype, a), b); }
inline int64_t truncdiv(int64_t x, int64_t y) { return x / y; }

inline Expr truncmod(Expr a, int64_t b) { return truncmod(a, Expr::Const<int64_t>(a->dtype, b)); }
inline Expr truncmod(int64_t a, Expr b) { return truncmod(Expr::Const<int64_t>(b->dtype, a), b); }
inline int64_t truncmod(int64_t x, int64_t y) { return x % y; }

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_OP_H_
