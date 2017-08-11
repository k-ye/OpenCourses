#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "Ray.h"
#include "Hit.h"
#include "Texture.h"
#include "Noise.h"

class Material
{
  public:
    Material(const Vector3f &diffuseColor, 
             const Vector3f &specularColor = Vector3f::ZERO, 
             float shininess = 0,
             float refractionIndex = 0) :
        _diffuseColor(diffuseColor),
        _specularColor(specularColor),
        _shininess(shininess),
        _refractionIndex(refractionIndex)
    {
    }

    virtual ~Material()
    {
    }

    const Vector3f & getDiffuseColor() const {
        return _diffuseColor;
    }

    const Vector3f & getSpecularColor() const {
        return _specularColor;
    }

    float getRefractionIndex() const {
        return _refractionIndex;
    }

    void setNoise(const Noise &n) {
        _noise = n;
    }

    Vector3f getShadeDiffuseColor(const Ray& ray, const Hit& hit) const
    {
        Vector3f kd;
        if (_noise.isValid()) {
            kd = _noise.getColor(ray.getOrigin() + ray.getDirection() * hit.getT());
        } else if (_texture.isValid() && hit.hasTex()) {
            const Vector2f &texCoord = hit.getTexCoord();
            kd = _texture.getTexel(texCoord[0], texCoord[1]);
        } else {
            kd = _diffuseColor;
        }
        return kd;
    }

    Vector3f shade(const Ray &ray, 
                   const Hit &hit,
                   const Vector3f &dirToLight, 
                   const Vector3f &lightColor)
    {
        Vector3f kd;
        if (_noise.isValid()) {
            kd = _noise.getColor(ray.getOrigin() + ray.getDirection() * hit.getT());
        } else if (_texture.isValid() && hit.hasTex()) {
            const Vector2f &texCoord = hit.getTexCoord();
            kd = _texture.getTexel(texCoord[0], texCoord[1]);
        } else {
            kd = _diffuseColor;
        }

        Vector3f n = hit.getNormal().normalized();
        Vector3f color = clampedDot(dirToLight, n) * pointwiseDot(lightColor, kd);

        return color;
    }

    void loadTexture(const std::string &filename)
    {
        _texture.load(filename);
    }

    static Vector3f pointwiseDot(const Vector3f &v1, const Vector3f &v2) {
        return Vector3f(v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2]);
    }

protected:
    static float clampedDot(const Vector3f &L, const Vector3f &N) {
        float d = Vector3f::dot(L,N);
        return (d > 0) ? d : 0; 
    }

    Vector3f _diffuseColor;
    Vector3f _specularColor;
    float _shininess;
    float _refractionIndex;
    Texture _texture;
    Noise _noise;
};

#endif // MATERIAL_H
