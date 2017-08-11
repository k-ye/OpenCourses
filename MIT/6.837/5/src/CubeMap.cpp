#include "CubeMap.h"

#include <cmath>
#include <string>

CubeMap::CubeMap(const std::string &directory)
{
    std::string side[6] = { "left", "right", "up", "down", "front", "back" };
    for(int ii = 0 ;ii<6;ii++){
        std::string filename = directory + "/" + side[ii] + ".png";
        _t[ii].load(filename);
    }
}

Vector3f
CubeMap::getTexel(const Vector3f &direction) const
{
    Vector3f dir = direction.normalized();
    Vector3f outputColor(0.0f, 0.0f, 0.0f);
    if ((std::abs(dir[0]) >= std::abs(dir[1])) && (std::abs(dir[0]) >= std::abs(dir[2]))) {
        if (dir[0] > 0.0f) {
            outputColor = 
                _t[RIGHT].getTexel((dir[2] / dir[0] + 1.0f) * 0.5f, 
                                   (dir[1] / dir[0] + 1.0f) * 0.5f);
        } else if (dir[0] < 0.0f) {
            outputColor = 
                _t[LEFT].getTexel(       (dir[2] / dir[0] + 1.0f) * 0.5f, 
                                  1.0f - (dir[1] / dir[0] + 1.0f) * 0.5f);
        }
    } else if ((std::abs(dir[1]) >= std::abs(dir[0])) && (std::abs(dir[1]) >= std::abs(dir[2]))) {
        if (dir[1] > 0.0f) {
            outputColor = 
                _t[UP].getTexel((dir[0] / dir[1] + 1.0f) * 0.5f, 
                                (dir[2] / dir[1] + 1.0f) * 0.5f);
        } else if (dir[1] < 0.0f) {
            outputColor = 
                _t[DOWN].getTexel(1.0f - (dir[0] / dir[1] + 1.0f) * 0.5f, 
                                  1.0f - (dir[2] / dir[1] + 1.0f) * 0.5f);
        }
    } else if ((std::abs(dir[2]) >= std::abs(dir[0])) && (std::abs(dir[2]) >= std::abs(dir[1]))) {
        if (dir[2] > 0.0f) {
            outputColor = 
                _t[FRONT].getTexel(1.0f - (dir[0] / dir[2] + 1.0f) * 0.5f, 
                                          (dir[1] / dir[2] + 1.0f) * 0.5f);
        } else if (dir[2] < 0.0f) {
            outputColor = 
                _t[BACK].getTexel(       (dir[0] / dir[2] + 1.0f) * 0.5f, 
                                  1.0f - (dir[1] / dir[2] + 1.0f) * 0.5f);
        }
    }

    return outputColor;
}
