#ifndef MLC_PRINTER_AST_H_
#define MLC_PRINTER_AST_H_

/*! This file is mostly generated from `mlc.printer.ast.*` using `mlc.dataclasses.prototype_cxx` */
#include <mlc/core/all.h>

namespace mlc {
namespace printer {

using mlc::core::ObjectPath;

struct PrinterConfigObj : public Object {
  int32_t indent_spaces = 2;
  int8_t print_line_numbers = 0;
  int32_t num_context_lines = -1;
  mlc::List<ObjectPath> path_to_underline;

  PrinterConfigObj() = default;
  explicit PrinterConfigObj(int32_t indent_spaces, int8_t print_line_numbers, int32_t num_context_lines,
                            mlc::List<ObjectPath> path_to_underline)
      : indent_spaces(indent_spaces), print_line_numbers(print_line_numbers), num_context_lines(num_context_lines),
        path_to_underline(path_to_underline) {}
  MLC_DEF_DYN_TYPE(PrinterConfigObj, Object, "mlc.printer.PrinterConfig");
};

struct PrinterConfig : public ObjectRef {
  MLC_DEF_OBJ_REF(PrinterConfig, PrinterConfigObj, ObjectRef)
      .Field("indent_spaces", &PrinterConfigObj::indent_spaces)
      .Field("print_line_numbers", &PrinterConfigObj::print_line_numbers)
      .Field("num_context_lines", &PrinterConfigObj::num_context_lines)
      .Field("path_to_underline", &PrinterConfigObj::path_to_underline)
      .StaticFn("__init__", InitOf<PrinterConfigObj, int32_t, int8_t, int32_t, mlc::List<ObjectPath>>);
};

} // namespace printer
} // namespace mlc

/************** Node **************/
namespace mlc {
namespace printer {

struct NodeObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  explicit NodeObj(::mlc::List<::mlc::core::ObjectPath> source_paths) : source_paths(source_paths) {}
  MLC_DEF_DYN_TYPE(NodeObj, ::mlc::Object, "mlc.printer.ast.Node");
  mlc::Str ToPython(PrinterConfig cfg) const {
    static auto func = ::mlc::base::GetGlobalFuncCall<2>("mlc.printer.DocToPythonScript");
    return func({this, cfg});
  }
}; // struct NodeObj

struct Node : public ::mlc::ObjectRef {
  MLC_DEF_OBJ_REF(Node, NodeObj, ::mlc::ObjectRef)
      .Field("source_paths", &NodeObj::source_paths)
      .StaticFn("__init__", ::mlc::InitOf<NodeObj, ::mlc::List<::mlc::core::ObjectPath>>)
      .MemFn("to_python", &NodeObj::ToPython);
}; // struct Node

} // namespace printer
} // namespace mlc

/************** Expr **************/
namespace mlc {
namespace printer {

struct Expr;

struct ExprObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  explicit ExprObj(::mlc::List<::mlc::core::ObjectPath> source_paths) : source_paths(source_paths) {}

  Expr Attr(mlc::Str name) const;
  Expr Index(mlc::List<::mlc::printer::Expr> idx) const;
  Expr Call(mlc::List<::mlc::printer::Expr> args) const;
  Expr CallKw(mlc::List<::mlc::printer::Expr> args, mlc::List<::mlc::Str> kwargs_keys,
              mlc::List<::mlc::printer::Expr> kwargs_values) const;

  MLC_DEF_DYN_TYPE(ExprObj, ::mlc::printer::NodeObj, "mlc.printer.ast.Expr");
}; // struct ExprObj

struct Expr : public ::mlc::printer::Node {
  MLC_DEF_OBJ_REF(Expr, ExprObj, ::mlc::printer::Node)
      .Field("source_paths", &ExprObj::source_paths)
      .StaticFn("__init__", ::mlc::InitOf<ExprObj, ::mlc::List<::mlc::core::ObjectPath>>)
      .MemFn("attr", &ExprObj::Attr)
      .MemFn("index", &ExprObj::Index)
      .MemFn("call", &ExprObj::Call)
      .MemFn("call_kw", &ExprObj::CallKw);
}; // struct Expr

} // namespace printer
} // namespace mlc

/************** Stmt **************/
namespace mlc {
namespace printer {

struct StmtObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  explicit StmtObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment)
      : source_paths(source_paths), comment(comment) {}
  MLC_DEF_DYN_TYPE(StmtObj, ::mlc::printer::NodeObj, "mlc.printer.ast.Stmt");
}; // struct StmtObj

struct Stmt : public ::mlc::printer::Node {
  MLC_DEF_OBJ_REF(Stmt, StmtObj, ::mlc::printer::Node)
      .Field("source_paths", &StmtObj::source_paths)
      .Field("comment", &StmtObj::comment)
      .StaticFn("__init__", ::mlc::InitOf<StmtObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>>);
}; // struct Stmt

} // namespace printer
} // namespace mlc

/************** StmtBlock **************/
namespace mlc {
namespace printer {

struct StmtBlockObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::List<::mlc::printer::Stmt> stmts;
  explicit StmtBlockObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                        ::mlc::List<::mlc::printer::Stmt> stmts)
      : source_paths(source_paths), comment(comment), stmts(stmts) {}
  MLC_DEF_DYN_TYPE(StmtBlockObj, ::mlc::printer::StmtObj, "mlc.printer.ast.StmtBlock");
}; // struct StmtBlockObj

struct StmtBlock : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(StmtBlock, StmtBlockObj, ::mlc::printer::Stmt)
      .Field("source_paths", &StmtBlockObj::source_paths)
      .Field("comment", &StmtBlockObj::comment)
      .Field("stmts", &StmtBlockObj::stmts)
      .StaticFn("__init__", ::mlc::InitOf<StmtBlockObj, ::mlc::List<::mlc::core::ObjectPath>,
                                          ::mlc::Optional<::mlc::Str>, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct StmtBlock

} // namespace printer
} // namespace mlc

/************** Literal **************/
namespace mlc {
namespace printer {

struct LiteralObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Any value;
  explicit LiteralObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Any value)
      : source_paths(source_paths), value(value) {}
  MLC_DEF_DYN_TYPE(LiteralObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Literal");
}; // struct LiteralObj

struct Literal : public ::mlc::printer::Expr {
  static Literal Int(int64_t value) { return Literal(mlc::List<ObjectPath>(), Any(value)); }
  static Literal Str(mlc::Str value) { return Literal(mlc::List<ObjectPath>(), Any(value)); }
  static Literal Float(double value) { return Literal(mlc::List<ObjectPath>(), Any(value)); }
  static Literal Null() { return Literal(mlc::List<ObjectPath>(), Any()); }

  MLC_DEF_OBJ_REF(Literal, LiteralObj, ::mlc::printer::Expr)
      .Field("source_paths", &LiteralObj::source_paths)
      .Field("value", &LiteralObj::value)
      .StaticFn("__init__", ::mlc::InitOf<LiteralObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Any>);
}; // struct Literal

} // namespace printer
} // namespace mlc

/************** Id **************/
namespace mlc {
namespace printer {

struct IdObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Str name;
  explicit IdObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Str name)
      : source_paths(source_paths), name(name) {}
  explicit IdObj(::mlc::Str name) : source_paths(), name(name) {}
  MLC_DEF_DYN_TYPE(IdObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Id");
}; // struct IdObj

struct Id : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Id, IdObj, ::mlc::printer::Expr)
      .Field("source_paths", &IdObj::source_paths)
      .Field("name", &IdObj::name)
      .StaticFn("__init__", ::mlc::InitOf<IdObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Str>);
}; // struct Id

} // namespace printer
} // namespace mlc

/************** Attr **************/
namespace mlc {
namespace printer {

struct AttrObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::printer::Expr obj;
  ::mlc::Str name;
  explicit AttrObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::printer::Expr obj, ::mlc::Str name)
      : source_paths(source_paths), obj(obj), name(name) {}
  MLC_DEF_DYN_TYPE(AttrObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Attr");
}; // struct AttrObj

struct Attr : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Attr, AttrObj, ::mlc::printer::Expr)
      .Field("source_paths", &AttrObj::source_paths)
      .Field("obj", &AttrObj::obj)
      .Field("name", &AttrObj::name)
      .StaticFn("__init__",
                ::mlc::InitOf<AttrObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::printer::Expr, ::mlc::Str>);
}; // struct Attr

} // namespace printer
} // namespace mlc

/************** Index **************/
namespace mlc {
namespace printer {

struct IndexObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::printer::Expr obj;
  ::mlc::List<::mlc::printer::Expr> idx;
  explicit IndexObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::printer::Expr obj,
                    ::mlc::List<::mlc::printer::Expr> idx)
      : source_paths(source_paths), obj(obj), idx(idx) {}
  MLC_DEF_DYN_TYPE(IndexObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Index");
}; // struct IndexObj

struct Index : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Index, IndexObj, ::mlc::printer::Expr)
      .Field("source_paths", &IndexObj::source_paths)
      .Field("obj", &IndexObj::obj)
      .Field("idx", &IndexObj::idx)
      .StaticFn("__init__", ::mlc::InitOf<IndexObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::printer::Expr,
                                          ::mlc::List<::mlc::printer::Expr>>);
}; // struct Index

} // namespace printer
} // namespace mlc

/************** Call **************/
namespace mlc {
namespace printer {

struct CallObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::printer::Expr callee;
  ::mlc::List<::mlc::printer::Expr> args;
  ::mlc::List<::mlc::Str> kwargs_keys;
  ::mlc::List<::mlc::printer::Expr> kwargs_values;
  explicit CallObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::printer::Expr callee,
                   ::mlc::List<::mlc::printer::Expr> args, ::mlc::List<::mlc::Str> kwargs_keys,
                   ::mlc::List<::mlc::printer::Expr> kwargs_values)
      : source_paths(source_paths), callee(callee), args(args), kwargs_keys(kwargs_keys), kwargs_values(kwargs_values) {
  }
  MLC_DEF_DYN_TYPE(CallObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Call");
}; // struct CallObj

struct Call : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Call, CallObj, ::mlc::printer::Expr)
      .Field("source_paths", &CallObj::source_paths)
      .Field("callee", &CallObj::callee)
      .Field("args", &CallObj::args)
      .Field("kwargs_keys", &CallObj::kwargs_keys)
      .Field("kwargs_values", &CallObj::kwargs_values)
      .StaticFn(
          "__init__",
          ::mlc::InitOf<CallObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::printer::Expr,
                        ::mlc::List<::mlc::printer::Expr>, ::mlc::List<::mlc::Str>, ::mlc::List<::mlc::printer::Expr>>);
}; // struct Call

} // namespace printer
} // namespace mlc

/************** Operation **************/
namespace mlc {
namespace printer {

struct OperationObj : public ::mlc::Object {
  enum Kind : int64_t {
    // Unary operations
    kUnaryStart = 0,
    kUSub = 1,   // -x
    kInvert = 2, // ~x
    kNot = 3,    // not x
    kUnaryEnd = 4,
    // Binary operations
    kBinaryStart = 5,
    kAdd = 6,       // +
    kSub = 7,       // -
    kMult = 8,      // *
    kDiv = 9,       // /
    kFloorDiv = 10, // // in Python
    kMod = 11,      // % in Python
    kPow = 12,      // ** in Python
    kLShift = 13,   // <<
    kRShift = 14,   // >>
    kBitAnd = 15,   // &
    kBitOr = 16,    // |
    kBitXor = 17,   // ^
    kLt = 18,       // <
    kLtE = 19,      // <=
    kEq = 20,       // ==
    kNotEq = 21,    // !=
    kGt = 22,       // >
    kGtE = 23,      // >=
    kAnd = 24,      // and
    kOr = 25,       // or
    kBinaryEnd = 26,
    // Special operations
    kSpecialStart = 27,
    kIfThenElse = 28, // <operands[1]> if <operands[0]> else <operands[2]>
    kSpecialEnd = 29,
  };

  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  int64_t op; // OperationKind
  ::mlc::List<::mlc::printer::Expr> operands;
  explicit OperationObj(::mlc::List<::mlc::core::ObjectPath> source_paths, int64_t op,
                        ::mlc::List<::mlc::printer::Expr> operands)
      : source_paths(source_paths), op(op), operands(operands) {}
  MLC_DEF_DYN_TYPE(OperationObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Operation");
}; // struct OperationObj

struct Operation : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Operation, OperationObj, ::mlc::printer::Expr)
      .Field("source_paths", &OperationObj::source_paths)
      .Field("op", &OperationObj::op)
      .Field("operands", &OperationObj::operands)
      .StaticFn("__init__", ::mlc::InitOf<OperationObj, ::mlc::List<::mlc::core::ObjectPath>, int64_t,
                                          ::mlc::List<::mlc::printer::Expr>>);
}; // struct Operation

} // namespace printer
} // namespace mlc

/************** Lambda **************/
namespace mlc {
namespace printer {

struct LambdaObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::List<::mlc::printer::Id> args;
  ::mlc::printer::Expr body;
  explicit LambdaObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::List<::mlc::printer::Id> args,
                     ::mlc::printer::Expr body)
      : source_paths(source_paths), args(args), body(body) {}
  MLC_DEF_DYN_TYPE(LambdaObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Lambda");
}; // struct LambdaObj

struct Lambda : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Lambda, LambdaObj, ::mlc::printer::Expr)
      .Field("source_paths", &LambdaObj::source_paths)
      .Field("args", &LambdaObj::args)
      .Field("body", &LambdaObj::body)
      .StaticFn("__init__", ::mlc::InitOf<LambdaObj, ::mlc::List<::mlc::core::ObjectPath>,
                                          ::mlc::List<::mlc::printer::Id>, ::mlc::printer::Expr>);
}; // struct Lambda

} // namespace printer
} // namespace mlc

/************** Tuple **************/
namespace mlc {
namespace printer {

struct TupleObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::List<::mlc::printer::Expr> values;
  explicit TupleObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::List<::mlc::printer::Expr> values)
      : source_paths(source_paths), values(values) {}
  MLC_DEF_DYN_TYPE(TupleObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Tuple");
}; // struct TupleObj

struct Tuple : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Tuple, TupleObj, ::mlc::printer::Expr)
      .Field("source_paths", &TupleObj::source_paths)
      .Field("values", &TupleObj::values)
      .StaticFn("__init__",
                ::mlc::InitOf<TupleObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::List<::mlc::printer::Expr>>);
}; // struct Tuple

} // namespace printer
} // namespace mlc

/************** List **************/
namespace mlc {
namespace printer {

struct ListObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::List<::mlc::printer::Expr> values;
  explicit ListObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::List<::mlc::printer::Expr> values)
      : source_paths(source_paths), values(values) {}
  MLC_DEF_DYN_TYPE(ListObj, ::mlc::printer::ExprObj, "mlc.printer.ast.List");
}; // struct ListObj

struct List : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(List, ListObj, ::mlc::printer::Expr)
      .Field("source_paths", &ListObj::source_paths)
      .Field("values", &ListObj::values)
      .StaticFn("__init__",
                ::mlc::InitOf<ListObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::List<::mlc::printer::Expr>>);
}; // struct List

} // namespace printer
} // namespace mlc

/************** Dict **************/
namespace mlc {
namespace printer {

struct DictObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::List<::mlc::printer::Expr> keys;
  ::mlc::List<::mlc::printer::Expr> values;
  explicit DictObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::List<::mlc::printer::Expr> keys,
                   ::mlc::List<::mlc::printer::Expr> values)
      : source_paths(source_paths), keys(keys), values(values) {}
  MLC_DEF_DYN_TYPE(DictObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Dict");
}; // struct DictObj

struct Dict : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Dict, DictObj, ::mlc::printer::Expr)
      .Field("source_paths", &DictObj::source_paths)
      .Field("keys", &DictObj::keys)
      .Field("values", &DictObj::values)
      .StaticFn("__init__", ::mlc::InitOf<DictObj, ::mlc::List<::mlc::core::ObjectPath>,
                                          ::mlc::List<::mlc::printer::Expr>, ::mlc::List<::mlc::printer::Expr>>);
}; // struct Dict

} // namespace printer
} // namespace mlc

/************** Slice **************/
namespace mlc {
namespace printer {

struct SliceObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::printer::Expr> start;
  ::mlc::Optional<::mlc::printer::Expr> stop;
  ::mlc::Optional<::mlc::printer::Expr> step;
  explicit SliceObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::printer::Expr> start,
                    ::mlc::Optional<::mlc::printer::Expr> stop, ::mlc::Optional<::mlc::printer::Expr> step)
      : source_paths(source_paths), start(start), stop(stop), step(step) {}
  MLC_DEF_DYN_TYPE(SliceObj, ::mlc::printer::ExprObj, "mlc.printer.ast.Slice");
}; // struct SliceObj

struct Slice : public ::mlc::printer::Expr {
  MLC_DEF_OBJ_REF(Slice, SliceObj, ::mlc::printer::Expr)
      .Field("source_paths", &SliceObj::source_paths)
      .Field("start", &SliceObj::start)
      .Field("stop", &SliceObj::stop)
      .Field("step", &SliceObj::step)
      .StaticFn("__init__",
                ::mlc::InitOf<SliceObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::printer::Expr>,
                              ::mlc::Optional<::mlc::printer::Expr>, ::mlc::Optional<::mlc::printer::Expr>>);
}; // struct Slice

} // namespace printer
} // namespace mlc

/************** Assign **************/
namespace mlc {
namespace printer {

struct AssignObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr lhs;
  ::mlc::Optional<::mlc::printer::Expr> rhs;
  ::mlc::Optional<::mlc::printer::Expr> annotation;
  explicit AssignObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                     ::mlc::printer::Expr lhs, ::mlc::Optional<::mlc::printer::Expr> rhs,
                     ::mlc::Optional<::mlc::printer::Expr> annotation)
      : source_paths(source_paths), comment(comment), lhs(lhs), rhs(rhs), annotation(annotation) {}
  MLC_DEF_DYN_TYPE(AssignObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Assign");
}; // struct AssignObj

struct Assign : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Assign, AssignObj, ::mlc::printer::Stmt)
      .Field("source_paths", &AssignObj::source_paths)
      .Field("comment", &AssignObj::comment)
      .Field("lhs", &AssignObj::lhs)
      .Field("rhs", &AssignObj::rhs)
      .Field("annotation", &AssignObj::annotation)
      .StaticFn("__init__", ::mlc::InitOf<AssignObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                                          ::mlc::printer::Expr, ::mlc::Optional<::mlc::printer::Expr>,
                                          ::mlc::Optional<::mlc::printer::Expr>>);
}; // struct Assign

} // namespace printer
} // namespace mlc

/************** If **************/
namespace mlc {
namespace printer {

struct IfObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr cond;
  ::mlc::List<::mlc::printer::Stmt> then_branch;
  ::mlc::List<::mlc::printer::Stmt> else_branch;
  explicit IfObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                 ::mlc::printer::Expr cond, ::mlc::List<::mlc::printer::Stmt> then_branch,
                 ::mlc::List<::mlc::printer::Stmt> else_branch)
      : source_paths(source_paths), comment(comment), cond(cond), then_branch(then_branch), else_branch(else_branch) {}
  MLC_DEF_DYN_TYPE(IfObj, ::mlc::printer::StmtObj, "mlc.printer.ast.If");
}; // struct IfObj

struct If : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(If, IfObj, ::mlc::printer::Stmt)
      .Field("source_paths", &IfObj::source_paths)
      .Field("comment", &IfObj::comment)
      .Field("cond", &IfObj::cond)
      .Field("then_branch", &IfObj::then_branch)
      .Field("else_branch", &IfObj::else_branch)
      .StaticFn(
          "__init__",
          ::mlc::InitOf<IfObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>, ::mlc::printer::Expr,
                        ::mlc::List<::mlc::printer::Stmt>, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct If

} // namespace printer
} // namespace mlc

/************** While **************/
namespace mlc {
namespace printer {

struct WhileObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr cond;
  ::mlc::List<::mlc::printer::Stmt> body;
  explicit WhileObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                    ::mlc::printer::Expr cond, ::mlc::List<::mlc::printer::Stmt> body)
      : source_paths(source_paths), comment(comment), cond(cond), body(body) {}
  MLC_DEF_DYN_TYPE(WhileObj, ::mlc::printer::StmtObj, "mlc.printer.ast.While");
}; // struct WhileObj

struct While : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(While, WhileObj, ::mlc::printer::Stmt)
      .Field("source_paths", &WhileObj::source_paths)
      .Field("comment", &WhileObj::comment)
      .Field("cond", &WhileObj::cond)
      .Field("body", &WhileObj::body)
      .StaticFn("__init__", ::mlc::InitOf<WhileObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                                          ::mlc::printer::Expr, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct While

} // namespace printer
} // namespace mlc

/************** For **************/
namespace mlc {
namespace printer {

struct ForObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr lhs;
  ::mlc::printer::Expr rhs;
  ::mlc::List<::mlc::printer::Stmt> body;
  explicit ForObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                  ::mlc::printer::Expr lhs, ::mlc::printer::Expr rhs, ::mlc::List<::mlc::printer::Stmt> body)
      : source_paths(source_paths), comment(comment), lhs(lhs), rhs(rhs), body(body) {}
  MLC_DEF_DYN_TYPE(ForObj, ::mlc::printer::StmtObj, "mlc.printer.ast.For");
}; // struct ForObj

struct For : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(For, ForObj, ::mlc::printer::Stmt)
      .Field("source_paths", &ForObj::source_paths)
      .Field("comment", &ForObj::comment)
      .Field("lhs", &ForObj::lhs)
      .Field("rhs", &ForObj::rhs)
      .Field("body", &ForObj::body)
      .StaticFn("__init__",
                ::mlc::InitOf<ForObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                              ::mlc::printer::Expr, ::mlc::printer::Expr, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct For

} // namespace printer
} // namespace mlc

/************** With **************/
namespace mlc {
namespace printer {

struct WithObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::Optional<::mlc::printer::Expr> lhs;
  ::mlc::printer::Expr rhs;
  ::mlc::List<::mlc::printer::Stmt> body;
  explicit WithObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                   ::mlc::Optional<::mlc::printer::Expr> lhs, ::mlc::printer::Expr rhs,
                   ::mlc::List<::mlc::printer::Stmt> body)
      : source_paths(source_paths), comment(comment), lhs(lhs), rhs(rhs), body(body) {}
  MLC_DEF_DYN_TYPE(WithObj, ::mlc::printer::StmtObj, "mlc.printer.ast.With");
}; // struct WithObj

struct With : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(With, WithObj, ::mlc::printer::Stmt)
      .Field("source_paths", &WithObj::source_paths)
      .Field("comment", &WithObj::comment)
      .Field("lhs", &WithObj::lhs)
      .Field("rhs", &WithObj::rhs)
      .Field("body", &WithObj::body)
      .StaticFn("__init__", ::mlc::InitOf<WithObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                                          ::mlc::Optional<::mlc::printer::Expr>, ::mlc::printer::Expr,
                                          ::mlc::List<::mlc::printer::Stmt>>);
}; // struct With

} // namespace printer
} // namespace mlc

/************** ExprStmt **************/
namespace mlc {
namespace printer {

struct ExprStmtObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr expr;
  explicit ExprStmtObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                       ::mlc::printer::Expr expr)
      : source_paths(source_paths), comment(comment), expr(expr) {}
  MLC_DEF_DYN_TYPE(ExprStmtObj, ::mlc::printer::StmtObj, "mlc.printer.ast.ExprStmt");
}; // struct ExprStmtObj

struct ExprStmt : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(ExprStmt, ExprStmtObj, ::mlc::printer::Stmt)
      .Field("source_paths", &ExprStmtObj::source_paths)
      .Field("comment", &ExprStmtObj::comment)
      .Field("expr", &ExprStmtObj::expr)
      .StaticFn("__init__", ::mlc::InitOf<ExprStmtObj, ::mlc::List<::mlc::core::ObjectPath>,
                                          ::mlc::Optional<::mlc::Str>, ::mlc::printer::Expr>);
}; // struct ExprStmt

} // namespace printer
} // namespace mlc

/************** Assert **************/
namespace mlc {
namespace printer {

struct AssertObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Expr cond;
  ::mlc::Optional<::mlc::printer::Expr> msg;
  explicit AssertObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                     ::mlc::printer::Expr cond, ::mlc::Optional<::mlc::printer::Expr> msg)
      : source_paths(source_paths), comment(comment), cond(cond), msg(msg) {}
  MLC_DEF_DYN_TYPE(AssertObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Assert");
}; // struct AssertObj

struct Assert : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Assert, AssertObj, ::mlc::printer::Stmt)
      .Field("source_paths", &AssertObj::source_paths)
      .Field("comment", &AssertObj::comment)
      .Field("cond", &AssertObj::cond)
      .Field("msg", &AssertObj::msg)
      .StaticFn("__init__", ::mlc::InitOf<AssertObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                                          ::mlc::printer::Expr, ::mlc::Optional<::mlc::printer::Expr>>);
}; // struct Assert

} // namespace printer
} // namespace mlc

/************** Return **************/
namespace mlc {
namespace printer {

struct ReturnObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::Optional<::mlc::printer::Expr> value;
  explicit ReturnObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                     ::mlc::Optional<::mlc::printer::Expr> value)
      : source_paths(source_paths), comment(comment), value(value) {}
  MLC_DEF_DYN_TYPE(ReturnObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Return");
}; // struct ReturnObj

struct Return : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Return, ReturnObj, ::mlc::printer::Stmt)
      .Field("source_paths", &ReturnObj::source_paths)
      .Field("comment", &ReturnObj::comment)
      .Field("value", &ReturnObj::value)
      .StaticFn("__init__", ::mlc::InitOf<ReturnObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                                          ::mlc::Optional<::mlc::printer::Expr>>);
}; // struct Return

} // namespace printer
} // namespace mlc

/************** Function **************/
namespace mlc {
namespace printer {

struct FunctionObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Id name;
  ::mlc::List<::mlc::printer::Assign> args;
  ::mlc::List<::mlc::printer::Expr> decorators;
  ::mlc::Optional<::mlc::printer::Expr> return_type;
  ::mlc::List<::mlc::printer::Stmt> body;
  explicit FunctionObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                       ::mlc::printer::Id name, ::mlc::List<::mlc::printer::Assign> args,
                       ::mlc::List<::mlc::printer::Expr> decorators, ::mlc::Optional<::mlc::printer::Expr> return_type,
                       ::mlc::List<::mlc::printer::Stmt> body)
      : source_paths(source_paths), comment(comment), name(name), args(args), decorators(decorators),
        return_type(return_type), body(body) {
    for (Assign arg_doc : this->args) {
      if (arg_doc->comment.defined()) {
        MLC_THROW(ValueError) << "Function arg cannot have comment attached to them";
      }
    }
  }
  MLC_DEF_DYN_TYPE(FunctionObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Function");
}; // struct FunctionObj

struct Function : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Function, FunctionObj, ::mlc::printer::Stmt)
      .Field("source_paths", &FunctionObj::source_paths)
      .Field("comment", &FunctionObj::comment)
      .Field("name", &FunctionObj::name)
      .Field("args", &FunctionObj::args)
      .Field("decorators", &FunctionObj::decorators)
      .Field("return_type", &FunctionObj::return_type)
      .Field("body", &FunctionObj::body)
      .StaticFn(
          "__init__",
          ::mlc::InitOf<FunctionObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>,
                        ::mlc::printer::Id, ::mlc::List<::mlc::printer::Assign>, ::mlc::List<::mlc::printer::Expr>,
                        ::mlc::Optional<::mlc::printer::Expr>, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct Function

} // namespace printer
} // namespace mlc

/************** Class **************/
namespace mlc {
namespace printer {

struct ClassObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  ::mlc::printer::Id name;
  ::mlc::List<::mlc::printer::Expr> decorators;
  ::mlc::List<::mlc::printer::Stmt> body;
  explicit ClassObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment,
                    ::mlc::printer::Id name, ::mlc::List<::mlc::printer::Expr> decorators,
                    ::mlc::List<::mlc::printer::Stmt> body)
      : source_paths(source_paths), comment(comment), name(name), decorators(decorators), body(body) {}
  MLC_DEF_DYN_TYPE(ClassObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Class");
}; // struct ClassObj

struct Class : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Class, ClassObj, ::mlc::printer::Stmt)
      .Field("source_paths", &ClassObj::source_paths)
      .Field("comment", &ClassObj::comment)
      .Field("name", &ClassObj::name)
      .Field("decorators", &ClassObj::decorators)
      .Field("body", &ClassObj::body)
      .StaticFn(
          "__init__",
          ::mlc::InitOf<ClassObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>, ::mlc::printer::Id,
                        ::mlc::List<::mlc::printer::Expr>, ::mlc::List<::mlc::printer::Stmt>>);
}; // struct Class

} // namespace printer
} // namespace mlc

/************** Comment **************/
namespace mlc {
namespace printer {

struct CommentObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  explicit CommentObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment)
      : source_paths(source_paths), comment(comment) {}
  MLC_DEF_DYN_TYPE(CommentObj, ::mlc::printer::StmtObj, "mlc.printer.ast.Comment");
}; // struct CommentObj

struct Comment : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(Comment, CommentObj, ::mlc::printer::Stmt)
      .Field("source_paths", &CommentObj::source_paths)
      .Field("comment", &CommentObj::comment)
      .StaticFn("__init__",
                ::mlc::InitOf<CommentObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>>);
}; // struct Comment

} // namespace printer
} // namespace mlc

/************** DocString **************/
namespace mlc {
namespace printer {

struct DocStringObj : public ::mlc::Object {
  ::mlc::List<::mlc::core::ObjectPath> source_paths;
  ::mlc::Optional<::mlc::Str> comment;
  explicit DocStringObj(::mlc::List<::mlc::core::ObjectPath> source_paths, ::mlc::Optional<::mlc::Str> comment)
      : source_paths(source_paths), comment(comment) {}
  MLC_DEF_DYN_TYPE(DocStringObj, ::mlc::printer::StmtObj, "mlc.printer.ast.DocString");
}; // struct DocStringObj

struct DocString : public ::mlc::printer::Stmt {
  MLC_DEF_OBJ_REF(DocString, DocStringObj, ::mlc::printer::Stmt)
      .Field("source_paths", &DocStringObj::source_paths)
      .Field("comment", &DocStringObj::comment)
      .StaticFn("__init__",
                ::mlc::InitOf<DocStringObj, ::mlc::List<::mlc::core::ObjectPath>, ::mlc::Optional<::mlc::Str>>);
}; // struct DocString

} // namespace printer
} // namespace mlc

#endif // MLC_PRINTER_AST_H_
