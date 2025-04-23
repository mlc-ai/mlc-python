#ifndef MLC_CORE_DEP_GRAPH_H_
#define MLC_CORE_DEP_GRAPH_H_

#include <mlc/core/all.h>

namespace mlc {
namespace core {

/*!
 * \brief A dependency node in the dependency graph, which contains
 * information about the statement, its input and output vars, and
 * pointers to the previous and next nodes in the linked list.
 * All the nodes are linked together in a doubly linked list.
 */
struct DepNodeObj {
  MLCAny _mlc_header;
  /*! \brief The statement that this node represents */
  Any stmt;
  /*! \brief The list of input variables for this node */
  UList input_vars;
  /*! \brief The list of output variables for this node */
  UList output_vars;
  /*! \brief The previous node in the linked list */
  DepNodeObj *prev;
  /*! \brief The next node in the linked list */
  DepNodeObj *next;

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, DepNodeObj, Object, "mlc.core.DepNode");

  explicit DepNodeObj(Any stmt, UList input_vars, UList output_vars, DepNodeObj *prev, DepNodeObj *next)
      : stmt(stmt), input_vars(input_vars), output_vars(output_vars), prev(prev), next(next) {}

  void Clear() {
    this->stmt = Null;
    this->input_vars.clear();
    this->output_vars.clear();
    this->prev = nullptr;
    this->next = nullptr;
  }
};

struct DepNode : public ObjectRef {
  explicit DepNode(Any stmt, UList input_vars, UList output_vars)
      : DepNode(DepNode::New(stmt, input_vars, output_vars, nullptr, nullptr)) {}

  MLC_DEF_OBJ_REF(MLC_EXPORTS, DepNode, DepNodeObj, ObjectRef)
      .Field("stmt", &DepNodeObj::stmt, /*frozen=*/true)
      .Field("input_vars", &DepNodeObj::input_vars, /*frozen=*/true)
      .Field("output_vars", &DepNodeObj::output_vars, /*frozen=*/true)
      ._Field("_prev", offsetof(DepNodeObj, prev), sizeof(DepNodeObj *), true, ::mlc::core::ParseType<ObjectRef>())
      ._Field("_next", offsetof(DepNodeObj, next), sizeof(DepNodeObj *), true, ::mlc::core::ParseType<ObjectRef>())
      .StaticFn("__init__", InitOf<DepNodeObj, Any, UList, UList, DepNodeObj *, DepNodeObj *>);
};

struct DepGraphObj {
  MLCAny _mlc_header;
  /*! \brief A function that maps a stmt to a list of variables it consumes */
  Func stmt_to_inputs;
  /*! \brief A function that maps a stmt to a list of variables it produces */
  Func stmt_to_outputs;
  /*! \brief A map from a stmt to its node in the linked list */
  UDict stmt_to_node;
  /*! \brief Map from a variable to its producer nodes */
  UDict var_to_producer;
  /*! \brief Map from a variable to a list of consumer nodes */
  UDict var_to_consumers;
  /*! \brief The first node in the linked list */
  DepNode head;

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, DepGraphObj, Object, "mlc.core.DepGraph");

  explicit DepGraphObj(Func stmt_to_inputs, Func stmt_to_outputs, UDict stmt_to_node, UDict var_to_producer,
                       UDict var_to_consumers, DepNode head)
      : stmt_to_inputs(stmt_to_inputs), stmt_to_outputs(stmt_to_outputs), stmt_to_node(stmt_to_node),
        var_to_producer(var_to_producer), var_to_consumers(var_to_consumers), head(head) {}

  explicit DepGraphObj(UList input_vars, UList stmts, Func stmt_to_inputs, Func stmt_to_outputs)
      : stmt_to_inputs(stmt_to_inputs), stmt_to_outputs(stmt_to_outputs), stmt_to_node(), var_to_producer(),
        var_to_consumers(), head(DepNode(Any(), UList{}, input_vars)) {
    this->stmt_to_node[Any()] = this->head;
    for (const Any &var : input_vars) {
      this->var_to_producer[var] = this->head;
      this->var_to_consumers[var] = UList{};
    }
    DepNodeObj *prev = this->head.get();
    for (const Any &stmt : stmts) {
      DepNode node = this->CreateNode(stmt);
      this->InsertAfter(prev, node.get());
      prev = node.get();
    }
  }

  ~DepGraphObj() { this->Clear(); }

  /*!
   * \brief Clear the dependency graph.
   * \note This will unlink all nodes from the graph and clear the maps.
   */
  void Clear() {
    for (DepNodeObj *node = this->head.get(); node;) {
      DepNodeObj *next = node->next;
      node->Clear();
      node = next;
    }
    this->var_to_producer.clear();
    this->var_to_consumers.clear();
    this->stmt_to_node.clear();
  }
  /*!
   * \brief Create a new node which is not linked to the dependency graph.
   * \param stmt The statement that this node represents
   * \return The new node
   * \note This node is not part of the dependency graph until it's explicitly
   *       inserted using InsertBefore or InsertAfter.
   */
  DepNode CreateNode(Any stmt) { return DepNode(stmt, this->stmt_to_inputs(stmt), this->stmt_to_outputs(stmt)); }
  /*!
   * \brief Get a node containing the given statement
   * \param stmt The statement to get the node for
   * \return The node containing the statement
   * \note This will throw an error if the statement is not inserted into the graph.
   */
  DepNode GetNodeFromStmt(Any stmt) {
    if (auto it = this->stmt_to_node.find(stmt); it != this->stmt_to_node.end()) {
      return it->second;
    }
    MLC_THROW(RuntimeError) << "Stmt not in graph: " << stmt;
    MLC_UNREACHABLE();
  }
  /*!
   * \brief Insert a node before or after an anchor node.
   * \param anchor The anchor node, not nullptr
   * \param to_insert The node to insert
   * \note This will link the new node to the dependency graph.
   */
  void InsertBefore(DepNodeObj *anchor, DepNodeObj *to_insert) {
    if (anchor->prev == nullptr) {
      MLC_THROW(RuntimeError) << "Can't input before the input node: " << anchor->stmt;
    }
    if (!stmt_to_node.count(anchor->stmt)) {
      MLC_THROW(RuntimeError) << "Anchor node not in graph: " << anchor->stmt;
    }
    DepNodeObj *prev = anchor->prev;
    DepNodeObj *next = anchor;
    return _Insert(prev, next, to_insert);
  }
  /*!
   * \brief Insert a node before or after an anchor node.
   * \param anchor The anchor node, not nullptr
   * \param to_insert The node to insert
   * \note This will link the new node to the dependency graph.
   */
  void InsertAfter(DepNodeObj *anchor, DepNodeObj *to_insert) {
    if (!stmt_to_node.count(anchor->stmt)) {
      MLC_THROW(RuntimeError) << "Anchor node not in graph: " << anchor->stmt;
    }
    DepNodeObj *prev = anchor;
    DepNodeObj *next = anchor->next;
    return _Insert(prev, next, to_insert);
  }
  /*!
   * \brief Erase a node from the dependency graph.
   * \param to_erase The node to erase, not nullptr
   * \note This will unlink the node from the dependency graph, remove the variables
   *       it produces, and remove itself from the consumer lists of the variables it consumes.
   */
  void EraseNode(DepNodeObj *to_erase) {
    // Step 1. Unlink the node from the graph
    if (to_erase->prev == nullptr) {
      MLC_THROW(RuntimeError) << "Can't erase the input node: " << to_erase->stmt;
    }
    if (!this->stmt_to_node.count(to_erase->stmt)) {
      MLC_THROW(RuntimeError) << "Node not in graph: " << to_erase->stmt;
    }
    this->stmt_to_node.erase(to_erase->stmt);
    if (to_erase->prev != nullptr) {
      to_erase->prev->next = to_erase->next;
    } else {
      this->head = to_erase->next;
    }
    if (to_erase->next != nullptr) {
      to_erase->next->prev = to_erase->prev;
    }
    // Step 2. For each variable produced by the node
    // 1) check all its consumers are gone
    // 2) remove the producer
    for (const Any &var : to_erase->output_vars) {
      UListObj *consumers = this->var_to_consumers.at(var);
      if (!consumers->empty()) {
        MLC_THROW(RuntimeError) << "Removing a node which produces a variable that still has consumers in graph: "
                                << var;
      }
      this->var_to_producer.erase(var);
      this->var_to_consumers.erase(var);
    }
    // Step 3. For each varibale consumed by the node
    // 1) check if the var is in the graph
    // 2) remove the node from its consumer list
    for (const Any &var : to_erase->input_vars) {
      if (!this->var_to_producer.count(var)) {
        MLC_THROW(RuntimeError) << "Variable is not produced by any node in the graph: " << var;
      }
      UListObj *consumers = this->var_to_consumers.at(var);
      auto it = std::find_if(consumers->begin(), consumers->end(),
                             [to_erase](const Any &v) -> bool { return v.operator DepNodeObj *() == to_erase; });
      if (it == consumers->end()) {
        MLC_THROW(RuntimeError) << "Node is not a consumer of the variable: " << var;
      }
      consumers->erase(it);
    }
    // Step 4. Clear the node
    to_erase->Clear();
  }
  /*!
   * \brief Replace a node in the dependency graph with another node.
   * \param old_node The node to replace
   * \param new_node The new node to insert
   * \note This will unlink the old node from the dependency graph and link the new node.
   */
  void Replace(DepNodeObj *old_node, DepNodeObj *new_node) {
    if (old_node == new_node) {
      return;
    }
    if (old_node->prev == nullptr) {
      MLC_THROW(RuntimeError) << "Can't replace the input node: " << old_node->stmt;
    }
    if (!this->stmt_to_node.count(old_node->stmt)) {
      MLC_THROW(RuntimeError) << "Node not in graph: " << old_node->stmt;
    }
    if (new_node->prev != nullptr || new_node->next != nullptr) {
      MLC_THROW(RuntimeError) << "Node is already in the graph: " << new_node->stmt;
    }
    int64_t num_output_vars = old_node->output_vars.size();
    if (num_output_vars != new_node->output_vars.size()) {
      MLC_THROW(RuntimeError) << "Mismatched number of output_vars: " << num_output_vars << " vs "
                              << new_node->output_vars.size();
    }
    // Step 1. Replace each variable produced by the old node
    for (int64_t i = 0; i < num_output_vars; ++i) {
      Any old_var = old_node->output_vars[i];
      Any new_var = new_node->output_vars[i];
      Ref<UListObj> old_var_consumers = var_to_consumers.at(old_var);
      // Search through its consumers
      for (DepNodeObj *consumer : *old_var_consumers) {
        // Replace the input vars of each consumer
        for (Any &v : consumer->input_vars) {
          if (v.operator Object *() == old_var.operator Object *()) {
            v = new_var;
          }
        }
      }
      this->var_to_producer.erase(old_var);
      this->var_to_consumers.erase(old_var);
      this->var_to_producer[new_var] = new_node;
      this->var_to_consumers[new_var] = old_var_consumers;
    }
    // Step 2. Delete each variable consumed by the old node
    for (const Any &var : old_node->input_vars) {
      UListObj *consumers = this->var_to_consumers.at(var);
      if (auto it = std::find_if(consumers->begin(), consumers->end(),
                                 [old_node](const Any &v) -> bool { return v.operator DepNodeObj *() == old_node; });
          it != consumers->end()) {
        consumers->erase(it);
      } else {
        MLC_THROW(RuntimeError) << "Node is not a consumer of the variable: " << var;
      }
    }
    // Step 3. Add variables consumed by the new node
    for (const Any &var : new_node->input_vars) {
      if (!this->var_to_producer.count(var)) {
        MLC_THROW(RuntimeError) << "Variable is not produced by any node in the graph: " << var;
      }
      this->var_to_consumers.at(var).operator UListObj *()->push_back(new_node);
    }
    // Step 4. Link the new node into the graph
    new_node->prev = old_node->prev;
    new_node->next = old_node->next;
    if (old_node->prev != nullptr) {
      old_node->prev->next = new_node;
    } else {
      this->head = new_node;
    }
    if (old_node->next != nullptr) {
      old_node->next->prev = new_node;
    }
    this->stmt_to_node.erase(old_node->stmt);
    if (this->stmt_to_node.count(new_node->stmt)) {
      MLC_THROW(RuntimeError) << "Stmt already in the graph: " << new_node->stmt;
    } else {
      this->stmt_to_node[new_node->stmt] = new_node;
    }
    // Step 5. Clear the old node
    old_node->Clear();
  }
  /*!
   * \brief For a given node, returns its producers, i.e. a list of nodes that produce the input variables of the node.
   * \param node The node to get the input statements for
   * \return The list of input nodes for the node
   */
  UList GetNodeProducers(DepNodeObj *node) {
    UList ret;
    for (const Any &var : node->input_vars) {
      if (auto it = this->var_to_producer.find(var); it != this->var_to_producer.end()) {
        ret.push_back(it->second);
      } else {
        MLC_THROW(RuntimeError) << "Variable is not produced by any node in the graph: " << var;
      }
    }
    return ret;
  }
  /*!
   * \brief For a given node, returns its consumers, i.e. a list of nodes that consume the output variables of the node.
   * \param node The node to get the output statements for
   * \return The list of output statements for the node
   */
  UList GetNodeConsumers(DepNodeObj *node) {
    UList ret;
    for (const Any &var : node->output_vars) {
      if (auto it = this->var_to_consumers.find(var); it != this->var_to_consumers.end()) {
        UListObj *consumers = it->second;
        ret.insert(ret.end(), consumers->begin(), consumers->end());
      } else {
        MLC_THROW(RuntimeError) << "Variable is not consumed by any node in the graph: " << var;
      }
    }
    return ret;
  }
  /*!
   * \brief Find the producer of a variable in the dependency graph.
   * \param var The variable to find the producer for
   * \return The producer node for the variable
   */
  DepNode GetVarProducer(Any var) {
    if (auto it = this->var_to_producer.find(var); it != this->var_to_producer.end()) {
      return it->second;
    }
    MLC_THROW(RuntimeError) << "Variable is not produced by any node in the graph: " << var;
    MLC_UNREACHABLE();
  }
  /*!
   * \brief Find the consumers of a variable in the dependency graph.
   * \param var The variable to find the consumers for
   * \return The list of consumer nodes for the variable
   */
  UList GetVarConsumers(Any var) {
    if (auto it = this->var_to_consumers.find(var); it != this->var_to_consumers.end()) {
      return it->second;
    }
    MLC_THROW(RuntimeError) << "Variable is not consumed by any node in the graph: " << var;
    MLC_UNREACHABLE();
  }

  void _Insert(DepNodeObj *prev, DepNodeObj *next, DepNodeObj *to_insert) {
    if (to_insert->prev != nullptr || to_insert->next != nullptr) {
      MLC_THROW(RuntimeError) << "Node is already in the graph: " << to_insert->stmt;
    }
    // Step 1. Link the node into the graph
    if (this->stmt_to_node.count(to_insert->stmt)) {
      MLC_THROW(RuntimeError) << "Stmt already in the graph: " << to_insert->stmt;
    }
    this->stmt_to_node[to_insert->stmt] = to_insert;
    to_insert->prev = prev;
    to_insert->next = next;
    if (prev != nullptr) {
      prev->next = to_insert;
    } else {
      this->head = to_insert;
    }
    if (next != nullptr) {
      next->prev = to_insert;
    }
    // Step 2. For each variable produced by the node
    // 1) check if it doesn't have a producer yet
    // 2) record its producer as this node
    for (const Any &var : to_insert->output_vars) {
      if (auto it = this->var_to_producer.find(var); it != this->var_to_producer.end()) {
        MLC_THROW(RuntimeError) << "Variable already has a producer by another node: "
                                << it->second.operator DepNode()->stmt;
      } else {
        this->var_to_producer[var] = to_insert;
        this->var_to_consumers[var] = UList{};
      }
    }
    // Step 3. For each variable consumed by the node
    // 1) check if the var is in the graph
    // 1) add a new consumer of this var
    for (const Any &var : to_insert->input_vars) {
      if (!this->var_to_producer.count(var)) {
        MLC_THROW(RuntimeError) << "Variable is not produced by any node in the graph: " << var;
      }
      this->var_to_consumers.at(var).operator UListObj *()->push_back(to_insert);
    }
  }
};

struct DepGraph : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, DepGraph, DepGraphObj, ObjectRef)
      .Field("_stmt_to_inputs", &DepGraphObj::stmt_to_inputs, /*frozen=*/true)
      .Field("_stmt_to_outputs", &DepGraphObj::stmt_to_outputs, /*frozen=*/true)
      .Field("_stmt_to_node", &DepGraphObj::stmt_to_node, /*frozen=*/true)
      .Field("_var_to_producer", &DepGraphObj::var_to_producer, /*frozen=*/true)
      .Field("_var_to_consumers", &DepGraphObj::var_to_consumers, /*frozen=*/true)
      .Field("_head", &DepGraphObj::head, /*frozen=*/true)
      .StaticFn("__init__", InitOf<DepGraphObj, Func, Func, UDict, UDict, UDict, DepNode>)
      .StaticFn("_init_from_stmts", InitOf<DepGraphObj, UList, UList, Func, Func>)
      .MemFn("clear", &DepGraphObj::Clear)
      .MemFn("create_node", &DepGraphObj::CreateNode)
      .MemFn("get_node_from_stmt", &DepGraphObj::GetNodeFromStmt)
      .MemFn("insert_before", &DepGraphObj::InsertBefore)
      .MemFn("insert_after", &DepGraphObj::InsertAfter)
      .MemFn("erase_node", &DepGraphObj::EraseNode)
      .MemFn("replace", &DepGraphObj::Replace)
      .MemFn("get_node_producers", &DepGraphObj::GetNodeProducers)
      .MemFn("get_node_consumers", &DepGraphObj::GetNodeConsumers)
      .MemFn("get_var_producer", &DepGraphObj::GetVarProducer)
      .MemFn("get_var_consumers", &DepGraphObj::GetVarConsumers);

  explicit DepGraph(Func stmt_to_inputs, Func stmt_to_outputs, UDict stmt_to_node, UDict var_to_producer,
                    UDict var_to_consumers, DepNode head)
      : DepGraph(
            DepGraph::New(stmt_to_inputs, stmt_to_outputs, stmt_to_node, var_to_producer, var_to_consumers, head)) {}

  explicit DepGraph(UList input_vars, UList stmts, Func stmt_to_inputs, Func stmt_to_outputs)
      : DepGraph(DepGraph::New(input_vars, stmts, stmt_to_inputs, stmt_to_outputs)) {}
};

} // namespace core
} // namespace mlc

#endif // MLC_CORE_DEP_GRAPH_H_
