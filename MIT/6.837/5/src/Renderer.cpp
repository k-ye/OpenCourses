#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Hit.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>
#include <iostream>
#include <cstdlib>
#include <vector>

const float EPSILON = 1e-4f;

Renderer::Renderer(const ArgParser &args) :
    _args(args),
    _scene(args.input_file)
{
}

Renderer::~Renderer()
{
}

// IMPLEMENT THESE FUNCTIONS
// These function definitions are mere suggestions. Change them as you like.
Vector3f 
mirrorDirection(const Vector3f &normal, const Vector3f &incoming)
{
    // TODO: IMPLEMENT
    Vector3f norm = normal.normalized();
    Vector3f mirror = incoming - 2.0f * Vector3f::dot(incoming, norm) * norm;
    return mirror;
}

bool
transmittedDirection(const Vector3f &normal, 
                     const Vector3f &incoming, 
                     float index_i,
                     float index_t, 
                     Vector3f &transmitted)
{
    // TODO: IMPLEMENT
    float determinant = 1.0 - pow(Vector3f::dot(normal, incoming), 2);
    determinant = determinant * pow(index_i / index_t, 2);
    determinant = 1.0 - determinant;

    if (determinant < 0) return false;

    transmitted = incoming - Vector3f::dot(normal, incoming) * normal;
    transmitted *= (index_i / index_t);
    transmitted -= normal * sqrt(determinant);
    return true;
}

float randomNumber() 
{
    return (float) rand() / RAND_MAX - 0.5f;
}

void
Renderer::Render()
{
    // TODO: IMPLEMENT 

    // This is the main loop that should be based on your assignment 4 main
    // loop. You will need to modify it to switch from ray casting to ray
    // tracing, to cast shadows and anti-aliasing (via jittering and
    // filtering).
    Camera* camera = _scene.getCamera();
    Group* group = _scene.getGroup();

    int width = _args.width, height = _args.height;
    Image image(width, height);
    // A lot of redundant code, don't want to refactorize any more...
    if (_args.jitter)
    {
        int superWidth = width * 3, superHeight = height * 3;

        std::vector< std::vector<Vector3f> > jitterImage(superHeight, std::vector<Vector3f>(superWidth, Vector3f::ZERO));
        std::vector< std::vector<Vector3f> > filterImage(superHeight, std::vector<Vector3f>(superWidth, Vector3f::ZERO));
        
        // super sampling
        for (uint y = 0; y < superHeight; ++y)
        {
            for (uint x = 0; x < superWidth; ++x)
            {
                float xNIC = (float(x) + randomNumber()) / (superWidth - 1) * 2.0f - 1.0f;
                float yNIC = (float(y) + randomNumber()) / (superHeight - 1) * 2.0f - 1.0f;

                Ray ray = camera->generateRay(Vector2f(xNIC, yNIC));
                Hit hit;
                Vector3f pixelColor = traceRay(ray, EPSILON, 0, 1.0f, hit);

                jitterImage[y][x] = pixelColor;
            }

            if (y % 100 == 0)
            {
                std::cout << "ray traced for " << y << " slices" << std::endl;
            }
        }

        if (_args.filter)
        {
            float gaussianKernel[] = {0.1201f, 0.2339f, 0.2931f, 0.2339f, 0.1201f};

            // Horizontal filtering
            for (uint y = 0; y < superHeight; ++y)
            {
                for (uint x = 0; x < superWidth; ++x)
                {
                    int maskedIndex;
                    for (uint k = 0; k < 5; ++k)
                    {
                        maskedIndex = x + k - 2;
                        if (maskedIndex < 0) maskedIndex = 0;
                        else if (maskedIndex >= superWidth) maskedIndex = superWidth - 1;

                        filterImage[y][x] += gaussianKernel[k] * jitterImage[y][maskedIndex];
                    }
                }
            }
            jitterImage.swap(filterImage);

            // Verticle filtering
            for (uint y = 0; y < superHeight; ++y)
            {
                for (uint x = 0; x < superWidth; ++x)
                {
                    int maskedIndex;
                    filterImage[y][x] = Vector3f::ZERO;
                    for (uint k = 0; k < 5; ++k)
                    {
                        maskedIndex = y + k - 2;
                        if (maskedIndex < 0) maskedIndex = 0;
                        else if (maskedIndex >= superHeight) maskedIndex = superHeight - 1;
                        
                        filterImage[y][x] += gaussianKernel[k] * jitterImage[maskedIndex][x];
                    }
                }
            }
        }
        else
        {
            jitterImage.swap(filterImage);
        }

        // down sampling
        for (uint y = 0; y < height; ++y)
        {
            for (uint x = 0; x < width; ++x)
            {
                Vector3f pixelColor(Vector3f::ZERO);
                for (uint j = 0; j < 3; ++j)
                {
                    for (uint i = 0; i < 3; ++i)
                    {
                        pixelColor += filterImage[3 * y + j][3 * x + i];
                    }
                }
                pixelColor *= 1.0f / 9.0f;
                image.setPixel(x, y, pixelColor);
            }
        }
    }
    else
    {
        for (uint x = 0; x < width; ++x)
        {
            for (uint y = 0; y < height; ++y)
            {
                // Normalized Image Coordinate
                float xNIC = float(x) / (width - 1) * 2.0 - 1.0;
                float yNIC = float(y) / (height - 1) * 2.0 - 1.0;

                Ray ray = camera->generateRay(Vector2f(xNIC, yNIC));
                Hit hit;
                Vector3f pixelColor = traceRay(ray, EPSILON, 0, 1.0f, hit);
                image.setPixel(x, y, pixelColor);
            }
        }
    }
    image.savePNG(_args.output_file);

}

Vector3f
Renderer::traceRay(const Ray &ray, 
                   float tmin, 
                   int bounces,
                   float refr_index, 
                   Hit &hit) const
{
    // TODO: IMPLEMENT 

    Vector3f traceColor(Vector3f::ZERO);
    if (bounces > _args.bounces) return traceColor;

    hit = Hit(std::numeric_limits<float>::max(), NULL, Vector3f(0, 0, 0));
    Group* group = _scene.getGroup();
    
    if (group->intersect(ray, tmin, hit))
    {
        Vector3f hitPt(ray.pointAtParameter(hit.getT()));
        Vector3f hitNorm(hit.getNormal().normalized());
        Material* material = hit.getMaterial();

        // traceColor = material->getDiffuseColor() * _scene.getAmbientLight();
        traceColor = material->getShadeDiffuseColor(ray, hit) * _scene.getAmbientLight();
        // Recursive reflection and refraction comes before shading because we need
        // to first detect if we're inside the object. If it is, we will then needd
        // to flip the normal direction.
        if (bounces < _args.bounces)
        {
            // Detect whether we are inside the object.
            bool insideMaterial = Vector3f::dot(ray.getDirection(), hitNorm) > 0;
            float refr_t_index = material->getRefractionIndex();
            if (insideMaterial)
            {
                // the ray is shooting from some place inside the material out to the air
                refr_t_index = 1.0f;
                hitNorm = -hitNorm;
            }

            Hit forcedHit; // this is necessaary as the traceRay() requires a Hit object.
            // recursive reflection
            Ray reflectRay(hitPt, mirrorDirection(hitNorm, ray.getDirection()));
            Vector3f recReflectColor = traceRay(reflectRay, tmin, bounces + 1, refr_index, forcedHit);
            // recursive refraction
            Vector3f transmitted(ray.getDirection());
            // refr_t_index > 0 means this is a transparent material
            if (fabs(refr_t_index) > EPSILON && 
                transmittedDirection(hitNorm, ray.getDirection(), refr_index, refr_t_index, transmitted))
            {
                Ray refractRay(hitPt, transmitted);
                Vector3f recRefractColor = traceRay(refractRay, tmin, bounces + 1, refr_t_index, forcedHit);

                float R0 = pow((refr_t_index - refr_index) / (refr_t_index + refr_index), 2);
                // Blend two colors
                float c = refr_index > refr_t_index ? 
                            fabs(Vector3f::dot(transmitted.normalized(), hitNorm)) :
                            fabs(Vector3f::dot(ray.getDirection().normalized(), hitNorm));
                float R = R0 + (1.0f - R0) * pow(1.0f - c, 5);

                traceColor += R * recReflectColor * material->getSpecularColor();
                traceColor += (1.0f - R) * recRefractColor * material->getSpecularColor();
            }
            else
            {
                traceColor += recReflectColor * material->getSpecularColor();
            }
        }

        Vector3f dirToLight;
        Vector3f lightColor;
        float lightDist;

        
        for (int i = 0; i < _scene.getNumLights(); ++i)
        {
            _scene.getLight(i)->getIllumination(hitPt, dirToLight, lightColor, lightDist);
            if (_args.shadows)
            {
                // Detect shadow
                Ray shadowFeeler(hitPt, dirToLight);
                Hit shadowHit(std::numeric_limits<float>::max(), NULL, Vector3f(0, 0, 0));

                group->intersect(shadowFeeler, tmin, shadowHit);
                if (shadowHit.getT() < lightDist) continue; // skip shadowed light
            }
            
            traceColor += material->shade(ray, hit, dirToLight, lightColor);
        }
    }
    else
    {
        traceColor = _scene.getBackgroundColor(ray.getDirection());
    }
    
    return traceColor;
}
