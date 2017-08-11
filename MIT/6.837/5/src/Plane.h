#ifndef PLANE_H
#define PLANE_H

#include "Object3D.h"

#include <vecmath.h>
#include <cmath>

// TODO: Implement Plane representing an infinite plane
// Choose your representation, add more fields and fill in the functions
class Plane: public Object3D
{
  public:
    Plane(const Vector3f &normal, float d, Material *m)
    : Object3D(m)
    , _normal(normal)
    , _d(d) { }
    
    bool intersect(const Ray &r, float tmin, Hit &h) const override
    {
        float thit = _d - Vector3f::dot(_normal, r.getOrigin());
        thit /= Vector3f::dot(_normal, r.getDirection());

        if (thit >= tmin && thit <= h.getT())
        {
            // update t, material and normal
            h.set(thit, material, _normal);
            return true;
        }
        return false;  // TODO: implement
    }

 private:
    Vector3f _normal;
    float _d;
};
#endif //PLANE_H


