#ifndef MLC_PRINTER_ALL_H_
#define MLC_PRINTER_ALL_H_

#include "./ast.h"        // IWYU pragma: export
#include "./ir_printer.h" // IWYU pragma: export

namespace mlc {
namespace printer {

inline Expr ExprObj::Attr(mlc::Str name) const {
  return ::mlc::printer::Attr(::mlc::List<::mlc::core::ObjectPath>{}, Expr(this), name);
}

inline Expr ExprObj::Index(mlc::List<::mlc::printer::Expr> idx) const {
  return ::mlc::printer::Index(::mlc::List<::mlc::core::ObjectPath>{}, Expr(this), idx);
}

inline Expr ExprObj::Call(mlc::List<::mlc::printer::Expr> args) const {
  return ::mlc::printer::Call(::mlc::List<::mlc::core::ObjectPath>{}, Expr(this), args, mlc::List<::mlc::Str>{},
                              mlc::List<::mlc::printer::Expr>{});
}

inline Expr ExprObj::CallKw(mlc::List<::mlc::printer::Expr> args, mlc::List<::mlc::Str> kwargs_keys,
                            mlc::List<::mlc::printer::Expr> kwargs_values) const {
  return ::mlc::printer::Call(::mlc::List<::mlc::core::ObjectPath>{}, Expr(this), args, kwargs_keys, kwargs_values);
}

} // namespace printer
} // namespace mlc

#endif // MLC_PRINTER_ALL_H_
