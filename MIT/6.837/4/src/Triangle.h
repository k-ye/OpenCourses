#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Object3D.h"
#include <vecmath.h>
#include <cmath>
#include <iostream>

// TODO: implement this class.
class Triangle: public Object3D
{
  public:
    Triangle(const Vector3f &a, 
             const Vector3f &b, 
             const Vector3f &c, 
             const Vector3f &na,
             const Vector3f &nb,
             const Vector3f &nc,
             Material *m) :
        Object3D(m),
        _hasTex(false)
    {
        // TODO: implement
        _vertices[0] = a;
        _vertices[1] = b;
        _vertices[2] = c;

        _normals[0] = na;
        _normals[1] = nb;
        _normals[2] = nc;
    }

    virtual bool intersect(const Ray &ray, float tmin, Hit &hit) const {
        Vector3f a_b = _vertices[0] - _vertices[1];
        Vector3f a_c = _vertices[0] - _vertices[2];
        Vector3f d   = ray.getDirection();
        Vector3f a_o = _vertices[0] - ray.getOrigin();

        Matrix3f A(a_b, a_c, d, true);
        float detA_recpr = 1.0 / A.determinant();

        float beta = detA_recpr * Matrix3f(a_o, a_c, d, true).determinant();
        float gamma = detA_recpr * Matrix3f(a_b, a_o, d, true).determinant();

        if (0 <= beta && 0 <= gamma && beta + gamma <= 1.)
        {
            float thit = detA_recpr * Matrix3f(a_b, a_c, a_o, true).determinant();
            if (thit >= tmin && thit <= hit.getT())
            {
                float alpha = 1.0 - beta - gamma;
                // assert(alpha >= 0.);

                Vector3f hitNorm = alpha * _normals[0] + beta * _normals[1] + gamma * _normals[2];
                hit.set(thit, material, hitNorm);

                if (hasTex())
                {
                    Vector2f texCoord = alpha * _texCoords[0] + beta * _texCoords[1] + gamma * _texCoords[2];
                    hit.setTexCoord(texCoord);
                }
                return true;
            }
        }
    
        return false;  // TODO: implement
    }

    void setTex(const Vector2f &uva,
                const Vector2f &uvb,
                const Vector2f &uvc)
    {
        _texCoords[0] = uva;
        _texCoords[1] = uvb;
        _texCoords[2] = uvc;
        _hasTex = true;
    }

    bool hasTex() const {
        return _hasTex;
    }

  private:
    bool _hasTex;
    Vector3f _vertices[3];
    Vector3f _normals[3];
    Vector2f _texCoords[3];
};

#endif //TRIANGLE_H
