#ifndef SPHERE_H
#define SPHERE_H

#include "Object3D.h"
#include <vecmath.h>
#include <cmath>

using namespace std;
///TODO:
///Implement functions and add more fields as necessary
class Sphere: public Object3D
{
public:
  Sphere()
  : Object3D(NULL)
  , _center()
  , _radius(1.0) { 
    //unit ball at the center
  }

  Sphere(Vector3f center, float radius, Material* material )
  : Object3D(material)
  , _center(center)
  , _radius(radius) {

  }
  

  ~Sphere(){}

  bool intersect(const Ray &r, float tmin, Hit &h) const override {
    Vector3f e = r.getOrigin();
    Vector3f d = r.getDirection();
    Vector3f e_minus_c = e - _center;

    float A = Vector3f::dot(d, d); // A > 0
    float B = 2 * Vector3f::dot(d, e_minus_c);
    float C = Vector3f::dot(e_minus_c, e_minus_c) - _radius * _radius;

    float discriminant = B * B - 4 * A * C;
    if (discriminant >= 0.f) 
    {
      float A_recipr = 0.5 / A;
      float discrSqrt = sqrt(discriminant);
      float t1 = A_recipr * (-B - discrSqrt);
      float t2 = A_recipr * (-B + discrSqrt);

      assert(t1 <= t2);
      if (t1 >= tmin && t1 <= h.getT()) 
      {
        // update t, material and normal
        Vector3f hitP = r.pointAtParameter(t1);
        Vector3f hitNorm = (hitP - _center) / _radius;
        h.set(t1, material, hitNorm);
        return true;
      }
      else if (t2 >= tmin && t2 <= h.getT())
      {
        // update t, material and normal
        Vector3f hitP = r.pointAtParameter(t2);
        Vector3f hitNorm = (hitP - _center) / _radius;
        h.set(t2, material, hitNorm);
        return true;
      }
    }
    return false;
  }

protected:
  Vector3f _center;
  float _radius;

};


#endif
