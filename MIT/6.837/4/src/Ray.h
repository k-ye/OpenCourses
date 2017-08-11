#ifndef RAY_H
#define RAY_H

#include "Vector3f.h"

#include <cassert>
#include <iostream>

// Ray class mostly copied from Peter Shirley and Keith Morley
class Ray
{
  public:
    Ray(const Vector3f &orig, const Vector3f &dir) :
        _origin(orig),
        _direction(dir)
    {
    }

    const Vector3f & getOrigin() const {
        return _origin;
    }

    const Vector3f & getDirection() const {
        return _direction;
    }

    Vector3f pointAtParameter(float t) const {
        return _origin + _direction * t;
    }

  private:
    Vector3f _origin;
    Vector3f _direction;

};

inline std::ostream &
operator<<(std::ostream &os, const Ray &r)
{
    os << "Ray <" << r.getOrigin() << ", " << r.getDirection() << ">";
    return os;
}

#endif // RAY_H
