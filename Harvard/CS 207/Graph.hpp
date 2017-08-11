#ifndef CS207_GRAPH_HPP
#define CS207_GRAPH_HPP

/** @file Graph.hpp
 * @brief An undirected graph type
 */

#include <algorithm>
#include <vector>
#include <cassert>

#include "CS207/Util.hpp"
#include "CS207/Point.hpp"

#include <utility>

#include <boost/iterator/transform_iterator.hpp>
/** @class Graph
 * @brief A template for 3D undirected graphs.
 *
 * Users can add and retrieve nodes and edges. Edges are unique (there is at
 * most one edge between any pair of distinct nodes).
 */
template <typename V, typename E = bool>
class Graph {
 private:

  // Internal type for node
  struct internal_node;
  // Internal type for edge
  struct internal_edge;
 public:

  /////////////////////////////
  // PUBLIC TYPE DEFINITIONS //
  /////////////////////////////
  typedef V node_value_type;
  typedef E edge_value_type;

  /** Type of this graph. */
  typedef Graph graph_type;

  /** Predeclaration of Node type. */
  class Node;
  /** Synonym for Node (following STL conventions). */
  typedef Node node_type;

  /** Predeclaration of Edge type. */
  class Edge;
  /** Synonym for Edge (following STL conventions). */
  typedef Edge edge_type;

  /** Type of indexes and sizes.
      Return type of Graph::Node::index(), Graph::num_nodes(),
      Graph::num_edges(), and argument type of Graph::node(size_type) */
  typedef unsigned size_type;

  struct UidToNodeTransform;
  /** Type of node iterators, which iterate over all graph nodes. */
  class NodeIterator;
  /** Type of node range */
  struct NodeRange;
  /** Synonym for NodeIterator */
  typedef NodeIterator node_iterator;

  /** Type of edge iterators, which iterate over all graph edges. */
  class EdgeIterator;

  /** Type of edge range */
  struct EdgeRange;
  /** Synonym for EdgeIterator */
  typedef EdgeIterator edge_iterator;

  /** Type of incident iterators, which iterate incident edges to a node. */
  class IncidentIterator;

  /** Type of incident edges of a node range */
  struct IncidentRange;
  /** Synonym for IncidentIterator */
  typedef IncidentIterator incident_iterator;

  ////////////////////////////////
  // CONSTRUCTOR AND DESTRUCTOR //
  ////////////////////////////////

  /** Construct an empty graph. */
  Graph() : free_uid_(-1), edge_size_(0){
  }
  /** Default destructor */
  ~Graph() = default;

  /////////////
  // General //
  /////////////

  /** Return the number of nodes in the graph.
   *
   * Complexity: O(1).
   */
  size_type size() const {
    // HW0: YOUR CODE HERE
    return i2u_.size();
  }

  /** Remove all nodes and edges from this graph.
   * @post num_nodes() == 0 && num_edges() == 0
   *
   * Invalidates all outstanding Node and Edge objects.
   */
  void clear() {
    nodes_.clear();
    i2u_.clear();
    adjacent_.clear();
    edge_size_ = 0;
    free_uid_ = -1;
    edges_.clear();
  }

  /////////////////
  // GRAPH NODES //
  /////////////////

  /** @class Graph::Node
   * @brief Class representing the graph's nodes.
   *
   * Node objects are used to access information about the Graph's nodes.
   */
  class Node : private totally_ordered<Node> {
   public:
    /** Construct an invalid node.
     *
     * Valid nodes are obtained from the Graph class, but it
     * is occasionally useful to declare an @i invalid node, and assign a
     * valid node to it later.
     */
    Node() : graph_(nullptr), uid_(-1){
    }

    /** Return this node's position. */
    const Point& position() const {
      assert(valid());
      return fetch().position;
    }

    /* Inspect a node's position */
    Point& position() {
      assert(valid());
      return fetch().position;
    }

    /** Return this node's index, a number in the range [0, graph_size). */
    size_type index() const {
      assert(valid());
      return fetch().index;
    }

    /** Test whether this node and @a x are equal.
     *
     * Equal nodes have the same graph and the same index.
     */
    bool operator==(const Node& x) const {
      return (graph_ == x.graph_) && (uid_ == x.uid_);
    }

    /** Test whether this node is less than @a x in the global order.
     *
     * The node ordering relation must obey trichotomy: For any two nodes x
     * and y, exactly one of x == y, x < y, and y < x is true.
     */
    bool operator<(const Node& x) const {
      if(graph_ != x.graph_)
        return graph_ < x.graph_;
      else
        return uid_ < x.uid_;
    }

    /*
    * Modify @a value in this node
    */
    node_value_type& value() {
      assert(valid());
      return fetch().value;
    }

    /*
    * Inspect @a value in this node, promise not to change value
    */
    const node_value_type& value() const {
      assert(valid());
      return fetch().value;
    }

    /*
    * Return the number of edges connected to this node
    */
    size_type degree() const {
      assert(valid());
      return graph_->adjacent_[uid_].size();
    }

    /*
    * An iterator points to the first edge that is incident to this node
    */
    incident_iterator edge_begin() const {
      assert(valid());
      return incident_iterator(graph_, uid_, 0);
    }

    /*
    * An iterator points to the end of incident edges container
    */
    incident_iterator edge_end() const {
      assert(valid());
      return incident_iterator(graph_, uid_, degree());
    }

    IncidentRange incidentEdges() const {
      assert(valid());
      return {*this};
    }

   private:
    // Allow Graph to access Node's private member data and functions.
    friend class Graph;
    /* Graph pointer*/
    Graph* graph_;
    /* node Uid of this instance*/
    size_type uid_;

    Node(const Graph* graph, size_type uid)
      : graph_(const_cast<Graph*>(graph)), uid_(uid) {

    }

    /* Helper method to return the appropriate node.
    *  Due to the fact that nodes are stored in vector, which
    *  supports random access. The complexity is O(1)
    */
    internal_node& fetch() const {
      return graph_->nodes_[uid_];
    }

    /* Helper method to check the RI of Node.
    * Time complexity: O(1) */
    bool valid() const {
      return uid_ >= 0 && uid_ < graph_->nodes_.size()
        && graph_->nodes_[uid_].index < int(graph_->i2u_.size())
        && graph_->i2u_[graph_->nodes_[uid_].index] == uid_
        && graph_->nodes_[uid_].uid == uid_;
    }
  };

  /** Synonym for size(). */
  size_type num_nodes() const {
    return size();
  }

  /** Add a node to the graph, returning the added node.
   * @param[in] position The new node's position
   * @post new size() == old size() + 1
   * @post new adjacent_.size() = old adjacent_.size() + 1
   * @post result_node.index() == old size()
   * @post If old @a free_uid_ != -1, new_node.uid == old @a free_uid_,
   *        else, new_node.uid == @a size() - 1
   *
   * Complexity: O(1) amortized operations.
   */
  Node add_node(const Point& position, const node_value_type& val = node_value_type()) {
    internal_node new_node = internal_node(position, val);
    // new node's index should be @ i2u_.size()
    new_node.index = i2u_.size();

    if (free_uid_ == -1) {
      // new node's Uid should be @ nodes_.size()
      new_node.uid = nodes_.size();
      nodes_.push_back(new_node);
      adjacent_.push_back(std::vector<std::pair<size_type, size_type>>());
    }
    else {
      // update @a free_uid_ to the next
      auto reused_uid = free_uid_;
      free_uid_ = nodes_[free_uid_].index;
      // update related information for reused_uid, no need to push new element
      // into @a nodes_ or @a adjacent_
      new_node.uid = reused_uid;
      nodes_[reused_uid] = new_node;
      adjacent_[reused_uid].clear();
    }

    i2u_.push_back(new_node.uid);
    // nodes_.size() - 1 is the new_node's uid
    return Node(this, new_node.uid);
  }

  /** Determine if this Node belongs to this Graph
   * @return True if @a n is currently a Node of this Graph
   *
   * Complexity: O(1).
   */
  bool has_node(const Node& n) const {
    assert(n.index() < size() && n.uid_ < nodes_.size());
    return ((n.graph_ == this) && (n.uid_ == i2u_[n.index()]));
  }

  /** Return the node with index @a i.
   * @pre 0 <= @a i < num_nodes()
   * @post result_node.index() == i
   *
   * Complexity: O(1).
   */
  Node node(size_type i) const {
    assert(i < num_nodes() && nodes_[i2u_[i]].index == int(i)
      && i2u_[i] == nodes_[i2u_[i]].uid);

    return Node(this, i2u_[i]);
  }

   /** Remove a node from the graph.
   * @param[in] n Node to be removed
   * @return 1 if old has_node(n), 0 otherwise
   *
   * @post new size() == old size() - result.
   * @post @a free_uid_ != -1
   * @post nodes_[@a free_uid_].index == old @free_uid_
   *
   * Can invalidate outstanding iterators.
   * If old has_node(@a n), then @a n becomes invalid, as do any
   * other Node objects equal to @a n. All other Node objects remain valid.
   *
   * Complexity: O(degree^2).
   */
  size_type remove_node(const Node& n) {
    if(!has_node(n))
      return 0;
    /* Re-designed to use managed remove_edge() function, two advantages:
    * 1. Avoid managing @a edge_size_ manually
    * 2. Avoid calling adjacent_[n.uid_].clear() manually
    * This makes the program consistent in logic.
    */
    auto incidentInfo = adjacent_[n.uid_];
    for (auto icdt : incidentInfo) {
      remove_edge(node(nodes_[icdt.first].index), n);
    }
    // record the information of del_node and last_node, which will be swapped
    size_type del_index = n.index();
    size_type del_uid = i2u_[del_index];
    size_type last_index = i2u_.size() - 1;
    size_type last_uid = i2u_[last_index];
    // let the old_index record the node_uid of the last_index,
    // then change node with node_uid's index to old_index and pop the last element in @a i2u_
    i2u_[del_index] = last_uid;
    nodes_[last_uid].index = del_index;
    i2u_.pop_back();
    // use the del_node's index to record @a free_uid_
    nodes_[del_uid].index = free_uid_;
    free_uid_ = del_uid;
    return 1;
  }

  /** Remove a node from the graph.
   * @param[in] n_it Iterator references to a node that is to be deleted.
   * @return If n_it points to a valid (*n_it), returns the iterator that points
   *  to the node next to old (*n_it); otherwise return node_end()
   *
   * @pre n_it should point to a valid node in graph or node_end()
   * @post new size() == old size() - result.
   *
   * Can invalidate outstanding iterators.
   * If old has_node(@a n), then @a n becomes invalid, as do any
   * other Node objects equal to @a n. All other Node objects remain valid.
   *
   * Complexity: O(degree^2).
   */
  node_iterator remove_node(node_iterator n_it) {

    if (n_it != node_end())
      remove_node(*n_it);

    return n_it;
  }

  /* Range object for iterating all the nodes in the graph */
  NodeRange nodes() const {
    return {*this};
  }

  /////////////////
  // GRAPH EDGES //
  /////////////////

  /** @class Graph::Edge
   * @brief Class representing the graph's edges.
   *
   * Edges are order-insensitive pairs of nodes. Two Edges with the same nodes
   * are considered equal if they connect the same nodes, in either order.
   */
  class Edge : private totally_ordered<Edge> {
   public:
    /** Construct an invalid Edge. */
    Edge()  : graph_{nullptr} {
    }

    /** Return a node of this Edge */
    Node node1() const {
      return graph_->node(graph_->nodes_[uid1_].index);
    }

    /** Return the other node of this Edge */
    Node node2() const {
      return graph_->node(graph_->nodes_[uid2_].index);
    }

    /** Physical length of the Edge */
    double length() const {
      return norm(node1().position() - node2().position());
    }

    /** Modify @a value in this edge */
    edge_value_type & value() {
      return fetch().value;
    }

    /** Inspect @a value in this edge */
    const edge_value_type & value() const {
      return fetch().value;
    }

    size_type index() const {
      return edge_uid_;
    }

    /** Test whether this edge and @a x are equal.
     *
     * Equal edges are from the same graph and have the same nodes.
     * @pre If two edges are equal, their @a edge_uid_ should be the same.
     */
    bool operator==(const Edge& x) const {
      if (graph_ != x.graph_)
        return false;
      else {
        if (((uid1_ == x.uid1_) && (uid2_ == x.uid2_)) ||
          ((uid1_ == x.uid2_) && (uid2_ == x.uid1_))) {
          assert(edge_uid_ == x.edge_uid_);
          return true;
        }
      }
      return false;
    }

    /** Test whether this edge is less than @a x in the global order.
     *
     * The edge ordering relation must obey trichotomy: For any two edges x
     * and y, exactly one of x == y, x < y, and y < x is true.
     */
    bool operator<(const Edge& x) const {
      size_type selfSmallerUid = (std::min(node1(), node2())).uid_;
      size_type selfGreaterUid = (std::max(node1(), node2())).uid_;
      size_type xSmallerUid = (std::min(x.node1(), x.node2())).uid_;
      size_type xGreaterUid = (std::max(x.node1(), x.node2())).uid_;

      if (graph_ != x.graph_)
        return graph_ < x.graph_;
      else if (selfSmallerUid != xSmallerUid)
        return selfSmallerUid < xSmallerUid;
      else
        return selfGreaterUid < xGreaterUid;
    }

   private:
    // Allow Graph to access Edge's private member data and functions.
    friend class Graph;
    // Graph pointer
    Graph* graph_;
    // Uid of the first node this edge connects to
    size_type uid1_;
    // Uid of the second node this edge connects to
    size_type uid2_;
    // Uid of the edge data this Edge owns, equal to the index of the data in @a edges_
    size_type edge_uid_;

    Edge(const Graph* graph, size_type uid1, size_type uid2, size_type e_uid)
      : graph_(const_cast<Graph*>(graph)), uid1_(uid1), uid2_(uid2), edge_uid_(e_uid) {
    }

    /* Helper method to return the appropriate edge data.
    *  Due to the fact that nodes are stored in vector @a edges_, which
    *  supports random access. The complexity is O(1)
    */
    internal_edge& fetch() const {
      return graph_->edges_[edge_uid_];
    }
  };

  /** Return the total number of edges in the graph.
   *
   * Complexity: No more than O(num_nodes() + num_edges()), hopefully less
   */
  size_type num_edges() const {
    return edge_size_;
  }

  /** Add an edge to the graph, or return the current edge if it already exists.
   * @pre @a a and @a b are distinct valid nodes of this graph
   * @return an Edge object e with e.node1() == @a a and e.node2() == @a b
   * @post has_edge(@a a, @a b) == true
   * @post If old has_edge(@a a, @a b), new num_edges() == old num_edges().
   *       Else,                        new num_edges() == old num_edges() + 1.
   * @Return An edge instance, where edge.uid1 < edge.uid2 && edge == Edge(this, a, b)
   *
   * Can invalidate edge indexes -- in other words, old edge(@a i) might not
   * equal new edge(@a i). Must not invalidate outstanding Edge objects.
   *
   * Searches in the @a adjacent_ in position @a a.index(), the complexity depends on
   * the degree of @a a.
   * Complexity: O(degree(a))
   */
  Edge add_edge(const Node& a, const Node& b, const edge_value_type& val = edge_value_type()) {
    assert(a.index() != b.index());

    size_type e_uid = -1;

    if (!has_edge(a, b)) {
      internal_edge new_edge = internal_edge(val); //internal_edge(edge_value_type());
      // edges_size() is equavalent to new_edge's uid
      adjacent_[a.uid_].push_back(std::make_pair(b.uid_, edges_.size()));
      adjacent_[b.uid_].push_back(std::make_pair(a.uid_, edges_.size()));
      e_uid = edges_.size();

      edges_.push_back(new_edge);
      ++edge_size_;
    }
    else {
      for (auto edgeInfo : adjacent_[a.uid_]) {
        if (edgeInfo.first == b.uid_) {
          e_uid = edgeInfo.second;
          break;
        }
      }
    }

    size_type smallerUid = (std::min(a, b)).uid_;
    size_type greaterUid = (std::max(a, b)).uid_;
    return Edge(this, smallerUid, greaterUid, e_uid);
  }

  Edge edge(const Node& a, const Node& b) const {
    assert(a.index() < size());
    assert(b.index() < size());
    assert(has_edge(a, b));

    size_type smallerUid = (std::min(a, b)).uid_;
    size_type greaterUid = (std::max(a, b)).uid_;
    size_type e_uid = -1;
    for (auto edgeInfo : adjacent_[smallerUid]) {
      if (edgeInfo.first == greaterUid) {
        e_uid = edgeInfo.second;
        break;
      }
    }
    return Edge(this, smallerUid, greaterUid, e_uid);
  }

  /** Test whether two nodes are connected by an edge.
   * @pre @a a and @a b are valid nodes of this graph
   * @pre @a a and @a b are distinct nodes
   * @return true if, for some @a i, edge(@a i) connects @a a and @a b.
   *
   * Complexity: O(degree(a))
   */
  bool has_edge(const Node& a, const Node& b) const {
    assert(a.index() < size());
    assert(b.index() < size());

    if (a.index() == b.index() || !(has_node(a) && has_node(b)))
      return false;

    for(auto edgeInfo : adjacent_[a.uid_]) {
      if (edgeInfo.first == b.uid_)
        return true;
    }

    return false;
  }

  /** Return the edge with index @a i.
   * @pre 0 <= @a i < num_edges()
   * @post Returns the i-th edge in @a adjacent_
   *
   * Complexity: O(i)
   */
  Edge edge(size_type i) const {
    auto it = edge_begin();
    while ((i > 0) && (it != edge_end())) {
      i--;
      ++it;
    }
    return (*it);
  }
  /** Remove an edge from the graph.
   * @param[in] a, b Edge to be removed, whose two end nodes are @a a and @a b.
   * @return 1 if old has_edge(a, b), 0 otherwise
   *
   * @post new num_edges() == old num_edges() - result.
   *
   * Can invalidate outstanding iterators.
   * If old has_edge(@a a, @ b), then both @a Edge(a, b) and @a Edge(b,a)
   * becomes invalid. All other Edge objects remain valid.
   *
   * Complexity: O(degree).
   */
  size_type remove_edge(const Node& a, const Node& b) {
    if (!has_edge(a, b))
      return 0;

    for (auto it = adjacent_[a.uid_].begin(); it != adjacent_[a.uid_].end(); ++it) {
      if ((*it).first == b.uid_) {
        adjacent_[a.uid_].erase(it);
        break;
      }
    }
    for (auto it = adjacent_[b.uid_].begin(); it != adjacent_[b.uid_].end(); ++it) {
      if ((*it).first == a.uid_) {
        adjacent_[b.uid_].erase(it);
        break;
      }
    }

    --edge_size_;
    return 1;
  }
  /** Remove an edge from the graph.
   * @param[in] e Edge to be removed.
   * @return 1 if old has_edge(e.node1(), e.node2()), 0 otherwise
   *
   * @post new num_edges() == old num_edges() - result.
   *
   * Can invalidate outstanding iterators.
   * If old has_edge(e.node1(), e.node2()), then @ e becomes invalid.
   * All other Edge objects remain valid.
   *
   * Complexity: O(degree).
   */
  size_type remove_edge(const Edge& e) {
    return remove_edge(e.node1(), e.node2());
  }
  /** Remove an edge from the graph.
   * @param[in] e_it Iterator references to the edges that is to be removed.
   * @return If old has_edge((*e_it).node1(), (*e_it).node2()), returns the iterator that
   *  points to the edge next to old (*e_it); otherwise return edge_end()
   *
   * @post new num_edges() == old num_edges() - result.
   *
   * Can invalidate outstanding iterators. If old @a (*e_it) exists in Graph,
   * @a (*e_it) becomes invalid. All other Edge objects remain valid.
   *
   * Complexity: O(degree).
   */
  edge_iterator remove_edge(edge_iterator e_it) {
    if (e_it != edge_end()) {
      remove_edge((*e_it));
      /* Because inside operator* function of edge_iterator, fix() is called.
      Thus calling the dereference function can eliminate possible problem.

      One thing I am not really sure is that, since the dereferenced object is been
      used in nowhere, will the compiler simply disgard this command for optimization purpose?
      */
      (*e_it);
    }

    return e_it;
  }

  /* Range for iterating all the edges in the graph */
  EdgeRange edges() const {
    return {*this};
  }
  ///////////////
  // Iterators //
  ///////////////
  /** @class Graph::NodeIterator

   * @brief Iterator class for nodes. A forward iterator.
   * @RI graph_ != nullptr && index_ <= graph_->num_nodes()
   */
  class NodeIterator : private totally_ordered<NodeIterator> {

   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Node value_type;
    /** Type of pointers to elements. */
    typedef Node* pointer;
    /** Type of references to elements. */
    typedef Node& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;

    /** Construct an invalid NodeIterator. */
    NodeIterator() {
    }

    /* Dereference the iterator instance.
    *  Return an instance of Node that this iterator currently points to
    */
    Node operator*() const {
      return graph_->node(index_);
    }

    /* Forward the index this iterator points to by 1.
    * @post Return the iterator after properly forwarded.
    */
    NodeIterator& operator++() {
      if (index_ < graph_->size())
        ++index_;
      return (*this);
    }

    /* Test whether two NodeIterator instances are equal.
    *
    * Equal NodeIterator instances have the same graph pointer and same index
    */
    bool operator==(const NodeIterator& x) const {
      return ((graph_ == x.graph_) && (index_ == x.index_));
    }

   private:
    friend class Graph;

    NodeIterator(const Graph* g, size_type idx) :
    graph_(const_cast<Graph*>(g)), index_(idx) {
      assert(g != nullptr);
    }

    Graph* graph_;
    size_type index_;
  };

  /* Return an iterator points to the first node in this graph.
  */
  node_iterator node_begin() const {
    return node_iterator(this, 0);
  }
  /* Return an iterator points to the end of the node container in this graph.
  */
  node_iterator node_end() const {
    return node_iterator(this, size());
  }

  /** @class Graph::EdgeIterator
   * @brief Iterator class for edges. A forward iterator.
   * @RI: @a graph_!=nullptr && @a curr_i_uid_ <= graph_->adjacent_.size()
   *      && graph_->adjacent_[curr_i_uid_].size() != 0
   *      && @a curr_i_uid_ < graph_->adjacent_[curr_i_uid_][curr_j_]
   */
  class EdgeIterator : private totally_ordered<EdgeIterator> {

   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Edge value_type;
    /** Type of pointers to elements. */
    typedef Edge* pointer;
    /** Type of references to elements. */
    typedef Edge& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;

    /** Construct an invalid EdgeIterator. */
    EdgeIterator() {
    }
    /*
    * Return an instance of Edge that this iterator currently points to
    */
    Edge operator*() {
      fix();
      auto& edgeInfo = graph_->adjacent_[curr_i_uid_][curr_j_];
      return Edge(graph_, curr_i_uid_, edgeInfo.first, edgeInfo.second);
    }

    /* Forward the index this iterator points to to the next proper position.
    *
    * @post Return the iterator after properly forwarded with RI satisfied.
    */
    EdgeIterator& operator++() {
      if (curr_i_uid_ < graph_->adjacent_.size())
        ++curr_j_;
      fix();

      return *this;
    }
    /* Test whether two EdgeIterator instances are equal.
    * Equal EdgeIterator instances have the same graph pointer, same i, j position
    * on the adjacency list.
    */
    bool operator==(const EdgeIterator& x) const {
      return ((graph_ == x.graph_) && (curr_i_uid_ == x.curr_i_uid_) && (curr_j_ == x.curr_j_));
    }

   private:
    friend class Graph;

    EdgeIterator(const Graph *g, size_type i, size_type j) :
    graph_(const_cast<Graph*>(g)), curr_i_uid_(i), curr_j_(j) {
      assert(graph_ != nullptr);
      fix();
    }

    /* Graph pointer*/
    Graph* graph_;
    /* The first index of @a adjacent_ it points to, which is the same
    * as the first node.index of the edge
    */
    size_type curr_i_uid_;
    /* The second index of @a adjacent_ it points to, the content inside
    * is the second node.index of the edge
    */
    size_type curr_j_;

    /*First of all, if @a curr_j_ is greater than the degree node @a curr_i_uid_ has,
    * @a curr_j_ should be reset to zero, with @a curr_i_uid_ increased by 1.
    * Secondly, repeat increasing @a curr_i_uid_ by 1 if the node does not connect to any edge.
    * Finally, since edge.uid1 < edge.uid2, we should skip those such that
    * the index of node @a curr_j_ points to is less than that of @a curr_i_uid_.
    *
    * @pre Do not need to satisfy RI
    * @post satisfy RI
    */
    void fix()
    {
      if (curr_i_uid_ >= graph_->adjacent_.size())
        return;
      // this row has finished, move to next row
      if (curr_j_ >= graph_->adjacent_[curr_i_uid_].size()) {
        ++curr_i_uid_;
        curr_j_ = 0;
      }
      // if next row is empty, means that this node connects to nobody
      while((curr_i_uid_ < graph_->adjacent_.size()) &&
       (graph_->adjacent_[curr_i_uid_].size() == 0))
        ++curr_i_uid_;

      // since each edge is stored twice, skip those such that nd2.index < nd1.index
      while((curr_i_uid_ < graph_->adjacent_.size()) &&
       (graph_->adjacent_[curr_i_uid_][curr_j_].first < curr_i_uid_)) {
        curr_j_ ++;
        if (curr_j_ >= graph_->adjacent_[curr_i_uid_].size()) {
          ++curr_i_uid_;
          curr_j_ = 0;
        }
        while((curr_i_uid_<graph_->adjacent_.size()) && (graph_->adjacent_[curr_i_uid_].size() == 0))
          ++curr_i_uid_;
      }
    }
  };

  /* Return an iterator points to the first edge in this graph.
  * In case the first m nodes have no edge, skip until to the node that has.
  */
  edge_iterator edge_begin() const {
    return edge_iterator(this, 0, 0);
  }

  /* Return an iterator points to the end of the edge container in this graph.
  */
  edge_iterator edge_end() const {
    return edge_iterator(this, adjacent_.size(), 0);
  }

  /** @class Graph::IncidentIterator
   * @brief Iterator class for edges incident to a node. A forward iterator.
   * @RI graph_ != nullptr && spawn_uid_ < graph_->num_nodes()
   *      && incident_j_ <= graph_->node(spawn_uid_).degree()
   * Notice that when dereference this iterator, the edge does not need to
   * hold the RI that edge.node1().index() < edge.node2().index()
   */
  class IncidentIterator : private totally_ordered<IncidentIterator> {
   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Edge value_type;
    /** Type of pointers to elements. */
    typedef Edge* pointer;
    /** Type of references to elements. */
    typedef Edge& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;

    /** Construct an invalid IncidentIterator. */
    IncidentIterator() {
    }

    /*
    * Return an instance of Edge that this iterator currently points to,
    * which should be an incident edge of the node which owns this iterator.
    */
    Edge operator*() const {
      auto& edgeInfo = graph_->adjacent_[spawn_uid_][incident_j_];
      return Edge(graph_, spawn_uid_, edgeInfo.first, edgeInfo.second);
    }

    /* Forward the index this iterator points to by one.
    *
    * @post Return the iterator after properly forwarded.
    */
    IncidentIterator& operator++() {
      if (incident_j_ < graph_->adjacent_[spawn_uid_].size())
        ++incident_j_;
      return *this;
    }

    /* Test whether two IncidentIterator instances are equal.
    * Same IncidentIterator instances have the same graph pointer, same spawn_node index and
    * same index of the incident edges.
    */
    bool operator==(const IncidentIterator& x) const {
      return ((graph_ == x.graph_) &&
              (spawn_uid_ == x.spawn_uid_) &&
              (incident_j_ == x.incident_j_));
    }

   private:
    friend class Graph;
    // Pointer to a Graph
    Graph* graph_;
    // Identify which node spawns this iterator
    size_type spawn_uid_;
    // Identify the current position of the incident edges container this iterator points to
    size_type incident_j_;

    IncidentIterator(const Graph* g, size_type spawn, size_type idx) :
    graph_(const_cast<Graph*>(g)), spawn_uid_(spawn), incident_j_(idx) {
      assert(graph_ != nullptr);
    }
  };

  ////////////
  // Ranges //
  ////////////
  /* Range class to iterate all the nodes in @a graph_. Cannot use when removing something in a loop.
  * @param graph_: Graph instance to be iterated
  */
  struct NodeRange {
    NodeRange(const Graph<node_value_type, edge_value_type>& g) : graph_(g) {
    }

    node_iterator begin() const {
      return graph_.node_begin();
    }

    node_iterator end() const {
      return graph_.node_end();
    }

  private:
    const Graph<node_value_type, edge_value_type>& graph_;
  };

  /* Range class to iterate all the edges in @a graph_. Cannot use when removing something in a loop.
  * @param graph_: Graph instance to be iterated
  */
  struct EdgeRange {
    EdgeRange(const Graph<node_value_type, edge_value_type>& g) : graph_(g) {
    }

    edge_iterator begin() const {
      return graph_.edge_begin();
    }

    edge_iterator end() const {
      return graph_.edge_end();
    }

  private:
    const Graph<node_value_type, edge_value_type>& graph_;
  };

  /* Range class to iterate all the incident edges in @a node_. Cannot use when removing something in a loop.
  * @param node_: Node instance for which its incident edges will be iterated
  */
  struct IncidentRange
  {
    IncidentRange(const Node& n) : node_(n) {
    }

    incident_iterator begin() const {
      return node_.edge_begin();
    }

    incident_iterator end() const {
      return node_.edge_end();
    }

  private:
      Node node_;
  };

 private:
  /* Internal data structure for storing all the data of a node.
  */
  struct internal_node {
    /* Constructor
    * @a const_cast<type>(val) can cast a @a const type @val to non-const,
    * given that the alias of @a val itself is non-const
    */
    internal_node(const Point& p, const node_value_type& v)
      : index(-1), uid(-1), position(p), value(v) {

    }
    // index of the node in @a i2u_
    int index;
    // Uid of the node, equal to the index of this instance in @a nods_
    size_type uid;
    // Node's physical position
    Point position;
    // Other value
    node_value_type value;
  };

  /* Internal data structure for storing all the data of a node.
  */
  struct internal_edge {
    // data
    edge_value_type value;

    internal_edge(const edge_value_type& v) : value(v) {

    }
  };

  /* Container to store all nodes data, nodes can only be pushed in, with no way out*/
  std::vector<internal_node> nodes_;
  /* Container to store all valid nodes' Uid */
  std::vector<size_type> i2u_;
  /* Indicator of the first unused uid, its initial value will be -1
  * When adding a new node, we should first check if @a free_uid_ is -1. If
  *   it is, that means no uid is available for reusage, we should do in the regular way;
  *   it isn't, that means there exists uid available for reusage. We should use @a free_uid_
  *   as this new node's uid, which involves first updating @a free_uid_ to nodes_[free_uid_].index,
  *   and storing the internal_node into old @a free_uid_'s position of @a nodes_,
  * When removing a node, we should use this node's index in @a nodes_ to store the current @a free_uid_
  * and then update it to this node's uid. Then its like new @a free_uid_ == latest unused uid, and
  * nodes_[new @a free_uid_].index == old @a free_uid_.
  */
  int free_uid_;

  /* Container implemented as adjacency list, store all the
  * edge relationship between any two nodes given that they are connected.
  *
  * The index of the outer vector stands for the first node's Uid of the edge.
  * The internal data is a std::par<size_type, size_type>, where @a first stores the second
  * node's Uid of the edge, and @a second stores the Uid of this edge's data, which is
  * equivalant to the index of this data in @a edges_
  */
  std::vector<std::vector<std::pair<size_type, size_type>>> adjacent_;
  /* Container to store all edges data, edges can only be pushed in, with no way out.
  * This is because the only value to identify each edge is its edge Uid, which is the index
  * in this vector.
  */
  std::vector<internal_edge> edges_;
  /* Store the number of edges */
  size_type edge_size_;
};

#endif
