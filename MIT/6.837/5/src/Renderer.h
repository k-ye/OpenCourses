#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include "SceneParser.h"

class ArgParser;
class Hit;
class Vector3f;
class Ray;

class Renderer
{
  public:
    // Instantiates a renderer for the given scene.
    Renderer(const ArgParser &args);

    ~Renderer();

    // If depth_file is empty, a depth image is not generated.
    // Same for normals_file and the normal image.
    void Render();

  private:
    // IMPLEMENT
    Vector3f traceRay(const Ray &ray, float tmin, int bounces, 
                      float refr_index, Hit &hit) const;
    Vector3f computeShadowColor(Ray &ray, float distanceToLight) const;

    const ArgParser &_args;
    SceneParser _scene;
};

#endif // RENDERER_H
