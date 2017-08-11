#ifndef CAMERA_H
#define CAMERA_H

#include "Ray.h"

#include <vecmath.h>
#include <float.h>
#include <cmath>

class Camera
{
  public:
    virtual ~Camera() {}

    // Generate rays for each screen-space coordinate
    virtual Ray generateRay(const Vector2f &point) = 0; 

    virtual float getTMin() const = 0; 
};

/// TODO: Implement Perspective camera
/// Fill in functions and add more fields if necessary
class PerspectiveCamera : public Camera
{
  public:
    PerspectiveCamera(const Vector3f &center, 
                      const Vector3f &direction,
                      const Vector3f &up, 
                      float angle) :
        _center(center),
        _direction(direction.normalized()),
        _up(up),
        _angle(angle),
        _D(0.0f)
    {
        // TODO: implement
        _horizontal = Vector3f::cross(_direction, _up).normalized();
        _up = Vector3f::cross(_horizontal, _direction).normalized();

        _D = 1.0f / tan(angle * 0.5f); // angle is already in Radian
    }

    virtual Ray generateRay(const Vector2f &point)
    {
        // TODO: implement
        float x = point.x(), y = point.y();
        Vector3f dir = x * _horizontal + y * _up + _D * _direction;
        dir.normalize();

        return Ray(_center, dir);  
    }

    virtual float getTMin() const 
    { 
        return 0.0f;
    }

  private:
    Vector3f _center; 
    Vector3f _direction;
    Vector3f _up;
    float _angle;
    float _D;
    Vector3f _horizontal;
};

#endif //CAMERA_H
