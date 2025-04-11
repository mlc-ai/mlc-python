#ifndef MLC_SYM_ANALYZER_INTERVAL_SET_H_
#define MLC_SYM_ANALYZER_INTERVAL_SET_H_

#include <mlc/sym/all.h>

namespace mlc {
namespace sym {

struct IntervalSet;

struct IntervalSetObj {
  MLCAny _mlc_header;
  Expr min_value;
  Expr max_value;
  explicit IntervalSetObj(Expr min_value, Expr max_value) : _mlc_header{}, min_value(min_value), max_value(max_value) {}
  MLC_DEF_DYN_TYPE(MLC_EXPORTS, IntervalSetObj, ::mlc::Object, "mlc.sym.IntervalSet");

  bool HasUpperBound() const;
  bool HasLowerBound() const;
  bool IsSinglePoint() const;
  bool IsEmpty() const;
  bool IsEverything() const;
  std::string __str__() const {
    std::ostringstream os;
    os << "IntervalSet[" << min_value << ", " << max_value << "]";
    return os.str();
  }
  IntervalSet Union(IntervalSetObj *b, AnalyzerObj::Impl *analyzer) const;
  IntervalSet Intersect(IntervalSetObj *b, AnalyzerObj::Impl *analyzer) const;
};

struct IntervalSet : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, IntervalSet, IntervalSetObj, ObjectRef)
      .Field("min_value", &IntervalSetObj::min_value)
      .Field("max_value", &IntervalSetObj::max_value)
      .MemFn("__str__", &IntervalSetObj::__str__)
      .StaticFn("__init__", ::mlc::InitOf<IntervalSetObj, Expr, Expr>);
  explicit IntervalSet(Expr min_value, Expr max_value) : IntervalSet(IntervalSet::New(min_value, max_value)) {}
  static IntervalSet Nothing() { return IntervalSet::Empty(); }
  static IntervalSet SinglePoint(Expr value) { return IntervalSet(value, value); }
  static IntervalSet Everything();
  static IntervalSet Empty();
  static IntervalSet FromRange(const Range &range);
  static IntervalSet Interval(Expr min, Expr max);
  static IntervalSet Intersect(const List<IntervalSet> &sets, AnalyzerObj::Impl *analyzer);
};

struct IntervalSetAnalyzer {
  IntervalSet operator()(const Expr &expr, const Dict<Var, IntervalSet> &dom_map);
  IntervalSet operator()(const Expr &expr);
  void Update(const Var &var, const IntervalSet &new_interval_set, bool allow_override = false);
  void Bind(const Var &var, const Range &new_range, bool allow_override = false);
  std::function<void()> EnterConstraint(const Expr &constraint);
  explicit IntervalSetAnalyzer(AnalyzerObj::Impl *parent);
  ~IntervalSetAnalyzer();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sym
} // namespace mlc

#endif // MLC_SYM_ANALYZER_INTERVAL_SET_H_
