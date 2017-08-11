#ifndef TEXTURE_H
#define TEXTURE_H

#include "Image.h"
#include "Vector3f.h"

#include <string>

// Helper class that stores a texture and faciliates lookup
class Texture {
  public:
    Texture();

    ~Texture();

    bool isValid() const {
        return _image != NULL;
    }

    void load(const std::string &filename);

    // The UV (x, y) coordinates are assumed to be normalized between 0 and 1.
    // The resulting look up is box filtered in the local 2x2 neighborhood.
    Vector3f getTexel(float x, float y) const;

  private:
    template<typename T>
    static T 
    clamp(const T &v, const T &lower_range, const T &upper_range)
    {
        if (v < lower_range) {
            return lower_range;
        } else if (v >  upper_range) {
            return upper_range;
        } else {
            return v;
        }
    }

    const Vector3f & getPixel(int x, int y) const {
        x = clamp(x, 0, _image->getWidth() - 1);
        y = clamp(y, 0, _image->getHeight() - 1);
        return _image->getPixel(x, y);
    }

    Image *_image;
};

#endif
