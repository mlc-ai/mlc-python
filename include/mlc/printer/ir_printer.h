#ifndef MLC_PRINTER_IR_PRINTER_H
#define MLC_PRINTER_IR_PRINTER_H

#include "./ast.h"
#include <algorithm>

namespace mlc {
namespace printer {

struct DefaultFrameObj : public Object {
  mlc::List<Stmt> stmts;

  DefaultFrameObj() = default;
  explicit DefaultFrameObj(mlc::List<Stmt> stmts) : stmts(stmts) {}
  MLC_DEF_DYN_TYPE(MLC_EXPORTS, DefaultFrameObj, Object, "mlc.printer.DefaultFrame");
};

struct DefaultFrame : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, DefaultFrame, DefaultFrameObj, ObjectRef)
      .Field("stmts", &DefaultFrameObj::stmts)
      .StaticFn("__init__", InitOf<DefaultFrameObj, mlc::List<Stmt>>);
  explicit DefaultFrame() : DefaultFrame(DefaultFrame::New()) {}
  explicit DefaultFrame(mlc::List<Stmt> stmts) : DefaultFrame(DefaultFrame::New(stmts)) {}
}; // struct DefaultFrame

struct VarInfoObj : public Object {
  Optional<Str> name;
  Func creator;

  explicit VarInfoObj(Optional<Str> name, Func creator) : name(name), creator(creator) {}
  MLC_DEF_DYN_TYPE(MLC_EXPORTS, VarInfoObj, Object, "mlc.printer.VarInfo");
}; // struct VarInfoObj

struct VarInfo : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, VarInfo, VarInfoObj, ObjectRef)
      .Field("creator", &VarInfoObj::creator)
      .Field("name", &VarInfoObj::name)
      .StaticFn("__init__", InitOf<VarInfoObj, Optional<Str>, Func>);
  explicit VarInfo(Optional<Str> name, Func creator) : VarInfo(VarInfo::New(name, creator)) {}
}; // struct VarInfo

struct IRPrinterObj : public Object {
  PrinterConfig cfg;
  mlc::Dict<Any, VarInfo> obj2info;
  mlc::Dict<Str, int64_t> defined_names;
  mlc::UList frames;     // list[Frame]
  mlc::UDict frame_vars; // dict[Frame, list[Var]]

  explicit IRPrinterObj(PrinterConfig cfg) : cfg(cfg) {}
  explicit IRPrinterObj(PrinterConfig cfg, mlc::Dict<Any, VarInfo> obj2info, mlc::Dict<Str, int64_t> defined_names,
                        mlc::UList frames, mlc::UDict frame_vars)
      : cfg(cfg), obj2info(obj2info), defined_names(defined_names), frames(frames), frame_vars(frame_vars) {}

  bool VarIsDefined(const ObjectRef &obj) { return obj2info->count(obj) > 0; }

  Id VarDef(Str name_hint, const ObjectRef &obj, const Optional<ObjectRef> &frame) {
    if (auto it = obj2info.find(obj); it != obj2info.end()) {
      Optional<Str> name = (*it).second->name;
      return Id(name.value());
    }
    bool needs_normalize =
        std::any_of(name_hint->begin(), name_hint->end(), [](char c) { return c != '_' && !std::isalnum(c); });
    if (needs_normalize) {
      name_hint = Str(name_hint->c_str());
      for (char &c : name_hint) {
        if (c != '_' && !std::isalnum(c)) {
          c = '_';
        }
      }
    }
    // Find a unique name
    Str name = name_hint;
    for (int i = 1; defined_names.count(name) > 0; ++i) {
      name = name_hint.ToStdString() + '_' + std::to_string(i);
    }
    defined_names->Set(name, 1);
    this->_VarDef(VarInfo(name, Func([name]() { return Id(name); })), obj, frame);
    return Id(name);
  }

  void VarDefNoName(const Func &creator, const ObjectRef &obj, const Optional<ObjectRef> &frame) {
    if (obj2info.count(obj) > 0) {
      MLC_THROW(KeyError) << "Variable already defined: " << obj;
    }
    this->_VarDef(VarInfo(mlc::Null, creator), obj, frame);
  }

  void _VarDef(VarInfo var_info, const ObjectRef &obj, const Optional<ObjectRef> &_frame) {
    ObjectRef frame = _frame.defined() ? _frame.value() : this->frames.back().operator ObjectRef();
    obj2info->Set(obj, var_info);
    auto it = frame_vars.find(frame);
    if (it == frame_vars.end()) {
      MLC_THROW(KeyError) << "Frame is not pushed to IRPrinter: " << frame;
    } else {
      frame_vars[frame].operator mlc::UList().push_back(obj);
    }
  }

  void VarRemove(const ObjectRef &obj) {
    auto it = obj2info.find(obj);
    if (it == obj2info.end()) {
      MLC_THROW(KeyError) << "No such object: " << obj;
    }
    Optional<Str> name = (*it).second->name;
    if (name.has_value()) {
      defined_names.erase(name.value());
    }
    obj2info.erase(it);
  }

  Optional<Expr> VarGet(const ObjectRef &obj) {
    auto it = obj2info.find(obj);
    if (it == obj2info.end()) {
      return Null;
    }
    return (*it).second->creator();
  }

  Any operator()(Any source, ObjectPath path) const {
    if (source.type_index == kMLCNone) {
      return Literal::Null({path});
    }
    if (source.type_index == kMLCBool) {
      return Literal::Bool(source.operator bool(), {path});
    }
    if (source.type_index == kMLCInt) {
      return Literal::Int(source.operator int64_t(), {path});
    }
    if (source.type_index == kMLCStr || source.type_index == kMLCRawStr) {
      return Literal::Str(source.operator Str(), {path});
    }
    if (source.type_index == kMLCFloat) {
      return Literal::Float(source.operator double(), {path});
    }
    if (source.type_index < kMLCStaticObjectBegin) {
      MLC_THROW(ValueError) << "Unsupported type: " << source;
    }
    Node ret = ::mlc::Lib::IRPrint(source.operator Object *(), this, path);
    ret->source_paths->push_back(path);
    return ret;
  }

  template <typename T> mlc::List<T> ApplyToList(const UList &list, const ObjectPath &p) const {
    // TODO: expose a Python interface
    int64_t n = list->size();
    mlc::List<T> args;
    args.reserve(n);
    for (int64_t i = 0; i < n; ++i) {
      args->push_back(this->operator()(list[i], p->WithListIndex(i)));
    }
    return args;
  }

  void FramePush(const ObjectRef &frame) {
    frames.push_back(frame);
    frame_vars[frame] = mlc::UList();
  }

  void FramePop() {
    ObjectRef frame = frames.back();
    for (ObjectRef var : frame_vars[frame].operator UList()) {
      this->VarRemove(var);
    }
    frame_vars.erase(frame);
    frames.pop_back();
  }

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, IRPrinterObj, Object, "mlc.printer.IRPrinter");
}; // struct IRPrinterObj

struct IRPrinter : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, IRPrinter, IRPrinterObj, ObjectRef)
      .Field("cfg", &IRPrinterObj::cfg)
      .Field("obj2info", &IRPrinterObj::obj2info)
      .Field("defined_names", &IRPrinterObj::defined_names)
      .Field("frames", &IRPrinterObj::frames)
      .Field("frame_vars", &IRPrinterObj::frame_vars)
      .StaticFn(
          "__init__",
          InitOf<IRPrinterObj, PrinterConfig, mlc::Dict<Any, VarInfo>, mlc::Dict<Str, int64_t>, mlc::UList, mlc::UDict>)
      .MemFn("var_is_defined", &IRPrinterObj::VarIsDefined)
      .MemFn("var_def", &IRPrinterObj::VarDef)
      .MemFn("var_def_no_name", &IRPrinterObj::VarDefNoName)
      .MemFn("var_remove", &IRPrinterObj::VarRemove)
      .MemFn("var_get", &IRPrinterObj::VarGet)
      .MemFn("frame_push", &IRPrinterObj::FramePush)
      .MemFn("frame_pop", &IRPrinterObj::FramePop)
      .MemFn("__call__", &IRPrinterObj::operator());
  explicit IRPrinter(PrinterConfig cfg) : IRPrinter(IRPrinter::New(cfg)) {}
  explicit IRPrinter(PrinterConfig cfg, mlc::Dict<Any, VarInfo> obj2info, mlc::Dict<Str, int64_t> defined_names,
                     mlc::UList frames, mlc::UDict frame_vars)
      : IRPrinter(IRPrinter::New(cfg, obj2info, defined_names, frames, frame_vars)) {}
}; // struct IRPrinter

inline Str ToPython(const ObjectRef &obj, const PrinterConfig &cfg) {
  IRPrinter printer(cfg);
  DefaultFrame frame;
  printer->FramePush(frame);
  Node ret = ::mlc::Lib::IRPrint(obj, printer, ObjectPath::Root());
  printer->FramePop();
  if (frame->stmts->empty()) {
    return ret->ToPython(cfg);
  }
  if (const auto *block = ret.as<StmtBlockObj>()) {
    // TODO: support List::insert by iterator
    frame->stmts->insert(frame->stmts.size(), block->stmts->begin(), block->stmts->end());
  } else if (const auto *expr = ret.as<ExprObj>()) {
    frame->stmts->push_back(ExprStmt(mlc::List<ObjectPath>{}, Optional<Str>{}, Expr(expr)));
  } else if (const auto *stmt = ret.as<StmtObj>()) {
    frame->stmts->push_back(Stmt(stmt));
  } else {
    MLC_THROW(ValueError) << "Unsupported type: " << ret;
  }
  ret = StmtBlock(mlc::List<ObjectPath>{}, Optional<Str>{}, frame->stmts);
  return ret->ToPython(cfg);
}

} // namespace printer
} // namespace mlc

#endif // MLC_PRINTER_IR_PRINTER_H
