#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Object3D.h"
#include "vecmath.h"

// TODO implement this class
// So that the intersect function first transforms the ray
// Add more fields as necessary
class Transform: public Object3D
{
  public: 
    Transform(const Matrix4f &m, 
              Object3D *obj) 
    : _object(obj) 
    , w2o(m)
    , o2w(m.inverse())
    {
    }

    bool intersect(const Ray &r, float tmin, Hit &h) const override
    {
        // transform to object space
        Vector4f homO(r.getOrigin(), 1.0f), homD(r.getDirection(), 0.0f); // homogeneous coordinates
        Vector4f objO = o2w * homO, objD = o2w * homD;
        Ray objRay(objO.xyz(), objD.xyz()); // used in object space
        
        bool hasHit = _object->intersect(objRay, tmin, h);

        if (hasHit)
        {
            Vector4f objHitNorm(h.getNormal(), 0.0);
            Vector4f wHomHitNorm = o2w.transposed() * objHitNorm;
            h.setNormal(wHomHitNorm.xyz().normalized());
        }

        return hasHit;
    }

  protected:
    Object3D *_object; //un-transformed object	
    Matrix4f w2o; // transform from World to Object
    Matrix4f o2w; // inverse transform
};

#endif //TRANSFORM_H
