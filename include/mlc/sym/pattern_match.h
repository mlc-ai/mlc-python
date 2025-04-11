#ifndef MLC_SYM_PATTERN_MATCH_H_
#define MLC_SYM_PATTERN_MATCH_H_

#include <cmath>
#include <mlc/sym/expr.h>
#include <mlc/sym/expr_functor.h>
#include <mlc/sym/op.h>

namespace mlc {
namespace sym {

template <typename Derived> class Pattern {
public:
  using Nested = Derived;
  template <typename NodeType> inline bool Match(const NodeType &value) const {
    return Match(value, []() { return true; });
  }
  template <typename NodeType, typename Condition> bool Match(const NodeType &value, Condition cond) const {
    derived().InitMatch_();
    return derived().Match_(value) && cond();
  }
  const Derived &derived() const { return *static_cast<const Derived *>(this); }
};

template <typename T> class PEqualChecker {
public:
  bool operator()(const T &lhs, const T &rhs) const { return lhs == rhs; }
};

template <> class PEqualChecker<Expr> {
public:
  bool operator()(const Expr &lhs, const Expr &rhs) const {
    if (lhs.get() == rhs.get())
      return true;
    return ExprDeepEqual::Compare(lhs, rhs); // TODO: implement ExprDeepEqual
  }
};

template <> class PEqualChecker<IntImm> {
public:
  bool operator()(const IntImm &lhs, const IntImm &rhs) const { return lhs->value == rhs->value; }
};

template <> class PEqualChecker<FloatImm> {
public:
  bool operator()(const FloatImm &lhs, const FloatImm &rhs) const { return std::fabs(lhs->value - rhs->value) < 1e-20; }
};

template <> class PEqualChecker<Var> {
public:
  bool operator()(const Var &lhs, const Var &rhs) const { return lhs.get() == rhs.get(); }
};

template <typename T> class PVar : public Pattern<PVar<T>> {
public:
  // Store PVars by reference in the expression.
  using Nested = const PVar<T> &;

  void InitMatch_() const { value_ = Null; }

  bool Match_(const T &value) const {
    if (value_.has_value()) {
      return PEqualChecker<T>()(value_.value(), value);
    }
    this->value_ = value;
    return true;
  }

  template <typename NodeRefType, typename = typename std::enable_if<std::is_base_of<NodeRefType, T>::value>::type>
  bool Match_(const NodeRefType &value) const {
    if (const auto *ptr = value.template as<typename T::TObj>()) {
      return Match_(T(ptr));
    } else {
      return false;
    }
  }

  T Eval() const {
    if (!value_.has_value()) {
      MLC_THROW(InternalError) << "PVar is not filled";
    }
    return value_.value();
  }

  T EvalOr(const T &default_value) const { return value_.has_value() ? value_.value() : default_value; }

protected:
  mutable Optional<T> value_{Null};
};

template <typename T> class PConst : public Pattern<PConst<T>> {
public:
  PConst(T value) : value_(value) {}

  void InitMatch_() const {}

  bool Match_(const T &value) const { return PEqualChecker<T>()(value_, value); }

  T Eval() const { return value_; }

private:
  const T value_;
};

template <typename OpType, typename TA, typename TB> class PBinaryExpr : public Pattern<PBinaryExpr<OpType, TA, TB>> {
public:
  PBinaryExpr(const TA &a, const TB &b) : a_(a), b_(b) {}

  void InitMatch_() const {
    a_.InitMatch_();
    b_.InitMatch_();
  }

  bool Match_(const ObjectRef &node) const {
    using NodeType = typename OpType::TObj;
    if (const NodeType *ptr = node.as<NodeType>()) {
      if (!a_.Match_(ptr->a))
        return false;
      if (!b_.Match_(ptr->b))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const {
    Expr lhs = a_.Eval();
    Expr rhs = b_.Eval();
    if (auto ret = OpType::TryConstFold(lhs, rhs); ret.defined())
      return ret.value();
    return OpType(lhs, rhs);
  }

private:
  typename TA::Nested a_;
  typename TB::Nested b_;
};

template <typename TA> class PConstWithTypeLike : public Pattern<PConstWithTypeLike<TA>> {
public:
  PConstWithTypeLike(const TA &ref, int64_t value) : ref_(ref), value_(value) {}

  void InitMatch_() const {}

  bool Match_(const ObjectRef &node) const {
    if (const IntImmObj *ptr = node.as<IntImmObj>()) {
      return ptr->value == value_;
    } else {
      return false;
    }
  }

  Expr Eval() const {
    DLDataType dtype = ref_.Eval()->dtype;
    return Expr::Const(dtype, value_);
  }

private:
  typename TA::Nested ref_;
  int64_t value_;
};

template <typename TA> class PNotExpr : public Pattern<PNotExpr<TA>> {
public:
  explicit PNotExpr(const TA &value) : value_(value) {}

  void InitMatch_() const { value_.InitMatch_(); }

  bool Match_(const ObjectRef &node) const {
    if (const NotObj *ptr = node.as<NotObj>()) {
      if (!value_.Match_(ptr->a))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const { return Not(value_.Eval()); }

private:
  typename TA::Nested value_;
};

template <typename TA> inline PNotExpr<TA> operator!(const Pattern<TA> &value) { return PNotExpr<TA>(value.derived()); }

template <typename TCond, typename TA, typename TB> class PSelectExpr : public Pattern<PSelectExpr<TCond, TA, TB>> {
public:
  PSelectExpr(const TCond &condition, const TA &true_value, const TB &false_value)
      : condition_(condition), true_value_(true_value), false_value_(false_value) {}

  void InitMatch_() const {
    condition_.InitMatch_();
    true_value_.InitMatch_();
    false_value_.InitMatch_();
  }

  bool Match_(const ObjectRef &node) const {
    if (const SelectObj *ptr = node.as<SelectObj>()) {
      if (!condition_.Match_(ptr->cond))
        return false;
      if (!true_value_.Match_(ptr->true_value))
        return false;
      if (!false_value_.Match_(ptr->false_value))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const { return Select(condition_.Eval(), true_value_.Eval(), false_value_.Eval()); }

private:
  typename TCond::Nested condition_;
  typename TA::Nested true_value_;
  typename TB::Nested false_value_;
};

template <typename TCond, typename TA, typename TB>
inline PSelectExpr<TCond, TA, TB> select(const Pattern<TCond> &condition, const Pattern<TA> &true_value,
                                         const Pattern<TB> &false_value) {
  return PSelectExpr<TCond, TA, TB>(condition.derived(), true_value.derived(), false_value.derived());
}

template <typename DType, typename TA> class PCastExpr : public Pattern<PCastExpr<DType, TA>> {
public:
  PCastExpr(const DType &dtype, const TA &value) : dtype_(dtype), value_(value) {}

  void InitMatch_() const {
    dtype_.InitMatch_();
    value_.InitMatch_();
  }

  bool Match_(const ObjectRef &node) const {
    if (const CastObj *ptr = node.as<CastObj>()) {
      if (!dtype_.Match_(ptr->dtype))
        return false;
      if (!value_.Match_(ptr->value))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const { return CastDType(dtype_.Eval(), value_.Eval()); }

private:
  typename DType::Nested dtype_;
  typename TA::Nested value_;
};

template <typename DType, typename TA>
inline PCastExpr<DType, TA> cast(const Pattern<DType> &dtype, const Pattern<TA> &value) {
  return PCastExpr<DType, TA>(dtype.derived(), value.derived());
}

template <typename TBase, typename TStride, typename TLanes>
class PRampExpr : public Pattern<PRampExpr<TBase, TStride, TLanes>> {
public:
  PRampExpr(const TBase &base, const TStride &stride, const TLanes &lanes)
      : base_(base), stride_(stride), lanes_(lanes) {}

  void InitMatch_() const {
    base_.InitMatch_();
    stride_.InitMatch_();
    lanes_.InitMatch_();
  }

  bool Match_(const ObjectRef &node) const {
    if (const RampObj *ptr = node->as<RampObj>()) {
      if (!base_.Match_(ptr->base))
        return false;
      if (!stride_.Match_(ptr->stride))
        return false;
      if (!lanes_.Match_(ptr->lanes))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const { return Ramp(base_.Eval(), stride_.Eval(), lanes_.Eval()); }

private:
  typename TBase::Nested base_;
  typename TStride::Nested stride_;
  typename TLanes::Nested lanes_;
};

template <typename TBase, typename TStride, typename TLanes>
inline PRampExpr<TBase, TStride, TLanes> ramp(const Pattern<TBase> &base, const Pattern<TStride> &stride,
                                              const Pattern<TLanes> &lanes) {
  return PRampExpr<TBase, TStride, TLanes>(base.derived(), stride.derived(), lanes.derived());
}

template <typename TBase>
inline PRampExpr<TBase, PConstWithTypeLike<TBase>, PConst<int64_t>> ramp(const Pattern<TBase> &base, int stride,
                                                                         int lanes) {
  return PRampExpr<TBase, PConstWithTypeLike<TBase>, PConst<int64_t>>(
      base.derived(), PConstWithTypeLike<TBase>(base.derived(), stride), PConst<int64_t>(lanes));
}

template <typename TA, typename TLanes> class PBroadcastExpr : public Pattern<PBroadcastExpr<TA, TLanes>> {
public:
  PBroadcastExpr(const TA &value, const TLanes &lanes) : value_(value), lanes_(lanes) {}

  void InitMatch_() const {
    value_.InitMatch_();
    lanes_.InitMatch_();
  }

  bool Match_(const ObjectRef &node) const {
    if (const BroadcastObj *ptr = node->as<BroadcastObj>()) {
      if (!value_.Match_(ptr->value))
        return false;
      if (!lanes_.Match_(ptr->lanes))
        return false;
      return true;
    } else {
      return false;
    }
  }

  Expr Eval() const { return Broadcast(value_.Eval(), lanes_.Eval()); }

private:
  typename TA::Nested value_;
  typename TLanes::Nested lanes_;
};

template <typename TA, typename TLanes>
inline PBroadcastExpr<TA, TLanes> broadcast(const Pattern<TA> &value, const Pattern<TLanes> &lanes) {
  return PBroadcastExpr<TA, TLanes>(value.derived(), lanes.derived());
}

namespace detail {
template <bool stop, std::size_t I, typename F> struct tuple_for_each_dispatcher {
  template <typename TTuple> static void run(F &f, const TTuple &tuple) { // NOLINT(*)
    f(I, std::get<I>(tuple));
    tuple_for_each_dispatcher<(I + 1) == std::tuple_size<TTuple>::value, (I + 1), F>::run(f, tuple);
  }
};

template <std::size_t I, typename F> struct tuple_for_each_dispatcher<true, I, F> {
  template <typename TTuple> static void run(F &, const TTuple &) {} // NOLINT(*)
};

template <typename F, typename TTuple> inline void tuple_for_each(F &f, const TTuple &tuple) { // NOLINT(*)
  tuple_for_each_dispatcher<std::tuple_size<TTuple>::value == 0, 0, F>::run(f, tuple);
}

struct PCallExprInitMatchFunctor {
  template <typename T> void operator()(size_t, const T &pattern) const { pattern.InitMatch_(); }
};

struct PCallExprMatchFunctor {
  const CallObj *call_;
  bool matched_{true};

  explicit PCallExprMatchFunctor(const CallObj *call) : call_(call) {}

  template <typename T> void operator()(size_t i, const T &pattern) {
    matched_ = matched_ && pattern.Match_(call_->args[i]);
  }
};

struct PCallExprEvalArgsFunctor {
  List<Expr> args_;

  template <typename T> void operator()(size_t, const T &pattern) { args_.push_back(pattern.Eval()); }
};
} // namespace detail

template <typename Op, typename... TArgs> class PCallExpr : public Pattern<PCallExpr<Op, TArgs...>> {
public:
  explicit PCallExpr(const TArgs &...args) : args_(args...) {}

  void InitMatch_() const {
    detail::PCallExprInitMatchFunctor finit;
    detail::tuple_for_each(finit, args_);
  }

  bool Match_(const ObjectRef &node) const {
    if (const CallObj *ptr = node->as<CallObj>()) {
      if (ptr->args.size() != sizeof...(TArgs))
        return false;
      if (ptr->op.as<OpObj>() != Op::GetOp().get())
        return false;
      detail::PCallExprMatchFunctor fmatch(ptr);
      detail::tuple_for_each(fmatch, args_);
      return fmatch.matched_;
    } else {
      return false;
    }
  }

  Expr Eval() const {
    detail::PCallExprEvalArgsFunctor feval_args;
    detail::tuple_for_each(feval_args, args_);
    return Op::Eval(feval_args.args_);
  }

private:
  std::tuple<typename TArgs::Nested...> args_;
};

template <typename... TPattern> class PMatchesOneOf {
public:
  explicit PMatchesOneOf(const TPattern &...patterns) : patterns_{patterns...} {}

  template <typename NodeType> inline bool Match(const NodeType &value) const {
    return Match(value, []() { return true; });
  }

  template <typename NodeType, typename Condition> inline bool Match(const NodeType &value, Condition cond) const {
    return MatchImpl(value, cond, std::make_index_sequence<sizeof...(TPattern)>());
  }

private:
  template <typename NodeType, typename Condition>
  inline bool MatchImpl(const NodeType &, Condition, std::index_sequence<>) const {
    return false;
  }

  template <typename NodeType, typename Condition, size_t FirstIndex, size_t... RemainingIndices>
  inline bool MatchImpl(const NodeType &value, Condition cond,
                        std::index_sequence<FirstIndex, RemainingIndices...>) const {
    return std::get<FirstIndex>(patterns_).Match(value, cond) ||
           MatchImpl(value, cond, std::index_sequence<RemainingIndices...>());
  }

  std::tuple<const TPattern &...> patterns_;
};

template <typename... TPattern>
inline std::enable_if_t<(std::is_base_of_v<Pattern<TPattern>, TPattern> && ... && true), PMatchesOneOf<TPattern...>>
matches_one_of(const TPattern &...patterns) {
  return PMatchesOneOf<TPattern...>(patterns...);
}

// unary intrinsics
#define MLC_SYM_PM_UNARY_INTRIN(FuncName, OpName, IntrinOp)                                                            \
  struct OpName {                                                                                                      \
    static Expr Eval(List<Expr> args) { return Call(args[0]->dtype, OpName::GetOp(), args); }                          \
    static Op GetOp() { return IntrinOp; }                                                                             \
  };                                                                                                                   \
  template <typename TA> inline PCallExpr<OpName, TA> FuncName(const Pattern<TA> &a) {                                 \
    return PCallExpr<OpName, TA>(a.derived());                                                                         \
  }

// binary intrinsics
#define MLC_SYM_PM_BINARY_INTRIN(FuncName, OpName, IntrinOp)                                                           \
  struct OpName {                                                                                                      \
    static Expr Eval(List<Expr> args) { return Call(args[0]->dtype, OpName::GetOp(), args); }                          \
    static Op GetOp() { return IntrinOp; }                                                                             \
  };                                                                                                                   \
  template <typename TA, typename TB>                                                                                  \
  inline PCallExpr<OpName, TA, TB> FuncName(const Pattern<TA> &a, const Pattern<TB> &b) {                              \
    return PCallExpr<OpName, TA, TB>(a.derived(), b.derived());                                                        \
  }

// trinary intrinsics
#define MLC_SYM_PM_TERNARY_INTRIN(FuncName, OpName, IntrinOp)                                                          \
  struct OpName {                                                                                                      \
    static Expr Eval(List<Expr> args) { return Call(args[1]->dtype, OpName::GetOp(), args); }                          \
    static Op GetOp() { return IntrinOp; }                                                                             \
  };                                                                                                                   \
  template <typename TA, typename TB, typename TC>                                                                     \
  inline PCallExpr<OpName, TA, TB, TC> FuncName(const Pattern<TA> &a, const Pattern<TB> &b, const Pattern<TC> &c) {    \
    return PCallExpr<OpName, TA, TB, TC>(a.derived(), b.derived(), c.derived());                                       \
  }

#define MLC_SYM_PM_BIN_NODE_EX(FuncName, NodeName, CheckStep)                                                          \
  template <typename TA, typename TB>                                                                                  \
  inline PBinaryExpr<NodeName, TA, TB> FuncName(const Pattern<TA> &a, const Pattern<TB> &b) {                          \
    CheckStep;                                                                                                         \
    return PBinaryExpr<NodeName, TA, TB>(a.derived(), b.derived());                                                    \
  }                                                                                                                    \
  template <typename TA>                                                                                               \
  inline PBinaryExpr<NodeName, TA, PConstWithTypeLike<TA>> FuncName(const Pattern<TA> &a, int64_t b) {                 \
    CheckStep;                                                                                                         \
    return FuncName(a, PConstWithTypeLike<TA>(a.derived(), b));                                                        \
  }                                                                                                                    \
  template <typename TA>                                                                                               \
  inline PBinaryExpr<NodeName, PConstWithTypeLike<TA>, TA> FuncName(int64_t b, const Pattern<TA> &a) {                 \
    CheckStep;                                                                                                         \
    return FuncName(PConstWithTypeLike<TA>(a.derived(), b), a);                                                        \
  }

#define MLC_SYM_PM_BINARY(FuncName, NodeName) MLC_SYM_PM_BIN_NODE_EX(FuncName, NodeName, {})

// TODO: raise ambiguity error for operator overload of / and %

// MLC_SYM_PM_BIN_NODE_EX(operator/, Div, DivAmbiguityError(a));
// MLC_SYM_PM_BIN_NODE_EX(operator%, Mod, DivAmbiguityError(a));

// arithmetic expressions
MLC_SYM_PM_BINARY(operator+, Add)
MLC_SYM_PM_BINARY(operator-, Sub)
MLC_SYM_PM_BINARY(operator*, Mul)
MLC_SYM_PM_BINARY(min, Min)
MLC_SYM_PM_BINARY(max, Max)
MLC_SYM_PM_BINARY(div, Div)
MLC_SYM_PM_BINARY(truncdiv, Div)
MLC_SYM_PM_BINARY(truncmod, Mod)
MLC_SYM_PM_BINARY(floordiv, FloorDiv)
MLC_SYM_PM_BINARY(floormod, FloorMod)
// logical expressions
MLC_SYM_PM_BINARY(operator>, GT)
MLC_SYM_PM_BINARY(operator>=, GE)
MLC_SYM_PM_BINARY(operator<, LT)
MLC_SYM_PM_BINARY(operator<=, LE)
MLC_SYM_PM_BINARY(operator==, EQ)
MLC_SYM_PM_BINARY(operator!=, NE)
MLC_SYM_PM_BINARY(operator&&, And)
MLC_SYM_PM_BINARY(operator||, Or)

MLC_SYM_PM_UNARY_INTRIN(operator~, PBitwiseNotOp, Op_::bitwise_not)
MLC_SYM_PM_BINARY_INTRIN(operator<<, PLeftShiftOp, Op_::left_shift)
MLC_SYM_PM_BINARY_INTRIN(operator>>, PRightShiftOp, Op_::right_shift)
MLC_SYM_PM_BINARY_INTRIN(operator&, PBitwiseAndOp, Op_::bitwise_and)
MLC_SYM_PM_BINARY_INTRIN(operator|, PBitwiseOrOp, Op_::bitwise_or)
MLC_SYM_PM_BINARY_INTRIN(operator^, PBitwiseXorOp, Op_::bitwise_xor)
MLC_SYM_PM_TERNARY_INTRIN(if_then_else, PIfThenElseOp, Op_::if_then_else)

#undef MLC_SYM_PM_BIN_NODE_EX
#undef MLC_SYM_PM_BINARY
#undef MLC_SYM_PM_UNARY_INTRIN
#undef MLC_SYM_PM_BINARY_INTRIN
#undef MLC_SYM_PM_TERNARY_INTRIN

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_PATTERN_MATCH_H_
