// © 2022, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

// Include the GUI:
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// Include functions to display Mat4f and Vec4f in the GUI:
# include <imgui_cg1_helpers.h>

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

// Include GLFW/OpenGL:
#include <GLFW/glfw3.h>

// Include min, max functions:
#include <algorithm>

// Include IO utilities:
#include <iostream>

// Thread
#include <thread>

// Semaphore
#include "src/util/Semaphore.h"

// Include Mat4f class:
#include "src/util/math/Mat4.h"

// Coordinate system:
#include "src/util/gl/objects/GLCoordinateSystem.h"

// PC Streamer:
#include "src/pcstreamer/Streamer.h"

#include "src/pcstreamer/AzureKinectMKVStreamer.h"

// PC Fusion:
#include "src/pcrenderer/Renderer.h"
#include "src/pcrenderer/SimpleMeshRenderer.h"
#include "src/pcrenderer/SplatRenderer.h"
#include "src/pcrenderer/BlendedMeshRenderer.h"

// PC Filter:
#include "src/pcfilter/Filter.h"

#include <chrono>
using namespace std::chrono;

#define IMGUI_HEADCOL ImVec4(0.2f, 0.4f, 0.2f, 1.f)
#define IMGUI_HEADCOL_HOVER ImVec4(0.25f, 0.5f, 0.25f, 1.f)

#define IMGUI_HEADCOLBT ImVec4(0.2f, 0.4f, 0.2f, 1.f)
#define IMGUI_HEADCOLBT_HOVER ImVec4(0.25f, 0.5f, 0.25f, 1.f)

#define IMGUI_HEADCOLFR ImVec4(0.15f, 0.3f, 0.15f, 1.f)
#define IMGUI_HEADCOLFR_HOVER ImVec4(0.2f, 0.4f, 0.2f, 1.f)

#define IMGUI_BUTTONCOL_DISABLED ImVec4(0.4f, 0.4f, 0.4f, 1.f)
#define IMGUI_BUTTONCOL_DISABLED_HOVER ImVec4(0.5f, 0.5f, 0.5f, 1.f)

#define IMGUI_PIPELINE_HEADCOL ImVec4(0.4f, 0.2f, 0.2f, 1.f)
#define IMGUI_PIPELINE_HEADCOL_HOVER ImVec4(0.5f, 0.25f, 0.25f, 1.f)

#define MENU_WIDTH 260

/**
 * Initializes the application including the GUI
 */
int main(int argc, char** argv)
{
    // Setup window:
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    });

    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions:
    const char* glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window with graphics context:
    GLFWwindow* window = glfwCreateWindow(860, 560, "BlendPCR (GUI): Lite Version of PCRFramework", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLAD functions:
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style:
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends:
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Roboto Font:
    io.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-1.88/misc/fonts/Roboto-Medium.ttf", 16.0f);
    ImFont* fontSmall = io.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-1.88/misc/fonts/Roboto-Medium.ttf", 8.0f);

    // Enable depth test:
    glEnable(GL_DEPTH_TEST);

    // Last mouse position:
    ImVec2 lastMousePosition = ImGui::GetMousePos();

    // These variables sum the mouse movements in x and y direction
    // while the mouse was pressed in window coordinates (i.e. pixels):
    float viewPositionX = float(M_PI * 0.9);
    float viewPositionY = -0.2f;
    float viewDistance = 2.0f;

    Vec4f viewTarget(0, 0.7f, 0);

    // Opened windows:
    bool isFilterWindowOpen = false;
    bool isImGuiDemoWindowOpen = false;

    // Rendering timer
    float worldGPUTime = 0.f;
    float worldCPUTime = 0.f;

    // Time which is required for apply the filters:
    float filterTime = 0.f;

    // Time which is required for integrate the point cloud:
    float integrationTime = 0.f;

    // Streamer:
    int pcStreamerItemIdx = -1;
    int pcStreamerLoadedIdx = -1;
    std::shared_ptr<Streamer> pcStreamer;

    // Fusion / Renderer:
    int pcTechniqueItemIdx = 2;
    int pcTechniqueLoadedIdx = -1;
    std::shared_ptr<Renderer> pcRenderer;

    // TSDF Fusion Init params:
    int pcTSDFFusionDim = 512;
    float pcTSDFFusionSideLength = 2.f;
    bool shouldClose = false;

    // Attention Renderer:
    int attRenFlowRows = 288;

    //
    Semaphore integratePCSemaphore(1);

    // Last point clouds:
    std::vector<std::shared_ptr<OrganizedPointCloud>> lastProcessedPointClouds;

    // Filters:
    std::vector<std::shared_ptr<Filter>> pcFilters;
    #ifdef USE_CUDA
        // Filters:
        //std::shared_ptr<ClippingFilter> clippingFilter = std::make_shared<ClippingFilter>();
        //pcFilters.push_back(clippingFilter);

        std::shared_ptr<SpatialHoleFiller> holeFiller = std::make_shared<SpatialHoleFiller>();
        //pcFilters.push_back(holeFiller);
        std::shared_ptr<ErosionFilter> erosionFilter = std::make_shared<ErosionFilter>();
        //pcFilters.push_back(erosionFilter);
    #endif

    Semaphore pointCloudsAvailableSemaphore;
    Semaphore pointCloudsProcessedSemaphore(1);
    std::vector<std::shared_ptr<OrganizedPointCloud>> lastStreamedPointClouds;

    // Process callback when a tuple of new images is received from the pc streamer.
    // (This is assumed to be called from the streamer thread):
    std::function<void(std::vector<std::shared_ptr<OrganizedPointCloud>>)> streamerCallback =
        [&lastStreamedPointClouds, &pointCloudsAvailableSemaphore, &pointCloudsProcessedSemaphore](std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds)
    {
        pointCloudsProcessedSemaphore.acquire();
        lastStreamedPointClouds = pointClouds;
        pointCloudsAvailableSemaphore.release();
    };

    Semaphore lockFilterChangesSemaphore(1);

    std::thread filterAndIntegrateThread([&integratePCSemaphore, &pcRenderer, &pcFilters, &lastProcessedPointClouds, &filterTime, &integrationTime, &shouldClose, &lastStreamedPointClouds, &pointCloudsAvailableSemaphore, &pointCloudsProcessedSemaphore, &lockFilterChangesSemaphore](){
        while(!shouldClose){
            // Wait with processing until pcRenderer is available (we don't want to skip
            // initial point clouds, e.g. for recording):
            if(pcRenderer != nullptr){
                pointCloudsAvailableSemaphore.acquireAll();
                if(shouldClose)
                    return;

                std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds = lastStreamedPointClouds;

                auto filterStartTime = high_resolution_clock::now();

                // Apply filters:
                {
                    lockFilterChangesSemaphore.acquire();
                    for(std::shared_ptr<Filter>& filter : pcFilters){
                        if(filter->isActive){
                            filter->applyFilter(pointClouds);
                        }
                    }
                    lockFilterChangesSemaphore.release();
                }

                long long filterDuration = duration_cast<microseconds>(high_resolution_clock::now() - filterStartTime).count();
                filterTime = (filterDuration *0.001f) * 0.1f + filterTime * 0.9f;


                auto integrationStartTime = high_resolution_clock::now();
                // Integrate into fusion structure:
                {
                    integratePCSemaphore.acquire();
                    pcRenderer->integratePointClouds(pointClouds);
                    integratePCSemaphore.release();
                }
                long long integrationDuration = duration_cast<microseconds>(high_resolution_clock::now() - integrationStartTime).count();
                integrationTime = (integrationDuration *0.001f);// * 0.1f + integrationTime * 0.9f;

                lastProcessedPointClouds = pointClouds;
                pointCloudsProcessedSemaphore.release();
            }
        }
    });

    // Debug Cube:
    GLMesh cameraMesh = GLMesh(CMAKE_SOURCE_DIR "/data/model/camera.obj");

    // Splat shader:
    Shader singleColorShader = Shader(CMAKE_SOURCE_DIR "/shader/singleColorShader.vert", CMAKE_SOURCE_DIR "/shader/singleColorShader.frag");

    //
    GLCoordinateSystem coordinateSystem;

    bool trainAttention = true;
    bool trainOther = true;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Main loop which is executed every frame until the window is closed:
    while (!glfwWindowShouldClose(window))
    {
        auto frameStartTime = high_resolution_clock::now();


        // Start measuring time:
        GLuint timingQuery;
        glGenQueries(1, &timingQuery);
        glBeginQuery(GL_TIME_ELAPSED, timingQuery);

        // Processes all glfw events:
        glfwPollEvents();

        // Get the size of the window:
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Subtract right menu:
        display_w -= MENU_WIDTH;

        // Create the GUI:
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Collapsing Header Colors
            ImGui::PushStyleColor(ImGuiCol_Header, IMGUI_HEADCOL);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, IMGUI_HEADCOL);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IMGUI_HEADCOL_HOVER);

            // Create Window with settings:
            ImGui::SetNextWindowPos(ImVec2(0,0));
            ImGui::SetNextWindowSize(ImVec2(float(MENU_WIDTH),float(display_h)));
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            ImGui::Separator();
            if(ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##1", &pcStreamerItemIdx, Streamer::availableStreamerNames, Streamer::availableStreamerNum);
                ImGui::Separator();
                ImGui::Text("");
                ImGui::Separator();
                ImGui::Text("Settings:");
                ImGui::Separator();
                std::shared_ptr<FileStreamer> pcFileStreamer = std::dynamic_pointer_cast<FileStreamer>(pcStreamer);
                if(pcFileStreamer != nullptr){
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::SliderFloat("##123", &pcFileStreamer->currentTime, 0.0f, pcFileStreamer->getTotalTime());

                    if(ImGui::Button("   <   "))
                        pcFileStreamer->step(-1);

                    ImGui::SameLine();
                    if(pcFileStreamer->isPlaying){
                        if(ImGui::Button("Pause"))
                            pcFileStreamer->isPlaying = false;
                    } else {
                        if(ImGui::Button(" Play "))
                            pcFileStreamer->isPlaying = true;
                    }
                    ImGui::SameLine();

                    if(ImGui::Button("   >   "))
                        pcFileStreamer->step(1);

                    ImGui::Separator();

                    ImGui::Checkbox("Realtime (Skip Frames)", &pcFileStreamer->allowFrameSkipping);
                    ImGui::Checkbox("Loop", &pcFileStreamer->loop);
                }
                ImGui::Separator();

                std::shared_ptr<AzureKinectMKVStreamer> pcAzureKinectMKVStreamer = std::dynamic_pointer_cast<AzureKinectMKVStreamer>(pcStreamer);
                if(pcAzureKinectMKVStreamer != nullptr){
                    ImGui::Checkbox("Use Indices as Color", &pcAzureKinectMKVStreamer->useColorIndices);
                }

                ImGui::Separator();

                ImGui::Text("");

            }

            {
                // Update streamer if changed:
                if(pcStreamerItemIdx != pcStreamerLoadedIdx){
                    pcStreamer = Streamer::constructStreamerInstance(pcStreamerItemIdx);
                    if(pcStreamer != nullptr)
                        pcStreamer->setCallback(streamerCallback);
                    pcStreamerLoadedIdx = pcStreamerItemIdx;
                }
            }


            ImGui::Separator();
            if(ImGui::CollapsingHeader("Filter", ImGuiTreeNodeFlags_DefaultOpen)){
                int activeFilters = 0;
                for(std::shared_ptr<Filter>& filter : pcFilters)
                    activeFilters += filter->isActive;

                ImGui::TextUnformatted(std::string("Pipeline: " + std::to_string(pcFilters.size()) + " \t (Active: " + std::to_string(activeFilters) +")").c_str());
                ImGui::Separator();

                bool isOpenTmp = isFilterWindowOpen;
                    if(isOpenTmp){
                        ImGui::PushStyleColor(ImGuiCol_Button, IMGUI_BUTTONCOL_DISABLED);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IMGUI_BUTTONCOL_DISABLED_HOVER);
                    }
                    if(ImGui::Button("Open Window", ImVec2(ImGui::GetContentRegionAvail().x, 24))){
                        isFilterWindowOpen = !isFilterWindowOpen;
                    }
                    if(isOpenTmp){
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::Separator();

                ImGui::Text("");
            }

            ImGui::Separator();

            if(ImGui::CollapsingHeader("Render Techique", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##2", &pcTechniqueItemIdx, Renderer::availableAlgorithmNames, Renderer::availableAlgorithmNum);
                ImGui::Separator();

                std::shared_ptr<SplatRenderer> pcSplatRenderer = std::dynamic_pointer_cast<SplatRenderer>(pcRenderer);
                if(pcSplatRenderer != nullptr){
                    ImGui::TextWrapped("Rendering the point cloud using uniform sized splats.");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");
                    ImGui::Separator();
                    ImGui::SliderFloat("Point Size", &pcSplatRenderer->pointSize, 1.f, 8.0f);
                    pcSplatRenderer->pointSize = float(pcSplatRenderer->pointSize);
                    ImGui::Checkbox("Discard Black Pixels", &pcSplatRenderer->discardBlackPixels);
                }

                std::shared_ptr<SimpleMeshRenderer> pcSimpleMeshRenderer = std::dynamic_pointer_cast<SimpleMeshRenderer>(pcRenderer);
                if(pcSimpleMeshRenderer != nullptr){
                    ImGui::TextWrapped("Render the ordered pointcloud by generating a mesh for each depth image, with vertices at the coordinates of each pixel. Triangles of edges that are too large are removed to avoid a surface between foreground and background.");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");
                    ImGui::Separator();
                    ImGui::SliderFloat("Max edge length", &pcSimpleMeshRenderer->maxEdgeLength, 0.f, 0.2f);
                    ImGui::Checkbox("Discard Black Pixels", &pcSimpleMeshRenderer->discardBlackPixels);
                }

                std::shared_ptr<BlendedMeshRenderer> pcBlendedMeshFusion = std::dynamic_pointer_cast<BlendedMeshRenderer>(pcRenderer);
                if(pcBlendedMeshFusion != nullptr){
                    ImGui::TextWrapped("The renderer presented in the paper 'BlendPCR: Seamless and Efficient Rendering of Dynamic Point Clouds captured by Multiple RGB-D Cameras'");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");
                    ImGui::Separator();
                    ImGui::SliderFloat("Gauss H", &pcBlendedMeshFusion->implicitH, 0.001f, 0.08f);
                    ImGui::SliderFloat("Kernel Radius", &pcBlendedMeshFusion->kernelRadius, 3, 15);
                    ImGui::SliderFloat("Kernel Spread", &pcBlendedMeshFusion->kernelSpread, 1.f, 5.f);
                    ImGui::Separator();
                    ImGui::Checkbox("Use Indices as Color##2", &pcBlendedMeshFusion->useColorIndices);
                }

#ifdef USE_CUDA
                std::shared_ptr<TSDFMarchingCubesRenderer> pcTSDFFusion = std::dynamic_pointer_cast<TSDFMarchingCubesRenderer>(pcRenderer);
                if(pcTSDFFusion != nullptr){
                    ImGui::TextWrapped("Generating a TSDF volume from the depth images and triangulate it using marching cubes (both implemented in CUDA).");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");
                    ImGui::Separator();
                    ImGui::Text("Volume Params (per Dimension):");
                    ImGui::DragInt("Voxels", &pcTSDFFusionDim, 1.f, 32, 256);
                    ImGui::DragFloat("Length", &pcTSDFFusionSideLength, 0.1f, 0.5f, 4.0f);

                    if(pcTSDFFusionDim != pcTSDFFusion->xDim || abs(pcTSDFFusionSideLength - pcTSDFFusion->xDim*pcTSDFFusion->voxelSize) > 0.01f){
                        if(pcTSDFFusionDim >= 32 && pcTSDFFusionSideLength > 0.01f){
                            pcRenderer = std::make_shared<TSDFMarchingCubesRenderer>(pcTSDFFusionDim, pcTSDFFusionDim, pcTSDFFusionDim, pcTSDFFusionSideLength / pcTSDFFusionDim);
                            integratePCSemaphore.acquire();
                            pcRenderer->integratePointClouds(lastProcessedPointClouds);
                            integratePCSemaphore.release();
                        }
                    }
                }
#endif

#ifdef USE_TORCH
                std::shared_ptr<TemPCCRenderer> tempccRenderer = std::dynamic_pointer_cast<TemPCCRenderer>(pcRenderer);
                if(tempccRenderer != nullptr){
                    ImGui::TextWrapped("Experimental TemPCCRenderer.");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");

                    ImGui::Separator();

                    ImGui::SliderInt("Flow Precision", &attRenFlowRows, 36, 288);

                    attRenFlowRows = (18 * (2 << int(std::round(std::log2(attRenFlowRows / 18)) - 1)));

                    if(attRenFlowRows != tempccRenderer->flowRows){
                        pcRenderer = std::make_shared<TemPCCRenderer>(attRenFlowRows);
                        integratePCSemaphore.acquire();
                        pcRenderer->integratePointClouds(lastProcessedPointClouds);
                        integratePCSemaphore.release();
                    }
                    float learningRate = -std::log10(tempccRenderer->learningRate);
                    ImGui::SliderFloat("Learning Rate (1^-x)", &learningRate, 3, 10);
                    tempccRenderer->learningRate = std::pow(10, -learningRate);
                    ImGui::Checkbox("Use Attention Layer", &tempccRenderer->nativeModule.attention_enabled);
                    ImGui::Checkbox("Train Attention", &trainAttention);
                    ImGui::Checkbox("Train Other", &trainOther);
                    tempccRenderer->updateLearningEnabled(trainAttention, trainOther);
                    ImGui::SliderFloat("Scheduled Sampling (LSTM)", &tempccRenderer->scheduledSampling, 0, 1);
                    ImGui::SliderFloat("Allowed Training Div:", &tempccRenderer->allowedTrainingDivergence, 0, 1);
                    ImGui::Separator();
                    ImGui::Text("ValidTrainingNum: \t %u", tempccRenderer->validTrainingNum);
                    ImGui::Text("Training Offset: \t %u", tempccRenderer->currentTrainingOffset);

                    ImGui::Separator();
                    ImGui::Checkbox("Render Result", &tempccRenderer->shouldRender);
                    ImGui::SliderFloat("Point Size", &tempccRenderer->pointSize, 1.f, 40.0f);
                    ImGui::Checkbox("Draw Only Hidden Points", &tempccRenderer->onlyDrawHiddenPoints);
                    ImGui::Checkbox("Draw Only Visible Points", &tempccRenderer->onlyDrawVisiblePoints);
                    ImGui::Checkbox("Draw Lifetime Color", &tempccRenderer->drawLifeTimeColor);
                    ImGui::Checkbox("Draw 3D Flow", &tempccRenderer->drawFlow);
                    ImGui::Checkbox("Draw Ground Truth", &tempccRenderer->drawGTData);
                    ImGui::Checkbox("Use PDFlow", &tempccRenderer->usePDFlow);
                    ImGui::SliderInt("EveryNCamPoint", &tempccRenderer->everyNCamPoint, 1, 8);
                    ImGui::Checkbox("ApplyFlow", &tempccRenderer->applyFlow);

                    ImGui::Separator();
                    ImGui::Checkbox("Show Cam 1", &tempccRenderer->showCam1);
                    ImGui::Checkbox("Show Cam 2", &tempccRenderer->showCam2);
                    ImGui::Checkbox("Show Cam 3", &tempccRenderer->showCam3);

                    ImGui::Separator();

                    ImGui::Checkbox("Colorize Hidden Points", &tempccRenderer->colorizeHiddenPoints);
                    ImGui::Separator();
                    if(ImGui::Button("Load")){
                        tempccRenderer->loadNet();
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("Save")){
                        tempccRenderer->saveNet();
                    }
                    ImGui::Separator();
                    ImGui::Checkbox("Should Train", &tempccRenderer->shouldTrain);
                    ImGui::Checkbox("Write Training Data", &tempccRenderer->shouldStoreTrainingData);
                    ImGui::Separator();
                    ImGui::Text("Valid Cam Points: \t %u", tempccRenderer->validCamPointsNum);
                    ImGui::Text("Total Points: \t\t\t  %u", tempccRenderer->validPointsNum);
                    ImGui::Text("Last Loss: \t\t\t  %f", tempccRenderer->lastLoss);
                    ImGui::Separator();

                    if(ImGui::Button("Clear Points")){
                        tempccRenderer->clearPointsInNextFrame = true;
                    }
                    ImGui::Separator();

                    std::map<std::string, float> timeMeasures = tempccRenderer->getTimings();
                    for (const auto &elem : timeMeasures) {
                        ImGui::Text((elem.first+": %.3f ms").c_str(), elem.second);
                    }
                }
#endif
                ImGui::Separator();
                ImGui::Text("");
                ImGui::Separator();
            }

            // Change Point Cloud Renderer if another renderer was selected:
            {
                if(pcTechniqueItemIdx != pcTechniqueLoadedIdx){
                    integratePCSemaphore.acquire();
                    pcRenderer = Renderer::constructAlgorithmInstance(pcTechniqueItemIdx);
                    if(lastProcessedPointClouds.size() > 0)
                        pcRenderer->integratePointClouds(lastProcessedPointClouds);
                    pcTechniqueLoadedIdx = pcTechniqueItemIdx;
                    integratePCSemaphore.release();

#ifdef USE_CUDA
                    // Reinitialize Initialization params (syncronize):
                    std::shared_ptr<TSDFMarchingCubesRenderer> pcTSDFFusion = std::dynamic_pointer_cast<TSDFMarchingCubesRenderer>(pcRenderer);
                    if(pcTSDFFusion != nullptr){
                        pcTSDFFusionDim = pcTSDFFusion->xDim;
                        pcTSDFFusionSideLength = pcTSDFFusion->voxelSize * pcTSDFFusion->xDim;
                    }
#endif

#ifdef USE_TORCH
                    std::shared_ptr<TemPCCRenderer> tempccRenderer = std::dynamic_pointer_cast<TemPCCRenderer>(pcRenderer);
                    if(tempccRenderer != nullptr){
                        attRenFlowRows = tempccRenderer->flowRows;
                    }
#endif
                }
            }

            if(ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("Streaming: %.3f ms", pcStreamer != nullptr ? pcStreamer->getProcessingTime() : 0.f);
                ImGui::Separator();
                ImGui::Text("Filter: %.3f ms", filterTime);
                ImGui::Separator();
                ImGui::Text("Integration: %.3f ms", integrationTime);
                ImGui::Separator();
                ImGui::Text("CPU Render Time: %.3f ms", worldCPUTime);
                ImGui::Text("GL Render Time: %.3f ms", worldGPUTime);
            }

            ImGui::Text("");
            ImGui::Separator();
            if(ImGui::CollapsingHeader("Credits", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("By Andre Mühlenbrock");
                ImGui::Text("");
                ImGui::Text("Implemented at CGVR Research Lab:");
                ImGui::Text("Computergraphics and Virtual Reality");
                ImGui::Text("University of Bremen");
                ImGui::Text("");
                ImGui::Text("https://cgvr.cs.uni-bremen.de");
            }


            /*
            std::shared_ptr<BlendedMeshRenderer> pcBlendedMeshFusion = std::dynamic_pointer_cast<BlendedMeshRenderer>(pcRenderer);
            std::shared_ptr<SimpleMeshRenderer> pcSimpleMeshRenderer = std::dynamic_pointer_cast<SimpleMeshRenderer>(pcRenderer);
            std::shared_ptr<SplatRenderer> pcSplatRenderer = std::dynamic_pointer_cast<SplatRenderer>(pcRenderer);
            if(pcBlendedMeshFusion != nullptr){
                std::cout << worldCPUTime << "," << worldGPUTime << "," << integrationTime << "," << pcBlendedMeshFusion->uploadTime << std::endl;
            } else if(pcSimpleMeshRenderer != nullptr){
                std::cout << worldCPUTime << "," << worldGPUTime << "," << integrationTime << "," << pcSimpleMeshRenderer->uploadTime << std::endl;
            } else if(pcSplatRenderer != nullptr){
                std::cout << worldCPUTime << "," << worldGPUTime << "," << integrationTime << "," << pcSplatRenderer->uploadTime << std::endl;
            } else {
                std::cout << worldCPUTime << "," << worldGPUTime << "," << integrationTime << "," << 0.0f << std::endl;
            }*/

            ImGui::Separator();

            bool isOpenTmp = isImGuiDemoWindowOpen;
            if(isOpenTmp){
                ImGui::PushStyleColor(ImGuiCol_Button, IMGUI_BUTTONCOL_DISABLED);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IMGUI_BUTTONCOL_DISABLED_HOVER);
            }
            //if(ImGui::Button("Open ImGui Demo Window", ImVec2(ImGui::GetContentRegionAvail().x, 24))){
            //    isImGuiDemoWindowOpen = !isImGuiDemoWindowOpen;
            //}
            if(isOpenTmp){
                ImGui::PopStyleColor(2);
            }

            ImGui::End();

            if(isFilterWindowOpen){
                ImGui::SetNextWindowPos(ImVec2(float(display_w),0));
                ImGui::SetNextWindowSize(ImVec2(float(MENU_WIDTH),float(display_h)));
                display_w -= MENU_WIDTH;
                ImGui::Begin("PC Filters", &isFilterWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##1236673", "Select Filter to Add ..."))
                {
                    int i=0;
                    for(FilterFactory* factory : FilterFactory::availableFilterFactories){
                        if (ImGui::Selectable(factory->getDisplayName().c_str()))
                        {
                            pcFilters.emplace_back(factory->createInstance());
                        }

                        ++i;
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();
                ImGui::Text("Filters in Pipeline:");
                ImGui::Separator();

                // Collapsing Header Colors
                ImGui::PushStyleColor(ImGuiCol_Header, IMGUI_PIPELINE_HEADCOL);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, IMGUI_PIPELINE_HEADCOL);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IMGUI_PIPELINE_HEADCOL_HOVER);

                std::shared_ptr<std::function<void(void)>> delayedChangeFilterCall = nullptr;

                // Show all filters:
                for(int i=0; i < int(pcFilters.size()); ++i){
                    std::shared_ptr<Filter>& filter = pcFilters[i];

                    std::string name = "Unknown";
#ifdef USE_CUDA
                    std::shared_ptr<ClippingFilter> clippingFilter = std::dynamic_pointer_cast<ClippingFilter>(filter);
                    std::shared_ptr<TemporalNoiseFilter> temporalNoiseFilter = std::dynamic_pointer_cast<TemporalNoiseFilter>(filter);
                    std::shared_ptr<TemporalHoleFiller> temporalHoleFiller = std::dynamic_pointer_cast<TemporalHoleFiller>(filter);
                    std::shared_ptr<SpatialHoleFiller> spatialHoleFiller = std::dynamic_pointer_cast<SpatialHoleFiller>(filter);
                    std::shared_ptr<ErosionFilter> erosionFilter = std::dynamic_pointer_cast<ErosionFilter>(filter);

                    if(clippingFilter != nullptr)
                        name = "Clipping (in World Space)";
                    else if(temporalNoiseFilter != nullptr)
                        name = "Temporal Noise Filter";
                    else if(temporalHoleFiller != nullptr)
                        name = "Temporal Hole Filler";
                    else if(spatialHoleFiller != nullptr)
                        name = "Spatial Hole Filler";
                    else if(erosionFilter != nullptr)
                        name = "Erosion Filter";
#endif

                    name += "##" + std::to_string(filter->instanceID);

                    ImGui::PushFont(fontSmall);
                    if(ImGui::ArrowButton(("Up##" + std::to_string(i)).c_str(), ImGuiDir_Up)){
                        delayedChangeFilterCall = std::make_shared<std::function<void(void)>>([i, &lockFilterChangesSemaphore, &pcFilters](){
                            if(i > 0){
                                lockFilterChangesSemaphore.wait();
                                std::swap(pcFilters[i], pcFilters[i-1]);
                            }
                        });
                    }
                    ImGui::SameLine();
                    if(ImGui::ArrowButton(("Down##" + std::to_string(i)).c_str(), ImGuiDir_Down)){
                        delayedChangeFilterCall = std::make_shared<std::function<void(void)>>([i, &lockFilterChangesSemaphore, &pcFilters](){
                            if(i + 1 < int(pcFilters.size())){
                                lockFilterChangesSemaphore.wait();
                                std::swap(pcFilters[i], pcFilters[i+1]);
                            }
                        });
                    }
                    ImGui::SameLine();
                    if(ImGui::Button(("X##" + std::to_string(i)).c_str())){
                        delayedChangeFilterCall = std::make_shared<std::function<void(void)>>([i, &lockFilterChangesSemaphore, &pcFilters](){
                            lockFilterChangesSemaphore.wait();
                            pcFilters.erase(pcFilters.begin() + i);
                        });
                    }
                    ImGui::PopFont();
                    bool headerOpened = ImGui::CollapsingHeader(name.c_str());

#ifdef USE_CUDA
                    if(headerOpened){
                        ImGui::Separator();
                        if(clippingFilter != nullptr){
                            ImGui::Checkbox(("Active ##" + std::to_string(i)).c_str(), &clippingFilter->isActive);
                            ImGui::DragFloat3("Min", &clippingFilter->min.x, 0.02f, -3.f, 0.f);
                            ImGui::DragFloat3("Max", &clippingFilter->max.x, 0.02f, 0.f, 3.f);
                        }
                        if(temporalNoiseFilter != nullptr){
                            ImGui::Checkbox(("Active ##" + std::to_string(i)).c_str(), &temporalNoiseFilter->isActive);
                            ImGui::SliderFloat("Smoothing", &temporalNoiseFilter->smoothFactor, 0.f, 1.f);
                        }
                        if(temporalHoleFiller != nullptr){
                            ImGui::Checkbox(("Active ##" + std::to_string(i)).c_str(), &temporalHoleFiller->isActive);
                        }

                        if(spatialHoleFiller != nullptr){
                            ImGui::Checkbox(("Active ##" + std::to_string(i)).c_str(), &spatialHoleFiller->isActive);
                        }

                        if(erosionFilter != nullptr){
                            ImGui::Checkbox(("Active ##" + std::to_string(i)).c_str(), &erosionFilter->isActive);
                            ImGui::SliderInt("Intensity", &erosionFilter->intensity, 1, 20);
                            ImGui::SliderFloat("Max Dist", &erosionFilter->distanceThresholdPerMeter, 0.01f, 0.2f);
                        }
                        ImGui::Separator();
                    }
#endif

                    ImGui::Text("");
                }

                // We invoke the action delayed to avoid flickering:
                if(delayedChangeFilterCall != nullptr){
                    (*delayedChangeFilterCall)();
                }

                ImGui::PopStyleColor(3);
                ImGui::End();
            }

            if(isImGuiDemoWindowOpen){
                ImGui::ShowDemoWindow(&isImGuiDemoWindowOpen);
            }

            ImGui::PopStyleColor(3);
        }

        // If mouse is down, simply add the difference to viewPositionX and viewPositionY (ROTATION):
        if(ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse){
            ImVec2 mouseDelta = ImGui::GetMousePos() - lastMousePosition;

            // Add the delta to mouseMovementX and mouseMovementY:
            viewPositionX += mouseDelta.x * 0.01f;
            viewPositionY -= mouseDelta.y * 0.01f;
        }

        // Generate a rotation matrix (which is changed when the mouse is dragged):
        Mat4f rotationMat({cos(viewPositionX), -sin(viewPositionY) * -sin(viewPositionX), cos(viewPositionY) * -sin(viewPositionX), 0, 0, cos(viewPositionY), sin(viewPositionY), 0, sin(viewPositionX), -sin(viewPositionY) * cos(viewPositionX), cos(viewPositionY) * cos(viewPositionX), 0, 0,0,0,1});


        // If middle mouse is down, shift viewTarget (OFFSET):
        if(ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !ImGui::GetIO().WantCaptureMouse){
            ImVec2 mouseDelta = ImGui::GetMousePos() - lastMousePosition;

            Mat4f invRotationMat = rotationMat.inverse();
            Vec4f xDir = invRotationMat * Vec4f(1, 0, 0, 0);
            Vec4f yDir = invRotationMat * Vec4f(0, 1, 0, 0);

            viewTarget = viewTarget + (xDir * mouseDelta.x * 0.002f * viewDistance) + (yDir * mouseDelta.y * 0.002f * sqrt(viewDistance));
        }

        // If mouse wheel moved, change the view distance to the scene mid point:
        if(ImGui::GetIO().MouseWheel != 0 && !ImGui::GetIO().WantCaptureMouse){
            viewDistance *= ImGui::GetIO().MouseWheel > 0 ? 0.9f : 1.1f;
        }

        // Store this mouse position for the next frame:
        lastMousePosition = ImGui::GetMousePos();

        // Setup the GL viewport
        glViewport(MENU_WIDTH, 0, display_w, display_h);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate aspect ratio (might be used for perspective transformation):
        float aspectRatio = float(display_w) / display_h;

        // Ensure that both sides of the triangle are rendered:
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // Get projection matrix:
        Mat4f projection = Mat4f::perspectiveTransformation(aspectRatio, 45);

        // Generate the view matrix by using the rotation matrix and translate the camera
        // by the distance to the scene mid point:
        Mat4f view = Mat4f::translation(0, 0, viewDistance) * rotationMat * Mat4f::translation(-viewTarget.x, -viewTarget.y, -viewTarget.z);

        // Render all the objects in the scene:
        {
            if(pcRenderer != nullptr)
                pcRenderer->render(projection, view);
        }

        // End measuring time:
        glEndQuery(GL_TIME_ELAPSED);

        for(std::shared_ptr<OrganizedPointCloud> pc : lastProcessedPointClouds){
             // Camera:
             singleColorShader.bind();
             singleColorShader.setUniform("projection", projection);
             singleColorShader.setUniform("view", view);
             singleColorShader.setUniform("model", pc->modelMatrix * Mat4f::scale(0.05f));
             singleColorShader.setUniform("color", Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
             cameraMesh.render();
        }

        coordinateSystem.render(projection, view, Mat4f::scale(0.1f));


        // Wait until time is available:
        int resultAvailable = 0;
        while (!resultAvailable) {
            glGetQueryObjectiv(timingQuery, GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
        }

        // Render the GUI and draw it to the screen:
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Rendering time:
        GLuint64 time = 0;
        glGetQueryObjectui64v(timingQuery, GL_QUERY_RESULT, &time);
        worldGPUTime = (time / 1000000.f);// * 0.1f + worldGPUTime * 0.9f;

        // Delete queries:
        glDeleteQueries(1,&timingQuery);

        // Measure time (before swap):
        long long frameDuration = duration_cast<microseconds>(high_resolution_clock::now() - frameStartTime).count();
        worldCPUTime = (frameDuration *0.001f);//(frameDuration *0.001f) * 0.1f + worldCPUTime * 0.9f;

        // Swap Buffers (waits for vsync (?)):
        glfwSwapBuffers(window);
    }

    shouldClose = true;
    pointCloudsAvailableSemaphore.release();
    filterAndIntegrateThread.join();
    pointCloudsProcessedSemaphore.release();

    // Cleanup (so that the callback don't access deleted memory):
    pcStreamer = nullptr;

    // Cleanup (so that the callback don't access deleted memory):
    pcRenderer = nullptr;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup static shared memory (CUDA):
    OrganizedPointCloud::cleanupStaticMemory();

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
