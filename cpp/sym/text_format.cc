#include <mlc/core/all.h>
#include <mlc/printer/all.h>
#include <mlc/sym/all.h>

namespace mlc {
namespace sym {
namespace {

using mlc::core::_ReflectMemFn;
using mlc::core::ObjectPath;
using mlc::printer::IRPrinterObj;
using mlc::printer::Literal;
using mlc::printer::Operation;
using mlc::printer::OperationObj;
using PExpr = mlc::printer::Expr;

PExpr Prefix(const char *id) { return printer::Id({}, "S").Attr(id); }
PExpr IdCall(const char *id, const List<PExpr> &args, const ObjectPath &p) { return Prefix(id).Call(args).AddPath(p); }

PExpr PrintOp(OpObj *self, IRPrinterObj *, ObjectPath p) { //
  return IdCall("Op", {Literal::Str(self->name)}, p);
}

PExpr PrintVar(Var self, IRPrinterObj *d, ObjectPath p) {
  if (!d->VarIsDefined(self)) {
    printer::DefaultFrame frame = d->frames->back();
    PExpr name = printer::Id({p->WithField("name")}, self->name);
    PExpr dtype = Prefix(DType::Str(self->dtype).c_str()).AddPath(p->WithField("dtype"));
    if (d->cfg->def_free_var) {
      frame->stmts->push_back(printer::Assign({p}, Null, name, dtype.Call({Literal::Str(self->name)}), Null));
    }
    d->VarDef(self->name, self, frame);
  }
  if (Optional<PExpr> ret = d->VarGet(self)) {
    return ret.value().AddPath(p);
  }
  MLC_THROW(InternalError);
  MLC_UNREACHABLE();
}

PExpr PrintShapeVar(ShapeVar self, IRPrinterObj *d, ObjectPath p) {
  if (!d->VarIsDefined(self)) {
    printer::DefaultFrame frame = d->frames->back();
    PExpr name = printer::Id({p->WithField("name")}, self->name);
    PExpr dtype = Prefix(DType::Str(self->dtype).c_str()).AddPath(p->WithField("dtype"));
    if (d->cfg->def_free_var) {
      frame->stmts->push_back(printer::Assign({p}, Null, name,
                                              dtype.CallKw({Literal::Str(self->name)}, //
                                                           {"size"},                   //
                                                           {Literal::Bool(true)}),
                                              Null));
    }
    d->VarDef(self->name, self, frame);
  }
  if (Optional<PExpr> ret = d->VarGet(self)) {
    return ret.value().AddPath(p);
  }
  MLC_THROW(InternalError);
  MLC_UNREACHABLE();
}

Str StrVar(VarObj *self) {
  ObjectPath p = ObjectPath::Root();
  PExpr name = Literal::Str(self->name, {p->WithField("name")});
  PExpr dtype = Prefix(DType::Str(self->dtype).c_str()).AddPath(p->WithField("dtype"));
  printer::Node ret = dtype.Call({name});
  return ret->ToPython(printer::PrinterConfig());
}

Str StrShapeVar(ShapeVarObj *self) {
  ObjectPath p = ObjectPath::Root();
  PExpr name = Literal::Str(self->name, {p->WithField("name")});
  PExpr dtype = Prefix(DType::Str(self->dtype).c_str()).AddPath(p->WithField("dtype"));
  printer::Node ret = dtype.CallKw({name}, {"size_var"}, {Literal::Bool(true)});
  return ret->ToPython(printer::PrinterConfig());
}

PExpr PrintIntImm(IntImmObj *self, IRPrinterObj *, ObjectPath p) { return Literal::Int(self->value, {p}); }
PExpr PrintBoolImm(BoolImmObj *self, IRPrinterObj *, ObjectPath p) { return Literal::Bool(self->value != 0, {p}); }
PExpr PrintFloatImm(FloatImmObj *self, IRPrinterObj *, ObjectPath p) { return Literal::Float(self->value, {p}); }

PExpr PrintCast(CastObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr value = (*d)(self->value, p->WithField("value"));
  PExpr dtype = Literal::Str(DType::Str(self->dtype).c_str(), {p->WithField("dtype")});
  return value.Attr("cast").Call({dtype}).AddPath(p);
}

template <typename TObjRef, int32_t op_kind>
PExpr PrintUnaryOp(typename TObjRef::TObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr a = (*d)(self->a, p->WithField("a"));
  return Operation(List<ObjectPath>{p}, op_kind, List<PExpr>{a});
}

template <typename TObjRef, int32_t op_kind>
PExpr PrintBinaryOp(typename TObjRef::TObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr a = (*d)(self->a, p->WithField("a"));
  PExpr b = (*d)(self->b, p->WithField("b"));
  return Operation(List<ObjectPath>{p}, op_kind, List<PExpr>{a, b});
}

template <typename TObjRef, int32_t index>
PExpr PrintBinaryFn(typename TObjRef::TObj *self, IRPrinterObj *d, ObjectPath p) {
  constexpr std::string_view names[] = {
      "truncdiv",
      "truncmod",
      "min",
      "max",
  };
  constexpr std::string_view name = names[index];
  PExpr a = (*d)(self->a, p->WithField("a"));
  PExpr b = (*d)(self->b, p->WithField("b"));
  return IdCall(name.data(), {a, b}, p);
}

PExpr PrintSelect(SelectObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr a = (*d)(self->cond, p->WithField("cond"));
  PExpr b = (*d)(self->true_value, p->WithField("true_value"));
  PExpr c = (*d)(self->false_value, p->WithField("false_value"));
  return IdCall("select", {a, b, c}, p);
}

PExpr PrintLet(LetObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr value = (*d)(self->value, p->WithField("value"));
  {
    Var var = self->var;
    ObjectPath p_var = p->WithField("var");
    if (!d->VarIsDefined(var)) {
      printer::DefaultFrame frame = d->frames->back();
      PExpr lhs = printer::Id({p_var->WithField("name")}, var->name);
      PExpr rhs = (*d)(self->value, p->WithField("value"));
      frame->stmts->push_back(printer::Assign({p_var}, Null, lhs, rhs, Null));
      d->VarDef(var->name, var, frame);
    }
  }
  PExpr body = (*d)(self->body, p->WithField("body"));
  return body;
}

PExpr PrintCall(CallObj *self, IRPrinterObj *d, ObjectPath p) {
  List<PExpr> args = d->ApplyToList<PExpr>(self->args, p->WithField("args"));
  if (const OpObj *op = self->op.as<OpObj>()) {
    if (op->Same(Op_::left_shift)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kLShift, args);
    }
    if (op->Same(Op_::right_shift)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kRShift, args);
    }
    if (op->Same(Op_::bitwise_and)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kBitAnd, args);
    }
    if (op->Same(Op_::bitwise_or)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kBitOr, args);
    }
    if (op->Same(Op_::bitwise_xor)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kBitXor, args);
    }
    if (op->Same(Op_::bitwise_not)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kInvert, args);
    }
    if (op->Same(Op_::if_then_else)) {
      return Operation(List<ObjectPath>{p}, OperationObj::kIfThenElse, args);
    }
    return IdCall(op->name->data(), args, p);
  }
  args->insert(0, (*d)(self->op, p->WithField("op")));
  return IdCall("Call", args, p);
}

PExpr PrintRange(RangeObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr min = (*d)(self->min, p->WithField("min"));
  PExpr extent = (*d)(self->extent, p->WithField("extent"));
  return IdCall("Range", {min, extent}, p);
}

PExpr PrintRamp(RampObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr base = (*d)(self->base, p->WithField("base"));
  PExpr stride = (*d)(self->stride, p->WithField("stride"));
  PExpr lanes = (*d)(self->lanes, p->WithField("stride"));
  return IdCall("ramp", {base, stride, lanes}, p);
}

PExpr PrintBroadcast(BroadcastObj *self, IRPrinterObj *d, ObjectPath p) {
  PExpr value = (*d)(self->value, p->WithField("value"));
  PExpr lanes = (*d)(self->lanes, p->WithField("stride"));
  return IdCall("broadcast", {value, lanes}, p);
}

PExpr PrintShuffle(ShuffleObj *self, IRPrinterObj *d, ObjectPath p) {
  List<PExpr> vectors = d->ApplyToList<PExpr>(self->vectors, p->WithField("vectors"));
  List<PExpr> indices = d->ApplyToList<PExpr>(self->indices, p->WithField("indices"));
  vectors->insert(0, indices->begin(), indices->end());
  return IdCall("shuffle", vectors, p);
}

struct Register {
  static Str ToPython(const ObjectRef &self) {
    printer::PrinterConfig cfg;
    cfg->def_free_var = false;
    return ::mlc::printer::ToPython(self, cfg);
  }
  template <typename TObj, typename Callable> static void Run(Callable &&func) {
    _ReflectMemFn(TObj::_type_index, "__ir_print__", ::mlc::base::CallableToAny(func));
    _ReflectMemFn(TObj::_type_index, "__str__", ::mlc::base::CallableToAny(Register::ToPython));
  }
  inline static int32_t _reg = []() {
    _ReflectMemFn(VarObj::_type_index, "__str__", ::mlc::base::CallableToAny(StrVar));
    _ReflectMemFn(ShapeVarObj::_type_index, "__str__", ::mlc::base::CallableToAny(StrShapeVar));
    Run<OpObj>(PrintOp);
    Run<VarObj>(PrintVar);
    Run<ShapeVarObj>(PrintShapeVar);
    Run<IntImmObj>(PrintIntImm);
    Run<BoolImmObj>(PrintBoolImm);
    Run<FloatImmObj>(PrintFloatImm);
    Run<CastObj>(PrintCast);
    Run<AddObj>(PrintBinaryOp<Add, OperationObj::kAdd>);
    Run<SubObj>(PrintBinaryOp<Sub, OperationObj::kSub>);
    Run<MulObj>(PrintBinaryOp<Mul, OperationObj::kMult>);
    Run<DivObj>(PrintBinaryFn<Div, 0>);
    Run<ModObj>(PrintBinaryFn<Mod, 1>);
    Run<FloorDivObj>(PrintBinaryOp<FloorDiv, OperationObj::kFloorDiv>);
    Run<FloorModObj>(PrintBinaryOp<FloorMod, OperationObj::kMod>);
    Run<MinObj>(PrintBinaryFn<Min, 2>);
    Run<MaxObj>(PrintBinaryFn<Max, 3>);
    Run<EQObj>(PrintBinaryOp<EQ, OperationObj::kEq>);
    Run<NEObj>(PrintBinaryOp<NE, OperationObj::kNotEq>);
    Run<LTObj>(PrintBinaryOp<LT, OperationObj::kLt>);
    Run<LEObj>(PrintBinaryOp<LE, OperationObj::kLtE>);
    Run<GTObj>(PrintBinaryOp<GT, OperationObj::kGt>);
    Run<GEObj>(PrintBinaryOp<GE, OperationObj::kGtE>);
    Run<AndObj>(PrintBinaryOp<And, OperationObj::kAnd>);
    Run<OrObj>(PrintBinaryOp<Or, OperationObj::kOr>);
    Run<NotObj>(PrintUnaryOp<Not, OperationObj::kNot>);
    Run<SelectObj>(PrintSelect);
    Run<LetObj>(PrintLet);
    Run<RampObj>(PrintRamp);
    Run<BroadcastObj>(PrintBroadcast);
    Run<ShuffleObj>(PrintShuffle);
    Run<CallObj>(PrintCall);
    Run<RangeObj>(PrintRange);
    return 0;
  }();
};

} // namespace
} // namespace sym
} // namespace mlc
