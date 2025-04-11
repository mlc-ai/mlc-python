#include "./common.h"
#include <gtest/gtest.h>
#include <mlc/sym/all.h>

namespace {

using namespace mlc::sym;
using mlc::base::DType;

TEST(Pattern, Basic_1) {
  Var y("y", DType::Int(64));
  PVar<Expr> px, py, pz;
  auto r = 1 + (y + 1);
  ASSERT_FALSE((px + (px + px)).Match(r));
  ASSERT_FALSE((px + (py + py)).Match(r));
  ASSERT_TRUE((px + (py + pz)).Match(r));
}

TEST(Pattern, Basic_2) {
  Var y("y", DType::Int(64));
  PVar<Expr> px, py, pz;
  Expr r = 1 + (y + 1);
  ASSERT_TRUE((px + (py + px)).Match(r));
  Expr rr = (px + py).Eval();
  ASSERT_TRUE(ExprDeepEqual()(rr, 1 + y));
  ASSERT_TRUE(ExprDeepEqual()(px.Eval() + py.Eval(), 1 + y));
}

TEST(Pattern, Basic_3) {
  Var x("x", DType::Int(64));
  Var y("y", DType::Int(64));
  Var z("z", DType::Int(64));
  PVar<Expr> px, py, pz;
  ASSERT_TRUE((px + max(py, px)).Match((x + 1) + max(y, (x + 1))));
  ASSERT_TRUE(ExprDeepEqual()(px.Eval(), x + 1));
  ASSERT_TRUE(!(px + min(py, px)).Match((x + 1) + max(y, (x + 1))));
  ASSERT_TRUE((px + min(py, px)).Match(z + min(y, z)));
  ASSERT_TRUE((px + truncdiv(py, px * py)).Match(x + truncdiv(2, x * 2)));
  ASSERT_TRUE((px - truncmod(py, px * pz)).Match(x - truncmod(2, x * 2)));
  ASSERT_TRUE((px - floormod(py, px * 2)).Match(x - floormod(2, x * 2)));
}

TEST(Pattern, Logical) {
  Var x("x", DType::Int(64));
  Var y("y", DType::Int(64));
  Var z("z", DType::Int(64));
  PVar<Expr> px, py, pz;
  ASSERT_TRUE((px == pz).Match(x == 1));
  ASSERT_TRUE((px != pz).Match(x != 1));
  ASSERT_TRUE((px > py).Match(x > y));
  ASSERT_TRUE((px < py).Match(x < y));
  ASSERT_TRUE((px <= py).Match(x <= y));
  ASSERT_TRUE((px >= py).Match(x >= y));
  ASSERT_TRUE((px >= py && px < pz).Match(x >= y && x < z));
  ASSERT_TRUE((!(px > py || px != py)).Match(!(x > y || x != y)));
}

TEST(Pattern, Select) {
  Var x("x", DType::Int(64));
  Var y("y", DType::Int(64));
  Var z("z", DType::Int(64));
  PVar<Expr> px, py, pz;
  {
    ASSERT_TRUE(select(px >= pz, py, py + pz).Match(Select((x + 1) >= 1, y, y + 1)));
    ASSERT_TRUE(ExprDeepEqual()(px.Eval(), x + 1));
  }
  {
    ASSERT_TRUE(select(px > pz, py, py + pz).Match(Select(x > 1, y, y + 1)));
    ASSERT_EQ(pz.Eval().as<IntImmObj>()->value, 1);
  }
  ASSERT_TRUE(!select(px > pz, py, py + pz).Match(Select(x > 2, y, y + 1)));
  ASSERT_TRUE(!select(px > pz, py, py).Match(Select(x > 2, y, y + 1)));
  {
    ASSERT_TRUE(select(px, py, pz).Match(Select(x > 2, y, y + 1)));
    ASSERT_TRUE(ExprDeepEqual()(pz.Eval(), y + 1));
  }
}

TEST(Pattern, BitIntrinsics) {
  Var x("x", DType::Int(64));
  Var y("y", DType::Int(64));
  Var z("z", DType::Int(64));
  PVar<Expr> px, py, pz;
  ASSERT_TRUE((px << py).Match(x << 1));
  ASSERT_TRUE((px >> py).Match(x >> 1));
  ASSERT_TRUE((px & py).Match(x & 1));
  ASSERT_TRUE((px | py).Match(x | 1));
  ASSERT_TRUE((px ^ py).Match(x ^ 1));
  ASSERT_TRUE((~px).Match(~x));
  ASSERT_TRUE((px - (~(py | (px * pz)))).Match(x - (~(2 | (x * 2)))));
}

TEST(Pattern, IntImm) {
  Var tx("tx", DType::Int(64));
  Var ty("ty", DType::Int(64));
  PVar<IntImm> c;
  PVar<Var> v;
  {
    // We can match integer and Var, both of which are
    // special case container of Expr
    ASSERT_TRUE((v * c).Match(tx * 3));
    ASSERT_EQ(c.Eval()->value, 3);
    ASSERT_TRUE((v * 3).Match(tx * 3));
  }
  // cannot match c to ty
  ASSERT_TRUE(!(v * c).Match(tx * ty));
  // cannot match tx + 1 to v
  ASSERT_TRUE(!(v * c).Match((tx + 1) * 3));
}

TEST(Pattern, IfThenElse) {
  Var x("x", DType::Int(64));
  Var y("y", DType::Int(64));
  Var z("z", DType::Int(64));
  PVar<Expr> px, py, pz;
  ASSERT_TRUE(if_then_else(px > pz, py, py + pz).Match(if_then_else(x > 1, y, y + 1)));
  ASSERT_EQ(pz.Eval()->as<IntImmObj>()->value, 1);
}

TEST(Pattern, Ramp) {
  Var x("x", DType::Int(64));
  PVar<Expr> px;
  PVar<int64_t> lanes;
  ASSERT_TRUE(ramp(px, PConst<Expr>(Expr::Int64(1)), lanes).Match(Ramp(x, Expr::Int64(1), 10)));
  ASSERT_TRUE(lanes.Eval() == 10);
  ASSERT_TRUE(!ramp(px, PConst<Expr>(Expr::Int64(1)), lanes).Match(Ramp(x, Expr::Int64(2), 10)));
}

TEST(Pattern, Broadcast) {
  Var x("x", DType::Int(64));
  PVar<Expr> px, py;
  PVar<int64_t> lanes;
  ASSERT_TRUE(broadcast(px, lanes).Match(Broadcast(x, 10)));
  ASSERT_TRUE(lanes.Eval() == 10);
  ASSERT_TRUE(broadcast(px * py, lanes).Match(Broadcast(x * 10, 10)));
}

} // namespace
