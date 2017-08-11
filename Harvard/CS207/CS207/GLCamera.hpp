#ifndef CS207_GLCAMERA_HPP
#define CS207_GLCAMERA_HPP

/**
 * @file GLCamera.hpp
 * Provides a wrapper for the gluLookAt command
 *
 * @brief Keeps track of orientation, zoom, and view point to compute
 * the correct gluLookAt command. This provides more intuitive commands
 * such as pan, zoom, rotate, etc.
 */

#include <cmath>

#include <SDL/SDL_opengl.h>

#include "Point.hpp"


namespace CS207 {

/**
 * View point with vectors defining the local axis
 *
 *              upV
 *               |  rightV
 *               | /
 *               |/
 *               * point
 *                \
 *                 \
 *                  eyeV
 */
class GLCamera {
 private:

  float dist;

  Point point;
  Point upV;
  Point rightV;
  Point eyeV;

 public:

  /** Default Constructor */
  GLCamera()
    : dist(1), point(0, 0, 0), upV(0, 0, 1), rightV(0, 1, 0), eyeV(1, 0, 0) {
  }

  /** Set the OpenGL ModelView matrix to the Camera's orientation and axis
   * Replaces gluLookAt
   */
  inline void set_view() const {
    Point eye = point + eyeV * dist;

    float mat[16];
    mat[ 0] =  rightV.x;
    mat[ 4] =  rightV.y;
    mat[ 8] =  rightV.z;
    mat[12] = -inner_prod(rightV,eye);
    //------------------
    mat[ 1] =  upV.x;
    mat[ 5] =  upV.y;
    mat[ 9] =  upV.z;
    mat[13] = -inner_prod(upV,eye);
    //------------------
    mat[ 2] =  eyeV.x;
    mat[ 6] =  eyeV.y;
    mat[10] =  eyeV.z;
    mat[14] = -inner_prod(eyeV,eye);
    //------------------
    mat[ 3] = 0.0;
    mat[ 7] = 0.0;
    mat[11] = 0.0;
    mat[15] = 1.0;

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(mat);
  }

  /** Set up a perspective projection matrix
   * Replaces gluPerspective
   *
   * @param[in] fovy Field of view angle, in degrees, in the y direction.
   * @param[in] aspect Aspect ratio (width/height) defines the FOV in x
   * @param[in] zNear Distance from the viewer to the near clipping plane >= 0
   * @param[in] zFar Distance from the viewer to the far clipping plane >= 0
   */
  void set_perspective(double fovy, double aspect,
                       double znear, double zfar) const {
    GLdouble ymax = znear * tan(fovy * M_PI / 360.0);
    GLdouble ymin = -ymax;
    GLdouble xmin = ymin * aspect;
    GLdouble xmax = ymax * aspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(xmin, xmax, ymin, ymax, znear, zfar);
  }

  /** Set up an orthographic projection matrix
   * Replaces gluOrtho2D
   *
   * @param[in] left,right Coordinate for the left/right clipping planes
   * @param[in] bottom,top Coordinates for the bottom/top clipping planes
   */
  void set_ortho(double left, double right, double bottom, double top,
                 double znear = -1, double zfar = 1) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, znear, zfar);
  }

  /** Change the point we are viewing with respect to the local axes
   */
  inline void pan(float x, float y, float z) {
    point += rightV * x;
    point += upV    * y;
    point += eyeV   * z;
  }

  /** Set the new view point. The camera now looks at (x,y,z)
   */
  inline void view_point(float x, float y, float z) {
    point = Point(x, y, z);
  }
  /** @overload */
  inline void view_point(const Point& p) {
    point = p;
  }

  /** Rotate the view about the local x-axis
   */
  inline void rotate_x(float angle) {
    // Rotate eye vector about the right vector
    eyeV = eyeV * cosf(angle) + upV * sinf(angle);
    // Normalize for numerical stability
    eyeV /= norm(eyeV);

    // Compute the new up vector by cross product
    upV = cross(eyeV,rightV);
    // Normalize for numerical stability
    upV /= norm(upV);
  }

  /** Rotate the view about the local y-axis
   */
  inline void rotate_y(float angle) {
    // Rotate eye about the up vector
    eyeV = eyeV * cosf(angle) + rightV * sinf(angle);
    // Normalize for numerical stability
    eyeV /= norm(eyeV);

    // Compute the new right vector by cross product
    rightV = cross(upV,eyeV);
    // Normalize for numerical stability
    rightV /= norm(rightV);
  }

  /** Zoom by a scale factor
   */
  inline void zoom(float d) {
    dist *= d;
  }

  /** Set the zoom magnitude
   */
  inline void zoom_mag(float d) {
    dist = d;
  }
}; // end GLCamera

} // end namespace CS207
#endif
