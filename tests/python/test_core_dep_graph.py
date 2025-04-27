import mlc.dataclasses as mlcd
import pytest
from mlc import DepGraph


@mlcd.py_class(repr=False)
class Var(mlcd.PyClass):
    name: str

    def __str__(self) -> str:
        return self.name


@mlcd.py_class(repr=False)
class Stmt(mlcd.PyClass):
    args: list[Var]
    outs: list[Var]

    def __str__(self) -> str:
        lhs = ", ".join(x.name for x in self.outs) if self.outs else "(empty)"
        rhs = ", ".join(x.name for x in self.args) if self.args else "(empty)"
        return f"{lhs} := {rhs}"


def stmt_inputs(stmt: Stmt) -> list[Var]:
    return stmt.args


def stmt_outputs(stmt: Stmt) -> list[Var]:
    return stmt.outs


# --- Fixtures ---
@pytest.fixture
def simple_graph() -> tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]]:
    # a -> b -> c -> d
    a, b, c, d = Var("a"), Var("b"), Var("c"), Var("d")
    stmts = [Stmt(args=[a], outs=[b]), Stmt(args=[b], outs=[c]), Stmt(args=[c], outs=[d])]
    g = DepGraph.from_stmts(
        input_vars=[a],
        stmts=stmts,
        stmt_to_inputs=stmt_inputs,
        stmt_to_outputs=stmt_outputs,
    )
    return g, (a, b, c, d), stmts


@pytest.fixture
def branch_graph() -> tuple[DepGraph, tuple[Var, Var, Var, Var, Var], list[Stmt]]:
    a = Var("a")
    b = Var("b")
    c = Var("c")
    d = Var("d")
    e = Var("e")
    stmts = [Stmt(args=[a], outs=[b, c]), Stmt(args=[b], outs=[d]), Stmt(args=[c], outs=[e])]
    g = DepGraph.from_stmts([a], stmts, stmt_inputs, stmt_outputs)
    return g, (a, b, c, d, e), stmts


# --- Tests covering all APIs ---


def test_from_stmts_nodes_and_var_mappings(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    # Check nodes iteration: head + 3 stmt nodes
    nodes = list(g.nodes)
    assert len(nodes) == 4
    head, n1, n2, n3 = nodes
    assert head.stmt is None
    assert g.get_var_producer(a).is_(head)
    assert g.get_var_consumers(a) == [n1]
    assert g.get_var_producer(b).is_(n1)
    assert g.get_var_consumers(b) == [n2]
    assert g.get_var_producer(d).is_(n3)
    # heads next/prev
    assert head.next.is_(n1) and n1.prev.is_(head)  # type: ignore[union-attr]
    # get_node_from_stmt valid and invalid
    assert g.get_node_from_stmt(stmts[0]).is_(n1)
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(Stmt(args=[], outs=[]))


def test_create_and_insert(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, _, _, _), _ = simple_graph
    # create a new statement e := a
    e = Var("e")
    new_stmt = Stmt(args=[a], outs=[e])
    new_node = g.create_node(new_stmt)
    # not yet in graph: get_node_from_stmt should fail
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(new_stmt)
    # insert after head
    g.insert_after(g._head, new_node)
    # now in graph
    assert g.get_node_from_stmt(new_stmt).is_(new_node)
    # var mapping updated
    assert g.get_var_producer(e).is_(new_node)
    assert new_node in g.get_var_consumers(a)
    # remove the inserted node for cleanup
    g.erase_node(new_node)


def test_insert_before_and_order(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (_, b, _, _), _ = simple_graph
    nodes = list(g.nodes)
    _, n1, n2, _ = nodes
    x = Var("x")
    x_stmt = Stmt(args=[b], outs=[x])
    x_node = g.create_node(x_stmt)
    g.insert_before(n2, x_node)
    assert x_node.prev.is_(n1) and x_node.next.is_(n2)  # type: ignore[union-attr]
    assert n1.next.is_(x_node) and n2.prev.is_(x_node)  # type: ignore[union-attr]
    g.erase_node(x_node)


def test_erase_node_and_cleanup(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (_, _, _, d), _ = simple_graph
    nodes = list(g.nodes)
    _, _, _, n3 = nodes
    # safe to erase leaf n3 (d := c)
    g.erase_node(n3)
    # n3 removed from iteration
    remaining = list(g.nodes)
    assert n3 not in remaining
    # var d should be gone
    with pytest.raises(RuntimeError):
        g.get_var_producer(d)
    with pytest.raises(RuntimeError):
        g.get_var_consumers(d)


def test_replace_updates_links_and_vars(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (_, b, c, _), _ = simple_graph
    nodes = list(g.nodes)
    _, n1, n2, n3 = nodes
    # replace middle node n2 (c := b) with new node C2: e := b
    e = Var("e")
    c2_stmt = Stmt(args=[b], outs=[e])
    c2_node = g.create_node(c2_stmt)
    g.replace(n2, c2_node)
    # new_node in graph
    assert g.get_var_producer(e).is_(c2_node)
    # old var c removed
    with pytest.raises(RuntimeError):
        g.get_var_producer(c)
    # consumer of b updated: find c2_node and original n3 now consumes e
    assert c2_node in g.get_var_consumers(b)
    # link continuity: n1->c2_node->n3
    assert c2_node.prev.is_(n1) and c2_node.next.is_(n3)  # type: ignore[union-attr]


def test_clear_resets_graph(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    g.clear()
    # only head remains, with no links
    nodes = list(g.nodes)
    assert len(nodes) == 1
    head = nodes[0]
    assert head.prev is None and head.next is None
    # mappings empty
    with pytest.raises(RuntimeError):
        g.get_var_producer(a)
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(stmts[0])


def test_simple_graph_structure(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    nodes = list(g.nodes)
    assert len(nodes) == 4
    head, n1, n2, n3 = nodes
    assert head.stmt is None
    assert head.prev is None
    assert n3.next is None


def test_simple_graph_mappings(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    head, n1, n2, n3 = list(g.nodes)
    assert g.get_var_producer(a).is_(head)
    assert g.get_var_consumers(a) == [n1]
    assert g.get_var_producer(b).is_(n1)
    assert g.get_var_consumers(b) == [n2]
    assert g.get_var_producer(d).is_(n3)


def test_nodes_iteration(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    seq1 = [node for node in g.nodes]
    seq2 = []
    it = g.nodes
    for node in it:
        seq2.append(node)
    assert seq1 == seq2


def test_get_node_from_stmt_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(Stmt(args=[], outs=[]))


def test_create_and_insert_2(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    e = Var("e")
    new_stmt = Stmt(args=[a], outs=[e])
    new_node = g.create_node(new_stmt)
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(new_stmt)
    g.insert_after(g._head, new_node)
    assert g.get_node_from_stmt(new_stmt).is_(new_node)
    assert g.get_var_producer(e).is_(new_node)
    assert new_node in g.get_var_consumers(a)
    g.erase_node(new_node)


def test_insert_after_tail(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    tail = list(g.nodes)[-1]
    x = Var("x")
    x_stmt = Stmt(args=[d], outs=[x])
    x_node = g.create_node(x_stmt)
    g.insert_after(tail, x_node)
    assert tail.next.is_(x_node)  # type: ignore[union-attr]
    assert x_node.prev.is_(tail)  # type: ignore[union-attr]
    assert x_node.next is None
    g.erase_node(x_node)


def test_insert_before_head_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    x = Var("x")
    x_node = g.create_node(Stmt(args=[a], outs=[x]))
    with pytest.raises(RuntimeError):
        g.insert_before(g._head, x_node)


def test_insert_invalid_anchor_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    orphan = g.create_node(Stmt(args=[], outs=[]))
    y = Var("y")
    y_node = g.create_node(Stmt(args=[b], outs=[y]))
    with pytest.raises(RuntimeError):
        g.insert_after(orphan, y_node)


def test_insert_node_already_in_graph_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    e = Var("e")
    stmt_e = Stmt(args=[a], outs=[e])
    node_e = g.create_node(stmt_e)
    g.insert_after(g._head, node_e)
    with pytest.raises(RuntimeError):
        g.insert_after(g._head, node_e)
    g.erase_node(node_e)


def test_insert_duplicate_stmt_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    stmt = stmts[0]
    dup = g.create_node(stmt)
    with pytest.raises(RuntimeError):
        g.insert_after(g._head, dup)


def test_insert_duplicate_var_producer_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    new_stmt = Stmt(args=[a], outs=[b])
    dup = g.create_node(new_stmt)
    with pytest.raises(RuntimeError):
        g.insert_after(g._head, dup)


def test_erase_leaf(simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]]) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    g.erase_node(n3)
    assert n3 not in list(g.nodes)
    with pytest.raises(RuntimeError):
        g.get_var_producer(d)
    with pytest.raises(RuntimeError):
        g.get_var_consumers(d)


def test_erase_head_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.erase_node(g._head)


def test_erase_not_in_graph_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    orphan = g.create_node(Stmt(args=[a], outs=[Var("x")]))
    with pytest.raises(RuntimeError):
        g.erase_node(orphan)


def test_erase_with_consumers_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.erase_node(g.get_node_from_stmt(stmts[0]))


def test_replace_middle(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    e = Var("e")
    new = Stmt(args=[b], outs=[e])
    new_node = g.create_node(new)
    g.replace(n2, new_node)
    assert g.get_var_producer(e).is_(new_node)
    with pytest.raises(RuntimeError):
        g.get_var_producer(c)
    assert new_node.prev.is_(n1)  # type: ignore[union-attr]
    assert new_node.next.is_(n3)  # type: ignore[union-attr]


def test_replace_tail(simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]]) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    f = Var("f")
    new = Stmt(args=[c], outs=[f])
    new_node = g.create_node(new)
    g.replace(n3, new_node)
    assert g.get_var_producer(f).is_(new_node)
    assert new_node.next is None
    assert n2.next.is_(new_node)  # type: ignore[union-attr]


def test_replace_noop(simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]]) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    g.replace(n2, n2)
    assert g.get_node_from_stmt(stmts[1]).is_(n2)


def test_replace_head_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.replace(g._head, g.create_node(Stmt(args=[a], outs=[Var("x")])))


def test_replace_old_not_in_graph_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    orphan = g.create_node(Stmt(args=[a], outs=[Var("x")]))
    new = g.create_node(Stmt(args=[a], outs=[Var("y")]))
    with pytest.raises(RuntimeError):
        g.replace(orphan, new)


def test_replace_new_in_graph_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    new = Stmt(args=[b], outs=[Var("e")])
    n_new = g.create_node(new)
    g.insert_after(n1, n_new)
    with pytest.raises(RuntimeError):
        g.replace(n3, n_new)
    g.erase_node(n_new)


def test_replace_mismatched_output_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    new = Stmt(args=[b], outs=[Var("x"), Var("y")])
    new_node = g.create_node(new)
    with pytest.raises(RuntimeError):
        g.replace(n2, new_node)


def test_get_node_producers_success(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    assert g.get_node_producers(n2) == [n1]


def test_get_node_consumers_success(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    assert g.get_node_consumers(n1) == [n2]


def test_get_node_consumers_error_not_in_graph(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    orphan = g.create_node(Stmt(args=[a], outs=[Var("x")]))
    with pytest.raises(RuntimeError):
        g.get_node_consumers(orphan)


def test_get_var_producer_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.get_var_producer(Var("z"))


def test_get_var_consumers_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    with pytest.raises(RuntimeError):
        g.get_var_consumers(Var("z"))


def test_clear(simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]]) -> None:
    g, (a, b, c, d), stmts = simple_graph
    g.clear()
    nodes = list(g.nodes)
    assert len(nodes) == 1
    head = nodes[0]
    assert head.prev is None and head.next is None
    with pytest.raises(RuntimeError):
        g.get_var_producer(a)
    with pytest.raises(RuntimeError):
        g.get_node_from_stmt(stmts[0])


def test_branch_graph_structure(
    branch_graph: tuple[DepGraph, tuple[Var, Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d, e), stmts = branch_graph
    nodes = list(g.nodes)
    assert len(nodes) == 4


def test_branch_graph_mappings(
    branch_graph: tuple[DepGraph, tuple[Var, Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d, e), stmts = branch_graph
    _, n1, n2, n3 = list(g.nodes)
    assert g.get_var_producer(b).is_(n1)
    assert g.get_var_producer(c).is_(n1)
    assert g.get_var_consumers(b) == [n2]
    assert g.get_var_consumers(c) == [n3]


def test_duplicate_producer_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    dup_stmt = Stmt(args=[b], outs=[c])
    dup_node = g.create_node(dup_stmt)
    with pytest.raises(RuntimeError):
        g.insert_after(g.get_node_from_stmt(stmts[1]), dup_node)


def test_duplicate_stmt_error(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    dup = g.create_node(stmts[1])
    with pytest.raises(RuntimeError):
        g.insert_after(g.get_node_from_stmt(stmts[0]), dup)


def test_identity_equality(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    a2 = Var("a")
    new = Stmt(args=[a2], outs=[Var("x")])
    node = g.create_node(new)
    with pytest.raises(RuntimeError):
        g.insert_after(g._head, node)


def test_empty_graph() -> None:
    g = DepGraph.from_stmts([], [], stmt_inputs, stmt_outputs)
    nodes = list(g.nodes)
    assert len(nodes) == 1
    head = nodes[0]
    assert head.prev is None and head.next is None


def test_graph_only_inputs() -> None:
    a = Var("a")
    b = Var("b")
    g = DepGraph.from_stmts([a, b], [], stmt_inputs, stmt_outputs)
    nodes = list(g.nodes)
    assert len(nodes) == 1
    head = nodes[0]
    assert g.get_var_producer(a).is_(head)
    assert g.get_var_producer(b).is_(head)


def test_no_io_stmts() -> None:
    a = Var("a")
    stmts = [Stmt(args=[], outs=[]), Stmt(args=[], outs=[])]
    g = DepGraph.from_stmts([a], stmts, stmt_inputs, stmt_outputs)
    nodes = list(g.nodes)
    assert len(nodes) == 3


def test_long_chain() -> None:
    vars = [Var(str(i)) for i in range(10)]
    stmts = [Stmt(args=[vars[i]], outs=[vars[i + 1]]) for i in range(9)]
    g = DepGraph.from_stmts([vars[0]], stmts, stmt_inputs, stmt_outputs)
    assert len(list(g.nodes)) == 10


def test_multiple_consumers_for_single_var(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    x1 = Stmt(args=[a], outs=[Var("x")])
    x2 = Stmt(args=[a], outs=[Var("y")])
    n1 = g.create_node(x1)
    n2 = g.create_node(x2)
    g.insert_after(g._head, n1)
    g.insert_after(g._head, n2)
    consumers = g.get_var_consumers(a)
    assert n1 in consumers and n2 in consumers


def test_wrapper_prev_none(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    head = next(iter(g.nodes))
    assert head.prev is None


def test_wrapper_next_none(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    tail = list(g.nodes)[-1]
    assert tail.next is None


def test_wrapper_prev_next_valid(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    head, n1, _, _ = list(g.nodes)
    assert n1.prev.is_(head) and head.next.is_(n1)  # type: ignore[union-attr]


def test_graph_iter_stop_iteration(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    it = g.nodes
    for _ in range(len(list(g.nodes))):
        next(it)
    with pytest.raises(StopIteration):
        next(it)


def test_clear_idempotent(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    g.clear()
    g.clear()
    assert len(list(g.nodes)) == 1


def test_replace_sequence(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    u = Var("u")
    v = Var("v")
    g, (a, b, c, d), stmts = simple_graph
    _, n1, n2, n3 = list(g.nodes)
    n2b = g.create_node(Stmt(args=[b], outs=[u]))
    n3b = g.create_node(Stmt(args=[u], outs=[v]))
    g.replace(n2, n2b)
    g.replace(n3, n3b)
    assert g.get_var_producer(u).is_(n2b)
    assert g.get_var_producer(v).is_(n3b)


def test_multiple_inserts_order(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    n4 = g.create_node(Stmt(args=[a], outs=[Var("p")]))
    n5 = g.create_node(Stmt(args=[a], outs=[Var("q")]))
    g.insert_after(g._head, n4)
    g.insert_after(g._head, n5)
    seq = list(g.nodes)
    assert seq[1].is_(n5) and seq[2].is_(n4)


def test_multiple_inserts_before_tail(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    tail = list(g.nodes)[-1]
    n1 = g.create_node(Stmt(args=[c], outs=[Var("m")]))
    n2 = g.create_node(Stmt(args=[c], outs=[Var("n")]))
    g.insert_before(tail, n2)
    g.insert_before(tail, n1)
    seq = list(g.nodes)
    idx = seq.index(tail)
    assert seq[idx - 1].is_(n1) and seq[idx - 2].is_(n2)


def test_stmt_to_node_mapping_direct(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    n1 = g.get_node_from_stmt(stmts[0])
    assert g._stmt_to_node[stmts[0]].is_(n1)


def test_var_to_producer_direct(
    simple_graph: tuple[DepGraph, tuple[Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d), stmts = simple_graph
    head = g._head
    assert g._var_to_producer[a].is_(head)


def test_branch_graph_nodes_iteration(
    branch_graph: tuple[DepGraph, tuple[Var, Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d, e), stmts = branch_graph
    seq = list(g.nodes)
    assert len(seq) == 4


def test_graph_clear_branch_graph(
    branch_graph: tuple[DepGraph, tuple[Var, Var, Var, Var, Var], list[Stmt]],
) -> None:
    g, (a, b, c, d, e), stmts = branch_graph
    g.clear()
    assert len(list(g.nodes)) == 1
