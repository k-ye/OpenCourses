#ifndef CUBEMAP_H
#define CUBEMAP_H

#include "Texture.h"
#include "Vector3f.h"

class CubeMap {
  public:
    enum FACE {
        LEFT,
        RIGHT,
        UP,
        DOWN,
        FRONT,
        BACK,
    };

    // Assumes a directory containing {left,right,up,down,front,back}.png
    CubeMap(const std::string &directory);

    // Returns color for given directory
    Vector3f getTexel(const Vector3f &direction) const;

  private:
    Texture _t[6];
};

#endif
