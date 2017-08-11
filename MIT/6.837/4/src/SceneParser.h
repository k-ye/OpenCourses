#ifndef SCENE_PARSER_H
#define SCENE_PARSER_H

#include <cassert>
#include <vector>
#include <vecmath.h>

#include "SceneParser.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Object3D.h"
#include "Mesh.h"
#include "Group.h"
#include "Sphere.h"
#include "Plane.h"
#include "Triangle.h"
#include "Transform.h"

#define MAX_PARSER_TOKEN_LENGTH 100

class SceneParser
{
  public:
    SceneParser(const std::string &filename);
    ~SceneParser();

    Camera * getCamera() const {
        return _camera;
    }

    const Vector3f & getBackgroundColor() const {
        return _background_color;
    }

    const Vector3f & getAmbientLight() const {
        return _ambient_light;
    }

    int getNumLights() const {
        return _num_lights;
    }

    Light * getLight(int i) const {
        assert(i >= 0 && i < _num_lights);
        return _lights[i];
    }

    int getNumMaterials() const {
        return _num_materials;
    }

    Material * getMaterial(int i) const {
        assert(i >= 0 && i < _num_materials);
        return _materials[i];
    }

    Group * getGroup() const {
        return _group;
    }

  private:
    void parseFile();
    void parsePerspectiveCamera();
    void parseBackground();
    void parseLights();
    Light * parseDirectionalLight();
    Light * parsePointLight();
    void parseMaterials();
    Material * parseMaterial();

    Object3D * parseObject(char token[MAX_PARSER_TOKEN_LENGTH]);
    Group * parseGroup();
    Sphere * parseSphere();
    Plane * parsePlane();
    Triangle * parseTriangle();
    Mesh * parseTriangleMesh();
    Transform * parseTransform();

    int getToken(char token[MAX_PARSER_TOKEN_LENGTH]);
    Vector3f readVector3f();
    Vector2f readVec2f();
    float readFloat();
    int readInt();

    FILE * _file;
    Camera * _camera;
    Vector3f _background_color;
    Vector3f _ambient_light;
    int _num_lights;
    std::vector<Light*> _lights;
    int _num_materials;
    std::vector<Material*> _materials;
    Material * _current_material;
    Group * _group;
};

#endif // SCENE_PARSER_H
