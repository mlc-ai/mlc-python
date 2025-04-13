#ifndef MLC_SYM_EXPR_H_
#define MLC_SYM_EXPR_H_

#include <mlc/core/all.h>
#include <type_traits>

#define MLC_SYM_EXPORTS MLC_EXPORTS

namespace mlc {
namespace sym {
using mlc::base::DType;

struct OpObj {
  MLCAny _mlc_header;
  ::mlc::Str name;
  explicit OpObj(::mlc::Str name) : _mlc_header{}, name(name) {}
  inline bool Same(const Any &other) const {
    if (const OpObj *op = other.as<OpObj>()) {
      return this == op;
    }
    return false;
  }
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, OpObj, ::mlc::Object, "mlc.sym.Op");
}; // struct OpObj

struct Op : public ::mlc::ObjectRef {
  static constexpr bool is_logical = false;
  explicit Op(::mlc::Str name) : Op(Op::New(name)) {}
  static MLC_API Op Get(::mlc::Str name);
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Op, OpObj, ::mlc::ObjectRef)
      .Field("name", &OpObj::name)
      .Structure(StructureKind::kNoBind, {"name"})
      .StaticFn("get", Get)
      .StaticFn("__init__", ::mlc::InitOf<OpObj, ::mlc::Str>);
}; // struct Op
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct ExprObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ExprObj() = default;
  explicit ExprObj(DLDataType dtype) : _mlc_header{}, dtype(dtype) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, ExprObj, ::mlc::Object, "mlc.sym.Expr");
}; // struct ExprObj

struct Expr : public ::mlc::ObjectRef {
  static constexpr bool is_logical = false;
  static Expr Bool(bool value, int32_t lanes = 1);
  static Expr Int32(int64_t value, int32_t lanes = 1);
  static Expr Int64(int64_t value, int32_t lanes = 1);
  static Expr Float32(double value, int32_t lanes = 1);
  static Expr Float64(double value, int32_t lanes = 1);
  template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
  static Expr Const(DLDataType t, T value);
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Expr, ExprObj, ::mlc::ObjectRef).Field("dtype", &ExprObj::dtype);
}; // struct Expr
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct VarObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::Str name;
  explicit VarObj(DLDataType dtype, ::mlc::Str name) : _mlc_header{}, dtype(dtype), name(name) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, VarObj, ::mlc::sym::ExprObj, "mlc.sym.Var");
}; // struct VarObj

struct Var : public ::mlc::sym::Expr {
  explicit Var(::mlc::Str name, DLDataType dtype) : Var(Var::New(dtype, name)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Var, VarObj, ::mlc::sym::Expr)
      .Field("dtype", &VarObj::dtype)
      .Field("name", &VarObj::name)
      .Structure(StructureKind::kVar, {"dtype"})
      .StaticFn("__init__", ::mlc::InitOf<VarObj, DLDataType, ::mlc::Str>);
}; // struct Var
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct ShapeVarObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::Str name;
  explicit ShapeVarObj(DLDataType dtype, ::mlc::Str name) : _mlc_header{}, dtype(dtype), name(name) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, ShapeVarObj, ::mlc::sym::VarObj, "mlc.sym.ShapeVar");
}; // struct ShapeVarObj

struct ShapeVar : public ::mlc::sym::Var {
  explicit ShapeVar(::mlc::Str name, DLDataType dtype) : ShapeVar(ShapeVar::New(dtype, name)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, ShapeVar, ShapeVarObj, ::mlc::sym::Var)
      .Field("dtype", &ShapeVarObj::dtype)
      .Field("name", &ShapeVarObj::name)
      .Structure(StructureKind::kVar, {"dtype"})
      .StaticFn("__init__", ::mlc::InitOf<ShapeVarObj, DLDataType, ::mlc::Str>);
}; // struct ShapeVar
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct IntImmObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  int64_t value;
  explicit IntImmObj(DLDataType dtype, int64_t value) : _mlc_header{}, dtype(dtype), value(value) {
    if (DType::IsBool(dtype)) {
      MLC_THROW(InternalError) << "Bool type should be represented by BoolImm";
    }
    if (DType::IsFloat(dtype)) {
      MLC_THROW(InternalError) << "Float type should be represented by FloatImm";
    }
  }
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, IntImmObj, ::mlc::sym::ExprObj, "mlc.sym.IntImm");
}; // struct IntImmObj

struct IntImm : public ::mlc::sym::Expr {
  explicit IntImm(int64_t value, int32_t bits, int32_t lanes = 1)
      : IntImm(IntImm::New(DType::Int(bits, lanes), value)) {}
  explicit IntImm(int64_t value, DLDataType dtype) : IntImm(IntImm::New(dtype, value)) {}
  explicit IntImm(DLDataType dtype, int64_t value) : IntImm(IntImm::New(dtype, value)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, IntImm, IntImmObj, ::mlc::sym::Expr)
      .Field("dtype", &IntImmObj::dtype)
      .Field("value", &IntImmObj::value)
      .Structure(StructureKind::kNoBind, {"value", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<IntImmObj, DLDataType, int64_t>);
}; // struct IntImm
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct BoolImmObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  int64_t value;
  explicit BoolImmObj(DLDataType dtype, int64_t value) : _mlc_header{}, dtype(dtype), value(value) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, BoolImmObj, ::mlc::sym::IntImmObj, "mlc.sym.BoolImm");
}; // struct BoolImmObj

struct BoolImm : public ::mlc::sym::IntImm {
  explicit BoolImm(bool value, DLDataType dtype) : BoolImm(BoolImm::New(dtype, static_cast<int64_t>(value))) {}
  explicit BoolImm(bool value, int32_t lanes = 1)
      : BoolImm(BoolImm::New(DType::Bool(lanes), static_cast<int64_t>(value))) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, BoolImm, BoolImmObj, ::mlc::sym::IntImm)
      .Field("dtype", &BoolImmObj::dtype)
      .Field("value", &BoolImmObj::value)
      .Structure(StructureKind::kNoBind, {"value", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<BoolImmObj, DLDataType, int64_t>);
}; // struct BoolImm
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct FloatImmObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  double value;
  explicit FloatImmObj(DLDataType dtype, double value) : _mlc_header{}, dtype(dtype), value(value) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, FloatImmObj, ::mlc::sym::ExprObj, "mlc.sym.FloatImm");
}; // struct FloatImmObj

struct FloatImm : public ::mlc::sym::Expr {
  explicit FloatImm(double value, int32_t bits, int32_t lanes = 1)
      : FloatImm(FloatImm::New(DType::Float(bits, lanes), value)) {}
  explicit FloatImm(double value, DLDataType dtype) : FloatImm(FloatImm::New(dtype, value)) {}
  explicit FloatImm(DLDataType dtype, double value) : FloatImm(FloatImm::New(dtype, value)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, FloatImm, FloatImmObj, ::mlc::sym::Expr)
      .Field("dtype", &FloatImmObj::dtype)
      .Field("value", &FloatImmObj::value)
      .Structure(StructureKind::kNoBind, {"value", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<FloatImmObj, DLDataType, double>);
}; // struct FloatImm
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct CastObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr value;
  explicit CastObj(DLDataType dtype, ::mlc::sym::Expr value) : _mlc_header{}, dtype(dtype), value(value) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, CastObj, ::mlc::sym::ExprObj, "mlc.sym.Cast");
}; // struct CastObj

struct Cast : public ::mlc::sym::Expr {
  explicit Cast(DLDataType dtype, ::mlc::sym::Expr value) : Cast(Cast::New(dtype, value)) {}
  explicit Cast(::mlc::sym::Expr value, DLDataType dtype) : Cast(Cast::New(dtype, value)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Cast, CastObj, ::mlc::sym::Expr)
      .Field("dtype", &CastObj::dtype)
      .Field("value", &CastObj::value)
      .Structure(StructureKind::kNoBind, {"value", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<CastObj, DLDataType, ::mlc::sym::Expr>);
}; // struct DynCast
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct AddObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit AddObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, AddObj, ::mlc::sym::ExprObj, "mlc.sym.Add");
}; // struct AddObj

struct Add : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Add(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Add(Add::New(dtype, a, b)) {}
  explicit Add(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Add(Add::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Add, AddObj, ::mlc::sym::Expr)
      .Field("dtype", &AddObj::dtype)
      .Field("a", &AddObj::a)
      .Field("b", &AddObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<AddObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Add
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct SubObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit SubObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, SubObj, ::mlc::sym::ExprObj, "mlc.sym.Sub");
}; // struct SubObj

struct Sub : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Sub(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Sub(Sub::New(dtype, a, b)) {}
  explicit Sub(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Sub(Sub::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Sub, SubObj, ::mlc::sym::Expr)
      .Field("dtype", &SubObj::dtype)
      .Field("a", &SubObj::a)
      .Field("b", &SubObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<SubObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Sub
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct MulObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit MulObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, MulObj, ::mlc::sym::ExprObj, "mlc.sym.Mul");
}; // struct MulObj

struct Mul : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Mul(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Mul(Mul::New(dtype, a, b)) {}
  explicit Mul(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Mul(Mul::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Mul, MulObj, ::mlc::sym::Expr)
      .Field("dtype", &MulObj::dtype)
      .Field("a", &MulObj::a)
      .Field("b", &MulObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<MulObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Mul
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct DivObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit DivObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, DivObj, ::mlc::sym::ExprObj, "mlc.sym.Div");
}; // struct DivObj

struct Div : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Div(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Div(Div::New(dtype, a, b)) {}
  explicit Div(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Div(Div::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Div, DivObj, ::mlc::sym::Expr)
      .Field("dtype", &DivObj::dtype)
      .Field("a", &DivObj::a)
      .Field("b", &DivObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<DivObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Div
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct ModObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit ModObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, ModObj, ::mlc::sym::ExprObj, "mlc.sym.Mod");
}; // struct ModObj

struct Mod : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Mod(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Mod(Mod::New(dtype, a, b)) {}
  explicit Mod(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Mod(Mod::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Mod, ModObj, ::mlc::sym::Expr)
      .Field("dtype", &ModObj::dtype)
      .Field("a", &ModObj::a)
      .Field("b", &ModObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<ModObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Mod
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct FloorDivObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit FloorDivObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b)
      : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, FloorDivObj, ::mlc::sym::ExprObj, "mlc.sym.FloorDiv");
}; // struct FloorDivObj

struct FloorDiv : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit FloorDiv(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : FloorDiv(FloorDiv::New(dtype, a, b)) {}
  explicit FloorDiv(::mlc::sym::Expr a, ::mlc::sym::Expr b) : FloorDiv(FloorDiv::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, FloorDiv, FloorDivObj, ::mlc::sym::Expr)
      .Field("dtype", &FloorDivObj::dtype)
      .Field("a", &FloorDivObj::a)
      .Field("b", &FloorDivObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<FloorDivObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct FloorDiv
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct FloorModObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit FloorModObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b)
      : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, FloorModObj, ::mlc::sym::ExprObj, "mlc.sym.FloorMod");
}; // struct FloorModObj

struct FloorMod : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit FloorMod(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : FloorMod(FloorMod::New(dtype, a, b)) {}
  explicit FloorMod(::mlc::sym::Expr a, ::mlc::sym::Expr b) : FloorMod(FloorMod::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, FloorMod, FloorModObj, ::mlc::sym::Expr)
      .Field("dtype", &FloorModObj::dtype)
      .Field("a", &FloorModObj::a)
      .Field("b", &FloorModObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<FloorModObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct FloorMod
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct MinObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit MinObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, MinObj, ::mlc::sym::ExprObj, "mlc.sym.Min");
}; // struct MinObj

struct Min : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Min(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Min(Min::New(dtype, a, b)) {}
  explicit Min(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Min(Min::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Min, MinObj, ::mlc::sym::Expr)
      .Field("dtype", &MinObj::dtype)
      .Field("a", &MinObj::a)
      .Field("b", &MinObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<MinObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Min
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct MaxObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit MaxObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, MaxObj, ::mlc::sym::ExprObj, "mlc.sym.Max");
}; // struct MaxObj

struct Max : public ::mlc::sym::Expr {
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Max(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Max(Max::New(dtype, a, b)) {}
  explicit Max(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Max(Max::New(a->dtype, a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Max, MaxObj, ::mlc::sym::Expr)
      .Field("dtype", &MaxObj::dtype)
      .Field("a", &MaxObj::a)
      .Field("b", &MaxObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<MaxObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Max
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct EQObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit EQObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {
    if (!DType::Equal(a->dtype, b->dtype)) {
      MLC_THROW(InternalError) << "EQ: a and b must have the same dtype";
    }
  }
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, EQObj, ::mlc::sym::ExprObj, "mlc.sym.EQ");
}; // struct EQObj

struct EQ : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit EQ(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : EQ(EQ::New(dtype, a, b)) {}
  explicit EQ(::mlc::sym::Expr a, ::mlc::sym::Expr b) : EQ(EQ::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, EQ, EQObj, ::mlc::sym::Expr)
      .Field("dtype", &EQObj::dtype)
      .Field("a", &EQObj::a)
      .Field("b", &EQObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<EQObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct EQ
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct NEObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit NEObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, NEObj, ::mlc::sym::ExprObj, "mlc.sym.NE");
}; // struct NEObj

struct NE : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit NE(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : NE(NE::New(dtype, a, b)) {}
  explicit NE(::mlc::sym::Expr a, ::mlc::sym::Expr b) : NE(NE::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, NE, NEObj, ::mlc::sym::Expr)
      .Field("dtype", &NEObj::dtype)
      .Field("a", &NEObj::a)
      .Field("b", &NEObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<NEObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct NE
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct LTObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit LTObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, LTObj, ::mlc::sym::ExprObj, "mlc.sym.LT");
}; // struct LTObj

struct LT : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit LT(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : LT(LT::New(dtype, a, b)) {}
  explicit LT(::mlc::sym::Expr a, ::mlc::sym::Expr b) : LT(LT::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, LT, LTObj, ::mlc::sym::Expr)
      .Field("dtype", &LTObj::dtype)
      .Field("a", &LTObj::a)
      .Field("b", &LTObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<LTObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct LT
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct LEObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit LEObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, LEObj, ::mlc::sym::ExprObj, "mlc.sym.LE");
}; // struct LEObj

struct LE : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit LE(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : LE(LE::New(dtype, a, b)) {}
  explicit LE(::mlc::sym::Expr a, ::mlc::sym::Expr b) : LE(LE::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, LE, LEObj, ::mlc::sym::Expr)
      .Field("dtype", &LEObj::dtype)
      .Field("a", &LEObj::a)
      .Field("b", &LEObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<LEObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct LE
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct GTObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit GTObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, GTObj, ::mlc::sym::ExprObj, "mlc.sym.GT");
}; // struct GTObj

struct GT : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit GT(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : GT(GT::New(dtype, a, b)) {}
  explicit GT(::mlc::sym::Expr a, ::mlc::sym::Expr b) : GT(GT::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, GT, GTObj, ::mlc::sym::Expr)
      .Field("dtype", &GTObj::dtype)
      .Field("a", &GTObj::a)
      .Field("b", &GTObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<GTObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct GT
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct GEObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit GEObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, GEObj, ::mlc::sym::ExprObj, "mlc.sym.GE");
}; // struct GEObj

struct GE : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit GE(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : GE(GE::New(dtype, a, b)) {}
  explicit GE(::mlc::sym::Expr a, ::mlc::sym::Expr b) : GE(GE::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, GE, GEObj, ::mlc::sym::Expr)
      .Field("dtype", &GEObj::dtype)
      .Field("a", &GEObj::a)
      .Field("b", &GEObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<GEObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct GE
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct AndObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit AndObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, AndObj, ::mlc::sym::ExprObj, "mlc.sym.And");
}; // struct AndObj

struct And : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit And(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : And(And::New(dtype, a, b)) {}
  explicit And(::mlc::sym::Expr a, ::mlc::sym::Expr b) : And(And::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, And, AndObj, ::mlc::sym::Expr)
      .Field("dtype", &AndObj::dtype)
      .Field("a", &AndObj::a)
      .Field("b", &AndObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<AndObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct And
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct OrObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  ::mlc::sym::Expr b;
  explicit OrObj(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : _mlc_header{}, dtype(dtype), a(a), b(b) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, OrObj, ::mlc::sym::ExprObj, "mlc.sym.Or");
}; // struct OrObj

struct Or : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a, Expr b);
  explicit Or(DLDataType dtype, ::mlc::sym::Expr a, ::mlc::sym::Expr b) : Or(Or::New(dtype, a, b)) {}
  explicit Or(::mlc::sym::Expr a, ::mlc::sym::Expr b) : Or(Or::New(DType::Bool(a->dtype.lanes), a, b)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Or, OrObj, ::mlc::sym::Expr)
      .Field("dtype", &OrObj::dtype)
      .Field("a", &OrObj::a)
      .Field("b", &OrObj::b)
      .Structure(StructureKind::kNoBind, {"a", "b", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<OrObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Or
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct NotObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr a;
  explicit NotObj(DLDataType dtype, ::mlc::sym::Expr a) : _mlc_header{}, dtype(dtype), a(a) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, NotObj, ::mlc::sym::ExprObj, "mlc.sym.Not");
}; // struct NotObj

struct Not : public ::mlc::sym::Expr {
  static constexpr bool is_logical = true;
  static MLC_API Optional<Expr> TryConstFold(Expr a);
  explicit Not(DLDataType dtype, ::mlc::sym::Expr a) : Not(Not::New(dtype, a)) {}
  explicit Not(::mlc::sym::Expr a) : Not(Not::New(DType::Bool(a->dtype.lanes), a)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Not, NotObj, ::mlc::sym::Expr)
      .Field("dtype", &NotObj::dtype)
      .Field("a", &NotObj::a)
      .Structure(StructureKind::kNoBind, {"a", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<NotObj, DLDataType, ::mlc::sym::Expr>);
}; // struct Not
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct SelectObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr cond;
  ::mlc::sym::Expr true_value;
  ::mlc::sym::Expr false_value;
  explicit SelectObj(DLDataType dtype, ::mlc::sym::Expr cond, ::mlc::sym::Expr true_value, ::mlc::sym::Expr false_value)
      : _mlc_header{}, dtype(dtype), cond(cond), true_value(true_value), false_value(false_value) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, SelectObj, ::mlc::sym::ExprObj, "mlc.sym.Select");
}; // struct SelectObj

struct Select : public ::mlc::sym::Expr {
  explicit Select(DLDataType dtype, ::mlc::sym::Expr cond, ::mlc::sym::Expr true_value, ::mlc::sym::Expr false_value)
      : Select(Select::New(dtype, cond, true_value, false_value)) {}
  explicit Select(::mlc::sym::Expr cond, ::mlc::sym::Expr true_value, ::mlc::sym::Expr false_value)
      : Select(Select::New(true_value->dtype, cond, true_value, false_value)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Select, SelectObj, ::mlc::sym::Expr)
      .Field("dtype", &SelectObj::dtype)
      .Field("cond", &SelectObj::cond)
      .Field("true_value", &SelectObj::true_value)
      .Field("false_value", &SelectObj::false_value)
      .Structure(StructureKind::kNoBind, {"cond", "true_value", "false_value", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<SelectObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Select
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct RampObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr base;
  ::mlc::sym::Expr stride;
  int64_t lanes;
  explicit RampObj(DLDataType dtype, ::mlc::sym::Expr base, ::mlc::sym::Expr stride, int64_t lanes)
      : _mlc_header{}, dtype(dtype), base(base), stride(stride), lanes(lanes) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, RampObj, ::mlc::sym::ExprObj, "mlc.sym.Ramp");
}; // struct RampObj

struct Ramp : public ::mlc::sym::Expr {
  explicit Ramp(DLDataType dtype, ::mlc::sym::Expr base, ::mlc::sym::Expr stride, int64_t lanes)
      : Ramp(Ramp::New(dtype, base, stride, lanes)) {}
  explicit Ramp(::mlc::sym::Expr base, ::mlc::sym::Expr stride, int64_t lanes)
      : Ramp(Ramp::New(DLDataType{base->dtype.code, base->dtype.bits, static_cast<uint16_t>(lanes)}, base, stride,
                       lanes)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Ramp, RampObj, ::mlc::sym::Expr)
      .Field("dtype", &RampObj::dtype)
      .Field("base", &RampObj::base)
      .Field("stride", &RampObj::stride)
      .Field("lanes", &RampObj::lanes)
      .Structure(StructureKind::kNoBind, {"base", "stride", "lanes", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<RampObj, DLDataType, ::mlc::sym::Expr, ::mlc::sym::Expr, int64_t>);
}; // struct Ramp
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct BroadcastObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Expr value;
  int64_t lanes;
  explicit BroadcastObj(DLDataType dtype, ::mlc::sym::Expr value, int64_t lanes)
      : _mlc_header{}, dtype(dtype), value(value), lanes(lanes) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, BroadcastObj, ::mlc::sym::ExprObj, "mlc.sym.Broadcast");
}; // struct BroadcastObj

struct Broadcast : public ::mlc::sym::Expr {
  explicit Broadcast(DLDataType dtype, ::mlc::sym::Expr value, int64_t lanes)
      : Broadcast(Broadcast::New(dtype, value, lanes)) {}
  explicit Broadcast(::mlc::sym::Expr value, int64_t lanes)
      : Broadcast(DLDataType{value->dtype.code, value->dtype.bits, static_cast<uint16_t>(lanes)}, value, lanes) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Broadcast, BroadcastObj, ::mlc::sym::Expr)
      .Field("dtype", &BroadcastObj::dtype)
      .Field("value", &BroadcastObj::value)
      .Field("lanes", &BroadcastObj::lanes)
      .Structure(StructureKind::kNoBind, {"value", "lanes", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<BroadcastObj, DLDataType, ::mlc::sym::Expr, int64_t>);
}; // struct Broadcast
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct ShuffleObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::List<::mlc::sym::Expr> vectors;
  ::mlc::List<::mlc::sym::Expr> indices;
  explicit ShuffleObj(DLDataType dtype, ::mlc::List<::mlc::sym::Expr> vectors, ::mlc::List<::mlc::sym::Expr> indices)
      : _mlc_header{}, dtype(dtype), vectors(vectors), indices(indices) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, ShuffleObj, ::mlc::sym::ExprObj, "mlc.sym.Shuffle");
}; // struct ShuffleObj

struct Shuffle : public ::mlc::sym::Expr {
  explicit Shuffle(DLDataType dtype, ::mlc::List<::mlc::sym::Expr> vectors, ::mlc::List<::mlc::sym::Expr> indices)
      : Shuffle(Shuffle::New(dtype, vectors, indices)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Shuffle, ShuffleObj, ::mlc::sym::Expr)
      .Field("dtype", &ShuffleObj::dtype)
      .Field("vectors", &ShuffleObj::vectors)
      .Field("indices", &ShuffleObj::indices)
      .Structure(StructureKind::kNoBind, {"vectors", "indices", "dtype"})
      .StaticFn("__init__",
                ::mlc::InitOf<ShuffleObj, DLDataType, ::mlc::List<::mlc::sym::Expr>, ::mlc::List<::mlc::sym::Expr>>);
}; // struct Shuffle
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct LetObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::sym::Var var;
  ::mlc::sym::Expr value;
  ::mlc::sym::Expr body;
  explicit LetObj(DLDataType dtype, ::mlc::sym::Var var, ::mlc::sym::Expr value, ::mlc::sym::Expr body)
      : _mlc_header{}, dtype(dtype), var(var), value(value), body(body) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, LetObj, ::mlc::sym::ExprObj, "mlc.sym.Let");
}; // struct LetObj

struct Let : public ::mlc::sym::Expr {
  explicit Let(DLDataType dtype, ::mlc::sym::Var var, ::mlc::sym::Expr value, ::mlc::sym::Expr body)
      : Let(Let::New(dtype, var, value, body)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Let, LetObj, ::mlc::sym::Expr)
      .Field("dtype", &LetObj::dtype)
      .Field("var", &LetObj::var)
      .Field("value", &LetObj::value)
      .Field("body", &LetObj::body)
      .Structure(StructureKind::kBind, {"value", "var:bind", "body", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<LetObj, DLDataType, ::mlc::sym::Var, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Let
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct CallObj {
  MLCAny _mlc_header;
  DLDataType dtype;
  ::mlc::Any op;
  ::mlc::List<::mlc::sym::Expr> args;
  explicit CallObj(DLDataType dtype, ::mlc::Any op, ::mlc::List<::mlc::sym::Expr> args)
      : _mlc_header{}, dtype(dtype), op(op), args(args) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, CallObj, ::mlc::sym::ExprObj, "mlc.sym.Call");
}; // struct CallObj

struct Call : public ::mlc::sym::Expr {
  explicit Call(DLDataType dtype, ::mlc::Any op, ::mlc::List<::mlc::sym::Expr> args)
      : Call(Call::New(dtype, op, args)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Call, CallObj, ::mlc::sym::Expr)
      .Field("dtype", &CallObj::dtype)
      .Field("op", &CallObj::op)
      .Field("args", &CallObj::args)
      .Structure(StructureKind::kNoBind, {"op", "args", "dtype"})
      .StaticFn("__init__", ::mlc::InitOf<CallObj, DLDataType, ::mlc::Any, ::mlc::List<::mlc::sym::Expr>>);
}; // struct Call
} // namespace sym
} // namespace mlc
namespace mlc {
namespace sym {
struct RangeObj {
  MLCAny _mlc_header;
  ::mlc::sym::Expr min;
  ::mlc::sym::Expr extent;
  explicit RangeObj(::mlc::sym::Expr min, ::mlc::sym::Expr extent) : _mlc_header{}, min(min), extent(extent) {}
  MLC_DEF_DYN_TYPE(MLC_SYM_EXPORTS, RangeObj, ::mlc::Object, "mlc.sym.Range");
}; // struct RangeObj

struct Range : public ::mlc::ObjectRef {
  explicit Range(::mlc::sym::Expr min, ::mlc::sym::Expr extent) : Range(Range::New(min, extent)) {}
  MLC_DEF_OBJ_REF(MLC_SYM_EXPORTS, Range, RangeObj, ::mlc::ObjectRef)
      .Field("min", &RangeObj::min)
      .Field("extent", &RangeObj::extent)
      .Structure(StructureKind::kNoBind, {"min", "extent"})
      .StaticFn("__init__", ::mlc::InitOf<RangeObj, ::mlc::sym::Expr, ::mlc::sym::Expr>);
}; // struct Range
} // namespace sym
} // namespace mlc

namespace mlc {
namespace sym {

inline Expr Expr::Bool(bool value, int32_t lanes) { return BoolImm(value, lanes); }
inline Expr Expr::Int32(int64_t value, int32_t lanes) { return IntImm(value, 32, lanes); }
inline Expr Expr::Int64(int64_t value, int32_t lanes) { return IntImm(value, 64, lanes); }
inline Expr Expr::Float32(double value, int32_t lanes) { return FloatImm(value, 32, lanes); }
inline Expr Expr::Float64(double value, int32_t lanes) { return FloatImm(value, 64, lanes); }

template <typename T, typename> inline Expr Expr::Const(DLDataType t, T value) {
  if constexpr (std::is_same_v<T, bool>) {
    if (DType::IsBool(t)) {
      return BoolImm(value, t.lanes);
    }
  } else if constexpr (std::is_integral_v<T>) {
    if (DType::IsBool(t)) {
      return BoolImm(static_cast<bool>(value), t);
    }
    if (t.code == kDLInt || t.code == kDLUInt) {
      return IntImm(t, static_cast<int64_t>(value));
    }
    if (t.code == kDLFloat || t.code == kDLBfloat) {
      return FloatImm(t, static_cast<double>(value));
    }
  } else if constexpr (std::is_floating_point_v<T>) {
    if (t.code == kDLFloat || t.code == kDLBfloat) { // TODO: handle float8
      return FloatImm(t, static_cast<double>(value));
    }
    if (DType::IsBool(t)) {
      return BoolImm(static_cast<bool>(value), t);
    }
    if (t.code == kDLInt || t.code == kDLUInt) {
      return IntImm(t, static_cast<int64_t>(value));
    }
  }
  MLC_THROW(ValueError) << "Cannot make const for type " << DType::Str(t);
  throw;
}

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_EXPR_H_
