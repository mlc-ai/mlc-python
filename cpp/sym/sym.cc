#include "./analyzer_impl.h"
#include "./utils.h"
#include <algorithm>
#include <memory>
#include <mlc/sym/all.h>
#include <optional>
#include <unordered_set>

namespace mlc {
namespace sym {

// Section 1. Operators

struct OpRegistry {
  Dict<Str, Op> registry;

  Op RegisterOrGet(Str name) {
    auto it = registry.find(name);
    if (it == registry.end()) {
      Op ret(name);
      registry.Set(name, ret);
      return ret;
    }
    return (*it).second;
  }

  static OpRegistry &Global() {
    static OpRegistry inst;
    return inst;
  }
};

Op Op::Get(Str name) { return OpRegistry::Global().RegisterOrGet(name); }

// Section 2. Constant folding

#define TVM_ARITH_CONST_PROPAGATION(BODY)                                                                              \
  const IntImmObj *pa = a.as<IntImmObj>();                                                                             \
  const IntImmObj *pb = b.as<IntImmObj>();                                                                             \
  const FloatImmObj *fa = a.as<FloatImmObj>();                                                                         \
  const FloatImmObj *fb = b.as<FloatImmObj>();                                                                         \
  BODY;

#define TVM_INDEX_CONST_PROPAGATION(BODY)                                                                              \
  const IntImmObj *pa = a.as<IntImmObj>();                                                                             \
  const IntImmObj *pb = b.as<IntImmObj>();                                                                             \
  DLDataType ta = a->dtype;                                                                                            \
  DLDataType tb = b->dtype;                                                                                            \
  if (IsIndexType(ta) && IsIndexType(tb)) {                                                                            \
    BODY;                                                                                                              \
  }

inline int64_t GetFoldResultInt64Repr(int64_t x, DLDataType dtype) {
  if (dtype.bits < 64) {
    x &= (1LL << dtype.bits) - 1;
  }
  if (dtype.code == kDLInt) {
    // get sign extended value of integer with specified bits
    int64_t m = 1LL << (dtype.bits - 1);
    x = (x ^ m) - m;
  }
  return x;
}

inline double GetFoldResultDoubleRepr(float x) {
  double res = static_cast<double>(x);
  if (std::isinf(res) || std::isnan(res)) {
    return res;
  }
  // certain platform (eg, on gcc7-i386) do the folding arithmetic
  // on float and write back to double is optimized to double
  // precision arithmetic, this is legal and we check the output
  // range thus to ensure consistency when the float result is inf.
  if (res < std::numeric_limits<float>::lowest()) {
    // TODO: LOG(WARNING) << "underlying float value overflow";
    return -std::numeric_limits<double>::infinity();
  } else if (res > std::numeric_limits<float>::max()) {
    // TODO: LOG(WARNING) << "underlying float value overflow";
    return std::numeric_limits<double>::infinity();
  }
  return res;
}

Optional<Expr> Add::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      int64_t res = pa->value + pb->value;
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa && pa->value == 0)
      return b;
    if (pb && pb->value == 0)
      return a;
    if (fa && fb) {
      if (rtype.bits == 32) {
        return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) + static_cast<float>(fb->value)));
      } else if (rtype.bits == 64) {
        return FloatImm(rtype, fa->value + fb->value);
      }
    }
    if (fa && fa->value == 0)
      return b;
    if (fb && fb->value == 0)
      return a;
  });
  return Null;
}

Optional<Expr> Sub::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      int64_t res = pa->value - pb->value;
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pb && pb->value == 0)
      return a;
    if (fa && fb) {
      if (rtype.bits == 32) {
        return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) - static_cast<float>(fb->value)));
      } else if (rtype.bits == 64) {
        return FloatImm(rtype, fa->value - fb->value);
      }
    }
    if (fb && fb->value == 0)
      return a;
  });
  return Null;
}

Optional<Expr> Mul::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      int64_t res = pa->value * pb->value;
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa) {
      if (pa->value == 1)
        return b;
      if (pa->value == 0)
        return a;
    }
    if (pb) {
      if (pb->value == 1)
        return a;
      if (pb->value == 0)
        return b;
    }
    if (fa && fb) {
      if (rtype.bits == 32) {
        return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) * static_cast<float>(fb->value)));
      } else if (rtype.bits == 64) {
        return FloatImm(rtype, fa->value * fb->value);
      }
    }
    if (fa) {
      if (fa->value == 1)
        return b;
      if (fa->value == 0)
        return a;
    }
    if (fb) {
      if (fb->value == 1)
        return a;
      if (fb->value == 0)
        return b;
    }
  });
  return Null;
}

Optional<Expr> Div::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      // due to division and mod can have different modes
      // NOTE: this will assumes truc div.
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
      int64_t res = pa->value / pb->value;
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa) {
      if (pa->value == 0)
        return a;
    }
    if (pb) {
      if (pb->value == 1)
        return a;
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
    if (fa && fb) {
      if (fb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
      if (rtype.bits == 32) {
        return FloatImm(rtype, GetFoldResultDoubleRepr(static_cast<float>(fa->value) / static_cast<float>(fb->value)));
      } else if (rtype.bits == 64) {
        return FloatImm(rtype, fa->value / fb->value);
      }
    }
    if (fa && fa->value == 0)
      return a;
    if (fb) {
      if (fb->value == 1)
        return a;
      if (fb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
  });
  return Null;
}

Optional<Expr> Mod::TryConstFold(Expr a, Expr b) {
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
      int64_t res = pa->value % pb->value;
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa) {
      if (pa->value == 0)
        return a;
    }
    if (pb) {
      if (pb->value == 1)
        return IntImm(rtype, 0);
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
  });
  return Null;
}

Optional<Expr> FloorDiv::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
      int64_t res = floordiv(pa->value, pb->value);
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa) {
      if (pa->value == 0)
        return a;
    }
    if (pb) {
      if (pb->value == 1)
        return a;
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
    if (fa && fb && fb->value != 0) {
      if (rtype.bits == 32) {
        return FloatImm(
            rtype, GetFoldResultDoubleRepr(std::floor(static_cast<float>(fa->value) / static_cast<float>(fb->value))));
      } else if (rtype.bits == 64) {
        return FloatImm(rtype, std::floor(fa->value / fb->value));
      } else {
        return Null;
      }
    }
    if (fa && fa->value == 0)
      return a;
    if (fb) {
      if (fb->value == 1)
        return a;
      if (fb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
  });
  return Null;
}

Optional<Expr> FloorMod::TryConstFold(Expr a, Expr b) {
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb) {
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
      int64_t res = floormod(pa->value, pb->value);
      return IntImm(rtype, GetFoldResultInt64Repr(res, rtype));
    }
    if (pa) {
      if (pa->value == 0)
        return a;
    }
    if (pb) {
      if (pb->value == 1)
        return IntImm(rtype, 0);
      if (pb->value == 0) {
        MLC_THROW(ValueError) << "Divide by zero";
      }
    }
  });
  return Null;
}

Optional<Expr> Min::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb)
      return IntImm(rtype, std::min(pa->value, pb->value));
    if (fa && fb)
      return FloatImm(rtype, std::min(fa->value, fb->value));
  });
  if (a.same_as(b))
    return a;
  return Null;
}

Optional<Expr> Max::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb)
      return IntImm(rtype, std::max(pa->value, pb->value));
    if (fa && fb)
      return FloatImm(rtype, std::max(fa->value, fb->value));
  });
  if (a.same_as(b))
    return a;
  return Null;
}

Optional<Expr> GT::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value > pb->value);
    if (fa && fb)
      return BoolImm(fa->value > fb->value); // dtype: uint1
  });
  return Null;
}

Optional<Expr> GE::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value >= pb->value);
    if (fa && fb)
      return BoolImm(fa->value >= fb->value);
  });
  return Null;
}

Optional<Expr> LT::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value < pb->value);
    if (fa && fb)
      return BoolImm(fa->value < fb->value);
  });
  return Null;
}

Optional<Expr> LE::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value <= pb->value);
    if (fa && fb)
      return BoolImm(fa->value <= fb->value);
  });
  return Null;
}

Optional<Expr> EQ::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value == pb->value);
    if (fa && fb)
      return BoolImm(fa->value == fb->value);
  });
  return Null;
}

Optional<Expr> NE::TryConstFold(Expr a, Expr b) {
  TVM_ARITH_CONST_PROPAGATION({
    if (pa && pb)
      return BoolImm(pa->value != pb->value);
    if (fa && fb)
      return BoolImm(fa->value != fb->value);
  });
  return Null;
}

Optional<Expr> And::TryConstFold(Expr a, Expr b) {
  const IntImmObj *pa = a.as<IntImmObj>();
  const IntImmObj *pb = b.as<IntImmObj>();
  if (pa && pa->value)
    return b;
  if (pa && !pa->value)
    return a;
  if (pb && pb->value)
    return a;
  if (pb && !pb->value)
    return b;
  return Null;
}

Optional<Expr> Or::TryConstFold(Expr a, Expr b) {
  const IntImmObj *pa = a.as<IntImmObj>();
  const IntImmObj *pb = b.as<IntImmObj>();
  if (pa && pa->value)
    return a;
  if (pa && !pa->value)
    return b;
  if (pb && pb->value)
    return b;
  if (pb && !pb->value)
    return a;
  return Null;
}

Optional<Expr> Not::TryConstFold(Expr a) {
  const IntImmObj *pa = a.as<IntImmObj>();
  if (pa)
    return BoolImm(!pa->value);
  return Null;
}

// Section 3. Expr functor
// Section 3.1. ExprVisitor

template <typename T, typename F> inline void VisitArray(const List<T> &arr, F fvisit) {
  for (int64_t i = 0; i < arr.size(); i++) {
    fvisit(arr[i]);
  }
}

void ExprVisitor::VisitExpr_(const VarObj *) {}
void ExprVisitor::VisitExpr_(const ShapeVarObj *op) { this->VisitExpr_(reinterpret_cast<const VarObj *>(op)); }
void ExprVisitor::VisitExpr_(const IntImmObj *) {}
void ExprVisitor::VisitExpr_(const BoolImmObj *op) { this->VisitExpr_(reinterpret_cast<const IntImmObj *>(op)); }
void ExprVisitor::VisitExpr_(const FloatImmObj *) {}
void ExprVisitor::VisitExpr_(const CastObj *op) { this->VisitExpr(op->value); }
void ExprVisitor::VisitExpr_(const AddObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const SubObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const MulObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const DivObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const ModObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const FloorDivObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const FloorModObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const MinObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const MaxObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const EQObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const NEObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const LTObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const LEObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const GTObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const GEObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const AndObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const OrObj *op) {
  this->VisitExpr(op->a);
  this->VisitExpr(op->b);
}
void ExprVisitor::VisitExpr_(const NotObj *op) { this->VisitExpr(op->a); }
void ExprVisitor::VisitExpr_(const SelectObj *op) {
  this->VisitExpr(op->cond);
  this->VisitExpr(op->true_value);
  this->VisitExpr(op->false_value);
}
void ExprVisitor::VisitExpr_(const RampObj *op) {
  this->VisitExpr(op->base);
  this->VisitExpr(op->stride);
}
void ExprVisitor::VisitExpr_(const BroadcastObj *op) { this->VisitExpr(op->value); }
void ExprVisitor::VisitExpr_(const LetObj *op) {
  this->VisitExpr(op->var);
  this->VisitExpr(op->value);
  this->VisitExpr(op->body);
}
void ExprVisitor::VisitExpr_(const CallObj *op) {
  VisitArray(op->args, [this](const Expr &e) { this->VisitExpr(e); });
}
void ExprVisitor::VisitExpr_(const ShuffleObj *op) {
  VisitArray(op->indices, [this](const Expr &e) { this->VisitExpr(e); });
  VisitArray(op->vectors, [this](const Expr &e) { this->VisitExpr(e); });
}

// Section 3.2. ExprMutator

template <typename T, typename F> inline List<T> MutateArray(const List<T> &arr, F fvisit) {
  bool changed = false;
  List<T> result;
  result.reserve(arr.size());
  for (int64_t i = 0; i < arr.size(); i++) {
    T a = arr[i];
    T b = fvisit(a);
    if (a.get() != b.get()) {
      changed = true;
    }
    result.push_back(b);
  }
  return changed ? result : arr;
}

Expr ExprMutator::VisitExpr_(const VarObj *op) { return Expr(op); }
Expr ExprMutator::VisitExpr_(const ShapeVarObj *op) { return this->VisitExpr_(reinterpret_cast<const VarObj *>(op)); }
Expr ExprMutator::VisitExpr_(const IntImmObj *op) { return Expr(op); }
Expr ExprMutator::VisitExpr_(const BoolImmObj *op) { return this->VisitExpr_(reinterpret_cast<const IntImmObj *>(op)); }
Expr ExprMutator::VisitExpr_(const FloatImmObj *op) { return Expr(op); }
Expr ExprMutator::VisitExpr_(const CastObj *op) {
  Expr value = this->VisitExpr(op->value);
  if (value.get() == op->value.get()) {
    return Expr(op);
  } else {
    return Cast(op->dtype, value);
  }
}
#define MLC_SYM_EXPR_MUTATOR_BIN_OP(ObjRef)                                                                            \
  Expr ExprMutator::VisitExpr_(const ObjRef::TObj *op) {                                                               \
    Expr a = this->VisitExpr(op->a);                                                                                   \
    Expr b = this->VisitExpr(op->b);                                                                                   \
    if (a.get() == op->a.get() && b.get() == op->b.get()) {                                                            \
      return Expr(op);                                                                                                 \
    } else {                                                                                                           \
      return ObjRef(a->dtype, a, b);                                                                                   \
    }                                                                                                                  \
  }
#define MLC_SYM_EXPR_MUTATOR_CMP_OP(ObjRef)                                                                            \
  Expr ExprMutator::VisitExpr_(const ObjRef::TObj *op) {                                                               \
    Expr a = this->VisitExpr(op->a);                                                                                   \
    Expr b = this->VisitExpr(op->b);                                                                                   \
    if (a.get() == op->a.get() && b.get() == op->b.get()) {                                                            \
      return Expr(op);                                                                                                 \
    } else {                                                                                                           \
      return ObjRef(DType::Bool(a->dtype.lanes), a, b);                                                                \
    }                                                                                                                  \
  }
MLC_SYM_EXPR_MUTATOR_BIN_OP(Add)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Sub)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Mul)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Div)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Mod)
MLC_SYM_EXPR_MUTATOR_BIN_OP(FloorDiv)
MLC_SYM_EXPR_MUTATOR_BIN_OP(FloorMod)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Min)
MLC_SYM_EXPR_MUTATOR_BIN_OP(Max)
MLC_SYM_EXPR_MUTATOR_CMP_OP(EQ)
MLC_SYM_EXPR_MUTATOR_CMP_OP(NE)
MLC_SYM_EXPR_MUTATOR_CMP_OP(LT)
MLC_SYM_EXPR_MUTATOR_CMP_OP(LE)
MLC_SYM_EXPR_MUTATOR_CMP_OP(GT)
MLC_SYM_EXPR_MUTATOR_CMP_OP(GE)
MLC_SYM_EXPR_MUTATOR_CMP_OP(And)
MLC_SYM_EXPR_MUTATOR_CMP_OP(Or)
#undef MLC_SYM_EXPR_MUTATOR_CMP_OP
#undef MLC_SYM_EXPR_MUTATOR_BIN_OP

Expr ExprMutator::VisitExpr_(const NotObj *op) {
  Expr a = this->VisitExpr(op->a);
  if (a.get() == op->a.get()) {
    return Expr(op);
  } else {
    return Not(DType::Bool(a->dtype.lanes), a);
  }
}
Expr ExprMutator::VisitExpr_(const SelectObj *op) {
  Expr cond = this->VisitExpr(op->cond);
  Expr true_value = this->VisitExpr(op->true_value);
  Expr false_value = this->VisitExpr(op->false_value);
  if (cond.get() == op->cond.get() && true_value.get() == op->true_value.get() &&
      false_value.get() == op->false_value.get()) {
    return Expr(op);
  } else {
    return Select(true_value->dtype, cond, true_value, false_value);
  }
}
Expr ExprMutator::VisitExpr_(const RampObj *op) {
  Expr base = this->VisitExpr(op->base);
  Expr stride = this->VisitExpr(op->stride);
  int64_t lanes = op->lanes;
  if (base.get() == op->base.get() && stride.get() == op->stride.get()) {
    return Expr(op);
  } else {
    return Ramp(base, stride, lanes);
  }
}
Expr ExprMutator::VisitExpr_(const BroadcastObj *op) {
  Expr value = this->VisitExpr(op->value);
  int64_t lanes = op->lanes;
  DLDataType dtype = value->dtype;
  dtype.lanes = static_cast<uint16_t>(lanes);
  if (value.get() == op->value.get()) {
    return Expr(op);
  } else {
    return Broadcast(dtype, value, lanes);
  }
}
Expr ExprMutator::VisitExpr_(const LetObj *op) {
  Expr value = this->VisitExpr(op->value);
  Expr body = this->VisitExpr(op->body);
  if (value.get() == op->value.get() && body.get() == op->body.get()) {
    return Expr(op);
  } else {
    return Let(body->dtype, op->var, value, body);
  }
}
Expr ExprMutator::VisitExpr_(const CallObj *op) {
  List<Expr> args = MutateArray(op->args, [this](const Expr &e) { return this->VisitExpr(e); });
  if (args.get() == op->args.get()) {
    return Expr(op);
  } else {
    return Call(op->dtype, op->op, args);
  }
}
Expr ExprMutator::VisitExpr_(const ShuffleObj *op) {
  List<Expr> indices = MutateArray(op->indices, [this](const Expr &e) { return this->VisitExpr(e); });
  List<Expr> vectors = MutateArray(op->vectors, [this](const Expr &e) { return this->VisitExpr(e); });
  DLDataType dtype = vectors[0]->dtype;
  dtype.lanes = static_cast<uint16_t>(indices.size());
  if (indices.get() == op->indices.get() && vectors.get() == op->vectors.get()) {
    return Expr(op);
  } else {
    return Shuffle(dtype, indices, vectors);
  }
}

// Section 3.3. ExprDeepEqual

bool ExprDeepEqual::VisitExpr_(const VarObj *lhs, void *_rhs) {
  VarObj *rhs = static_cast<VarObj *>(_rhs);
  return lhs == rhs;
}
bool ExprDeepEqual::VisitExpr_(const ShapeVarObj *lhs, void *_rhs) {
  ShapeVarObj *rhs = static_cast<ShapeVarObj *>(_rhs);
  return lhs == rhs;
}
bool ExprDeepEqual::VisitExpr_(const IntImmObj *lhs, void *_rhs) {
  IntImmObj *rhs = static_cast<IntImmObj *>(_rhs);
  return lhs->value == rhs->value;
}
bool ExprDeepEqual::VisitExpr_(const BoolImmObj *lhs, void *_rhs) {
  BoolImmObj *rhs = static_cast<BoolImmObj *>(_rhs);
  return lhs->value == rhs->value;
}
bool ExprDeepEqual::VisitExpr_(const FloatImmObj *lhs, void *_rhs) {
  FloatImmObj *rhs = static_cast<FloatImmObj *>(_rhs);
  return lhs->value == rhs->value;
}
bool ExprDeepEqual::VisitExpr_(const CastObj *lhs, void *_rhs) {
  CastObj *rhs = static_cast<CastObj *>(_rhs);
  return DType::Equal(lhs->dtype, rhs->dtype) && VisitExpr(lhs->value, rhs->value.get());
}
#define MLC_SYM_EXPR_DEEP_EQ_BIN_OP(Obj)                                                                               \
  bool ExprDeepEqual::VisitExpr_(const Obj *lhs, void *_rhs) {                                                         \
    Obj *rhs = static_cast<Obj *>(_rhs);                                                                               \
    return VisitExpr(lhs->a, rhs->a.get()) && VisitExpr(lhs->b, rhs->b.get());                                         \
  }
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(AddObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(SubObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(MulObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(DivObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(ModObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(FloorDivObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(FloorModObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(MinObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(MaxObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(EQObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(NEObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(LTObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(LEObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(GTObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(GEObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(AndObj)
MLC_SYM_EXPR_DEEP_EQ_BIN_OP(OrObj)
#undef MLC_SYM_EXPR_DEEP_EQ_BIN_OP

bool ExprDeepEqual::VisitExpr_(const NotObj *lhs, void *_rhs) {
  NotObj *rhs = static_cast<NotObj *>(_rhs);
  return VisitExpr(lhs->a, rhs->a.get());
}
bool ExprDeepEqual::VisitExpr_(const SelectObj *lhs, void *_rhs) {
  SelectObj *rhs = static_cast<SelectObj *>(_rhs);
  return VisitExpr(lhs->cond, rhs->cond.get()) && VisitExpr(lhs->true_value, rhs->true_value.get()) &&
         VisitExpr(lhs->false_value, rhs->false_value.get());
}
bool ExprDeepEqual::VisitExpr_(const RampObj *lhs, void *_rhs) {
  RampObj *rhs = static_cast<RampObj *>(_rhs);
  return lhs->lanes == rhs->lanes && VisitExpr(lhs->base, rhs->base.get()) && VisitExpr(lhs->stride, rhs->stride.get());
}
bool ExprDeepEqual::VisitExpr_(const BroadcastObj *lhs, void *_rhs) {
  BroadcastObj *rhs = static_cast<BroadcastObj *>(_rhs);
  return lhs->lanes == rhs->lanes && VisitExpr(lhs->value, rhs->value.get());
}
bool ExprDeepEqual::VisitExpr_(const LetObj *lhs, void *_rhs) {
  LetObj *rhs = static_cast<LetObj *>(_rhs);
  return VisitExpr(lhs->var, rhs->var.get()) && VisitExpr(lhs->value, rhs->value.get()) &&
         VisitExpr(lhs->body, rhs->body.get());
}
bool ExprDeepEqual::VisitExpr_(const CallObj *lhs, void *_rhs) {
  CallObj *rhs = static_cast<CallObj *>(_rhs);
  const OpObj *lhs_op = lhs->op.as<OpObj>();
  const OpObj *rhs_op = rhs->op.as<OpObj>();
  if (lhs_op == nullptr || rhs_op == nullptr) {
    MLC_THROW(InternalError) << "`Call::op` must be `Op` in ExprDeepEqual";
  }
  if (lhs_op != rhs_op)
    return false;
  if (lhs->args.size() != rhs->args.size())
    return false;
  for (int64_t i = 0; i < lhs->args.size(); ++i)
    if (!this->Visit(lhs->args[i], rhs->args[i]))
      return false;
  return true;
}
bool ExprDeepEqual::VisitExpr_(const ShuffleObj *lhs, void *_rhs) {
  ShuffleObj *rhs = static_cast<ShuffleObj *>(_rhs);
  if (lhs->vectors.size() != rhs->vectors.size() || lhs->indices.size() != rhs->indices.size())
    return false;
  for (int64_t i = 0; i < lhs->vectors.size(); ++i)
    if (!this->Visit(lhs->vectors[i], rhs->vectors[i]))
      return false;
  for (int64_t i = 0; i < lhs->indices.size(); ++i)
    if (!this->Visit(lhs->indices[i], rhs->indices[i]))
      return false;
  return true;
}

// Section 4. Operations

void BinaryOpMatchTypes(Expr &lhs, Expr &rhs) { // NOLINT(*)
  // TODO: recover float8-related handling
  DLDataType ltype = lhs->dtype;
  DLDataType rtype = rhs->dtype;
  if (DType::Equal(lhs->dtype, rhs->dtype))
    return;
  if (ltype.lanes == 1 && rtype.lanes != 1) {
    lhs = Broadcast(lhs, rtype.lanes);
    ltype = lhs->dtype;
  } else if (ltype.lanes != 1 && rtype.lanes == 1) {
    rhs = Broadcast(rhs, ltype.lanes);
    rtype = rhs->dtype;
  } else if (ltype.lanes != rtype.lanes) {
    MLC_THROW(ValueError) << "Incompatible broadcast types: " << DType::Str(ltype) << " vs " << DType::Str(rtype);
  }
  if (DType::Equal(ltype, rtype))
    return;
  // We keep dtypes conversion to be relatively consistent to reduce the amount code generated by
  // operators. This can be helpful for users to find potential type conversion problems. The
  // following are exceptions:
  if (ltype.code == kDLFloat && rtype.code == kDLFloat) {
    // Given two dissimilar floats, cast the lower bit version to the higher bit version.
    // E.g. fp16 + fp32 --> fp32 + fp32
    if (ltype.bits < rtype.bits) {
      lhs = cast(rtype, lhs);
    } else {
      rhs = cast(ltype, rhs);
    }
  } else if (ltype.code != kDLFloat && rtype.code == kDLFloat) {
    // Cast int->float when the other operand is a float
    lhs = cast(rtype, lhs);
  } else if (ltype.code == kDLFloat && rtype.code != kDLFloat) {
    // Cast int->float when the other operand is a float
    rhs = cast(ltype, rhs);
  } else if (ltype.code != kDLBfloat && (rtype.code == kDLBfloat)) {
    // Cast int->bfloat16 when the other operand is a bfloat16
    lhs = cast(rtype, lhs);
  } else if (ltype.code == kDLBfloat && rtype.code != kDLBfloat) {
    // Cast int->bfloat16 when the other operand is a bfloat16
    rhs = cast(ltype, rhs);
  } else if ((ltype.code == kDLInt && rtype.code == kDLInt) || (ltype.code == kDLUInt && rtype.code == kDLUInt)) {
    // Promote int to higher bits e.g. int8 + int16 --> int16 + int16
    if (ltype.bits < rtype.bits) {
      lhs = cast(rtype, lhs);
    } else {
      rhs = cast(ltype, rhs);
    }
  } else if ((ltype.code == kDLInt && rtype.code == kDLUInt) || (ltype.code == kDLUInt && rtype.code == kDLInt)) {
    // Handle mixing signed and unsigned integers
    if (ltype.bits < rtype.bits) {
      lhs = cast(rtype, lhs);
    } else if (ltype.bits > rtype.bits) {
      rhs = cast(ltype, rhs);
    } else {
      // The width of signed and unsigned integers is same.
      if (ltype.code == kDLUInt) {
        rhs = cast(ltype, rhs);
      } else {
        lhs = cast(rtype, lhs);
      }
    }
  } else {
    MLC_THROW(ValueError) << "Cannot match type " << DType::Str(ltype) << " vs " << DType::Str(rtype);
  }
}

Expr cast(DLDataType t, Expr value) {
  if (DType::Equal(value->dtype, t))
    return value;
  // const fold IntImm as they are used in index computations
  if (t.lanes == 1) {
    if (const IntImmObj *i = value.as<IntImmObj>()) {
      return Expr::Const(t, i->value);
    } else if (const FloatImmObj *f = value.as<FloatImmObj>()) {
      return Expr::Const(t, f->value);
    } else if (value->dtype.code == kDLOpaqueHandle) {
      MLC_THROW(ValueError) << "Cannot cast opaque handle to other types";
    }
    return Cast(t, value);
  }
  DLDataType vtype{t.code, t.bits, 1};
  if (value->dtype.lanes == 1) {
    // manually unroll cast
    if (!DType::Equal(value->dtype, vtype)) {
      if (const IntImmObj *i = value.as<IntImmObj>()) {
        value = Expr::Const(vtype, i->value);
      } else if (const FloatImmObj *f = value.as<FloatImmObj>()) {
        value = Expr::Const(vtype, f->value);
      } else {
        value = Cast(vtype, value);
      }
    }
    return Broadcast(value, t.lanes);
  }
  if (value->dtype.lanes != t.lanes) {
    MLC_THROW(ValueError) << "Cannot cast between vectors of different lanes";
  }
  if (const auto *broadcast = value.as<BroadcastObj>()) {
    return Broadcast(cast(vtype, broadcast->value), t.lanes);
  }
  if (const auto *ramp = value.as<RampObj>()) {
    if (t.code == kDLInt || t.code == kDLUInt) {
      return Ramp(cast(vtype, ramp->base), cast(vtype, ramp->stride), ramp->lanes);
    }
  }
  return Cast(t, value);
}

Expr add(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = Add::TryConstFold(a, b))
    return ret.value();
  return Add(a->dtype, a, b);
}

Expr sub(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = Sub::TryConstFold(a, b))
    return ret.value();
  return Sub(a->dtype, a, b);
}

Expr mul(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = Mul::TryConstFold(a, b))
    return ret.value();
  return Mul(a->dtype, a, b);
}

Expr neg(Expr a) {
  const IntImmObj *pa = a.as<IntImmObj>();
  const FloatImmObj *fa = a.as<FloatImmObj>();
  if (pa)
    return IntImm(a->dtype, -pa->value);
  if (fa)
    return FloatImm(a->dtype, -fa->value);
  return sub(Expr::Const(a->dtype, 0), a);
}

#define MLC_SYM_EXPECT_INT_OR_UINT(dtype)                                                                              \
  if (dtype.code != kDLInt && dtype.code != kDLUInt) {                                                                 \
    MLC_THROW(ValueError) << "Expected integer type, but get: " << DType::Str(dtype);                                  \
  }

Expr truncdiv(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  if (auto ret = Div::TryConstFold(a, b))
    return ret.value();
  return Div(a->dtype, a, b);
}

Expr truncmod(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  if (auto ret = Mod::TryConstFold(a, b))
    return ret.value();
  return Mod(a->dtype, a, b);
}

Expr floordiv(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  if (auto ret = FloorDiv::TryConstFold(a, b))
    return ret.value();
  return FloorDiv(a->dtype, a, b);
}

Expr floormod(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = FloorMod::TryConstFold(a, b))
    return ret.value();
  return FloorMod(a->dtype, a, b);
}

Expr min(Expr a, Expr b) {
  // inf-aware simplificaiton
  if (is_pos_inf(a))
    return b;
  if (is_neg_inf(a))
    return a;
  if (is_pos_inf(b))
    return a;
  if (is_neg_inf(b))
    return b;
  BinaryOpMatchTypes(a, b);
  if (auto ret = Min::TryConstFold(a, b))
    return ret.value();
  return Min(a->dtype, a, b);
}

Expr max(Expr a, Expr b) {
  // inf-aware simplificaiton
  if (is_pos_inf(a))
    return a;
  if (is_neg_inf(a))
    return b;
  if (is_pos_inf(b))
    return b;
  if (is_neg_inf(b))
    return a;
  BinaryOpMatchTypes(a, b);
  if (auto ret = Max::TryConstFold(a, b))
    return ret.value();
  return Max(a->dtype, a, b);
}

Expr max_value(DLDataType dtype) {
  constexpr auto i64_max = std::numeric_limits<int64_t>::max();
  constexpr auto f64_max = std::numeric_limits<double>::max();
  constexpr auto f32_max = std::numeric_limits<float>::max();
  if (dtype.lanes != 1) {
    MLC_THROW(ValueError) << "Can't obtain max value for vector dtypes";
  }
  if (dtype.code == kDLInt) {
    if (dtype.bits == 64) {
      return IntImm(dtype, i64_max);
    } else if (dtype.bits < 64) {
      int64_t val = 1;
      val <<= dtype.bits - 1;
      val -= 1;
      return IntImm(dtype, val);
    }
  } else if (dtype.code == kDLUInt) {
    if (dtype.bits == 64) {
      // NOTE: we don't support `u64_max` for overflow concerns
      return IntImm(dtype, i64_max);
    } else if (dtype.bits < 64) {
      uint64_t val = 1;
      val <<= dtype.bits;
      val -= 1;
      return IntImm(dtype, static_cast<int64_t>(val));
    }
  } else if (dtype.code == kDLFloat) {
    if (dtype.bits == 64) {
      return FloatImm(dtype, f64_max);
    } else if (dtype.bits == 32) {
      return FloatImm(dtype, f32_max);
    } else if (dtype.bits == 16) {
      return FloatImm(dtype, 65504.0);
    }
  } else if (dtype.code == kDLBfloat) {
    return FloatImm(dtype, f32_max);
  }
  // TODO: support float8 and float4
  MLC_THROW(ValueError) << "Cannot decide max_value for type" << DType::Str(dtype);
  MLC_UNREACHABLE();
}

Expr min_value(DLDataType dtype) {
  constexpr auto f64_min = std::numeric_limits<double>::lowest();
  constexpr auto f32_min = std::numeric_limits<float>::lowest();
  if (dtype.lanes != 1) {
    MLC_THROW(ValueError) << "Can't obtain max value for vector dtypes";
  }
  if (dtype.code == kDLInt) {
    if (dtype.bits == 64) {
      return IntImm(dtype, std::numeric_limits<int64_t>::lowest());
    } else if (dtype.bits < 64) {
      int64_t val = 1;
      val <<= dtype.bits - 1;
      val = -val;
      return IntImm(dtype, val);
    }
  } else if (dtype.code == kDLUInt) {
    return IntImm(dtype, 0);
  } else if (dtype.code == kDLFloat) {
    if (dtype.bits == 64) {
      return FloatImm(dtype, f64_min);
    } else if (dtype.bits == 32) {
      return FloatImm(dtype, f32_min);
    } else if (dtype.bits == 16) {
      return FloatImm(dtype, -65504.0);
    }
  } else if (dtype.code == kDLBfloat) {
    return FloatImm(dtype, f32_min);
  }
  // TODO: support float8 and float4
  MLC_THROW(ValueError) << "Cannot decide max_value for type" << DType::Str(dtype);
  MLC_UNREACHABLE();
}

Expr if_then_else(Expr cond, Expr true_value, Expr false_value) {
  if (!DType::IsBool(cond->dtype)) {
    MLC_THROW(ValueError) << "if_then_else only accept the condition to be boolean type.";
  }
  BinaryOpMatchTypes(true_value, false_value);
  if (const IntImmObj *op = cond.as<IntImmObj>()) {
    if (op->value != 0) {
      return true_value;
    } else {
      return false_value;
    }
  }
  return Call(true_value->dtype, Op_::if_then_else, {cond, true_value, false_value});
}

Expr select(Expr cond, Expr true_value, Expr false_value) {
  if (!DType::IsBool(cond->dtype)) {
    MLC_THROW(ValueError) << "if_then_else only accept the condition to be boolean type.";
  }
  BinaryOpMatchTypes(true_value, false_value);
  if (const IntImmObj *op = cond.as<IntImmObj>()) {
    if (op->value != 0) {
      return true_value;
    } else {
      return false_value;
    }
  }
  return Select(true_value->dtype, cond, true_value, false_value);
}

Expr greater(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = GT::TryConstFold(a, b))
    return ret.value();
  return GT(a, b);
}

Expr greater_equal(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = GE::TryConstFold(a, b))
    return ret.value();
  return GE(a, b);
}

Expr less(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = LT::TryConstFold(a, b))
    return ret.value();
  return LT(a, b);
}

Expr less_equal(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = LE::TryConstFold(a, b))
    return ret.value();
  return LE(a, b);
}

Expr equal(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = EQ::TryConstFold(a, b))
    return ret.value();
  return EQ(a, b);
}

Expr not_equal(Expr a, Expr b) {
  BinaryOpMatchTypes(a, b);
  if (auto ret = NE::TryConstFold(a, b))
    return ret.value();
  return NE(a, b);
}

#define MLC_SYM_EXPECT_BOOLEAN(dtype)                                                                                  \
  if (!DType::IsBool(dtype)) {                                                                                         \
    MLC_THROW(ValueError) << "Expected boolean type, but get: " << DType::Str(dtype);                                  \
  }

Expr logical_and(Expr a, Expr b) {
  MLC_SYM_EXPECT_BOOLEAN(a->dtype);
  MLC_SYM_EXPECT_BOOLEAN(b->dtype);
  if (auto ret = And::TryConstFold(a, b))
    return ret.value();
  return And(a, b);
}

Expr logical_or(Expr a, Expr b) {
  MLC_SYM_EXPECT_BOOLEAN(a->dtype);
  MLC_SYM_EXPECT_BOOLEAN(b->dtype);
  if (auto ret = Or::TryConstFold(a, b))
    return ret.value();
  return Or(a, b);
}

Expr logical_not(Expr a) {
  MLC_SYM_EXPECT_BOOLEAN(a->dtype);
  if (auto ret = Not::TryConstFold(a))
    return ret.value();
  return Not(a);
}

Expr right_shift(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pb) {
      if (pb->value < 0 || pb->value >= rtype.bits) {
        MLC_THROW(ValueError) << "Shift amount must be non-negative and less than " << rtype.bits << " bit(s) for type "
                              << DType::Str(rtype);
      }
    }
    if (pa && pb) {
      return IntImm(rtype, (pa->value >> pb->value));
    }
    if (pb) {
      if (pb->value == 0)
        return a;
    }
  });
  return Call(a->dtype, Op_::right_shift, {a, b});
}

Expr left_shift(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pb) {
      if (pb->value < 0 || pb->value >= rtype.bits) {
        MLC_THROW(ValueError) << "Shift amount must be non-negative and less than " << rtype.bits << " bit(s) for type "
                              << DType::Str(rtype);
      }
    }
    if (pa && pb)
      return IntImm(rtype, (pa->value << pb->value));
    if (pb) {
      if (pb->value == 0)
        return a;
    }
  });
  return Call(a->dtype, Op_::left_shift, {a, b});
}

Expr bitwise_and(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb)
      return IntImm(rtype, (pa->value & pb->value));
  });
  return Call(a->dtype, Op_::bitwise_and, {a, b});
}

Expr bitwise_or(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb)
      return IntImm(rtype, (pa->value | pb->value));
  });
  return Call(a->dtype, Op_::bitwise_or, {a, b});
}

Expr bitwise_xor(Expr a, Expr b) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  MLC_SYM_EXPECT_INT_OR_UINT(b->dtype);
  BinaryOpMatchTypes(a, b);
  TVM_INDEX_CONST_PROPAGATION({
    DLDataType rtype = a->dtype;
    if (pa && pb)
      return IntImm(rtype, (pa->value ^ pb->value));
  });
  return Call(a->dtype, Op_::bitwise_xor, {a, b});
}

Expr bitwise_neg(Expr a) {
  MLC_SYM_EXPECT_INT_OR_UINT(a->dtype);
  return Call(a->dtype, Op_::bitwise_not, {a});
}

Expr abs(Expr x) {
  DLDataType dtype = x->dtype;
  if (dtype.code == kDLInt) {
    const IntImmObj *px = x.as<IntImmObj>();
    if (px) {
      return IntImm(dtype, std::abs(px->value));
    }
    return Select(dtype, x >= 0, x, -x);
  } else if (dtype.code == kDLFloat || dtype.code == kDLBfloat) {
    const FloatImmObj *fx = x.as<FloatImmObj>();
    if (fx) {
      return FloatImm(dtype, std::fabs(fx->value));
    }
    return Call(dtype, Op_::fabs, {x});
  } else if (dtype.code == kDLUInt) {
    return x;
  }
  MLC_THROW(ValueError) << "Data type not supported for `abs`: " << DType::Str(dtype);
  throw;
}

// Section 5. Constraint extraction

template <typename F> void CollectConstraints(Expr expr, F callback, bool keep_composite_constraints) {
  if (keep_composite_constraints) {
    callback(expr);
  }
  PVar<Expr> x, y;
  if ((x && y).Match(expr)) {
    CollectConstraints(x.Eval(), callback, keep_composite_constraints);
    CollectConstraints(y.Eval(), callback, keep_composite_constraints);
  } else if (!keep_composite_constraints) {
    callback(expr);
  }
}

template <typename F> void CollectComponents(Expr expr, F callback) {
  PVar<Expr> x, y;
  if ((x || y).Match(expr)) {
    CollectComponents(x.Eval(), callback);
    CollectComponents(y.Eval(), callback);
  } else {
    callback(expr);
  }
}

std::vector<Expr> ExtractConstraints(const Expr &expr, bool keep_composite_constraints) {
  std::vector<Expr> out;
  CollectConstraints(
      expr,
      [&](const Expr &part) { //
        out.push_back(part);
      },
      keep_composite_constraints);
  return out;
}

std::vector<Expr> ExtractComponents(const Expr &expr) {
  std::vector<Expr> out;
  CollectComponents(expr, [&](const Expr &part) { out.push_back(part); });
  return out;
}

// Section 6: ConstraintContext

ConstraintContext::ConstraintContext(AnalyzerObj::Impl *analyzer, Expr constraint)
    : analyzer_(analyzer), constraint_(constraint) {
  // ICHECK(recovery_functions_.size() == 0);
  recovery_functions_.push_back(analyzer_->const_int_bound.EnterConstraint(constraint_));
  recovery_functions_.push_back(analyzer_->modular_set.EnterConstraint(constraint_));
  recovery_functions_.push_back(analyzer_->rewrite_simplify.EnterConstraint(constraint_));
  recovery_functions_.push_back(analyzer_->interval_set.EnterConstraint(constraint_));
  recovery_functions_.push_back(analyzer_->transitive_comparisons.EnterConstraint(constraint_));
}

ConstraintContext::~ConstraintContext() {
  while (recovery_functions_.size()) {
    if (auto &func = recovery_functions_.back(); func) {
      func();
    }
    recovery_functions_.pop_back();
  }
}

// Section 7. Conjunctive Normal Form

namespace {
class AndOfOrs {
public:
  explicit AndOfOrs(const Expr &expr);
  Expr AsExpr() const;
  void Simplify(AnalyzerObj::Impl *analyzer);

  /*! \brief Internal utility, simplify within each group of expressions
   *
   * For each pair of values within a chunk, attempt to simplify them into
   * a single expression.
   *
   * For example,
   *    before = (a == 5) && ((b < 10) || (b > 10))
   *    after  = (a == 5) && ((b != 10) || false)
   */
  void SimplifyWithinChunks(AnalyzerObj::Impl *analyzer);
  /*! \brief Internal utility, simplify across groups of expressions
   *
   * For each pair of chunks, if the two chunks differ by only a single
   * term, attempt to simplify those differing terms.
   *
   * For example,
   *    before = ((a == 5) || (b <= 10)) && ((a == 5) || (b >= 10))
   *    after  = ((a == 5) || (b == 10)) && ((a == 5) || true)
   */
  void SimplifyAcrossChunks(AnalyzerObj::Impl *analyzer);
  /*! \brief Remove instances of true/false from internal representation
   *
   * To avoid invalidating iterators, `SimplifyWithinChunks` and
   * `SimplifyAcrossChunks` may replace keys, but may not remove keys
   * from the internal representation.  For example, `(a < 5) && (a <
   * 10)` would be simplified to `(a < 5) && true`.  The
   * `RemoveTrueFalse` function removes these leftover instances of
   * true/false.
   */
  void RemoveTrueFalse();

  /*! \brief Internal utility function used to convert to internal form */
  static void VisitAndExpressions(const Expr &expr, std::function<void(const Expr &)> callback);
  /*! \brief Internal utility function used to convert to internal form */
  static void VisitOrExpressions(const Expr &expr, std::function<void(const Expr &)> callback);

  /* \brief Type-safe wrapper class that represents an Expr
   *
   * Because integer indices are used frequently through this class,
   * maintaining a separation between integer indices used to access
   * specific elements of the internal representation, and unique
   * identifiers used to represent expressions Expr, is useful.
   */
  using Key = size_t;

  /*! \brief Convert a Expr to a Key */
  Key GetKey(const Expr &expr);

  /*! \brief Convert a Key to a Expr */
  Expr GetExpr(Key key) const;

  /*! \brief Attempt to simplify (a && b)
   *
   * If successful, will overwrite the parameters `a` and `b` with the
   * simplified form.
   */
  void TrySimplifyOr(Key *a, Key *b, AnalyzerObj::Impl *analyzer);

  /*! \brief Attempt to simplify (a || b)
   *
   * If successful, will overwrite the parameters `a` and `b` with the
   * simplified form.
   */
  void TrySimplifyAnd(Key *a, Key *b, AnalyzerObj::Impl *analyzer);

  /*! \brief The internal representation
   *
   * `chunks[i][j]` is the j-th expression in the i-th OR-group.
   */
  std::vector<std::vector<Key>> chunks_;

  /*! \brief Mapping from internal Key to Expr */
  std::unordered_map<Key, Expr> key_to_expr_;
  /*! \brief Mapping from Expr to internal Key */
  std::unordered_map<Expr, Key, StructuralHash, StructuralEqual<false>> expr_to_key_;
  /*! \brief Cached key representing tir::Bool(true) */
  Key key_true_;
  /*! \brief Cached key representing tir::Bool(false) */
  Key key_false_;
};

AndOfOrs::AndOfOrs(const Expr &expr)
    : key_true_(GetKey(BoolImm(true))), //
      key_false_(GetKey(BoolImm(false))) {
  VisitAndExpressions(expr, [&](const Expr &outer_expr) {
    std::vector<Key> or_components;
    VisitOrExpressions(outer_expr, [&](const Expr &inner_expr) {
      Key key = GetKey(inner_expr);
      bool is_duplicate =
          std::any_of(or_components.begin(), or_components.end(), [&](Key prev) { return prev == key; });
      if (!is_duplicate) {
        or_components.push_back(key);
      }
    });
    bool is_permutation = std::any_of(chunks_.begin(), chunks_.end(), [&](const std::vector<Key> &prev_components) {
      return or_components.size() == prev_components.size() &&
             std::is_permutation(prev_components.begin(), prev_components.end(), or_components.begin());
    });
    if (!is_permutation) {
      chunks_.push_back(std::move(or_components));
    }
  });
}

void AndOfOrs::VisitAndExpressions(const Expr &expr, std::function<void(const Expr &)> callback) {
  PVar<Expr> x, y, z;
  if ((x && y).Match(expr)) {
    // These are separate AND conditions, recurse into them in case
    // they contain AND internally.
    VisitAndExpressions(x.Eval(), callback);
    VisitAndExpressions(y.Eval(), callback);
  } else if ((x || y).Match(expr)) {
    // This may be the bottom-most breakdown, but either x or y may
    // themselves contain AND.  (e.g. (A && B) || (C && D) should be
    // split into (A || C), (A || D), (B || C), and (B || D).)
    // Recurse into each, then reconstruct an OR condition.
    VisitAndExpressions(x.Eval(), [&](const Expr &x_part) {
      VisitAndExpressions(y.Eval(), [&](const Expr &y_part) { callback(x_part || y_part); });
    });
  } else {
    // This is bottom-most breakdown.
    callback(expr);
  }
}

void AndOfOrs::VisitOrExpressions(const Expr &expr, std::function<void(const Expr &)> callback) {
  PVar<Expr> x, y, z;
  if ((x || y).Match(expr)) {
    // These are separate OR conditions, recurse into them in case
    // they contain OR internally.
    VisitOrExpressions(x.Eval(), callback);
    VisitOrExpressions(y.Eval(), callback);
  } else if ((x && y).Match(expr)) {
    // This may be the bottom-most breakdown, but either x or y may
    // themselves contain OR.  (e.g. (A || B) && (C || D) should be
    // split into (A && C), (A && D), (B && C), and (B && D).)
    // Recurse into each, then reconstruct an AND condition.
    VisitOrExpressions(x.Eval(), [&](const Expr &x_part) {
      VisitOrExpressions(y.Eval(), [&](const Expr &y_part) { callback(x_part && y_part); });
    });
  } else {
    // This is bottom-most breakdown.
    callback(expr);
  }
}

AndOfOrs::Key AndOfOrs::GetKey(const Expr &expr) {
  auto it = expr_to_key_.find(expr);
  if (it != expr_to_key_.end()) {
    return it->second;
  }

  Key key{expr_to_key_.size()};
  expr_to_key_[expr] = key;
  key_to_expr_.emplace(key, expr);
  return key;
}

Expr AndOfOrs::GetExpr(AndOfOrs::Key key) const {
  auto it = key_to_expr_.find(key);
  // TODO: recover
  // ICHECK(it != key_to_expr_.end());
  return it->second;
}

Expr AndOfOrs::AsExpr() const {
  Expr expr = BoolImm(true);
  for (const auto &chunk : chunks_) {
    Expr chunk_expr = BoolImm(false);
    for (Key j : chunk) {
      chunk_expr = chunk_expr || GetExpr(j);
    }
    expr = expr && chunk_expr;
  }
  return expr;
}

void AndOfOrs::TrySimplifyOr(Key *a_ptr, Key *b_ptr, AnalyzerObj::Impl *analyzer) {
  Key &a = *a_ptr;
  Key &b = *b_ptr;
  Expr joint = GetExpr(a) || GetExpr(b);
  Expr simplified = analyzer->rewrite_simplify(joint);
  if (!ExprDeepEqual()(simplified, joint)) {
    if (auto *simplified_or = simplified.as<OrObj>()) {
      a = GetKey(simplified_or->a);
      b = GetKey(simplified_or->b);
    } else {
      a = key_false_;
      b = GetKey(simplified);
    }
  }
}

void AndOfOrs::TrySimplifyAnd(Key *a_ptr, Key *b_ptr, AnalyzerObj::Impl *analyzer) {
  Key &a = *a_ptr;
  Key &b = *b_ptr;
  Expr joint = GetExpr(a) && GetExpr(b);
  Expr simplified = analyzer->rewrite_simplify(joint);
  if (!ExprDeepEqual()(simplified, joint)) {
    if (auto *simplified_and = simplified.as<AndObj>()) {
      a = GetKey(simplified_and->a);
      b = GetKey(simplified_and->b);
    } else {
      a = key_true_;
      b = GetKey(simplified);
    }
  }
}

void AndOfOrs::Simplify(AnalyzerObj::Impl *analyzer) {
  SimplifyWithinChunks(analyzer);
  RemoveTrueFalse();
  SimplifyAcrossChunks(analyzer);
  RemoveTrueFalse();
}

void AndOfOrs::SimplifyWithinChunks(AnalyzerObj::Impl *analyzer) {
  for (auto &chunk : chunks_) {
    for (size_t expr_i = 0; expr_i < chunk.size(); expr_i++) {
      for (size_t expr_j = expr_i + 1; expr_j < chunk.size(); expr_j++) {
        Key &key_i = chunk[expr_i];
        Key &key_j = chunk[expr_j];

        TrySimplifyOr(&key_i, &key_j, analyzer);
      }
    }
  }
}

void AndOfOrs::SimplifyAcrossChunks(AnalyzerObj::Impl *analyzer) {
  for (size_t i_and = 0; i_and < chunks_.size(); i_and++) {
    for (size_t j_and = i_and + 1; j_and < chunks_.size(); j_and++) {
      auto &i_chunk = chunks_[i_and];
      auto &j_chunk = chunks_[j_and];

      if (i_chunk.size() == 1 && j_chunk.size() == 1) {
        auto &key_i = i_chunk[0];
        auto &key_j = j_chunk[0];
        TrySimplifyAnd(&key_i, &key_j, analyzer);
        continue;
      }
      constexpr size_t kNonExist = std::numeric_limits<size_t>::max();
      std::unordered_set<Key> j_set(j_chunk.begin(), j_chunk.end());
      size_t i_distinct_index = kNonExist;
      for (size_t i = 0; i < i_chunk.size(); i++) {
        if (!j_set.count(i_chunk[i])) {
          i_distinct_index = i;
          break;
        }
      }
      if (i_distinct_index == kNonExist) {
        // I = (i_0 || i_1 || ... || i_N)
        // J = (i_0 || i_1 || ... || i_N || j_0 || ... || j_N)
        // I && J == I == I && true

        j_chunk = {key_true_};
        continue;
      }
      std::unordered_set<Key> i_set(i_chunk.begin(), i_chunk.end());
      size_t j_distinct_index = kNonExist;
      for (size_t j = 0; j < j_chunk.size(); j++) {
        if (!i_set.count(j_chunk[j])) {
          j_distinct_index = j;
          break;
        }
      }

      if (j_distinct_index == kNonExist) {
        // I = (i_0 || ... || i_N || j_0 || ... || j_N)
        // J = (j_0 || ... || j_N)
        // I && J == J == true && J

        i_chunk = {key_true_};
        continue;
      }

      if (i_chunk.size() == j_chunk.size()) {
        size_t num_shared_exprs = 0;
        for (const auto &j_key : j_chunk) {
          if (i_set.count(j_key)) {
            ++num_shared_exprs;
          }
        }

        if (num_shared_exprs + 1 == i_chunk.size()) {
          // All but one of the expressions are shared.  If the AND
          // of the distinct expressions can be simplified, we can
          // replace.
          //
          // (A or B) and (A or C) => A or (B and C)
          auto &key_i = i_chunk[i_distinct_index];
          auto &key_j = j_chunk[j_distinct_index];

          // When attempting to simplify (B and C), the analyzer may
          // assume that A is false.
          Expr known = [&]() {
            Expr known = BoolImm(true);
            for (const auto &key : i_chunk) {
              if (&key != &key_i) {
                known = known && analyzer->Simplify(!GetExpr(key));
              }
            }
            return known;
          }();
          {
            ConstraintContext context(analyzer, known);
            TrySimplifyAnd(&key_i, &key_j, analyzer);
          }
        }
      }
    }
  }
}

void AndOfOrs::RemoveTrueFalse() {
  for (auto &chunk : chunks_) {
    // Any occurrence of True inside an OR makes the entire expression True.
    if (std::any_of(chunk.begin(), chunk.end(), [&](Key key) { return key == key_true_; })) {
      chunk = {key_true_};
    } else {
      // Any occurrence of False inside an OR can be removed
      chunk.erase(std::remove_if(chunk.begin(), chunk.end(), [&](Key key) { return key == key_false_; }), chunk.end());
    }
  }

  // Any occurence of False inside an AND makes the entire expression False.
  if (std::any_of(chunks_.begin(), chunks_.end(), [&](const std::vector<Key> &chunk) { return chunk.size() == 0; })) {
    chunks_ = {{}};
  } else {
    // Any occurrence of True inside an AND can be removed.
    chunks_.erase(
        std::remove_if(chunks_.begin(), chunks_.end(),
                       [&](const std::vector<Key> &chunk) { return chunk.size() == 1 && chunk[0] == key_true_; }),
        chunks_.end());
  }
}

// Helper utility for temporarily disabling the
// kConvertBooleanToAndOfOrs flag on an analyzer, to prevent infinite
// recursion.
class DisableAndOfOrRecursion {
public:
  explicit DisableAndOfOrRecursion(AnalyzerObj::Impl *analyzer)
      : analyzer_(analyzer), cached_flags_(analyzer->rewrite_simplify.GetEnabledExtensions()) {
    auto new_flags =
        static_cast<RewriteSimplifier::Extension>(cached_flags_ & (~RewriteSimplifier::kConvertBooleanToAndOfOrs));
    analyzer->rewrite_simplify.SetEnabledExtensions(new_flags);
  }
  ~DisableAndOfOrRecursion() { analyzer_->rewrite_simplify.SetEnabledExtensions(cached_flags_); }

  DisableAndOfOrRecursion(const DisableAndOfOrRecursion &) = delete;
  DisableAndOfOrRecursion &operator=(const DisableAndOfOrRecursion &) = delete;

  AnalyzerObj::Impl *analyzer_;
  RewriteSimplifier::Extension cached_flags_;
};

} // namespace

Expr SimplifyAsAndOfOrs(const Expr &expr, AnalyzerObj::Impl *analyzer) {
  DisableAndOfOrRecursion context(analyzer);
  AndOfOrs repr(analyzer->Simplify(expr));
  repr.Simplify(analyzer);
  return repr.AsExpr();
}

// Section 8. IRMutatorWithAnalyzer

// IRMutatorWithAnalyzer

Expr IRMutatorWithAnalyzer::VisitExpr_(const CallObj *call) {
  // add condition context to if_then_else
  if (Op_::if_then_else->Same(call->op)) {
    Expr cond = this->VisitExpr(call->args[0]);
    Expr true_value = [this, cond, e = call->args[1]]() {
      ConstraintContext constraint(this->analyzer_, cond);
      return this->VisitExpr(e);
    }();
    Expr false_value = [this, cond, e = call->args[2]]() {
      Expr not_cond = Not(cond);
      ConstraintContext constraint(this->analyzer_, not_cond);
      return this->VisitExpr(e);
    }();
    if (IsConstInt(cond, 0)) {
      return false_value;
    }
    if (IsConstInt(cond, 1)) {
      return true_value;
    }
    if (cond.get() == call->args[0].get() &&       //
        true_value.get() == call->args[1].get() && //
        false_value.get() == call->args[2].get()) {
      return Expr(call);
    } else {
      return Call(call->dtype, call->op, List<Expr>{cond, true_value, false_value});
    }
  }
  return ExprMutator::VisitExpr_(call);
}

Expr IRMutatorWithAnalyzer::VisitExpr_(const LetObj *op) {
  Expr value = this->VisitExpr(op->value);
  analyzer_->Bind(op->var, value);
  // We keep the let-binding here as sub-class may or maynot choose to replace it.
  Expr body = this->VisitExpr(op->body);
  if (value.get() == op->value.get() && body.get() == op->body.get()) {
    return Expr(op);
  } else {
    return Let(op->dtype, op->var, value, body);
  }
}

Expr IRMutatorWithAnalyzer::VisitExpr_(const SelectObj *op) {
  Expr cond = this->VisitExpr(op->cond);
  Expr true_value = [&]() {
    ConstraintContext constraint(analyzer_, cond);
    return VisitExpr(op->true_value);
  }();
  Expr false_value = [&]() {
    ConstraintContext constraint(analyzer_, analyzer_->rewrite_simplify(Not(cond)));
    return VisitExpr(op->false_value);
  }();
  if (IsConstInt(cond, 0)) {
    return false_value;
  }
  if (IsConstInt(cond, 1)) {
    return true_value;
  }
  // normal path
  if (cond.get() == op->cond.get() && true_value.get() == op->true_value.get() &&
      false_value.get() == op->false_value.get()) {
    return Expr(op);
  } else {
    return Select(cond, true_value, false_value);
  }
}

struct AnalyzerObj::Testing {
  // Those methods below are used only for testing
  template <typename Lambda> static void _Register(const char *name, Lambda func) {
    Lib::FuncSetGlobal(name, ::mlc::Func(func).get());
  }

  static int32_t Register() {
    _Register("mlc.sym._internal.Analyzer.ConstIntBound",
              [](AnalyzerObj *analyzer, Expr expr) { return analyzer->impl_->const_int_bound(expr); });
    _Register("mlc.sym._internal.Analyzer.ModularSet",
              [](AnalyzerObj *analyzer, Expr expr) { return analyzer->impl_->modular_set(expr); });
    _Register("mlc.sym._internal.Analyzer.RewriteSimplify",
              [](AnalyzerObj *analyzer, Expr expr) { return analyzer->impl_->rewrite_simplify(expr); });
    _Register("mlc.sym._internal.Analyzer.CanonicalSimplify",
              [](AnalyzerObj *analyzer, Expr expr) { return analyzer->impl_->canonical_simplify(expr); });
    _Register("mlc.sym._internal.Analyzer.IntervalSet",
              [](AnalyzerObj *analyzer, Expr expr, Dict<Var, IntervalSet> dom_map) {
                return analyzer->impl_->interval_set(expr, dom_map);
              });
    _Register("mlc.sym._internal.Analyzer.ConstIntBoundUpdate",
              [](AnalyzerObj *analyzer, Var var, ConstIntBound info, bool allow_override) {
                return analyzer->impl_->const_int_bound.Update(var, info, allow_override);
              });
    _Register("mlc.sym._internal.Analyzer.GetEnabledExtensions", [](AnalyzerObj *analyzer) {
      return static_cast<int64_t>(analyzer->impl_->rewrite_simplify.GetEnabledExtensions());
    });
    _Register("mlc.sym._internal.Analyzer.SetEnabledExtensions", [](AnalyzerObj *analyzer, int64_t flags) {
      return analyzer->impl_->rewrite_simplify.SetEnabledExtensions(static_cast<RewriteSimplifier::Extension>(flags));
    });
    _Register("mlc.sym._internal.Analyzer.EnterConstraint", [](AnalyzerObj *analyzer, Expr constraint) {
      return Func(
          [ctx = std::make_unique<ConstraintContext>(analyzer->impl_.get(), constraint)]() mutable { ctx.reset(); });
    });
    _Register("mlc.sym.op.cast", [](DLDataType dtype, Expr expr) { return cast(dtype, expr); });
    _Register("mlc.sym.op.add", [](Expr a, Expr b) { return add(a, b); });
    _Register("mlc.sym.op.sub", [](Expr a, Expr b) { return sub(a, b); });
    _Register("mlc.sym.op.mul", [](Expr a, Expr b) { return mul(a, b); });
    _Register("mlc.sym.op.neg", [](Expr a) { return neg(a); });
    _Register("mlc.sym.op.truncdiv", [](Expr a, Expr b) { return truncdiv(a, b); });
    _Register("mlc.sym.op.truncmod", [](Expr a, Expr b) { return truncmod(a, b); });
    _Register("mlc.sym.op.floordiv", [](Expr a, Expr b) { return floordiv(a, b); });
    _Register("mlc.sym.op.floormod", [](Expr a, Expr b) { return floormod(a, b); });
    _Register("mlc.sym.op.min", [](Expr a, Expr b) { return min(a, b); });
    _Register("mlc.sym.op.max", [](Expr a, Expr b) { return max(a, b); });
    _Register("mlc.sym.op.max_value", [](DLDataType dtype) { return max_value(dtype); });
    _Register("mlc.sym.op.min_value", [](DLDataType dtype) { return min_value(dtype); });
    _Register("mlc.sym.op.if_then_else",
              [](Expr cond, Expr true_value, Expr false_value) { return if_then_else(cond, true_value, false_value); });
    _Register("mlc.sym.op.select",
              [](Expr cond, Expr true_value, Expr false_value) { return select(cond, true_value, false_value); });
    _Register("mlc.sym.op.greater", [](Expr a, Expr b) { return greater(a, b); });
    _Register("mlc.sym.op.greater_equal", [](Expr a, Expr b) { return greater_equal(a, b); });
    _Register("mlc.sym.op.less", [](Expr a, Expr b) { return less(a, b); });
    _Register("mlc.sym.op.less_equal", [](Expr a, Expr b) { return less_equal(a, b); });
    _Register("mlc.sym.op.equal", [](Expr a, Expr b) { return equal(a, b); });
    _Register("mlc.sym.op.not_equal", [](Expr a, Expr b) { return not_equal(a, b); });
    _Register("mlc.sym.op.logical_and", [](Expr a, Expr b) { return logical_and(a, b); });
    _Register("mlc.sym.op.logical_or", [](Expr a, Expr b) { return logical_or(a, b); });
    _Register("mlc.sym.op.logical_not", [](Expr a) { return logical_not(a); });
    _Register("mlc.sym.op.right_shift", [](Expr a, Expr b) { return right_shift(a, b); });
    _Register("mlc.sym.op.left_shift", [](Expr a, Expr b) { return left_shift(a, b); });
    _Register("mlc.sym.op.bitwise_and", [](Expr a, Expr b) { return bitwise_and(a, b); });
    _Register("mlc.sym.op.bitwise_or", [](Expr a, Expr b) { return bitwise_or(a, b); });
    _Register("mlc.sym.op.bitwise_xor", [](Expr a, Expr b) { return bitwise_xor(a, b); });
    _Register("mlc.sym.op.bitwise_neg", [](Expr a) { return bitwise_neg(a); });
    _Register("mlc.sym.op.abs", [](Expr x) { return abs(x); });
    return 0;
  }
};

[[maybe_unused]] int32_t reg = AnalyzerObj::Testing::Register();

AnalyzerObj::AnalyzerObj() : impl_(std::make_unique<Impl>()) {}
AnalyzerObj::~AnalyzerObj() {}
void AnalyzerObj::MarkGlobalNonNegValue(const Expr &value) { impl_->MarkGlobalNonNegValue(value); }
void AnalyzerObj::Bind(const Var &var, const Expr &expr, bool allow_override) {
  impl_->Bind(var, expr, allow_override);
}
void AnalyzerObj::Bind(const Var &var, const Range &range, bool allow_override) {
  impl_->Bind(var, range, allow_override);
}
void AnalyzerObj::Bind(const Dict<Var, Range> &variables, bool allow_override) {
  impl_->Bind(variables, allow_override);
}
bool AnalyzerObj::CanProveGreaterEqual(const Expr &expr, int64_t lower_bound) {
  return impl_->CanProveGreaterEqual(expr, lower_bound);
}
bool AnalyzerObj::CanProveLess(const Expr &expr, int64_t upper_bound) { return impl_->CanProveLess(expr, upper_bound); }
bool AnalyzerObj::CanProveEqual(const Expr &lhs, const Expr &rhs) { return impl_->CanProveEqual(lhs, rhs); }
bool AnalyzerObj::CanProveLessEqualThanSymbolicShapeValue(const Expr &lhs, const Expr &shape) {
  return impl_->CanProveLessEqualThanSymbolicShapeValue(lhs, shape);
}
bool AnalyzerObj::CanProve(const Expr &cond, ProofStrength strength) { return impl_->CanProve(cond, strength); }
Expr AnalyzerObj::Simplify(const Expr &expr, int steps) { return impl_->Simplify(expr, steps); }

} // namespace sym
} // namespace mlc
