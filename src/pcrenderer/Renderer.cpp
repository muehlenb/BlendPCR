// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#include "src/pcrenderer/Renderer.h"
#include "src/pcrenderer/SplatRenderer.h"
#include "src/pcrenderer/SimpleMeshRenderer.h"
#include "src/pcrenderer/BlendPCR.h"

const char* Renderer::availableAlgorithmNames[] = { "Splats (Uniform)", "Naive Mesh", "BlendPCR"};

const unsigned int Renderer::availableAlgorithmNum = 3;

std::shared_ptr<Renderer> Renderer::constructAlgorithmInstance(int type){
    if(type == 0){
        return std::make_shared<SplatRenderer>();
    } else if(type == 1){
        return std::make_shared<SimpleMeshRenderer>();
    } else if(type == 2){
        return std::make_shared<BlendPCR>();
    }

    std::cout << "Created " << availableAlgorithmNames[type] << std::endl;
    return nullptr;
};
