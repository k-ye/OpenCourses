#pragma once
/** @file Mesh.hpp
 * @brief A Mesh is composed of nodes, edges, and triangles such that:
 *  -- All triangles have three nodes and three edges.
 *  -- All edges belong to at least one triangle and at most two triangles.
 */

#include <boost/iterator/transform_iterator.hpp>
#include <cmath>
#include "Graph.hpp"

/** @class Mesh
 * @brief A template for 3D triangular meshes.
 *
 * Users can add triangles and retrieve nodes, edges, and triangles.
 */
template <typename N, typename E, typename T>
class Mesh {
  // HW3: YOUR CODE HERE
  // Write all typedefs, public member functions, private member functions,
  //   inner classes, private member data, etc.
 public:
  /** Type of indexes and sizes. Return type of Mesh::num_nodes(). */
  typedef int size_type;
  
  typedef N graph_node_value_type;
  typedef E graph_edge_value_type;
  typedef T triangle_value_type;
  typedef Graph<graph_node_value_type, graph_edge_value_type> GraphType;
  
 private:
  struct internal_triangle;  
 public:
  typedef typename GraphType::Node Node;
  typedef Node node_type;
  typedef typename GraphType::NodeRange node_range;
  
  typedef typename GraphType::Edge Edge;
  typedef Edge edge_type;
  typedef typename GraphType::EdgeRange edge_range;
  
  typedef typename GraphType::incident_iterator incident_iterator;
  typedef typename GraphType::node_iterator node_iterator;
  typedef typename GraphType::edge_iterator edge_iterator;
  
  class TriangleIterator;

  typedef TriangleIterator triangle_iterator;
  /* Iterator to iterator through all 3 adjacent triangles of a given triangle */
  class TriTriangleIterator;

  class NodeTriangleIterator; 

  class TriangleRange;

  // class TriTriangleRange;

  // class NodeTriangleRange;
  
  class Triangle : private totally_ordered<Triangle> {
   public:    
    /** Access the ith node of the triangle.
      * @param[in] i 0, 1, or 2
      *
      * Complexity: O(1).
      */
    Node node(size_type i) const {
      assert(0 <= i && i <= 2);
      return mesh_->graph_.node(fetch().nodes[i]);
    }
    
    /** Access the ith edge of the triangle.
      * @param[in] i 0, 1, or 2
      *
      * Complexity: O(1).
      */
    Edge edge(size_type i) const {
      assert(0 <= i && i <= 2);
      if (i == 0) {
        return mesh_->graph_.edge(node(0), node(1));
      } else if (i == 1) {
        return mesh_->graph_.edge(node(0), node(2));
      } else {
        return mesh_->graph_.edge(node(1), node(2));
      }
    }

    /** Calculate the area of the triangle.
      *
      * Complexity: O(1).
      */
    double area() const {
      // using heron's formula
      double s = (edge(0).length() + edge(1).length() + edge(2).length())/2;
      return std::sqrt(s *
                      (s - edge(0).length()) *
                      (s - edge(1).length()) *
                      (s - edge(2).length())
                      );
    }

    /** Access the index of the triangle.
      *
      * Complexity: O(1).
      */
    size_type index() const {
      return index_;
    }

    /** Access the value(s) stored in the triangle.
      *
      * Complexity: O(1).
      */
    triangle_value_type& value() {
      return fetch().value;
    }

    /** Access the value(s) stored in the triangle.
      *
      * Complexity: O(1).
      */
    const triangle_value_type& value() const {
      return fetch().value;
    }
    
    bool operator==(const Triangle& x) const {
      return std::tie(mesh_, node(0), node(1), node(2)) == std::tie(x.mesh_, x.node(0), x.node(1), x.node(2));
    }
    
    bool operator<(const Triangle& x) const {
      return std::tie(mesh_, node(0), node(1), node(2)) < std::tie(x.mesh_, x.node(0), x.node(1), x.node(2));
    }


    /** Return the iterator which corresponds to the first adjacent triangle. */
    TriTriangleIterator begin() const {
      return {mesh_, index_, 0};
    }

    /** Return the iterator which corresponds to the past-the-last adjacent triangle. */
    TriTriangleIterator end() const {
      return {mesh_, index_, 3};
    }

    /** Construct invalid triangle. */
    Triangle() : mesh_(nullptr), index_(-1) {}
    
    bool valid() {
      return index_ != -1;
    }

  private:
    friend class Mesh;
    /** Private valid constructor */
    Triangle(const Mesh* m, size_type i) : mesh_(const_cast<Mesh*>(m)), index_(i) {
    }
    /** Pointer to the Mesh object this triangle belongs */
    Mesh* mesh_;
    /** Index of this triangle, we do not distinguish UID from index in Triangle */
    size_type index_;

    internal_triangle& fetch() const {
      return mesh_->triangles_[index_];
    }
  };
  
  /** Return the number of nodes in the mesh. */
  size_type num_nodes() const {
    return graph_.num_nodes();
  }

  /** Return the number of edges in the mesh. */
  size_type num_edges() const {
    return graph_.num_edges();
  }

  /** Return the number of triangles in the mesh. */
  size_type num_triangles() const {
    return triangles_.size();
  }
  
  /** Add a triangle.
   * @param[in] n1, n2, n3 Valid nodes belonging to @a graph_.
   * @pre n1.index() != n2.index() != n3.index()
   * @pre For all 1 <= i < j <= 3, @a graph_.edge(ni, nj) has at most 1 triangle; or if graph_.edge(ni, nj) has 2 triangles, then has_triangle(ni, nj, nk)
   * @post If !has_triangle(n1, n2, n3),
   *   new @a num_triangles() == old @a num_triangles() + 1
   *   For all 1 <= i < j <= 3, @a graph_.edge(ni, nj) has 1 more triangle
   * Else
   *   No members are modified.
   * @post For all 1 <= i < j < k <= 3, if nk is on the left(right) side of the hyperplane formed by the vector from ni.position() to nj.position(), where ni < nj,
   *   then graph_.edge(ni, nj).left(right) == triangle(ni, nj, nk)
   * @return Triangle with vertices n1, n2, n3.
   *
   * Complexity: O(1).
   */
  Triangle add_triangle(const Node& n1, const Node& n2, const Node& n3, const triangle_value_type& val = triangle_value_type()) {

    assert(n1 != n2);
    assert(n1 != n3);
    assert(n2 != n3);

    if (has_triangle(n1, n2, n3)) {
      return triangle(n1, n2, n3);
    }
    // Pre-process the nodes, sort them so that the relationship if temp[0] < temp[1] < temp[2]
    auto new_triangle = internal_triangle(val);
    new_triangle.nodes.push_back(n1.index());
    new_triangle.nodes.push_back(n2.index());
    new_triangle.nodes.push_back(n3.index());
    std::sort(new_triangle.nodes.begin(), new_triangle.nodes.end());
    new_triangle.index = triangles_.size();
    triangles_.push_back(new_triangle);
    // add new triangle.index to each nodes adjacent list
    node_adj_triangles_[n1.index()].push_back(new_triangle.index);
    node_adj_triangles_[n2.index()].push_back(new_triangle.index);
    node_adj_triangles_[n3.index()].push_back(new_triangle.index);

    // Add edge (if it doesn't already exist), and assign the triangle toward whichever
    // side of the edge it faces.
    for (size_type i=0; i < 3; ++i) {
        Edge edge = graph_.add_edge(node(new_triangle.nodes[i % 3]), node(new_triangle.nodes[(i+1) % 3]));
        if (graph_.num_edges() > edge_adj_triangles_.size()) {
          // a new edge has been added
          edge_adj_triangles_.push_back(std::vector<size_type>());
        }
        if (edge_adj_triangles_[edge.index()].size() < 2) {
          // this is a new triangle of this edge
          edge_adj_triangles_[edge.index()].push_back(new_triangle.index);
        }
    }

    return triangle(new_triangle.index);
  }

  bool has_triangle(const Node& n1, const Node& n2, const Node& n3) const {
    if (!(graph_.has_edge(n1, n2)) || !(graph_.has_edge(n1, n3)) || !(graph_.has_edge(n2, n3))) {
        return false;
    }

    Edge edge = graph_.edge(n1, n2);
    for (size_type tri_idx : edge_adj_triangles_[edge.index()]) {
      auto tri = triangle(tri_idx);
      if ((n3 == tri.node(0) || n3 == tri.node(1) || n3 == tri.node(2)))
        return true;
    }
    return false;
  }

  Triangle triangle(const Node& n1, const Node& n2, const Node& n3) const {
    if (!has_triangle(n1, n2, n3))
      return Triangle();

    Edge edge = graph_.edge(n1, n2);
    for (size_type tri_idx : edge_adj_triangles_[edge.index()]) {
      auto tri = triangle(tri_idx);
      if ((n3 == tri.node(0) || n3 == tri.node(1) || n3 == tri.node(2)))
        return tri;
    }
    return Triangle();
  }
  
  Triangle triangle(size_type i) const {
    return {this, i};
  }
  
  /** Add a node.
   * Satisfies the conditions for @a graph_'s add_node().
   *
   * Complexity: O(1).
   */
  Node add_node(const Point& position, const graph_node_value_type& val = graph_node_value_type()) {
    node_adj_triangles_.push_back(std::vector<size_type>());

    return graph_.add_node(position, val);
  }

  Node node(size_type i) const {
    return graph_.node(i);
  }

  /** Return the outward normal of an edge given the triangle.
    * @param[in] triangle A valid triangle containing @a edge in the mesh.
    * @param[in] edge A valid edge.
    * @return Point which specifies the outward normal vector.
    *
    * Complexity: O(1).
    */
  Point out_norm(const Triangle& triangle, const Edge& edge) const {
    size_type n3_idx = -1;
    for (size_type i = 0; i < 3; ++i) {
      if ((triangle.node(i) != edge.node1()) && (triangle.node(i) != edge.node2())) {
        n3_idx = triangle.node(i).index();
        break;
      }
    }
    // make edge.node1() the origin point
    Point v1 = edge.node2().position() - edge.node1().position();
    Point v2 = node(n3_idx).position() - edge.node1().position();
    Point plane_norm = cross(v1, v2);

    Point edge_norm = cross(v1, plane_norm);
    // make it outward
    if (dot(edge_norm, v2) > 0) {
      edge_norm = -1 * edge_norm;
    }

    edge_norm = edge_norm / norm(edge_norm) * edge.length();
    return edge_norm;
  }

  size_type adj_triangle1_index(const Edge& edge) const {
    return edge_adj_triangle_index(edge, 1);
  }

  size_type adj_triangle2_index(const Edge& edge) const {
    return edge_adj_triangle_index(edge, 2);
  }

  class TriangleIterator : private totally_ordered<TriangleIterator> {
   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Triangle value_type;
    /** Type of pointers to elements. */
    typedef Triangle* pointer;
    /** Type of references to elements. */
    typedef Triangle& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;

    /** Construct invalid iterator. */
    TriangleIterator() : mesh_(nullptr), index_(-1) { 
    
    }

    /** Return the triangle at the iterator. */
    Triangle operator*() {
      return mesh_->triangle(index_);
    }

    /** Increment iterator. */
    TriangleIterator operator++() {
      if (index_ < mesh_->num_triangles())
        ++index_;
      return(*this);
    }

    /** Compare if two iterators are the same. */
    bool operator==(const TriangleIterator& x) const {
      return std::tie(mesh_, index_) == std::tie(x.mesh_, x.index_);
    }

  private:
    friend class Mesh;
    
    TriangleIterator(const Mesh* m, size_type idx) : 
      mesh_(const_cast<Mesh*>(m)), index_(idx) {
    }

    Mesh* mesh_;
    size_type index_;
  };

  /** Return the iterator which corresponds to the first triangle. */
  TriangleIterator triangle_begin() const {
    return {this, 0};
  }

  /** Return the iterator which corresponds to the past-the-last triangle. */
  TriangleIterator triangle_end() const {
    return {this, num_triangles()};
  }

  class TriangleRange {
    const Mesh& mesh_;
  public:
    TriangleRange(const Mesh& m) : mesh_(m) {

    }
    
    TriangleIterator begin() const {
      return mesh_.triangle_begin();
    }

    TriangleIterator end() const {
      return mesh_.triangle_end();
    }
  };

  TriangleRange triangles() const {
    return {*this};
  }

  node_iterator node_begin() const {
    return graph_.node_begin();
  }
  
  node_iterator node_end() const {
    return graph_.node_end();
  }

  node_range nodes() const {
    return graph_.nodes();
  }
  
  edge_iterator edge_begin() const {
    return graph_.edge_begin();
  }
  
  edge_iterator edge_end() const {
    return graph_.edge_end();
  }

  edge_range edges() const {
    return graph_.edges();
  }

  class NodeTriangleIterator : private totally_ordered<NodeTriangleIterator> {
   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Triangle value_type;
    /** Type of pointers to elements. */
    typedef Triangle* pointer;
    /** Type of references to elements. */
    typedef Triangle& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;

    /** Construct invalid iterator. */
    NodeTriangleIterator() :
      mesh_(nullptr), node_index_(-1), index_(-1) {
    }

    /** Return the adjacent triangle at the iterator. */
    Triangle operator*() {
      size_type tri_index = mesh_->node_adj_triangles_[node_index_][index_];
      return mesh_->triangle(tri_index);
    }

    /** Increment iterator. */
    NodeTriangleIterator operator++() {
      ++index_;
      return (*this);
    }

    /** Compare if two iterators are the same. */
    bool operator==(const NodeTriangleIterator& x) const {
      return std::tie(mesh_, node_index_, index_) == std::tie(x.mesh_, x.node_index_, x.index_);
    }

   private:
    friend class Mesh;
    NodeTriangleIterator(const Mesh* m, size_type n_idx, size_type idx) :
      mesh_(const_cast<Mesh*>(m)), node_index_(n_idx), index_(idx) {
    }

    Mesh* mesh_;
    size_type node_index_;
    size_type index_;
  };

  /** Return the iterator which corresponds to the first adjacent triangle. */
  NodeTriangleIterator adjacent_triangle_begin(const Node& node) const {
    return {this, (size_type)node.index(), 0};
  }

  /** Return the iterator which corresponds to the past-the-last adjacent triangle. */
  NodeTriangleIterator adjacent_triangle_end(const Node& node) const {
    return {this, (size_type)node.index(), (size_type)node_adj_triangles_[node.index()].size()};
  }

  class TriTriangleIterator : private totally_ordered<TriTriangleIterator> {
   public:
    // These type definitions help us use STL's iterator_traits.
    /** Element type. */
    typedef Triangle value_type;
    /** Type of pointers to elements. */
    typedef Triangle* pointer;
    /** Type of references to elements. */
    typedef Triangle& reference;
    /** Iterator category. */
    typedef std::input_iterator_tag iterator_category;
    /** Difference between iterators */
    typedef std::ptrdiff_t difference_type;


    /** Construct invalid iterator. */
    TriTriangleIterator() :
      mesh_(nullptr), triangle_index_(-1), edge_index_(-1) {
    }

    /** Return the adjacent triangle at the iterator. */
    Triangle operator*() {
      Edge edge = mesh_->triangle(triangle_index_).edge(edge_index_);

      for (size_type adj_tri_idx : mesh_->edge_adj_triangles_[edge.index()]) {
        if (adj_tri_idx != triangle_index_) {
          return mesh_->triangle(adj_tri_idx);
        }
      }
      return Triangle();
    }

    /** Increment iterator. */
    TriTriangleIterator operator++() {
      if (edge_index_ < 3)
        ++edge_index_;
      return(*this);
    }

    /** Compare if two iterators are the same. */
    bool operator==(const TriTriangleIterator& x) const {
      return std::tie(mesh_, triangle_index_, edge_index_) ==
             std::tie(x.mesh_, x.triangle_index_, x.edge_index_);
    }

  private:
    friend class Mesh;
    TriTriangleIterator(const Mesh* m, size_type tri_idx, size_type e) :
      mesh_(const_cast<Mesh*>(m)), triangle_index_(tri_idx), edge_index_(e) {
    }

    Mesh* mesh_;
    size_type triangle_index_;
    size_type edge_index_;
  };

 private:

  size_type edge_adj_triangle_index(const Edge& edge, size_type order_th) const {
    assert(order_th > 0);
    if ((size_type)edge_adj_triangles_[edge.index()].size() < order_th)
      return -1;
    return edge_adj_triangles_[edge.index()][order_th-1];
  }

  GraphType graph_;
  /* Internal data structure for triangle*/
  struct internal_triangle {  
    internal_triangle(const triangle_value_type& val) : value(val) {
    }
    
    /* Vector to stores three indices of the nodes on the triangle */
    std::vector<size_type> nodes;
    /** Index of the triangle */
    size_type index;
    triangle_value_type value;
  };
  // can be used for triangle traversal
  std::vector<internal_triangle> triangles_;
  // adjacent triangle indices of a node, match with each index(uid in this case)
  std::vector<std::vector<size_type>> node_adj_triangles_;
  //adjacent triangle indices of an edge, match with each index(uid in this case)
  std::vector<std::vector<size_type>> edge_adj_triangles_;
};