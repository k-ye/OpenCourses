#ifndef LIGHT_H
#define LIGHT_H

#include <Vector3f.h>

#include "Object3D.h"

#include <limits>

class Light
{
  public:
    virtual ~Light()
    {
    }

    virtual void getIllumination(const Vector3f &p, 
                                 Vector3f &dir, 
                                 Vector3f &col, 
                                 float &distanceToLight) const = 0;
};

class DirectionalLight : public Light
{
  public:
    DirectionalLight(const Vector3f &d, const Vector3f &c) :
        _direction(d.normalized()),
        _color(c)
    {
    }

    ~DirectionalLight()
    {
    }

    /// @param p unsed in this function
    /// @param distanceToLight not well defined because it's not a point light
    virtual void getIllumination(const Vector3f &p, 
                                 Vector3f &dir, 
                                 Vector3f &col, 
                                 float &distanceToLight) const
    {
        // the direction to the light is the opposite of the
        // direction of the directional light source
        dir = -_direction;
        col = _color;
        distanceToLight = std::numeric_limits<float>::max();
    }

  private:
    Vector3f _direction;
    Vector3f _color;
};

class PointLight : public Light
{
  public:
    PointLight(const Vector3f &p, const Vector3f &c) :
        _position(p),
        _color(c)
    {
    }

    ~PointLight()
    {
    }

    virtual void getIllumination(const Vector3f &p, 
                                 Vector3f &dir, 
                                 Vector3f &col, 
                                 float &distanceToLight) const
    {
        // the direction to the light is the opposite of the
        // direction of the directional light source
        dir = (_position - p);
        distanceToLight = dir.abs();
        dir = dir / distanceToLight;
        col = _color;
    }

  private:
    Vector3f _position;
    Vector3f _color;
};

#endif // LIGHT_H
