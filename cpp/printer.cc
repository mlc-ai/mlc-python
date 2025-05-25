#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <mlc/printer/all.h>
#include <sstream>
#include <string_view>
#include <utility>

namespace mlc {
namespace printer {
namespace {
using ByteSpan = std::pair<size_t, size_t>;

class DocPrinter {
public:
  explicit DocPrinter(const PrinterConfig &options);
  virtual ~DocPrinter() = default;

  void Append(const Node &doc);
  void Append(const Node &doc, const PrinterConfig &cfg);
  ::mlc::Str GetString() const;

protected:
  void PrintDoc(const Node &doc);
  virtual void PrintTypedDoc(const Literal &doc) = 0;
  virtual void PrintTypedDoc(const Id &doc) = 0;
  virtual void PrintTypedDoc(const Attr &doc) = 0;
  virtual void PrintTypedDoc(const Index &doc) = 0;
  virtual void PrintTypedDoc(const Operation &doc) = 0;
  virtual void PrintTypedDoc(const Call &doc) = 0;
  virtual void PrintTypedDoc(const Lambda &doc) = 0;
  virtual void PrintTypedDoc(const List &doc) = 0;
  virtual void PrintTypedDoc(const Tuple &doc) = 0;
  virtual void PrintTypedDoc(const Dict &doc) = 0;
  virtual void PrintTypedDoc(const Slice &doc) = 0;
  virtual void PrintTypedDoc(const StmtBlock &doc) = 0;
  virtual void PrintTypedDoc(const Assign &doc) = 0;
  virtual void PrintTypedDoc(const If &doc) = 0;
  virtual void PrintTypedDoc(const While &doc) = 0;
  virtual void PrintTypedDoc(const For &doc) = 0;
  virtual void PrintTypedDoc(const With &doc) = 0;
  virtual void PrintTypedDoc(const ExprStmt &doc) = 0;
  virtual void PrintTypedDoc(const Assert &doc) = 0;
  virtual void PrintTypedDoc(const Return &doc) = 0;
  virtual void PrintTypedDoc(const Function &doc) = 0;
  virtual void PrintTypedDoc(const Class &doc) = 0;
  virtual void PrintTypedDoc(const Comment &doc) = 0;
  virtual void PrintTypedDoc(const DocString &doc) = 0;

  void PrintTypedDoc(const NodeObj *doc) {
    using PrinterVTable = std::unordered_map<int32_t, std::function<void(DocPrinter *, const NodeObj *)>>;
    // clang-format off
    #define MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Type) { Type::TObj::_type_index, [](DocPrinter *printer, const NodeObj *doc) { printer->PrintTypedDoc(Type(doc->DynCast<Type::TObj>())); } }
    // clang-format on
    static PrinterVTable vtable{
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Literal),   MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Id),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Attr),      MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Index),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Operation), MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Call),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Lambda),    MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(List),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Tuple),     MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Dict),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Slice),     MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(StmtBlock),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Assign),    MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(If),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(While),     MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(For),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(With),      MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(ExprStmt),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Assert),    MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Return),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Function),  MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Class),
        MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(Comment),   MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_(DocString),
    };
    // clang-format off
    #undef MLC_PRINTER_PRINT_TYPED_DOC_ENTRY_
    // clang-format on
    vtable.at(doc->GetTypeIndex())(this, doc);
  }
  void IncreaseIndent() { indent_ += options_->indent_spaces; }
  void DecreaseIndent() { indent_ -= options_->indent_spaces; }
  std::ostream &NewLine() {
    size_t start_pos = output_.tellp();
    output_ << "\n";
    line_starts_.push_back(output_.tellp());
    for (int i = 0; i < indent_; ++i) {
      output_ << ' ';
    }
    size_t end_pos = output_.tellp();
    underlines_exempted_.push_back({start_pos, end_pos});
    return output_;
  }
  std::ostringstream output_;
  std::vector<ByteSpan> underlines_exempted_;

private:
  using ObjectPath = ::mlc::core::ObjectPath;

  void MarkSpan(const ByteSpan &span, const ObjectPath &path);
  PrinterConfig options_;
  int indent_ = 0;
  std::vector<size_t> line_starts_;
  mlc::List<ObjectPath> path_to_underline_;
  std::vector<std::vector<ByteSpan>> current_underline_candidates_;
  std::vector<int> current_max_path_length_;
  std::vector<ByteSpan> underlines_;
};

inline const char *OpKindToString(OperationObj::Kind kind) {
  switch (kind) {
  case OperationObj::Kind::kUSub:
    return "-";
  case OperationObj::Kind::kInvert:
    return "~";
  case OperationObj::Kind::kNot:
    return "not ";
  case OperationObj::Kind::kAdd:
    return "+";
  case OperationObj::Kind::kSub:
    return "-";
  case OperationObj::Kind::kMult:
    return "*";
  case OperationObj::Kind::kDiv:
    return "/";
  case OperationObj::Kind::kFloorDiv:
    return "//";
  case OperationObj::Kind::kMod:
    return "%";
  case OperationObj::Kind::kPow:
    return "**";
  case OperationObj::Kind::kLShift:
    return "<<";
  case OperationObj::Kind::kRShift:
    return ">>";
  case OperationObj::Kind::kBitAnd:
    return "&";
  case OperationObj::Kind::kBitOr:
    return "|";
  case OperationObj::Kind::kBitXor:
    return "^";
  case OperationObj::Kind::kLt:
    return "<";
  case OperationObj::Kind::kLtE:
    return "<=";
  case OperationObj::Kind::kEq:
    return "==";
  case OperationObj::Kind::kNotEq:
    return "!=";
  case OperationObj::Kind::kGt:
    return ">";
  case OperationObj::Kind::kGtE:
    return ">=";
  case OperationObj::Kind::kAnd:
    return "and";
  case OperationObj::Kind::kOr:
    return "or";
  default:
    MLC_THROW(ValueError) << "Unknown operation kind: " << static_cast<int>(kind);
  }
  MLC_UNREACHABLE();
}

inline const char *OpKindToString(int64_t kind) { return OpKindToString(static_cast<OperationObj::Kind>(kind)); }

/*!
 * \brief Operator precedence based on https://docs.python.org/3/reference/expressions.html#operator-precedence
 */
enum class ExprPrecedence : int32_t {
  /*! \brief Unknown precedence */
  kUnkown = 0,
  /*! \brief Lambda Expression */
  kLambda = 1,
  /*! \brief Conditional Expression */
  kIfThenElse = 2,
  /*! \brief Boolean OR */
  kBooleanOr = 3,
  /*! \brief Boolean AND */
  kBooleanAnd = 4,
  /*! \brief Boolean NOT */
  kBooleanNot = 5,
  /*! \brief Comparisons */
  kComparison = 6,
  /*! \brief Bitwise OR */
  kBitwiseOr = 7,
  /*! \brief Bitwise XOR */
  kBitwiseXor = 8,
  /*! \brief Bitwise AND */
  kBitwiseAnd = 9,
  /*! \brief Shift Operators */
  kShift = 10,
  /*! \brief Addition and subtraction */
  kAdd = 11,
  /*! \brief Multiplication, division, floor division, remainder */
  kMult = 12,
  /*! \brief Positive negative and bitwise NOT */
  kUnary = 13,
  /*! \brief Exponentiation */
  kExp = 14,
  /*! \brief Index access, attribute access, call and atom expression */
  kIdentity = 15,
};

inline ExprPrecedence GetExprPrecedence(const Expr &doc) {
  // Key is the type index of Doc
  static const std::unordered_map<uint32_t, ExprPrecedence> doc_type_precedence = {
      {LiteralObj::_type_index, ExprPrecedence::kIdentity}, {IdObj::_type_index, ExprPrecedence::kIdentity},
      {AttrObj::_type_index, ExprPrecedence::kIdentity},    {IndexObj::_type_index, ExprPrecedence::kIdentity},
      {CallObj::_type_index, ExprPrecedence::kIdentity},    {LambdaObj::_type_index, ExprPrecedence::kLambda},
      {TupleObj::_type_index, ExprPrecedence::kIdentity},   {ListObj::_type_index, ExprPrecedence::kIdentity},
      {DictObj::_type_index, ExprPrecedence::kIdentity},
  };
  // Key is the value of OperationDocNode::Kind
  static const std::vector<ExprPrecedence> op_kind_precedence = []() {
    using OpKind = OperationObj::Kind;
    std::map<OpKind, ExprPrecedence> raw_table = {
        {OpKind::kUSub, ExprPrecedence::kUnary},        {OpKind::kInvert, ExprPrecedence::kUnary},
        {OpKind::kNot, ExprPrecedence::kBooleanNot},    {OpKind::kAdd, ExprPrecedence::kAdd},
        {OpKind::kSub, ExprPrecedence::kAdd},           {OpKind::kMult, ExprPrecedence::kMult},
        {OpKind::kDiv, ExprPrecedence::kMult},          {OpKind::kFloorDiv, ExprPrecedence::kMult},
        {OpKind::kMod, ExprPrecedence::kMult},          {OpKind::kPow, ExprPrecedence::kExp},
        {OpKind::kLShift, ExprPrecedence::kShift},      {OpKind::kRShift, ExprPrecedence::kShift},
        {OpKind::kBitAnd, ExprPrecedence::kBitwiseAnd}, {OpKind::kBitOr, ExprPrecedence::kBitwiseOr},
        {OpKind::kBitXor, ExprPrecedence::kBitwiseXor}, {OpKind::kLt, ExprPrecedence::kComparison},
        {OpKind::kLtE, ExprPrecedence::kComparison},    {OpKind::kEq, ExprPrecedence::kComparison},
        {OpKind::kNotEq, ExprPrecedence::kComparison},  {OpKind::kGt, ExprPrecedence::kComparison},
        {OpKind::kGtE, ExprPrecedence::kComparison},    {OpKind::kAnd, ExprPrecedence::kBooleanAnd},
        {OpKind::kOr, ExprPrecedence::kBooleanOr},      {OpKind::kIfThenElse, ExprPrecedence::kIfThenElse},
    };
    std::vector<ExprPrecedence> table(static_cast<size_t>(OpKind::kSpecialEnd) + 1, ExprPrecedence::kUnkown);
    for (const auto &kv : raw_table) {
      table[static_cast<int>(kv.first)] = kv.second;
    }
    return table;
  }();
  if (const auto *op_doc = doc->as<OperationObj>()) {
    ExprPrecedence precedence = op_kind_precedence.at(op_doc->op);
    if (precedence == ExprPrecedence::kUnkown) {
      MLC_THROW(ValueError) << "Unknown precedence for operator: " << op_doc->op;
    }
    return precedence;
  } else if (auto it = doc_type_precedence.find(doc->GetTypeIndex()); it != doc_type_precedence.end()) {
    return it->second;
  }
  MLC_THROW(ValueError) << "Unknown precedence for doc type: " << doc->GetTypeKey();
  MLC_UNREACHABLE();
}

inline std::vector<ByteSpan> MergeAndExemptSpans(const std::vector<ByteSpan> &spans,
                                                 const std::vector<ByteSpan> &spans_exempted) {
  // use prefix sum to merge and exempt spans
  std::vector<ByteSpan> res;
  std::vector<std::pair<size_t, int>> prefix_stamp;
  for (ByteSpan span : spans) {
    prefix_stamp.push_back({span.first, 1});
    prefix_stamp.push_back({span.second, -1});
  }
  // at most spans.size() spans accumulated in prefix sum
  // use spans.size() + 1 as stamp unit to exempt all positive spans
  // with only one negative span
  int max_n = static_cast<int>(spans.size()) + 1;
  for (ByteSpan span : spans_exempted) {
    prefix_stamp.push_back({span.first, -max_n});
    prefix_stamp.push_back({span.second, max_n});
  }
  std::sort(prefix_stamp.begin(), prefix_stamp.end());
  int prefix_sum = 0;
  int n = static_cast<int>(prefix_stamp.size());
  for (int i = 0; i < n - 1; ++i) {
    prefix_sum += prefix_stamp[i].second;
    // positive prefix sum leads to spans without exemption
    // different stamp positions guarantee the stamps in same position accumulated
    if (prefix_sum > 0 && prefix_stamp[i].first < prefix_stamp[i + 1].first) {
      if (res.size() && res.back().second == prefix_stamp[i].first) {
        // merge to the last spans if it is successive
        res.back().second = prefix_stamp[i + 1].first;
      } else {
        // add a new independent span
        res.push_back({prefix_stamp[i].first, prefix_stamp[i + 1].first});
      }
    }
  }
  return res;
}

inline size_t GetTextWidth(const mlc::Str &text, const ByteSpan &span) {
  // FIXME: this only works for ASCII characters.
  // To do this "correctly", we need to parse UTF-8 into codepoints
  // and call wcwidth() or equivalent for every codepoint.
  size_t ret = 0;
  for (size_t i = span.first; i != span.second; ++i) {
    if (isprint(text[i])) {
      ret += 1;
    }
  }
  return ret;
}

inline size_t MoveBack(size_t pos, size_t distance) { return distance > pos ? 0 : pos - distance; }

inline size_t MoveForward(size_t pos, size_t distance, size_t max) {
  return distance > max - pos ? max : pos + distance;
}

inline size_t GetLineIndex(size_t byte_pos, const std::vector<size_t> &line_starts) {
  auto it = std::upper_bound(line_starts.begin(), line_starts.end(), byte_pos);
  return (it - line_starts.begin()) - 1;
}

using UnderlineIter = typename std::vector<ByteSpan>::const_iterator;

inline ByteSpan PopNextUnderline(UnderlineIter *next_underline, UnderlineIter end_underline) {
  if (*next_underline == end_underline) {
    constexpr size_t kMaxSizeT = 18446744073709551615U;
    return {kMaxSizeT, kMaxSizeT};
  } else {
    return *(*next_underline)++;
  }
}

inline void PrintChunk(const std::pair<size_t, size_t> &lines_range,
                       const std::pair<UnderlineIter, UnderlineIter> &underlines, const mlc::Str &text,
                       const std::vector<size_t> &line_starts, const PrinterConfig &options, size_t line_number_width,
                       std::ostringstream *out) {
  UnderlineIter next_underline = underlines.first;
  ByteSpan current_underline = PopNextUnderline(&next_underline, underlines.second);

  for (size_t line_idx = lines_range.first; line_idx < lines_range.second; ++line_idx) {
    if (options->print_line_numbers) {
      (*out) << std::setw(line_number_width) << std::right << std::to_string(line_idx + 1) << ' ';
    }
    size_t line_start = line_starts.at(line_idx);
    size_t line_end = line_idx + 1 == line_starts.size() ? text.size() : line_starts.at(line_idx + 1);
    (*out) << std::string_view(text.data() + line_start, line_end - line_start);
    bool printed_underline = false;
    size_t line_pos = line_start;
    bool printed_extra_caret = 0;
    while (current_underline.first < line_end) {
      if (!printed_underline) {
        for (size_t i = 0; i < line_number_width; ++i) {
          (*out) << ' ';
        }
        printed_underline = true;
      }
      size_t underline_end_for_line = std::min<size_t>(line_end, current_underline.second);
      size_t num_spaces = GetTextWidth(text, {line_pos, current_underline.first});
      if (num_spaces > 0 && printed_extra_caret) {
        num_spaces -= 1;
        printed_extra_caret = false;
      }
      for (size_t i = 0; i < num_spaces; ++i) {
        (*out) << ' ';
      }

      size_t num_carets = GetTextWidth(text, {current_underline.first, underline_end_for_line});
      if (num_carets == 0 && !printed_extra_caret) {
        // Special case: when underlineing an empty or unprintable string, make sure to print
        // at least one caret still.
        num_carets = 1;
        printed_extra_caret = true;
      } else if (num_carets > 0 && printed_extra_caret) {
        num_carets -= 1;
        printed_extra_caret = false;
      }
      for (size_t i = 0; i < num_carets; ++i) {
        (*out) << '^';
      }

      line_pos = current_underline.first = underline_end_for_line;
      if (current_underline.first == current_underline.second) {
        current_underline = PopNextUnderline(&next_underline, underlines.second);
      }
    }
    if (printed_underline) {
      (*out) << '\n';
    }
  }
}

inline void PrintCut(size_t num_lines_skipped, std::ostringstream *out) {
  if (num_lines_skipped != 0) {
    (*out) << "(... " << num_lines_skipped << " lines skipped ...)\n";
  }
}

inline std::pair<size_t, size_t> GetLinesForUnderline(const ByteSpan &underline, const std::vector<size_t> &line_starts,
                                                      size_t num_lines, const PrinterConfig &options) {
  size_t first_line_of_underline = GetLineIndex(underline.first, line_starts);
  size_t first_line_of_chunk = MoveBack(first_line_of_underline, options->num_context_lines);
  size_t end_line_of_underline = GetLineIndex(underline.second - 1, line_starts) + 1;
  size_t end_line_of_chunk = MoveForward(end_line_of_underline, options->num_context_lines, num_lines);

  return {first_line_of_chunk, end_line_of_chunk};
}

// If there is only one line between the chunks, it is better to print it as is,
// rather than something like "(... 1 line skipped ...)".
constexpr const size_t kMinLinesToCutOut = 2;

inline bool TryMergeChunks(std::pair<size_t, size_t> *cur_chunk, const std::pair<size_t, size_t> &new_chunk) {
  if (new_chunk.first < cur_chunk->second + kMinLinesToCutOut) {
    cur_chunk->second = new_chunk.second;
    return true;
  } else {
    return false;
  }
}

inline size_t GetNumLines(const mlc::Str &text, const std::vector<size_t> &line_starts) {
  if (static_cast<int64_t>(line_starts.back()) == text.size()) {
    // Final empty line doesn't count as a line
    return line_starts.size() - 1;
  } else {
    return line_starts.size();
  }
}

inline size_t GetLineNumberWidth(size_t num_lines, const PrinterConfig &options) {
  if (options->print_line_numbers) {
    return std::to_string(num_lines).size() + 1;
  } else {
    return 0;
  }
}

inline mlc::Str DecorateText(const mlc::Str &text, const std::vector<size_t> &line_starts, const PrinterConfig &options,
                             const std::vector<ByteSpan> &underlines) {
  size_t num_lines = GetNumLines(text, line_starts);
  size_t line_number_width = GetLineNumberWidth(num_lines, options);

  std::ostringstream ret;
  if (underlines.empty()) {
    PrintChunk({0, num_lines}, {underlines.begin(), underlines.begin()}, text, line_starts, options, line_number_width,
               &ret);
    return ret.str();
  }

  size_t last_end_line = 0;
  std::pair<size_t, size_t> cur_chunk = GetLinesForUnderline(underlines[0], line_starts, num_lines, options);
  if (cur_chunk.first < kMinLinesToCutOut) {
    cur_chunk.first = 0;
  }

  auto first_underline_in_cur_chunk = underlines.begin();
  for (auto underline_it = underlines.begin() + 1; underline_it != underlines.end(); ++underline_it) {
    std::pair<size_t, size_t> new_chunk = GetLinesForUnderline(*underline_it, line_starts, num_lines, options);

    if (!TryMergeChunks(&cur_chunk, new_chunk)) {
      PrintCut(cur_chunk.first - last_end_line, &ret);
      PrintChunk(cur_chunk, {first_underline_in_cur_chunk, underline_it}, text, line_starts, options, line_number_width,
                 &ret);
      last_end_line = cur_chunk.second;
      cur_chunk = new_chunk;
      first_underline_in_cur_chunk = underline_it;
    }
  }

  PrintCut(cur_chunk.first - last_end_line, &ret);
  if (num_lines - cur_chunk.second < kMinLinesToCutOut) {
    cur_chunk.second = num_lines;
  }
  PrintChunk(cur_chunk, {first_underline_in_cur_chunk, underlines.end()}, text, line_starts, options, line_number_width,
             &ret);
  PrintCut(num_lines - cur_chunk.second, &ret);
  return ret.str();
}

inline DocPrinter::DocPrinter(const PrinterConfig &options) : options_(options) { line_starts_.push_back(0); }

inline void DocPrinter::Append(const Node &doc) { Append(doc, PrinterConfig()); }

inline void DocPrinter::Append(const Node &doc, const PrinterConfig &cfg) {
  for (ObjectPath p : cfg->path_to_underline) {
    path_to_underline_.push_back(p);
    current_max_path_length_.push_back(0);
    current_underline_candidates_.push_back(std::vector<ByteSpan>());
  }
  PrintDoc(doc);
  for (const auto &c : current_underline_candidates_) {
    underlines_.insert(underlines_.end(), c.begin(), c.end());
  }
}

inline ::mlc::Str DocPrinter::GetString() const {
  std::string text = output_.str();
  // Remove any trailing indentation
  while (!text.empty() && text.back() == ' ') {
    text.pop_back();
  }
  if (!text.empty() && text.back() != '\n') {
    text.push_back('\n');
  }
  return DecorateText(text, line_starts_, options_, MergeAndExemptSpans(underlines_, underlines_exempted_));
}

inline void DocPrinter::PrintDoc(const Node &doc) {
  size_t start_pos = output_.tellp();
  this->PrintTypedDoc(doc.get());
  size_t end_pos = output_.tellp();
  for (ObjectPath path : doc->source_paths) {
    MarkSpan({start_pos, end_pos}, path);
  }
}

inline void DocPrinter::MarkSpan(const ByteSpan &span, const ObjectPath &path) {
  int n = static_cast<int>(path_to_underline_.size());
  for (int i = 0; i < n; ++i) {
    ObjectPath p = path_to_underline_[i];
    if (path->length >= current_max_path_length_[i] && path->IsPrefixOf(p.get())) {
      if (path->length > current_max_path_length_[i]) {
        current_max_path_length_[i] = static_cast<int>(path->length);
        current_underline_candidates_[i].clear();
      }
      current_underline_candidates_[i].push_back(span);
    }
  }
}

class PythonDocPrinter : public DocPrinter {
public:
  explicit PythonDocPrinter(const PrinterConfig &options) : DocPrinter(options) {}

protected:
  using DocPrinter::PrintDoc;

  void PrintTypedDoc(const Literal &doc) final;
  void PrintTypedDoc(const Id &doc) final;
  void PrintTypedDoc(const Attr &doc) final;
  void PrintTypedDoc(const Index &doc) final;
  void PrintTypedDoc(const Operation &doc) final;
  void PrintTypedDoc(const Call &doc) final;
  void PrintTypedDoc(const Lambda &doc) final;
  void PrintTypedDoc(const List &doc) final;
  void PrintTypedDoc(const Dict &doc) final;
  void PrintTypedDoc(const Tuple &doc) final;
  void PrintTypedDoc(const Slice &doc) final;
  void PrintTypedDoc(const StmtBlock &doc) final;
  void PrintTypedDoc(const Assign &doc) final;
  void PrintTypedDoc(const If &doc) final;
  void PrintTypedDoc(const While &doc) final;
  void PrintTypedDoc(const For &doc) final;
  void PrintTypedDoc(const ExprStmt &doc) final;
  void PrintTypedDoc(const Assert &doc) final;
  void PrintTypedDoc(const Return &doc) final;
  void PrintTypedDoc(const With &doc) final;
  void PrintTypedDoc(const Function &doc) final;
  void PrintTypedDoc(const Class &doc) final;
  void PrintTypedDoc(const Comment &doc) final;
  void PrintTypedDoc(const DocString &doc) final;

private:
  void NewLineWithoutIndent() {
    size_t start_pos = output_.tellp();
    output_ << "\n";
    size_t end_pos = output_.tellp();
    underlines_exempted_.push_back({start_pos, end_pos});
  }

  template <typename DocType> void PrintJoinedDocs(const mlc::List<DocType> &docs, const char *separator) {
    bool is_first = true;
    for (DocType doc : docs) {
      if (is_first) {
        is_first = false;
      } else {
        output_ << separator;
      }
      PrintDoc(doc);
    }
  }

  void PrintIndentedBlock(const mlc::List<Stmt> &docs) {
    IncreaseIndent();
    for (Stmt d : docs) {
      NewLine();
      PrintDoc(d);
    }
    if (docs.empty()) {
      NewLine();
      output_ << "pass";
    }
    DecreaseIndent();
  }

  void PrintDecorators(const mlc::List<Expr> &decorators) {
    for (Expr decorator : decorators) {
      output_ << "@";
      PrintDoc(decorator);
      NewLine();
    }
  }

  /*!
   * \brief Print expression and add parenthesis if needed.
   */
  void PrintChildExpr(const Expr &doc, ExprPrecedence parent_precedence, bool parenthesis_for_same_precedence = false) {
    ExprPrecedence doc_precedence = GetExprPrecedence(doc);
    if (doc_precedence < parent_precedence ||
        (parenthesis_for_same_precedence && doc_precedence == parent_precedence)) {
      output_ << "(";
      PrintDoc(doc);
      output_ << ")";
    } else {
      PrintDoc(doc);
    }
  }

  /*!
   * \brief Print expression and add parenthesis if doc has lower precedence than parent.
   */
  void PrintChildExpr(const Expr &doc, const Expr &parent, bool parenthesis_for_same_precedence = false) {
    ExprPrecedence parent_precedence = GetExprPrecedence(parent);
    return PrintChildExpr(doc, parent_precedence, parenthesis_for_same_precedence);
  }

  /*!
   * \brief Print expression and add parenthesis if doc doesn't have higher precedence than parent.
   *
   * This function should be used to print an child expression that needs to be wrapped
   * by parenthesis even if it has the same precedence as its parent, e.g., the `b` in `a + b`
   * and the `b` and `c` in `a if b else c`.
   */
  void PrintChildExprConservatively(const Expr &doc, const Expr &parent) {
    PrintChildExpr(doc, parent, /*parenthesis_for_same_precedence=*/true);
  }

  void MaybePrintCommentInline(const Stmt &stmt) {
    if (const mlc::StrObj *comment = stmt->comment.get()) {
      bool has_newline = std::find(comment->begin(), comment->end(), '\n') != comment->end();
      if (has_newline) {
        MLC_THROW(ValueError) << "ValueError: Comment string of " << stmt->GetTypeKey()
                              << " cannot have newline, but got: " << comment;
      }
      size_t start_pos = output_.tellp();
      output_ << "  # " << comment->data();
      size_t end_pos = output_.tellp();
      underlines_exempted_.push_back({start_pos, end_pos});
    }
  }

  void MaybePrintCommenMultiLines(const Stmt &stmt, bool new_line = false) {
    if (const mlc::StrObj *comment = stmt->comment.get()) {
      bool first_line = true;
      size_t start_pos = output_.tellp();
      for (const std::string_view &line : comment->Split('\n')) {
        if (first_line) {
          output_ << "# " << line;
          first_line = false;
        } else {
          NewLine() << "# " << line;
        }
      }
      size_t end_pos = output_.tellp();
      underlines_exempted_.push_back({start_pos, end_pos});
      if (new_line) {
        NewLine();
      }
    }
  }

  void PrintDocString(const mlc::Str &comment) {
    size_t start_pos = output_.tellp();
    output_ << "\"\"\"";
    for (const std::string_view &line : comment->Split('\n')) {
      if (line.empty()) {
        // No indentation on empty line
        output_ << "\n";
      } else {
        NewLine() << line;
      }
    }
    NewLine() << "\"\"\"";
    size_t end_pos = output_.tellp();
    underlines_exempted_.push_back({start_pos, end_pos});
  }
  void PrintBlockComment(const mlc::Str &comment) {
    IncreaseIndent();
    NewLine();
    PrintDocString(comment);
    DecreaseIndent();
  }
};

inline void PythonDocPrinter::PrintTypedDoc(const Literal &doc) {
  Any value = doc->value;
  int32_t type_index = value.GetTypeIndex();
  if (!value.defined()) {
    output_ << "None";
  } else if (type_index == kMLCBool) {
    output_ << (value.operator bool() ? "True" : "False");
  } else if (type_index == kMLCInt) {
    output_ << value.operator int64_t();
  } else if (type_index == kMLCFloat) {
    double v = value.operator double();
    // TODO(yelite): Make float number printing roundtrippable
    if (std::isinf(v) || std::isnan(v)) {
      output_ << '"' << v << '"';
    } else if (std::nearbyint(v) == v) {
      // Special case for floating-point values which would be
      // formatted using %g, are not displayed in scientific
      // notation, and whose fractional part is zero.
      //
      // By default, using `operator<<(std::ostream&, double)`
      // delegates to the %g printf formatter.  This strips off any
      // trailing zeros, and also strips the decimal point if no
      // trailing zeros are found.  When parsed in python, due to the
      // missing decimal point, this would incorrectly convert a float
      // to an integer.  Providing the `std::showpoint` modifier
      // instead delegates to the %#g printf formatter.  On its own,
      // this resolves the round-trip errors, but also prevents the
      // trailing zeros from being stripped off.
      std::showpoint(output_);
      std::fixed(output_);
      output_.precision(1);
      output_ << v;
    } else {
      std::defaultfloat(output_);
      std::noshowpoint(output_);
      output_.precision(17);
      output_ << v;
    }
  } else if (type_index == kMLCStr) {
    (value.operator mlc::Str())->PrintEscape(output_);
  } else {
    MLC_THROW(TypeError) << "TypeError: Unsupported literal value type: " << value.GetTypeKey();
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const Id &doc) { output_ << doc->name; }

inline void PythonDocPrinter::PrintTypedDoc(const Attr &doc) {
  PrintChildExpr(doc->obj, doc);
  output_ << "." << doc->name;
}

inline void PythonDocPrinter::PrintTypedDoc(const Index &doc) {
  PrintChildExpr(doc->obj, doc);
  if (doc->idx.size() == 0) {
    output_ << "[()]";
  } else {
    output_ << "[";
    PrintJoinedDocs(doc->idx, ", ");
    output_ << "]";
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const Operation &doc) {
  using OpKind = OperationObj::Kind;
  if (doc->op < OpKind::kUnaryEnd) {
    // Unary Operators
    if (doc->operands.size() != 1) {
      MLC_THROW(ValueError) << "ValueError: Unary operator requires 1 operand, but got " << doc->operands.size();
    }
    output_ << OpKindToString(doc->op);
    PrintChildExpr(doc->operands[0], doc);
  } else if (doc->op == OpKind::kPow) {
    // Power operator is different than other binary operators
    // It's right-associative and binds less tightly than unary operator on its right.
    // https://docs.python.org/3/reference/expressions.html#the-power-operator
    // https://docs.python.org/3/reference/expressions.html#operator-precedence
    if (doc->operands.size() != 2) {
      MLC_THROW(ValueError) << "Operator '**' requires 2 operands, but got " << doc->operands.size();
    }
    PrintChildExprConservatively(doc->operands[0], doc);
    output_ << " ** ";
    PrintChildExpr(doc->operands[1], ExprPrecedence::kUnary);
  } else if (doc->op < OpKind::kBinaryEnd) {
    // Binary Operator
    if (doc->operands.size() != 2) {
      MLC_THROW(ValueError) << "Binary operator requires 2 operands, but got " << doc->operands.size();
    }
    PrintChildExpr(doc->operands[0], doc);
    output_ << " " << OpKindToString(doc->op) << " ";
    PrintChildExprConservatively(doc->operands[1], doc);
  } else if (doc->op == OpKind::kIfThenElse) {
    if (doc->operands.size() != 3) {
      MLC_THROW(ValueError) << "IfThenElse requires 3 operands, but got " << doc->operands.size();
    }
    PrintChildExpr(doc->operands[1], doc);
    output_ << " if ";
    PrintChildExprConservatively(doc->operands[0], doc);
    output_ << " else ";
    PrintChildExprConservatively(doc->operands[2], doc);
  } else {
    MLC_THROW(ValueError) << "Unknown OperationDocNode::Kind " << static_cast<int>(doc->op);
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const Call &doc) {
  PrintChildExpr(doc->callee, doc);
  output_ << "(";
  // Print positional args
  bool is_first = true;
  for (Expr arg : doc->args) {
    if (is_first) {
      is_first = false;
    } else {
      output_ << ", ";
    }
    PrintDoc(arg);
  }
  // Print keyword args
  if (doc->kwargs_keys.size() != doc->kwargs_values.size()) {
    MLC_THROW(ValueError) << "CallDoc should have equal number of elements in kwargs_keys and kwargs_values.";
  }
  for (int64_t i = 0; i < doc->kwargs_keys.size(); i++) {
    if (is_first) {
      is_first = false;
    } else {
      output_ << ", ";
    }
    const mlc::Str &keyword = doc->kwargs_keys[i];
    output_ << keyword;
    output_ << "=";
    PrintDoc(doc->kwargs_values[i]);
  }

  output_ << ")";
}

inline void PythonDocPrinter::PrintTypedDoc(const Lambda &doc) {
  output_ << "lambda ";
  PrintJoinedDocs(doc->args, ", ");
  output_ << ": ";
  PrintChildExpr(doc->body, doc);
}

inline void PythonDocPrinter::PrintTypedDoc(const List &doc) {
  output_ << "[";
  PrintJoinedDocs(doc->values, ", ");
  output_ << "]";
}

inline void PythonDocPrinter::PrintTypedDoc(const Tuple &doc) {
  output_ << "(";
  if (doc->values.size() == 1) {
    PrintDoc(doc->values[0]);
    output_ << ",";
  } else {
    PrintJoinedDocs(doc->values, ", ");
  }
  output_ << ")";
}

inline void PythonDocPrinter::PrintTypedDoc(const Dict &doc) {
  if (doc->keys.size() != doc->values.size()) {
    MLC_THROW(ValueError) << "DictDoc should have equal number of elements in keys and values.";
  }
  output_ << "{";
  size_t idx = 0;
  for (Expr key : doc->keys) {
    if (idx > 0) {
      output_ << ", ";
    }
    PrintDoc(key);
    output_ << ": ";
    PrintDoc(doc->values[idx]);
    idx++;
  }
  output_ << "}";
}

inline void PythonDocPrinter::PrintTypedDoc(const Slice &doc) {
  if (doc->start != nullptr) {
    PrintDoc(doc->start.value());
  }
  output_ << ":";
  if (doc->stop != nullptr) {
    PrintDoc(doc->stop.value());
  }
  if (doc->step != nullptr) {
    output_ << ":";
    PrintDoc(doc->step.value());
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const StmtBlock &doc) {
  bool is_first = true;
  for (Stmt stmt : doc->stmts) {
    if (is_first) {
      is_first = false;
    } else {
      NewLine();
    }
    PrintDoc(stmt);
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const Assign &doc) {
  bool lhs_empty = false;
  if (const auto *tuple_doc = doc->lhs->as<TupleObj>()) {
    if (tuple_doc->values.size() == 0) {
      lhs_empty = true;
      if (doc->annotation.defined()) {
        MLC_THROW(ValueError) << "ValueError: `Assign.annotation` should be None when `Assign.lhs` is empty, "
                                 "but got: "
                              << doc->annotation.value();
      }
    } else {
      PrintJoinedDocs(tuple_doc->values, ", ");
    }
  } else {
    PrintDoc(doc->lhs);
  }

  if (doc->annotation.defined()) {
    output_ << ": ";
    PrintDoc(doc->annotation.value());
  }
  if (doc->rhs.defined()) {
    if (!lhs_empty) {
      output_ << " = ";
    }
    if (const auto *tuple_doc = doc->rhs.as<TupleObj>()) {
      if (tuple_doc->values.size() > 1) {
        PrintJoinedDocs(tuple_doc->values, ", ");
      } else {
        PrintDoc(doc->rhs.value());
      }
    } else {
      PrintDoc(doc->rhs.value());
    }
  }
  MaybePrintCommentInline(doc);
}

inline void PythonDocPrinter::PrintTypedDoc(const If &doc) {
  MaybePrintCommenMultiLines(doc, true);
  output_ << "if ";
  PrintDoc(doc->cond);
  output_ << ":";
  PrintIndentedBlock(doc->then_branch);
  if (!doc->else_branch.empty()) {
    NewLine();
    output_ << "else:";
    PrintIndentedBlock(doc->else_branch);
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const While &doc) {
  MaybePrintCommenMultiLines(doc, true);
  output_ << "while ";
  PrintDoc(doc->cond);
  output_ << ":";
  PrintIndentedBlock(doc->body);
}

inline void PythonDocPrinter::PrintTypedDoc(const For &doc) {
  MaybePrintCommenMultiLines(doc, true);
  output_ << "for ";
  if (const auto *tuple = doc->lhs->as<TupleObj>()) {
    if (tuple->values.size() == 1) {
      PrintDoc(tuple->values[0]);
      output_ << ",";
    } else {
      PrintJoinedDocs(tuple->values, ", ");
    }
  } else {
    PrintDoc(doc->lhs);
  }
  output_ << " in ";
  PrintDoc(doc->rhs);
  output_ << ":";
  PrintIndentedBlock(doc->body);
}

inline void PythonDocPrinter::PrintTypedDoc(const With &doc) {
  MaybePrintCommenMultiLines(doc, true);
  output_ << "with ";
  PrintDoc(doc->rhs);
  if (doc->lhs != nullptr) {
    output_ << " as ";
    PrintDoc(doc->lhs.value());
  }
  output_ << ":";

  PrintIndentedBlock(doc->body);
}

inline void PythonDocPrinter::PrintTypedDoc(const ExprStmt &doc) {
  PrintDoc(doc->expr);
  MaybePrintCommentInline(doc);
}

inline void PythonDocPrinter::PrintTypedDoc(const Assert &doc) {
  output_ << "assert ";
  PrintDoc(doc->cond);
  if (doc->msg.defined()) {
    output_ << ", ";
    PrintDoc(doc->msg.value());
  }
  MaybePrintCommentInline(doc);
}

inline void PythonDocPrinter::PrintTypedDoc(const Return &doc) {
  output_ << "return";
  if (doc->value.defined()) {
    output_ << " ";
    PrintDoc(doc->value.value());
  }
  MaybePrintCommentInline(doc);
}

inline void PythonDocPrinter::PrintTypedDoc(const Function &doc) {
  PrintDecorators(doc->decorators);
  output_ << "def ";
  PrintDoc(doc->name);
  output_ << "(";
  PrintJoinedDocs(doc->args, ", ");
  output_ << ")";
  if (doc->return_type.defined()) {
    output_ << " -> ";
    PrintDoc(doc->return_type.value());
  }
  output_ << ":";
  if (doc->comment.defined()) {
    PrintBlockComment(doc->comment.value());
  }
  PrintIndentedBlock(doc->body);
  NewLineWithoutIndent();
}

inline void PythonDocPrinter::PrintTypedDoc(const Class &doc) {
  PrintDecorators(doc->decorators);
  output_ << "class ";
  PrintDoc(doc->name);
  output_ << ":";
  if (doc->comment.defined()) {
    PrintBlockComment(doc->comment.value());
  }
  PrintIndentedBlock(doc->body);
}

inline void PythonDocPrinter::PrintTypedDoc(const Comment &doc) {
  if (doc->comment.defined()) {
    MaybePrintCommenMultiLines(doc, false);
  }
}

inline void PythonDocPrinter::PrintTypedDoc(const DocString &doc) {
  if (doc->comment.defined()) {
    mlc::Str comment(doc->comment.value());
    if (!comment->empty()) {
      PrintDocString(comment);
    }
  }
}

} // namespace
} // namespace printer
} // namespace mlc

namespace mlc {
namespace registry {
mlc::Str DocToPythonScript(mlc::printer::Node node, mlc::printer::PrinterConfig cfg) {
  if (cfg->num_context_lines < 0) {
    constexpr int32_t kMaxInt32 = 2147483647;
    cfg->num_context_lines = kMaxInt32;
  }
  printer::PythonDocPrinter printer(cfg);
  printer.Append(node, cfg);
  mlc::Str result = printer.GetString();
  while (!result->empty() && std::isspace(result->back())) {
    result->pop_back();
  }
  return result;
}
} // namespace registry
} // namespace mlc
