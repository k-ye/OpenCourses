#ifndef CS207_SDLVIEWER_HPP
#define CS207_SDLVIEWER_HPP

/**
 * @file SDLViewer.hpp
 * Interactive OpenGL Viewer
 *
 * @brief Provides a OpenGL window with some basic interactivity.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <algorithm>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "GLCamera.hpp"
#include "Util.hpp"
#include "Point.hpp"
#include "Color.hpp"

#include <boost/signals2/signal.hpp>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>



namespace CS207 {

// Map C++ types to GL types at compilation time: gltype<T>::value
template <GLenum V> struct gltype_v {
  static constexpr GLenum value = V;
};
template <typename T> struct gltype {};
template<> struct gltype<unsigned char>  : public gltype_v<GL_UNSIGNED_BYTE> {};
template<> struct gltype<unsigned short> : public gltype_v<GL_UNSIGNED_SHORT> {};
template<> struct gltype<unsigned int>   : public gltype_v<GL_UNSIGNED_INT> {};
template<> struct gltype<short>          : public gltype_v<GL_SHORT> {};
template<> struct gltype<int>            : public gltype_v<GL_INT> {};
template<> struct gltype<float>          : public gltype_v<GL_FLOAT> {};
template<> struct gltype<double>         : public gltype_v<GL_DOUBLE> {};

// A default color functor that returns white for anything it recieves
struct DefaultColor {
  template <typename NODE>
  Color operator()(const NODE&) {
    return Color(1);
  }
};

// A default position functor that returns node.position() for any node
struct DefaultPosition {
  template <typename NODE>
  Point operator()(const NODE& node) {
    return node.position();
  }
};

/** SDLViewer class to view points and edges
 */
class SDLViewer {
 private:
  // Rendering surface
  SDL_Surface* surface_;
  // Window width and height
  int window_width_;
  int window_height_;

  // Event handler for interactivity
  SDL_Thread* event_thread_;
  // Synchronization lock
  SDL_mutex* lock_;
  // Queue lock
  bool render_requested_;

  // OpenGL Camera to track the current view
  GLCamera camera_;

  // Vertexes
  std::vector<Point> coords_;

  // Colors
  std::vector<Color> colors_;

  // Edges (index pairs)
  std::vector<unsigned> edges_;

  // Currently displayed label
  std::string label_;

  struct safe_lock {
    SDLViewer* v_;
    bool ok_;
    safe_lock(SDLViewer* v)
        : v_(v) {
      if (SDL_LockMutex(v_->lock_)) {
        fprintf(stderr, "\nSDL Mutex lock error: %s", SDL_GetError());
        ok_ = false;
      } else
        ok_ = true;
    }
    void unlock() {
      if (ok_ && SDL_UnlockMutex(v_->lock_))
        fprintf(stderr, "\nSDL Mutex unlock error: %s", SDL_GetError());
      else
        ok_ = false;
    }
    ~safe_lock() {
      unlock();
    }
  };
  friend struct safe_lock;

  boost::signals2::signal<void (SDLKey)> keyboard_signals_;
 public:
  /** Functions to handle keyboard events */
  typedef boost::function<void (SDLKey)> KeyboardEvent;

  void add_keyboard_listener(const KeyboardEvent& func) {
    keyboard_signals_.connect(func);
  }

    /** Constructor */
  SDLViewer()
      : surface_(nullptr), event_thread_(nullptr), lock_(nullptr),
        render_requested_(false) {
  }

  /** Destructor - Waits until the event thread exits, then cleans up
   */
  ~SDLViewer() {
    if (event_thread_ != nullptr)
      SDL_WaitThread(event_thread_, nullptr);
    SDL_FreeSurface(surface_);
    SDL_DestroyMutex(lock_);
    SDL_Quit();
  }

  /** Disable copy and assignment of SDLViewers */
  SDLViewer(const SDLViewer&) = delete;
  void operator=(const SDLViewer&) = delete;

  /** Launch a new SDL window. */
  void launch() {
    if (event_thread_ == nullptr) {
      lock_ = SDL_CreateMutex();
      if (lock_ == nullptr)
        fprintf(stderr, "Unable to create mutex: %s\n", SDL_GetError());

      event_thread_ = SDL_CreateThread(SDLViewer::event_loop_wrapper, this);
      if (event_thread_ == nullptr)
        fprintf(stderr, "Unable to create thread: %s\n", SDL_GetError());
    }
  }

  /** Return window width. */
  int window_width() const {
    return window_width_;
  }

  /** Return window height. */
  int window_height() const {
    return window_height_;
  }

  /** Erase graphics. */
  void clear() {
    { safe_lock mutex(this);
      coords_.clear();
      colors_.clear();
      edges_.clear();
      label_.clear();
    }
    request_render();
  }

  /** Replace graphics with the points in @a g.
   * @pre The G type and @a g object must have the following properties:
   *   <ol><li>G::size_type and G::node_type are types.</li>
   *       <li>G::size_type g.num_nodes() returns the number of nodes.</li>
   *	     <li>G::node_type g.node(G::size_type i) returns a node object.</li>
   *       <li>Point g.node(i).position() returns the position of node i.</li>
   *   </ol>
   *
   * This function clears all edges, drawing only nodes. All nodes are drawn
   * as white. */
  template <typename G>
  void draw_graph_nodes(const G& g) {
    // Lock for data update
    { safe_lock mutex(this);

      // Convenience aliases
      using size_type = typename G::size_type;
      // using node_type = typename G::node_type;

      // Clear the data
      coords_.clear();
      edges_.clear();

      // Insert the nodes
      size_type num_nodes = g.num_nodes();
      for (size_type i = 0; i < num_nodes; ++i)
        coords_.push_back(g.node(i).position());

      // Set all nodes to be white
      colors_ = std::vector<Color>(coords_.size(), Color(1,1,1));
    }

    request_render();
  }

  /** Replace graphics with the points in @a g.
   * @pre The G type and @a g object must have the properties listed in
   *   draw_graph_nodes(), and in addition:
   *   <ol><li>G::edge_type is a type.</li>
   *       <li>G::size_type g.node(i).index() returns the node index.</li>
   *       <li>G::size_type g.num_edges() returns the number of edges.</li>
   *       <li>G::edge_type g.edge(G::size_type i) returns an edge object.</li>
   *       <li>G::node_type g.edge(i).node1() and node2() return node objs.</li>
   *   </ol>
   *
   * This function draws both nodes and edges. All nodes are drawn as
   * white. */
  template <typename G>
  void draw_graph(const G& g) {
    // Lock for data update
    { safe_lock mutex(this);

      // Convenience aliases
      using size_type = typename G::size_type;
      using node_type = typename G::node_type;
      using edge_type = typename G::edge_type;

      // Clear the data
      coords_.clear();
      edges_.clear();
      std::map<size_type, unsigned> nodemap;

      // Insert the nodes and record mapping
      size_type num_nodes = g.num_nodes();
      for (size_type i = 0; i < num_nodes; ++i) {
        node_type n = g.node(i);
        nodemap[n.index()] = coords_.size();
        coords_.push_back(n.position());
      }

      // Set all nodes to be white
      colors_ = std::vector<Color>(coords_.size(), Color(1,1,1));

      // Insert edges accoring to our mapping
      size_type num_edges = g.num_edges();
      for (size_type i = 0; i < num_edges; ++i) {
        edge_type e = g.edge(i);
        auto it1 = nodemap.find(e.node1().index());
        auto it2 = nodemap.find(e.node2().index());
        if (it1 != nodemap.end() && it2 != nodemap.end()) {
          edges_.push_back(it1->second);
          edges_.push_back(it2->second);
        }
      }
    }

    request_render();
  }

  /** Return an empty node map designed for the input graph.
   *
   * Node maps are passed to, and modified by, add_nodes() and add_edges(). */
  template <typename G>
  std::map<typename G::node_type, unsigned> empty_node_map(const G&) const {
    return std::map<typename G::node_type, unsigned>();
  }

  /** Add the nodes in the range [first, last) to the display.
   * @param[in,out] node_map Tracks node identities for use by add_edges().
   *    Create this argument by calling empty_node_map<Node>() for your
   *    node type.
   *
   * Uses white for color and node.position() for position. It's OK to add a
   * Node more than once. Second and subsequent adds update the existing
   * node's position. */
  template <typename InputIterator, typename Map>
  void add_nodes(InputIterator first, InputIterator last,
                 Map& node_map) {
    return add_nodes(first, last, DefaultColor(), DefaultPosition(), node_map);
  }

  /** Add the nodes in the range [first, last) to the display.
   * @param[in] color_function Returns a Color for each node.
   * @param[in,out] node_map Tracks node identities for use by add_edges().
   *    Create this argument by calling empty_node_map<Node>() for your
   *    node type.
   *
   * Uses node.position() for position. It's OK to add a Node more than once.
   * Second and subsequent adds update the existing node's position. */
  template <typename InputIterator, typename ColorFn, typename Map>
  void add_nodes(InputIterator first, InputIterator last,
                 ColorFn color_function, Map& node_map) {
    return add_nodes(first, last, color_function, DefaultPosition(), node_map);
  }

  /** Add the nodes in the range [first, last) to the display.
   * @param[in] color_function Returns a Color for each node.
   * @param[in] position_function Returns a Point for each node.
   * @param[in,out] node_map Tracks node identities for add_edges().
   *    Create this argument by calling empty_node_map<Node>() for your
   *    node type.
   *
   * It's OK to add a Node more than once. Second and subsequent adds update
   * the existing node's position. */
  template <typename InputIterator,
            typename ColorFn, typename PointFn, typename Map>
  void add_nodes(InputIterator first, InputIterator last,
                 ColorFn color_function, PointFn position_function,
                 Map& node_map) {
    // Lock for data update
    { safe_lock mutex(this);

      for (; first != last; ++first) {
        // Get node and record the index mapping
        auto n = *first;
        auto r = node_map.insert(typename Map::value_type(n,coords_.size()));
        if (r.second) {   // new node was inserted
          coords_.push_back(position_function(n));
          colors_.push_back(color_function(n));
        } else {          // node already exists and not updated
          unsigned index = r.first->second;
          coords_[index] = position_function(n);
          colors_[index] = color_function(n);
        }
      }
    }

    request_render();
  }

  /** Add the edges in the range [first, last) to the display.
   * @param[in] node_map Tracks node identities.
   *
   * The InputIterator forward iterator must return edge objects. Given an
   * edge object e, the calls e.node1() and e.node2() must return its
   * endpoints.
   *
   * Edges whose endpoints weren't previously added to the node_map by
   * add_nodes() are ignored. */
  template <typename InputIterator, typename Map>
  void add_edges(InputIterator first, InputIterator last, const Map& node_map) {
    // Lock for data update
    { safe_lock mutex(this);

      for (; first != last; ++first) {
        auto edge = *first;
        auto n1 = node_map.find(edge.node1());
        auto n2 = node_map.find(edge.node2());
        if (n1 != node_map.end() && n2 != node_map.end()) {
          edges_.push_back(n1->second);
          edges_.push_back(n2->second);
        }
      }
    }

    request_render();
  }

  /** Set a string label to display "green LCD" style. */
  void set_label(const std::string& str) {
    safe_lock mutex(this);
    if (str != label_) {
      label_ = str;
      request_render();
    }
  }

  /** Set a label to display "green LCD" style. */
  void set_label(double d) {
    std::stringstream ss;
    // At most 4 digits past the decimal point, and at most 4 trailing zeros
    ss << std::fixed << std::setprecision(4) << d;
    std::string s = ss.str();
    size_t dot;
    while ((dot = s.find('.')) != std::string::npos
           && dot + 5 < s.length()
           && s[s.length() - 1] == '0')
      s.erase(s.length() - 1);
    set_label(s);
  }

  /** Center view.
   *
   * Attempts to center the OpenGL view on the object by setting the new
   * viewpoint to the average of all nodes
   */
  void center_view() {
    { safe_lock mutex(this);

      // Sum the points in coords_
      Point c = std::accumulate(coords_.begin(), coords_.end(), Point(0));
      // Set the new view point
      camera_.view_point(c / coords_.size());
    }

    // Queue for rendering
    request_render();
  }

  /** Request that the screen update shortly. */
  void request_render() {
    if (!render_requested_) {
      render_requested_ = true;
      // User event to force a render
      SDL_Event render_event;
      render_event.type = SDL_USEREVENT;
      render_event.user.code = 0;
      render_event.user.data1 = nullptr;
      render_event.user.data2 = nullptr;
      // Thread safe push event
      SDL_PushEvent(&render_event);
    }
  }

 private:

  /** Initialize the SDL Window
   */
  void init() {
     // Set X11 driver
    std::string driver = "SDL_VIDEODRIVER=x11";
    SDL_putenv((char*) driver.c_str());

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      fprintf(stderr, "Init Error: %s\n", SDL_GetError());
      return;
    }

    window_width_ = 640;
    window_height_ = 480;

    int bits_per_pixel = 32;

    // Create the window and rendering surface
    surface_ = SDL_SetVideoMode(window_width_, window_height_, bits_per_pixel,
                                SDL_HWSURFACE|SDL_GL_DOUBLEBUFFER|SDL_OPENGL);
    if (surface_ == nullptr) {
      fprintf(stderr, "Display Error: %s\n", SDL_GetError());
      return;
    }

    // Background Color
    glClearColor(0.0, 0.0, 0.0, 1.0);
    // Initialize View
    glViewport(0, 0, window_width_, window_height_);

    // Set projection matrix
    camera_.set_perspective(60, window_width_/float(window_height_), 0.05, 1000);
    // Set up the camera view
    camera_.zoom_mag(2);
    //camera_.rotate_y(0.5);
    //camera_.rotate_x(0.5);

    // Point system
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // OpenGL Fog for better depth perception
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_DENSITY, 0.3);
  }

  /** Static event loop wrapper for thread creation
   *
   * @param[in] _viewer_ptr Addtional data added when thread is launched.
   *                        Interpreted as an SDLViewer*
   * @return 0 Required by thread launcher
   */
  static int event_loop_wrapper(void* _viewer_ptr) {
    reinterpret_cast<SDLViewer*>(_viewer_ptr)->event_loop();
    return 0;
  }

  /** Main Render Loop
   *
   * Executed by the event thread until interrupt or killed
   */
  void event_loop() {
    { safe_lock mutex(this);
      init();
    }

    SDL_Event event;
    while (SDL_WaitEvent(&event) >= 0) {
      safe_lock mutex(this);
      handle_event(event);
    }
  }

  /** Main Event Handler
   *
   * @param[in] _event The SDL mouse, keyboard, or screen event to be handled
   */
  void handle_event(SDL_Event event) {
    switch (event.type) {
      // The mouse moved over the screen
      case SDL_MOUSEMOTION: {
        // Left mouse button is down
        if (event.motion.state == SDL_BUTTON(1)) {
          camera_.rotate_x( 0.01*event.motion.yrel);
          camera_.rotate_y(-0.01*event.motion.xrel);
          request_render();
        }
        // Right mouse button is down
        if (event.motion.state == SDL_BUTTON(3)) {
          camera_.pan(-0.004*event.motion.xrel, 0.004*event.motion.yrel, 0);
          request_render();
        }
        // Avoid rendering on every mouse motion event
      } break;

      case SDL_MOUSEBUTTONDOWN: {
        // Wheel event
        if (event.button.button == SDL_BUTTON_WHEELUP)
          camera_.zoom(1.25);
        if (event.button.button == SDL_BUTTON_WHEELDOWN)
          camera_.zoom(0.8);

        request_render();
      } break;

      case SDL_KEYDOWN: {
        // Keyboard 'c' to center
        if (event.key.keysym.sym == SDLK_c)
          center_view();
        // Keyboard 'esc' to exit
        if (event.key.keysym.sym == SDLK_ESCAPE
            || event.key.keysym.sym == SDLK_q)
          exit(0);

        keyboard_signals_(event.key.keysym.sym);
        request_render();
      } break;

      case SDL_VIDEOEXPOSE:
      case SDL_USEREVENT: {
        // Render event
        render();
      } break;

        // Window 'x' to close
      case SDL_QUIT: {
        exit(0);
      } break;
    }
  }

  /** Print any outstanding OpenGL error to std::cerr. */
  void check_gl_error(const char* context = nullptr) {
    GLenum errCode = glGetError();;
    if (errCode != GL_NO_ERROR) {
      std::cerr << "OpenGL Error";
      if (context)
        std::cerr << " at " << context;
      std::cerr << ": " << errCode << "\n";
    }
  }

  /** Render an "LCD digit" specified by the on bits in @a segments.
   *
   * Renders to the rectangle [0,0,1,2]. */
  void render_lcd_segments(unsigned segments) {
    glBegin(GL_LINES);
    for (int h = 0; h < 3; ++h, segments >>= 3) {
      if (segments & 1) {
        glVertex2f(0, h);
        glVertex2f(1, h);
      }
      if (segments & 2) {
        glVertex2f(0, h);
        glVertex2f(0, h + 1);
      }
      if (segments & 4) {
        glVertex2f(1, h);
        glVertex2f(1, h + 1);
      }
    }
    glEnd();
  }

  /** Render a label in "Green LCD" style. Only knows digits, spaces, and '.'
   */
  void render_label() {
    static const GLfloat skewscalem[] = {
      8, 0, 0, 0, 1, 8, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1
    };
    static const unsigned digit_segments[] = {
      0167, 0044, 0153, 0155, 0074, 0135, 0137, 0144, 0177, 0175
    };

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);

    // Set both relevant matrices for 2D display. The projection matrix is
    // orthographic; the model view matrix is the identity.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, window_width_, 0, window_height_, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Draw in green with fat points & lines.
    glColor3f(0, .759, .437);
    glPointSize(3);
    glLineWidth(1.5);

    // Estimate width.
    double expected_width = 0;
    for (auto it = label_.begin(); it != label_.end(); ++it)
      expected_width += (*it == '.' ? 0.6 : (*it == ' ' ? 0.8 : 1.6));
    expected_width = roundf(std::max(expected_width * 8 + 10.0, 90.0));
    // Translate and skew.
    glTranslatef(window_width_ - expected_width + 0.375, 10.375, 0);
    glMultMatrixf(skewscalem);

    // Draw label.
    for (auto it = label_.begin(); it != label_.end(); ++it) {
      if (*it >= '0' && *it <= '9') {
        render_lcd_segments(digit_segments[*it - '0']);
        glTranslatef(1.6, 0, 0);
      } else if (*it == '.') {
        glBegin(GL_POINTS);
        glVertex2f(0, 0);
        glEnd();
        glTranslatef(0.6, 0, 0);
      } else if (*it == ' ') {
        glTranslatef(0.8, 0, 0);
      } else {
        render_lcd_segments(0001);
        glTranslatef(1.6, 0, 0);
      }
    }

    // Restore settings.
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    check_gl_error();
  }

  /** Render to the screen
   *
   * Double buffered rendering to the SDL Surface using OpenGL
   */
  void render() {
    safe_lock mutex(this);

    // Clear the screen and z-buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Construct the view matrix
    camera_.set_view();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Define the vertex interpreter
    glVertexPointer(Point::size(), gltype<typename Point::value_type>::value,
                    0, coords_.data());

    // Define the color interpreter
    glColorPointer(3, gltype<typename Color::value_type>::value,
                   0, colors_.data());

    // Draw the points
    glPointSize(1.5);
    glDrawArrays(GL_POINTS, 0, coords_.size());

    // Draw the lines
    glLineWidth(1);
    glDrawElements(GL_LINES, edges_.size(),
                   gltype<unsigned>::value, edges_.data());

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    // Draw the label
    if (!label_.empty())
      render_label();

    // Make visible
    SDL_GL_SwapBuffers();

    render_requested_ = false;
  }

}; // class SDLViewer

} // nsamespace CS207
#endif
