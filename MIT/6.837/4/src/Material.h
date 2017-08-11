#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "Hit.h"
#include "Ray.h"
#include "Texture.h"

// TODO: Implement Shade function that uses ambient, diffuse, specular and texture
class Material
{
  public:
    Material(const Vector3f &diffuseColor,
             const Vector3f &specularColor = Vector3f::ZERO, 
             float shininess = 0) :
        _diffuseColor(diffuseColor),
        _specularColor(specularColor), 
        _shininess(shininess)
    {
    }

    virtual ~Material()
    {
    }

    virtual const Vector3f & getDiffuseColor() const 
    { 
        return _diffuseColor;
    }

    Vector3f shade(const Ray &ray, 
                   const Hit &hit,
                   const Vector3f &dirToLight, 
                   const Vector3f &lightColor) 
    {
        // TODO: implement
        Vector3f L(dirToLight.normalized()), pNorm(hit.getNormal().normalized());
        // ideal diffuse shaing
        float cost = Vector3f::dot(pNorm, L);
        if (cost < 0) cost = 0.0f;

        Vector3f k_d = (hit.hasTex() && _texture.isValid())
                        ? _texture.getTexel(hit.getTexCoord().x(), hit.getTexCoord().y()) 
                        : _diffuseColor;
        Vector3f color = k_d * lightColor * cost;

        // specular reflection
        Vector3f R = ray.getDirection(); // this is the opposite direction of what actual R should be
        R = R - 2. * Vector3f::dot(R, pNorm) * pNorm;
        R.normalize();

        float cs = Vector3f::dot(L, R);
        cs = cs < 0.0 ? 0.0 : pow(cs, _shininess);

        color += _specularColor * lightColor * cs;
        return color;
    }

    void loadTexture(const std::string &filename)
    {
        _texture.load(filename);
    }

  private:
    Vector3f _diffuseColor;
    Vector3f _specularColor;
    float _shininess;
    Texture _texture;
};

#endif // MATERIAL_H
