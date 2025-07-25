// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
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
#include "src/pcrenderer/BlendPCR.h"

// PC Filter:
#include "src/pcfilter/Filter.h"

#ifdef USE_CUDA
#include "src/pcfilter/ErosionFilter.h"
#include "src/pcfilter/SpatialHoleFiller.h"
#include "src/pcfilter/ClippingFilter.h"
#endif

#include <imfilebrowser.h>

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

    float dpiXScale, dpiYScale;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorContentScale(monitor, &dpiXScale, &dpiYScale);
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Setup Dear ImGui context:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Load Roboto Font:
    io.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-1.88/misc/fonts/Roboto-Medium.ttf", 14.0f * dpiXScale);
    ImFont* fontSmall = io.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-1.88/misc/fonts/Roboto-Medium.ttf", 10.0f * dpiXScale);

    int menuWidth = int(MENU_WIDTH * dpiXScale);

    int initWindowWidth = std::min(int(1600 * dpiXScale), int(mode->width * 0.86));
    int initWindowHeight = std::min(int(900 * dpiXScale), int(mode->height * 0.86));

    // Create window with graphics context:
    GLFWwindow* window = glfwCreateWindow(initWindowWidth, initWindowHeight, "BlendPCR (GUI): Lite Version of PCRFramework", NULL, NULL);
    if (window == NULL)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    // Initialize GLAD functions:
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // Setup Dear ImGui style:
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends:
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Enable depth test:
    glEnable(GL_DEPTH_TEST);

    // Last mouse position:
    ImVec2 lastMousePosition = ImGui::GetMousePos();

    // These variables sum the mouse movements in x and y direction
    // while the mouse was pressed in window coordinates (i.e. pixels):
    float viewPositionX = -float(M_PI * 0.2);
    float viewPositionY = -0.2f;
    float viewDistance = 2.6f;

    Vec4f viewTarget(0, 0.9f, 0);

    // Opened windows:
    bool isFilterWindowOpen = false;
    bool isImGuiDemoWindowOpen = false;

    // Rendering timer
    float worldCPUTime = 0.f;

    // Time which is required for apply the filters:
    float filterTime = 0.f;

    // Time which is required for integrate the point cloud:
    float integrationTime = 0.f;

    // Streamer:
    int pcStreamerItemIdx = 0;
    int pcStreamerLoadedIdx = -1;

    int pcStreamerOfCurrentFileDialog = -1;
    std::shared_ptr<Streamer> pcStreamer;

    // Fusion / Renderer:
    int pcTechniqueItemIdx = 2;
    int pcTechniqueLoadedIdx = -1;
    std::shared_ptr<Renderer> pcRenderer;

    bool shouldClose = false;
    bool vSyncActive = false;

    int absoluteFPS = 0;

    //
    Semaphore integratePCSemaphore(1);

    // Last point clouds:
    std::vector<std::shared_ptr<OrganizedPointCloud>> lastProcessedPointClouds;

    // Filters:
    std::vector<std::shared_ptr<Filter>> pcFilters;

    /**
     * Filter pipeline used in paper (don't use it with the buffered streamer!).
     */

    /*
    #ifdef USE_CUDA
        std::shared_ptr<ClippingFilter> clippingFilter = std::make_shared<ClippingFilter>();
        pcFilters.push_back(clippingFilter);
        std::shared_ptr<SpatialHoleFiller> holeFiller = std::make_shared<SpatialHoleFiller>();
        pcFilters.push_back(holeFiller);
        std::shared_ptr<ErosionFilter> erosionFilter = std::make_shared<ErosionFilter>();
        pcFilters.push_back(erosionFilter);
    #endif*/

    Semaphore pointCloudsAvailableSemaphore;
    Semaphore pointCloudsProcessedSemaphore(1);
    std::vector<std::shared_ptr<OrganizedPointCloud>> lastStreamedPointClouds;

    // Process callback when a tuple of new images is received from the pc streamer.
    // (This is assumed to be called from the streamer thread):
    std::function<void(std::vector<std::shared_ptr<OrganizedPointCloud>>)> streamerCallback =
        [&lastStreamedPointClouds, &pointCloudsAvailableSemaphore, &pointCloudsProcessedSemaphore](std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds)
    {
        pointCloudsProcessedSemaphore.acquireAll();
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGui::FileBrowser fileDialog;
    fileDialog.SetTitle("Select 'cameraconfig.json' of CWIPC-SXR dataset:");
    fileDialog.SetTypeFilters({ ".json" });
    fileDialog.SetWindowSize(1060,620);

    int loopCounter = 0;
    auto lastReport = std::chrono::steady_clock::now();

    // Main loop which is executed every frame until the window is closed:
    while (!glfwWindowShouldClose(window))
    {
        glfwSwapInterval(vSyncActive ? 1 : 0);

        ++loopCounter;
        auto frameStartTime = high_resolution_clock::now();

        // Processes all glfw events:
        glfwPollEvents();

        // Get the size of the window:
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Subtract right menu:
        display_w -= menuWidth;

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
            ImGui::SetNextWindowSize(ImVec2(float(menuWidth),float(display_h)));
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            ImGui::Separator();
            if(ImGui::CollapsingHeader("Source Mode", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##1", &pcStreamerItemIdx, Streamer::availableStreamerNames, Streamer::availableStreamerNum);
                ImGui::Separator();

                std::shared_ptr<FileStreamer> pcFileStreamer = std::dynamic_pointer_cast<FileStreamer>(pcStreamer);
                if(pcFileStreamer != nullptr){
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if(ImGui::Button("Load new Scene")){
                        fileDialog.Open();
                        pcFileStreamer->isPlaying = false;
                    }
                    ImGui::Separator();
                }

                ImGui::Text("");
                ImGui::Separator();
                ImGui::Text("CWIPC-SXR (Buffered) Load Settings:");
                ImGui::SliderInt("Start Frame", &Streamer::BufferedStartFrameOffset, 0, 1000);
                ImGui::SliderInt("Max Frames", &Streamer::BufferedMaxFrameCount, 0, 1000);
                ImGui::Text(("Approx. required RAM: " + std::to_string(int(std::ceil(0.058 * Streamer::BufferedMaxFrameCount))) + " GB").c_str());
                ImGui::Separator();
                ImGui::Text("");
                ImGui::Separator();
                ImGui::Text("Settings:");
                ImGui::Separator();

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

                    if(pcStreamerLoadedIdx == 1)
                        ImGui::Checkbox("Realtime (Skip Frames)", &pcFileStreamer->allowFrameSkipping);
                    ImGui::Checkbox("Loop", &pcFileStreamer->loop);
                    ImGui::Separator();
                }

                std::shared_ptr<AzureKinectMKVStreamer> pcAzureKinectMKVStreamer = std::dynamic_pointer_cast<AzureKinectMKVStreamer>(pcStreamer);
                if(pcAzureKinectMKVStreamer != nullptr){
                    if(pcStreamerLoadedIdx == 1)
                        ImGui::Checkbox("High Resolution Encoding", &pcAzureKinectMKVStreamer->useColorIndices);

                } else {
                    ImGui::TextWrapped("Please select \"Source\" to load a scene.");
                }
                ImGui::Separator();
                ImGui::Text("");
            }

            {
                // Update streamer if changed:
                if(pcStreamerItemIdx != pcStreamerLoadedIdx){
                    if(pcStreamerItemIdx > 0){
                        pcStreamerOfCurrentFileDialog = pcStreamerItemIdx;
                        pcStreamerItemIdx = pcStreamerLoadedIdx;
                        fileDialog.Open();
                    } else {
                        pcStreamer = nullptr;
                        pcStreamerLoadedIdx = pcStreamerItemIdx;
                    }
                }

                fileDialog.Display();

                if(fileDialog.HasSelected())
                {
                    pcStreamer = Streamer::constructStreamerInstance(pcStreamerOfCurrentFileDialog, fileDialog.GetSelected().string());
                    if(pcStreamer != nullptr)
                        pcStreamer->setCallback(streamerCallback);
                    pcStreamerItemIdx = pcStreamerLoadedIdx = pcStreamerOfCurrentFileDialog;
                    fileDialog.ClearSelected();
                }
            }

            ImGui::Separator();

            if(ImGui::CollapsingHeader("Rendering Techique", ImGuiTreeNodeFlags_DefaultOpen)){
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

                std::shared_ptr<BlendPCR> pcBlendPCRenderer = std::dynamic_pointer_cast<BlendPCR>(pcRenderer);
                if(pcBlendPCRenderer != nullptr){
                    ImGui::TextWrapped("The renderer presented in our paper 'BlendPCR: Seamless and Efficient Rendering of Dynamic Point Clouds captured by Multiple RGB-D Cameras'");
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Text("Settings:");
                    ImGui::Separator();
                    ImGui::SliderFloat("Gauss H", &pcBlendPCRenderer->implicitH, 0.001f, 0.08f);
                    ImGui::SliderFloat("Kernel Radius", &pcBlendPCRenderer->kernelRadius, 3, 15);
                    ImGui::SliderFloat("Kernel Spread", &pcBlendPCRenderer->kernelSpread, 1.f, 5.f);
                    ImGui::Separator();
                    ImGui::Checkbox("High Resolution Encoding##2", &pcBlendPCRenderer->useColorIndices);
                    ImGui::Separator();
                    ImGui::Text("");
                    ImGui::Separator();
                    ImGui::Checkbox("Reimpl. Filters", &pcBlendPCRenderer->useReimplementedFilters);
                    ImGui::Separator();
                    ImGui::Checkbox("Reimpl. Clipping Filter", &pcBlendPCRenderer->shouldClip);
                    ImGui::DragFloat3("Clip Min", &pcBlendPCRenderer->clipMin.x, 0.02f, -2.f, 0.5f);
                    ImGui::DragFloat3("Clip Max", &pcBlendPCRenderer->clipMax.x, 0.02f, -0.5f, 2.f);
                    ImGui::Separator();
                }

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
                }
            }

            if(ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("Streaming: %.3f ms", pcStreamer != nullptr ? pcStreamer->getProcessingTime() : 0.f);
                ImGui::Separator();
                ImGui::Text("Filter: %.3f ms", filterTime);
                ImGui::Separator();
                ImGui::Text("Integration: %.3f ms", integrationTime);
                ImGui::Separator();
                ImGui::Text("Render Loop (synced)*: %.3f ms", worldCPUTime);
                ImGui::Separator();
                ImGui::Text("Framerate*: %.i", absoluteFPS);
                ImGui::Separator();
                ImGui::Checkbox("VSync activated", &vSyncActive);
                ImGui::Text("");
                ImGui::Text("*Includes GUI, PC Passes & Screen Passes");
            }

            ImGui::Separator();
            if(ImGui::CollapsingHeader("Credits", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("Computergraphics and Virtual Reality");
                ImGui::Text("University of Bremen");
                ImGui::Separator();
                ImGui::Text("By Andre Mühlenbrock");
            }

            ImGui::Separator();

            bool isOpenTmp = isImGuiDemoWindowOpen;
            if(isOpenTmp){
                ImGui::PushStyleColor(ImGuiCol_Button, IMGUI_BUTTONCOL_DISABLED);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IMGUI_BUTTONCOL_DISABLED_HOVER);
            }

            if(isOpenTmp){
                ImGui::PopStyleColor(2);
            }

            ImGui::End();

            if(isFilterWindowOpen){
                ImGui::SetNextWindowPos(ImVec2(float(display_w),0));
                ImGui::SetNextWindowSize(ImVec2(float(menuWidth),float(display_h)));
                display_w -= menuWidth;
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
        glViewport(menuWidth, 0, display_w, display_h);
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
        // glEndQuery(GL_TIME_ELAPSED);

        // Copy to avoid memory issues in the following for loop (when new processed point cloud is set
        // by another thread):
        std::vector<std::shared_ptr<OrganizedPointCloud>> tmpPCs = lastProcessedPointClouds;

        for(std::shared_ptr<OrganizedPointCloud> pc : tmpPCs){
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
        // int resultAvailable = 0;
        // while (!resultAvailable) {
        //     glGetQueryObjectiv(timingQuery, GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
        // }

        // Render the GUI and draw it to the screen:
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Ensure all GL Commands are finished before measure time:
        glFinish();

        // Measure time (before swap):
        long long frameDuration = duration_cast<microseconds>(high_resolution_clock::now() - frameStartTime).count();
        worldCPUTime = (frameDuration *0.001f) * 0.1f + worldCPUTime * 0.9f;

        // Swap Buffers (waits for vsync (?)):
        glfwSwapBuffers(window);

        auto now = std::chrono::steady_clock::now();
        if (duration_cast<seconds>(now - lastReport).count() >= 1) {
            absoluteFPS = loopCounter;

            loopCounter = 0;
            lastReport = now;
        }
    }


    // Cleanup (so that the callback don't access deleted memory):
    pcStreamer = nullptr;

    // Cleanup (so that the callback don't access deleted memory):
    pcRenderer = nullptr;

    shouldClose = true;
    pointCloudsAvailableSemaphore.release();
    filterAndIntegrateThread.join();
    pointCloudsProcessedSemaphore.release();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

