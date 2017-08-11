#include "Renderer.h"

#include "SceneParser.h"
#include "Image.h"
#include "Camera.h"

Renderer::Renderer(const std::string &sceneFilename)
: _sceneParser(sceneFilename)
{
    // TODO: implement

    // Parse the scene using SceneParser.
}

Renderer::~Renderer()
{
    // TODO: free up resources
}

void
Renderer::Render(int width,
                 int height,
                 const std::string &outputFilename,
                 float minDepth,
                 float maxDepth,
                 const std::string &depthFilename,
                 const std::string &normalFilename)
{
    // TODO: implement

    // Loop over each pixel in the image, shooting a ray
    // through that pixel and finding its intersection with
    // the scene. Write the color at the intersection to that
    // pixel in your output image.
    //
    // Use the Image class to write out the requested output files.
    Camera* camera = _sceneParser.getCamera();
    Group* group = _sceneParser.getGroup();
    Vector3f ambLight = _sceneParser.getAmbientLight();
    Vector3f backgroundColor = _sceneParser.getBackgroundColor();

    Image image(width, height);

    for (uint x = 0; x < width; ++x)
    {
        for (uint y = 0; y < height; ++y)
        {
            // Normalized Image Coordinate
            float xNIC = float(x) / width * 2.0 - 1.0;
            float yNIC = float(y) / height * 2.0 - 1.0;

            Ray ray = camera->generateRay(Vector2f(xNIC, yNIC));
            // std::cout << "X: " << x << ", Y: " << y << std::endl;
            // std::cout << "Ray: " << ray.getDirection() << std::endl;
            Hit hit;
            if (group->intersect(ray, 1e-7f, hit))
            {
                // Vector3f pixelColor(1.0, 0.0, 0.0);
                // Vector3f pixelColor(hit.getNormal());
                Vector3f p(ray.pointAtParameter(hit.getT()));
                Vector3f dirToLight, lightColor;
                float dist;
                Vector3f pixelColor(Vector3f::ZERO);

                Material* material = hit.getMaterial();
                for (int i = 0; i < _sceneParser.getNumLights(); ++i)
                {
                    _sceneParser.getLight(i)->getIllumination(p, dirToLight, lightColor, dist);
                    pixelColor += material->shade(ray, hit, dirToLight, lightColor);
                }

                pixelColor += ambLight;
                image.setPixel(x, y, pixelColor);
                // image.setPixel(x, y, hit.getNormal());
            }
            else
            {
                image.setPixel(x, y, backgroundColor);
            }
        }
    }

    image.savePNG(outputFilename);
}
