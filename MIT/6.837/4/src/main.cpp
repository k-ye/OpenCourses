#include <cstring>
#include <iostream>

#include "Renderer.h"

int
main(int argc, const char **argv)
{
    // Report help usage if no args specified.
    if (argc == 1) {
        std::cout << "Usage: a4 "
            << "-input <scene> -size <width> <height> -output <image.png> -depth <depth_min> <depth_max> <depth_image.png> [-normals <normals_image.png>]\n";
        return 1;
    }

    std::string sceneFilename;
    int width = 0;
    int height = 0;
    std::string outputFilename;
    std::string depthFilename;
    float minDepth = 0.0f;
    float maxDepth = 0.0f;
    std::string normalFilename;

    for( int argNum = 1; argNum < argc; ++argNum )
    {
        // parse command line args
        if (strcmp(argv[argNum], "-input") == 0)
          sceneFilename = std::string(argv[argNum + 1]);
        if (strcmp(argv[argNum], "-output") == 0)
          outputFilename = std::string(argv[argNum + 1]);
        if (strcmp(argv[argNum], "-size") == 0)
        {
          width = atoi(argv[argNum + 1]);
          height = atoi(argv[argNum + 2]);
        }
    }

    // Fill in your implementation here.
    // Parse arguments.

    // This loop loops over each of the input arguments.
    // argNum is initialized to 1 because the first
    // "argument" provided to the program is actually the
    // name of the executable (in our case, "a4").
    for (int argNum = 1; argNum < argc; ++argNum) {
        std::cout 
            << "Argument " << argNum 
            << " is: " << argv[argNum] << std::endl;
    }

    Renderer renderer(sceneFilename);
    renderer.Render(width, height, 
                    outputFilename,
                    minDepth, maxDepth,
                    depthFilename, 
                    normalFilename);

    return 0;
}
