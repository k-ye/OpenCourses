#ifndef MESH_H
#define MESH_H

#include "Object3D.h"
#include "Triangle.h"
#include "Vector2f.h"
#include "Vector3f.h"

#include <array>
#include <vector>

class Mesh : public Object3D {
  public:
    Mesh(const std::string &filename, Material *m);

    virtual bool intersect(const Ray &r, float tmin, Hit &h) const;

  private:
    // By default counterclockwise winding is front face
    struct ObjTriangle {
        ObjTriangle() :
            x{ { 0, 0, 0 } },
            texID{ { 0, 0, 0 } }
        {
        }

        int & operator[](int i) {
            return x[i];
        }

        std::array<int, 3> x;
        std::array<int, 3> texID;
    };

    std::vector<Triangle> _triangles;
};

#endif
