#ifndef MLC_PRINTER_IR_PRINTER_H
#define MLC_PRINTER_IR_PRINTER_H

#include "./ast.h"

namespace mlc {
namespace printer {

struct DefaultFrameObj : public Object {
  mlc::List<Stmt> stmts;

  DefaultFrameObj() = default;
  explicit DefaultFrameObj(mlc::List<Stmt> stmts) : stmts(stmts) {}
  MLC_DEF_DYN_TYPE(DefaultFrameObj, Object, "mlc.printer.DefaultFrame");
};

struct DefaultFrame : public ObjectRef {
  MLC_DEF_OBJ_REF(DefaultFrame, DefaultFrameObj, ObjectRef)
      .Field("stmts", &DefaultFrameObj::stmts)
      .StaticFn("__init__", InitOf<DefaultFrameObj, mlc::List<Stmt>>);
}; // struct DefaultFrame

struct VarInfoObj : public Object {
  Optional<Str> name;
  Func creator;

  explicit VarInfoObj(Optional<Str> name, Func creator) : name(name), creator(creator) {}
  MLC_DEF_DYN_TYPE(VarInfoObj, Object, "mlc.printer.VarInfo");
}; // struct VarInfoObj

struct VarInfo : public ObjectRef {
  MLC_DEF_OBJ_REF(VarInfo, VarInfoObj, ObjectRef)
      .Field("creator", &VarInfoObj::creator)
      .Field("name", &VarInfoObj::name)
      .StaticFn("__init__", InitOf<VarInfoObj, Optional<Str>, Func>);
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
    // Legalize characters in the name
    for (char &c : name_hint) {
      if (c != '_' && !std::isalnum(c)) {
        c = '_';
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

  Any operator()(const Optional<ObjectRef> &opt_obj, const ObjectPath &path) const {
    if (!opt_obj.has_value()) {
      return Literal::Null();
    }
    Node ret = ::mlc::base::LibState::IRPrint(opt_obj.value(), this, path);
    ret->source_paths->push_back(path);
    return ret;
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

  MLC_DEF_DYN_TYPE(IRPrinterObj, Object, "mlc.printer.IRPrinter");
}; // struct IRPrinterObj

struct IRPrinter : public ObjectRef {
  MLC_DEF_OBJ_REF(IRPrinter, IRPrinterObj, ObjectRef)
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
}; // struct IRPrinter

inline Str ToPython(const ObjectRef &obj, const PrinterConfig &cfg) {
  IRPrinter printer(cfg);
  printer->FramePush(DefaultFrame());
  Node ret = ::mlc::base::LibState::IRPrint(obj, printer, ObjectPath::Root());
  printer->FramePop();
  return ret->ToPython(cfg);
}

} // namespace printer
} // namespace mlc

#endif // MLC_PRINTER_IR_PRINTER_H
