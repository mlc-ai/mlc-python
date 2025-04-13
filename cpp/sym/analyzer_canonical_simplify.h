#ifndef MLC_SYM_ANALYZER_CANONICAL_SIMPLIFY_H_
#define MLC_SYM_ANALYZER_CANONICAL_SIMPLIFY_H_

#include "./utils.h"
#include <mlc/sym/all.h>

namespace mlc {
namespace sym {

enum class DivMode {
  kTruncDiv = 0,
  kFloorDiv = 1,
};

class CanonicalSimplifier {
public:
  Expr operator()(const Expr &expr);
  void Update(const Var &var, const Expr &new_expr, bool allow_override = false);

  explicit CanonicalSimplifier(AnalyzerObj::Impl *parent);
  ~CanonicalSimplifier();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

struct SplitExpr;
struct SumExpr;

struct SplitExprObj : public ExprObj {
  MLC_DEF_DYN_TYPE(MLC_EXPORTS, SplitExprObj, ExprObj, "mlc.sym.SplitExpr");

  Expr index;
  int64_t lower_factor; // = 1
  int64_t upper_factor; // = kPosInf
  int64_t scale;        // = 1
  DivMode div_mode;     // = DivMode::kTruncDiv

  explicit SplitExprObj(DLDataType dtype, Expr index, int64_t lower_factor, int64_t upper_factor, int64_t scale,
                        DivMode div_mode)
      : ExprObj(dtype), index(index), lower_factor(lower_factor), upper_factor(upper_factor), scale(scale),
        div_mode(div_mode) {}

  void Verify() const {
    if (!(upper_factor == kPosInf || upper_factor % lower_factor == 0)) {
      MLC_THROW(InternalError) << "Failed verification";
    }
  }
  std::string __str__() const {
    std::ostringstream os;
    os << "SplitExpr(index=" << this->index       //
       << ", lower_factor=" << this->lower_factor //
       << ", upper_factor=" << this->upper_factor //
       << ", scale=" << this->scale               //
       << ", div_mode=" << (this->div_mode == DivMode::kTruncDiv ? "kTruncDiv" : "kFloorDiv") << ")";
    return os.str();
  }
  Expr NormalizeWithScale(int64_t sscale) const;
  Expr Normalize() const { return NormalizeWithScale(1); }
  void MulToSelf(int64_t s) { this->scale *= s; }
  bool CanPushCastToChildren(DLDataType dtype, AnalyzerObj::Impl *analyzer) const;
  void PushCastToChildren(DLDataType dtype);
  inline bool IndexEqual(const SplitExpr &other) const;
  inline bool DivModeCompatibleTo(DivMode mode) const;
};

struct SplitExpr : public Expr {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, SplitExpr, SplitExprObj, Expr) //
      .MemFn("__str__", &SplitExprObj::__str__);
  MLC_DEF_OBJ_REF_COW_();
  explicit SplitExpr(DLDataType dtype, Expr index, int64_t lower_factor = 1, int64_t upper_factor = kPosInf,
                     int64_t scale = 1, DivMode div_mode = DivMode::kTruncDiv)
      : SplitExpr(SplitExpr::New(dtype, index, lower_factor, upper_factor, scale, div_mode)) {}
};

struct SumExprObj : public ExprObj {
  MLC_DEF_DYN_TYPE(MLC_EXPORTS, SumExprObj, ExprObj, "mlc.sym.SumExpr");

  std::vector<SplitExpr> args;
  int64_t base{0};
  explicit SumExprObj(DLDataType dtype, std::vector<SplitExpr> args, int64_t base) : args(std::move(args)), base(base) {
    this->dtype = dtype;
  }
  explicit SumExprObj(DLDataType dtype) : args(), base(0) { this->dtype = dtype; }

  bool IsZero() const { return base == 0 && args.size() == 0; }
  Expr Normalize() const;
  bool DivisibleBy(int64_t scale);
  void MulToSelf(int64_t scale);
  void DivideBy(int64_t scale);
  void AddToSelf(int64_t value) { this->base += value; }
  void AddToSelf(SplitExpr other, int64_t scale);
  void AddToSelf(const SumExpr &other, int64_t scale);
  bool CanPushCastToChildren(DLDataType dtype, AnalyzerObj::Impl *analyzer) const;
  void PushCastToChildren(DLDataType dtype);
  std::string __str__() const {
    std::ostringstream os;
    os << "SumExpr(base=" << this->base << ", args=[";
    bool is_first = true;
    for (const auto &arg : this->args) {
      if (!is_first) {
        os << ", ";
      } else {
        is_first = false;
      }
      os << arg->__str__();
    }
    os << "])";
    return os.str();
  }

private:
  static std::vector<SplitExpr> SimplifySplitExprs(std::vector<SplitExpr> args);
  static Expr Normalize_(DLDataType dtype, const std::vector<SplitExpr> &args, int64_t base);
};

struct SumExpr : public Expr {
  SumExpr(DLDataType dtype) : SumExpr(SumExpr::New(dtype)) {}
  SumExpr(DLDataType dtype, std::vector<SplitExpr> args, int64_t base) : SumExpr(SumExpr::New(dtype, args, base)) {}
  MLC_DEF_OBJ_REF(MLC_EXPORTS, SumExpr, SumExprObj, Expr) //
      .MemFn("__str__", &SumExprObj::__str__);
  MLC_DEF_OBJ_REF_COW_();
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_CANONICAL_SIMPLIFY_H_
