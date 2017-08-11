/**
 * @file shallow_water.cpp
 * Implementation of a shallow water system using Mesh
 *
 * @brief Reads in two files specified on the command line.
 * First file: 3D point list (one per line) defined by three doubles
 * Second file: Triangles (one per line) defined by 3 indices into the point list
 */

#include <fstream>
#include <cmath>

#include "CS207/SDLViewer.hpp"
#include "CS207/Util.hpp"
#include "CS207/Color.hpp"

#include "CS207/Point.hpp"
#include "Mesh.hpp"

// Standard gravity (average gravity at Earth's surface) in meters/sec^2
static constexpr double grav = 9.80665;

/** Water column characteristics */
struct QVar {
  double h;   // Height of column
  double hx;  // Height times average x velocity of column
  double hy;  // Height times average y velocity of column

  /** Default constructor.
   *
   * A default water column is 1 unit high with no velocity. */
  QVar()
      : h(1), hx(0), hy(0) {
  }
  /** Construct the given water column. */
  QVar(double h_, double hx_, double hy_)
      : h(h_), hx(hx_), hy(hy_) {
  }
  // More operators?
  QVar operator+(const QVar& q) const {
    return {h + q.h, hx + q.hx, hy + q.hy};
  }
  
  QVar operator-(const QVar& q) const {
    return {h - q.h, hx - q.hx, hy - q.hy};
  }
  
  QVar operator*(double c) const {
    return {h * c, hx * c, hy * c};
  }
  
  QVar operator/(double c) const {
    return {h / c, hx / c, hy / c};
  }
};

// HW4B: Placeholder for Mesh Type!
// Define NodeData, EdgeData, TriData, etc
// or redefine for your particular Mesh
//typedef Mesh<int,int,int> MeshType;
typedef Mesh<QVar, bool, QVar> MeshType;
typedef MeshType::Node Node;
typedef MeshType::Edge Edge;
typedef MeshType::Triangle Triangle;
typedef MeshType::size_type size_type;

/** Function object for calculating shallow-water flux.
 *          |n
 *   T_k    |---> n = (nx,ny)   T_m
 *   QBar_k |                   QBar_m
 *          |
 * @param[in] nx, ny Defines the 2D outward normal vector n = (@a nx, @a ny)
 *            from triangle T_k to triangle T_m. The length of n is equal to the
 *            the length of the edge, |n| = |e|.
 * @param[in] dt The time step taken by the simulation. Used to compute the
 *               Lax-Wendroff dissipation term.
 * @param[in] qk The values of the conserved variables on the left of the edge.
 * @param[in] qm The values of the conserved variables on the right of the edge.
 * @return The flux of the conserved values across the edge e
 */
struct EdgeFluxCalculator {
  QVar operator()(double nx, double ny, double dt,
                  const QVar& qk, const QVar& qm) {
    // Normalize the (nx,ny) vector
    double n_length = std::sqrt(nx*nx + ny*ny);
    nx /= n_length;
    ny /= n_length;

    // The velocities normal to the edge
    double wm = (qm.hx*nx + qm.hy*ny) / qm.h;
    double wk = (qk.hx*nx + qk.hy*ny) / qk.h;

    // Lax-Wendroff local dissipation coefficient
    double vm = sqrt(grav*qm.h) + sqrt(qm.hx*qm.hx + qm.hy*qm.hy) / qm.h;
    double vk = sqrt(grav*qk.h) + sqrt(qk.hx*qk.hx + qk.hy*qk.hy) / qk.h;
    double a  = dt * std::max(vm*vm, vk*vk);

    // Helper values
    double scale = 0.5 * n_length;
    double gh2   = 0.5 * grav * (qm.h*qm.h + qk.h*qk.h);

    // Simple flux with dissipation for stability
    return QVar(scale * (wm*qm.h  + wk*qk.h)           - a * (qm.h  - qk.h),
                scale * (wm*qm.hx + wk*qk.hx + gh2*nx) - a * (qm.hx - qk.hx),
                scale * (wm*qm.hy + wk*qk.hy + gh2*ny) - a * (qm.hy - qk.hy));
  }
};

/** Node position function object for use in the SDLViewer. */
struct NodePosition {
  template <typename NODE>
  Point operator()(const NODE& n) {
    return {n.position().x, n.position().y, n.value().h};
  }
};

struct CoolColor
{
  /* data */
  CS207::Color operator()(const Node& node) {
    double val = 1.2 * (node.value().h - .8);
    val = std::min(0.3, std::max(0.2, val));
    //std::cout << node.value().h << std::endl;
    return CS207::Color::make_heat(val);
  }
};


/** Integrate a hyperbolic conservation law defined over the mesh m
 * with flux functor f by dt in time.
 */
template <typename MESH, typename FLUX>
double hyperbolic_step(MESH& m, FLUX& f, double t, double dt) {
  // HW4B: YOUR CODE HERE
  // Step the finite volume model in time by dt.

  // Pseudocode:
  // Compute all fluxes. (before updating any triangle Q_bars)
  // For each triangle, update Q_bar using the fluxes as in Equation 8.
  //  NOTE: Much like symp_euler_step, this may require TWO for-loops

  /* 1. Initialize a vector temp_Q of size @a m.num_triangles() for storing all temp Q_bar values
   * 2. Use a TriangleIterator to iterate through all the triangles, for each tri_it:
   *      a. Compute the sum of fluxes sum_fluxes
   *      b. temp_Q[(*it).index()] = sum_fluxes
   * 3. Use indices of triangles to do another for loop, for each triangle(i):
   *      a. triangle_i.value().Q_bar = temp_Q[i]
   */
  std::vector<QVar> temp_fluxes(m.num_triangles(), QVar());
  for (auto tri_it = m.triangle_begin(); tri_it != m.triangle_end(); ++tri_it) {
    auto qk = (*tri_it).value();
    
    QVar F_k(0, 0, 0);
    for (size_type i = 0; i < 3; ++i) {
      auto edge = (*tri_it).edge(i);
      QVar qm(0, 0, 0);
      if (m.adj_triangle1_index(edge) == -1 || m.adj_triangle2_index(edge) == -1) {
          //set_boundary_conditions
        qm = QVar(qk.h, 0, 0);
      } else {
        size_type adj_tri_idx = m.adj_triangle1_index(edge) == (*tri_it).index() ?
            m.adj_triangle2_index(edge) : m.adj_triangle1_index(edge);

        auto adj_triangle = m.triangle(adj_tri_idx);
        qm = adj_triangle.value(); 
      }
      auto norm_ke = m.out_norm((*tri_it), edge); 
      F_k = F_k + f(norm_ke.x, norm_ke.y, dt, qk, qm);
    } 
    // print_triangle(m, (*tri_it), f, t, dt);
    temp_fluxes[(*tri_it).index()] = qk - F_k * (dt / (*tri_it).area());
  }
  
  for (auto tri_it = m.triangle_begin(); tri_it != m.triangle_end(); ++tri_it) {
    (*tri_it).value() = temp_fluxes[(*tri_it).index()];
  }
  
  return t + dt;
}

/** Convert the triangle-averaged values to node-averaged values for viewing. */
template <typename MESH>
void post_process(MESH& m) {
  // HW4B: Post-processing step
  // Translate the triangle-averaged values to node-averaged values
  // Implement Equation 9 from your pseudocode here
  for (auto nit = m.node_begin(); nit != m.node_end(); ++nit) {
      double total_area = 0;
      QVar Q_tot(0, 0, 0);
      int count = 0;
  
      for (auto tri_it = m.adjacent_triangle_begin(*nit); tri_it != m.adjacent_triangle_end(*nit); ++tri_it) {
        Q_tot = Q_tot + (*tri_it).value() * (*tri_it).area();
        total_area += (*tri_it).area();
        ++count;
      }
      (*nit).value() = Q_tot / total_area;
  }
}

/** Use the Curiously Recurring Template Pattern for initializing conditions! */
template <typename T>
struct BaseInitializer {
    std::pair<double, double> operator()(MeshType& mesh) const {
        double max_height = derived().initialize_node_values(mesh);
        
        double min_edge_length = (*mesh.edge_begin()).length();
        for (auto eit = mesh.edge_begin(); eit != mesh.edge_end(); ++eit) {
            min_edge_length = std::min(min_edge_length, (*eit).length());
        }
    
        for (auto tri_it = mesh.triangle_begin(); tri_it != mesh.triangle_end(); ++tri_it) {
            (*tri_it).value() = ((*tri_it).node(0).value() + (*tri_it).node(1).value() + (*tri_it).node(2).value()) / 3.0;
        }
        return std::make_pair(max_height, min_edge_length);
    }
  private:
    const T& derived() const {
        return static_cast<const T&>(*this);  
    }
};

struct PebbleRipple : public BaseInitializer<PebbleRipple> {
  private:
    friend struct BaseInitializer<PebbleRipple>; 
    
    double initialize_node_values(MeshType& mesh) const {
        double max_height = 0;
        for (auto nit = mesh.node_begin(); nit != mesh.node_end(); ++nit) {
            (*nit).value() = QVar(1 - 0.75 * exp(
              -80*(pow((*nit).position().x - 0.75, 2) + pow((*nit).position().y, 2))
              ), 0, 0);

            max_height = std::max(max_height, (*nit).value().h);
        }

        return max_height;
    }
};

struct SharpWave : public BaseInitializer<SharpWave> {
  private:
    friend struct BaseInitializer<SharpWave>;
    
    double initialize_node_values(MeshType& mesh) const {
        double max_height = 0;
        for (auto nit = mesh.node_begin(); nit != mesh.node_end(); ++nit) {
            unsigned H = 0;
            if ((*nit).position().x < 0) H = 1;
            (*nit).value() = QVar(1 + 0.75 * H * (pow(((*nit).position().x - 0.75), 2) + 
                      pow((*nit).position().y, 2) - 0.15 * 0.15), 0, 0);
            max_height = std::max(max_height, (*nit).value().h);
        }
        
        return max_height;
    }

};

struct DamBreak : public BaseInitializer<DamBreak> {
  private:
    friend struct BaseInitializer<DamBreak>;
    
    double initialize_node_values(MeshType& mesh) const {
        double max_height = 0;
        for (auto nit = mesh.node_begin(); nit != mesh.node_end(); ++nit) {
            if ((*nit).position().x < 0) {
                (*nit).value() = QVar(1.75, 0, 0);
            }
            else {
                (*nit).value() = QVar(1, 0, 0);
            }
            max_height = std::max(max_height, (*nit).value().h);
        }
        
        return max_height;
    }
};

int main(int argc, char* argv[])
{
  // Check arguments
  if (argc < 3) {
    std::cerr << "Usage: shallow_water NODES_FILE TRIS_FILE\n";
    exit(1);
  }

  MeshType mesh;
  // HW4B: Need node_type before this can be used!
#if 1
  std::vector<typename MeshType::node_type> mesh_node;
#endif

  // Read all Points and add them to the Mesh
  std::ifstream nodes_file(argv[1]);
  Point p;
  while (CS207::getline_parsed(nodes_file, p)) {
    // HW4B: Need to implement add_node before this can be used!
#if 1
    mesh_node.push_back(mesh.add_node(p));
#endif
  }

  // Read all mesh triangles and add them to the Mesh
  std::ifstream tris_file(argv[2]);
  std::array<int,3> t;
  while (CS207::getline_parsed(tris_file, t)) {
    // HW4B: Need to implement add_triangle before this can be used!
#if 1
    mesh.add_triangle(mesh_node[t[0]], mesh_node[t[1]], mesh_node[t[2]]);
#endif
  }

  // Print out the stats
  std::cout << mesh.num_nodes() << " "
            << mesh.num_edges() << " "
            << mesh.num_triangles() << std::endl;

  // HW4B Initialization
  // Set the initial conditions based off third argument
  // Perform any needed precomputation
  std::pair<double, double> value_pair;
  if ((*argv[3]) == '0') {
      std::cout << "Pebble Ripple" << std::endl;
      value_pair = PebbleRipple()(mesh);
  } else if ((*argv[3]) == '1') {
      std::cout << "Sharp Wave" << std::endl;
      value_pair = SharpWave()(mesh);
  } else {
      std::cout << "Dam Break" << std::endl;
      value_pair = DamBreak()(mesh);
  }
  
  double max_height = value_pair.first;
  double min_edge_length = value_pair.second;

  // Launch the SDLViewer
  CS207::SDLViewer viewer;
  viewer.launch();

  // HW4B: Need to define Mesh::node_type and node/edge iterator
  // before these can be used!
#if 1
  auto node_map = viewer.empty_node_map(mesh);
  viewer.add_nodes(mesh.node_begin(), mesh.node_end(),
                   CS207::DefaultColor(), NodePosition(), node_map);
  viewer.add_edges(mesh.edge_begin(), mesh.edge_end(), node_map);
#endif
  viewer.center_view();
  // CFL stability condition requires dt <= dx / max|velocity|
  // For the shallow water equations with u = v = 0 initial conditions
  //   we can compute the minimum edge length and maximum original water height
  //   to set the time-step
  // Compute the minimum edge length and maximum water height for computing dt
#if 1
  double dt = 0.25 * min_edge_length / (sqrt(grav * max_height));
#else
  // Placeholder!! Delete me when min_edge_length and max_height can be computed!
  double dt = 0.1;
#endif
  double t_start = 0;
  double t_end = 5;

  // Preconstruct a Flux functor
  EdgeFluxCalculator f;

  // Begin the time stepping
  for (double t = t_start; t < t_end; t += dt) {
    // Step forward in time with forward Euler
    hyperbolic_step(mesh, f, t, dt);
    // Update node values with triangle-averaged values
    post_process(mesh);
    
    // Update the viewer with new node positions
#if 1
    // Update viewer with nodes' new positions
    viewer.add_nodes(mesh.node_begin(), mesh.node_end(),
                     CoolColor(), NodePosition(), node_map);
#endif
    viewer.set_label(t);

    // These lines slow down the animation for small meshes.
    // Feel free to remove them or tweak the constants.
    if (mesh.num_nodes() < 100)
      CS207::sleep(0.05);
  }

  return 0;
}
