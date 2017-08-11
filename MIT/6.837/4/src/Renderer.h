#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include "SceneParser.h"

// TODO: implement this class.
class Renderer
{
  public:
    // Instantiates a renderer for the given scene.
    Renderer(const std::string &sceneFilename);

    ~Renderer();

    // If depthFilename is empty, a depth image is not generated.
    // Same for normalFilename and the normal image.
    void Render(int width,
                int height,
                const std::string &outputFilename,
                float minDepth = 0.0f,
                float maxDepth = 0.0f,
                const std::string &depthFilename = "",
                const std::string &normalFilename = "");

  private:
    SceneParser _sceneParser;
};

#endif // RENDERER_H
