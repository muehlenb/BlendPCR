// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/pcrenderer/Renderer.h"

#include "src/util/gl/TextureFBO.h"
#include "src/util/gl/Texture2D.h"
#include "src/util/gl/Shader.h"

using namespace std::chrono;

#define CAMERA_COUNT 7
#define CAMERA_IMAGE_WIDTH 640
#define CAMERA_IMAGE_HEIGHT 576
#define LOOKUP_IMAGE_SIZE 1024

class BlendedMeshRenderer : public Renderer {
    std::vector<std::shared_ptr<OrganizedPointCloud>> currentPointClouds;

    bool isInitialized = false;

    /**
     * Declares all FBOs and textures which we need.
     */

    unsigned int highres_colors[CAMERA_COUNT];

    unsigned int texture2D_inputVertices[CAMERA_COUNT];
    unsigned int texture2D_inputRGB[CAMERA_COUNT];
    unsigned int texture2D_inputLookup3DToImage[CAMERA_COUNT];

    unsigned int fbo_mls[CAMERA_COUNT];
    unsigned int texture2D_mlsVertices[CAMERA_COUNT];

    unsigned int fbo_normals[CAMERA_COUNT];
    unsigned int texture2D_normals[CAMERA_COUNT];

    unsigned int fbo_rejection[CAMERA_COUNT];
    unsigned int texture2D_rejection[CAMERA_COUNT];

    unsigned int fbo_edgeProximity[CAMERA_COUNT];
    unsigned int texture2D_edgeProximity[CAMERA_COUNT];

    unsigned int fbo_qualityEstimate[CAMERA_COUNT];
    unsigned int texture2D_qualityEstimate[CAMERA_COUNT];

    unsigned int fbo_screen[CAMERA_COUNT];
    unsigned int texture2D_screenColor[CAMERA_COUNT];
    unsigned int texture2D_screenVertices[CAMERA_COUNT];
    unsigned int texture2D_screenNormals[CAMERA_COUNT];
    unsigned int texture2D_screenDepth[CAMERA_COUNT];

    unsigned int fbo_majorCam;
    unsigned int texture2D_majorCam;

    unsigned int fbo_cameraWeights;
    unsigned int texture2D_cameraWeightsA;
    unsigned int texture2D_cameraWeightsB;


    int fbo_screen_width = -1;
    int fbo_screen_height = -1;

    int fbo_mini_screen_width = -1;
    int fbo_mini_screen_height = -1;

    bool lookupTablesUploaded = false;

    std::vector<unsigned int> usedCameraIDs;

    /**
     * Define all shaders which are needed.
     */
    Shader rejectionShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.frag");
    Shader edgeProximityShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.frag");
    Shader mlsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.frag");
    Shader normalsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.frag");
    Shader qualityEstimateShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.frag");

    Shader renderShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.frag", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.geo");
    Shader majorCamShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.frag");
    Shader cameraWeightsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.frag");
    Shader blendingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.frag");

    /**
     * Defines the mesh
     */
    unsigned int indicesSize;
    unsigned int* indices;

    float* gridData;

    unsigned int VBO_pc;
    unsigned int VBO_indices;
    unsigned int VAO;

    /**
     * Defines the quad which is used for rendering in every pass.
     */

    unsigned int VBO_quad;
    unsigned int VAO_quad;
    float* quadData = new float[12]{1.f,-1.f, -1.f,-1.f, -1.f,1.f, -1.f,1.f, 1.f,1.f, 1.f,-1.f};


    std::map<std::string, high_resolution_clock::time_point> startTimeMeasureMap;
    std::map<std::string, float> timeMeasureMap;
    std::map<std::string, GLuint> timeQueries;

    void startTimeMeasure(std::string str, bool gpu = false){
        /*
        auto time = high_resolution_clock::now();
        startTimeMeasureMap[str] = time;

        if(gpu){
            GLuint query;
            glGenQueries(1, &query);
            timeQueries[str] = query;
            glBeginQuery(GL_TIME_ELAPSED, timeQueries[str]);
        }*/
    }

    void endTimeMeasure(std::string str, bool gpu = false){
        /*
        auto time = high_resolution_clock::now();

        if(gpu){
            glEndQuery(GL_TIME_ELAPSED);

            GLint done = 0;
            while (!done) {
                glGetQueryObjectiv(timeQueries[str], GL_QUERY_RESULT_AVAILABLE, &done);
            }
            GLuint64 timeElapsed;
            glGetQueryObjectui64v(timeQueries[str], GL_QUERY_RESULT, &timeElapsed);
            timeMeasureMap[str] = timeElapsed * 1.0e-6;
            glDeleteQueries(1, &timeQueries[str]);
        } else {
            float duration = duration_cast<microseconds>(time - startTimeMeasureMap[str]).count() / 1000.f;
            timeMeasureMap[str] = duration;
        }*/
    }

    void initQuadBuffer(){
        glGenVertexArrays(1, &VAO_quad);
        glBindVertexArray(VAO_quad);

        glGenBuffers(1, &VBO_quad);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_quad);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), quadData, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }


    void generateAndBind2DTexture(
        unsigned int& texture,
        unsigned int width,
        unsigned int height,
        unsigned int internalFormat, // eg. GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_R32F,...
        unsigned int format, // e.g. only GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA without Sizes!
        unsigned int type, // egl. GL_UNSIGNED_BYTE, GL_FLOAT
        unsigned int filter // GL_LINEAR or GL_NEAREST
        ){
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);

        if(filter != GL_NONE){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }

        if(format == GL_DEPTH_COMPONENT){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }
    };


    void initMesh(){
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        int c = 0;

        indicesSize = CAMERA_IMAGE_WIDTH * CAMERA_IMAGE_HEIGHT * 6;
        indices = new unsigned int[indicesSize * 6];

        for(unsigned int h=0; h < CAMERA_IMAGE_HEIGHT - 1; ++h){
            for(unsigned int w=0; w < CAMERA_IMAGE_WIDTH - 1; ++w){

                indices[c++] = w + h * CAMERA_IMAGE_WIDTH;
                indices[c++] = w + (h+1) * CAMERA_IMAGE_WIDTH;
                indices[c++] = (w+1) + (h+1) * CAMERA_IMAGE_WIDTH;

                indices[c++] = (w+1) + (h+1) * CAMERA_IMAGE_WIDTH;
                indices[c++] = (w+1) + h * CAMERA_IMAGE_WIDTH;
                indices[c++] = w + h * CAMERA_IMAGE_WIDTH;
            }
        }

        glGenBuffers(1, &VBO_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);

        // Generate Grid Vertices:
        {
            gridData = new float[CAMERA_IMAGE_WIDTH * CAMERA_IMAGE_HEIGHT * 3];
            for(int h = 0; h < CAMERA_IMAGE_HEIGHT; ++h){
                for(int w = 0; w < CAMERA_IMAGE_WIDTH; ++w){
                    float x = (w / (float)CAMERA_IMAGE_WIDTH);
                    float y = (h / (float)CAMERA_IMAGE_HEIGHT);

                    int i = w + h * CAMERA_IMAGE_WIDTH;

                    gridData[i*3] = x;
                    gridData[i*3+1] = y;
                    gridData[i*3+2] = 2;
                }
            }
        }

        glGenBuffers(1, &VBO_pc);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_pc);
        glBufferData(GL_ARRAY_BUFFER, CAMERA_IMAGE_WIDTH * CAMERA_IMAGE_HEIGHT * 3 * sizeof(GLfloat), gridData, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
    }


    void init(){
        int mainViewport[4];
        glGetIntegerv(GL_VIEWPORT, mainViewport);

        // If screen size changed:
        if(mainViewport[2] != fbo_screen_width || mainViewport[3] != fbo_screen_height){
            for(unsigned int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
                // Delete old frame buffer + texture:
                if(fbo_screen_width != -1){
                    glDeleteFramebuffers(1, &fbo_screen[cameraID]);
                    glDeleteTextures(1, &texture2D_screenColor[cameraID]);
                    glDeleteTextures(1, &texture2D_screenVertices[cameraID]);
                    glDeleteTextures(1, &texture2D_screenDepth[cameraID]);
                }

                // Generate resources for SCREEN SPACE PASS:
                {
                    std::cout << "Generate FBO Screen" << std::endl;

                    glGenFramebuffers(1, &fbo_screen[cameraID]);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_screen[cameraID]);

                    generateAndBind2DTexture(texture2D_screenColor[cameraID], mainViewport[2], mainViewport[3], GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_screenColor[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenVertices[cameraID], mainViewport[2], mainViewport[3], GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_screenVertices[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenNormals[cameraID], mainViewport[2], mainViewport[3], GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texture2D_screenNormals[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenDepth[cameraID], mainViewport[2], mainViewport[3], GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture2D_screenDepth[cameraID], 0);

                }
            }

            fbo_screen_width = mainViewport[2];
            fbo_screen_height = mainViewport[3];

            // Delete old frame buffer + texture:
            if(fbo_mini_screen_width != -1){
                glDeleteFramebuffers(1, &fbo_majorCam);
                glDeleteTextures(1, &texture2D_majorCam);

                glDeleteFramebuffers(1, &fbo_cameraWeights);
                glDeleteTextures(1, &texture2D_cameraWeightsA);
                glDeleteTextures(1, &texture2D_cameraWeightsB);
            }

            int requestedMiniScreenWidth = mainViewport[2] / 4;
            int requestedMiniScreenHeight = mainViewport[3] / 4;


            glGenFramebuffers(1, &fbo_majorCam);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_majorCam);

            generateAndBind2DTexture(texture2D_majorCam, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_majorCam, 0);

            glGenFramebuffers(1, &fbo_cameraWeights);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_cameraWeights);

            generateAndBind2DTexture(texture2D_cameraWeightsA, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_cameraWeightsA, 0);

            generateAndBind2DTexture(texture2D_cameraWeightsB, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_cameraWeightsB, 0);

            fbo_mini_screen_width = requestedMiniScreenWidth;
            fbo_mini_screen_height = requestedMiniScreenHeight;

            std::cout << "Reinitialized Screen FBOS!" << std::endl;
        }


        // If already initialized, don't to it again and simply return:
        if(isInitialized)
            return;

        // Init the quadbuffer:
        initQuadBuffer();

        // Init mesh (grid):
        initMesh();

        unsigned int imageWidth = CAMERA_IMAGE_WIDTH;
        unsigned int imageHeight = CAMERA_IMAGE_HEIGHT;

        // Generate highres color textures:
        for(unsigned int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
            generateAndBind2DTexture(highres_colors[cameraID], 2048, 1536, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
        }

        for(unsigned int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
            // Generate resources for INPUT
            {
                // Input point cloud texture
                generateAndBind2DTexture(texture2D_inputVertices[cameraID], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR);

                // Input color texture
                generateAndBind2DTexture(texture2D_inputRGB[cameraID], imageWidth, imageHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);

                // Input lookup table texture
                generateAndBind2DTexture(texture2D_inputLookup3DToImage[cameraID], LOOKUP_IMAGE_SIZE, LOOKUP_IMAGE_SIZE, GL_RG32F, GL_RG, GL_FLOAT, GL_LINEAR);
            }

            /**
             * Generate resources for REJECTION PASS.
             *
             * This pass calculates whether a vertex is valid or not
             * (and considers the distance to neighbouring vertices).
             *
             * A red-value of 0 means valid, 1 means invalid.
             */

            {
                glGenFramebuffers(1, &fbo_rejection[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_rejection[cameraID]);

                generateAndBind2DTexture(texture2D_rejection[cameraID], imageWidth, imageHeight, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_rejection[cameraID], 0);

                TextureFBO({
                    TextureType(GL_RED, GL_RED, GL_UNSIGNED_BYTE)
                });
            }

            /**
             * Generate resources for EDGE NEARNESS PASS.
             *
             * This pass calculates the nearness to invalid pixels.
             *
             * A red-value of 0 means far away from edge, 1 means on edge.
             *
             * The kernelRadius (uniform var) defines the search radius.
             * Vertices which are more than [kernelRadius] units away from
             * invalid pixels get the value 0.
             */

            {
                glGenFramebuffers(1, &fbo_edgeProximity[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_edgeProximity[cameraID]);

                generateAndBind2DTexture(texture2D_edgeProximity[cameraID], imageWidth, imageHeight, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_edgeProximity[cameraID], 0);
            }


            /**
             * Generate resources for OVERLAP PASS.
             *
             * This pass calcluates whether a vertex has overlaps in other
             * cameras. If there are overlaps, the value is 1 (in the
             * respective color-value), if there are no overlaps, the value
             * will be 0.
             */

            {
                glGenFramebuffers(1, &fbo_qualityEstimate[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[cameraID]);

                generateAndBind2DTexture(texture2D_qualityEstimate[cameraID], imageWidth, imageHeight, GL_RG32F, GL_RG, GL_FLOAT, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_qualityEstimate[cameraID], 0);
            }


            /**
             * A(X) Kernel PASS: Generate frame buffer and render texture.
             *
             * This pass smoothes the vertices with a weighted moving least
             * squares kernel while being weighted with the edgeNearness
             * (to smooth the edges).
             */

            {
                glGenFramebuffers(1, &fbo_mls[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[cameraID]);

                generateAndBind2DTexture(texture2D_mlsVertices[cameraID], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_mlsVertices[cameraID], 0);
            }

            /**
             * N(X) Kernel PASS: Generate frame buffer and render texture.
             *
             * Calculates normals for the vertices using cholesky, eigenvalues...
             */

            {
                glGenFramebuffers(1, &fbo_normals[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[cameraID]);

                generateAndBind2DTexture(texture2D_normals[cameraID], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_normals[cameraID], 0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        std::cout << "Initialized FrameBuffers for smooth rendering!" << std::endl;
        isInitialized = true;
    }

    bool timeLabelsPrinted = false;

public:
    float implicitH = 0.01f;
    float kernelRadius = 10.f;
    float kernelSpread = 1.f;

    bool useColorIndices = false;

    float uploadTime = 0;

    BlendedMeshRenderer(){

    }

    /**
     * Integrate new RGB XYZ images.
     */
    virtual void integratePointClouds(std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds) override {
        // This renderer accesses the point cloud in CPU-memory, so we need to ensure that it is there:
        for(std::shared_ptr<OrganizedPointCloud> pc : pointClouds){
            pc->toCPU();
        }

        currentPointClouds = pointClouds;
    };

    /**
     * Renders the point cloud
     */
    virtual void render(Mat4f projection, Mat4f view) override {
        if(currentPointClouds.size() < 1)
            return;

        startTimeMeasure("0) Init");

        glDisable(GL_BLEND);
        // If opengl resources are not initialized yet, do it:
        init();

        // Settings:
        bool useFusion = true;

        // Stores camera ids of cameras which should be rendered:
        std::vector<unsigned int> cameraIDsThatCanBeRendered;

        // Stores if the camera with the respective ID is active:
        bool isCameraActive[CAMERA_COUNT];
        std::fill_n(isCameraActive, CAMERA_COUNT, false);

        // Iterate over all cameras:
        for(unsigned int i = 0; i < currentPointClouds.size(); ++i){
            cameraIDsThatCanBeRendered.push_back(i);
            isCameraActive[i] = true;
        }

        // Used camera ids:
        usedCameraIDs = cameraIDsThatCanBeRendered;

        // Check if cameras for rendering are available:
        if(cameraIDsThatCanBeRendered.size() == 0){
            //std::cout << "ExperimentalPCPreprocessor: NO CAMERAS FOR RENDERING AVAILABLE!" << std::endl;
            return;
        }

        // Cache main viewport and initialize viewport for depth camera image passes:
        int mainViewport[4];
        glGetIntegerv(GL_VIEWPORT, mainViewport);

        // Initialize viewport for depth camera passes:
        glViewport(0, 0, CAMERA_IMAGE_WIDTH, CAMERA_IMAGE_HEIGHT);
        glDisable(GL_CULL_FACE);

        endTimeMeasure("0) Init");

        //startTimeMeasure("1) UploadTextures");

        auto time = high_resolution_clock::now();
        startTimeMeasure("1a) Highres");

        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            if(currentPointClouds[cameraID]->width != CAMERA_IMAGE_WIDTH || currentPointClouds[cameraID]->height != CAMERA_IMAGE_HEIGHT){
                std::cout << "SIZE ERROR! " << currentPointClouds[cameraID]->width << " x " << currentPointClouds[cameraID]->height << std::endl;
                continue;
            }

            std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];
            if(currentPC->highResColors != nullptr){
                glBindTexture(GL_TEXTURE_2D, highres_colors[cameraID]);
                glTexSubImage2D(GL_TEXTURE_2D,  0, 0, 0, 2048, 1536, GL_RGBA, GL_UNSIGNED_BYTE, currentPC->highResColors);
            }
        }
        endTimeMeasure("1a) Highres");

        startTimeMeasure("1b) Positions");
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            if(currentPointClouds[cameraID]->width != CAMERA_IMAGE_WIDTH || currentPointClouds[cameraID]->height != CAMERA_IMAGE_HEIGHT){
                std::cout << "SIZE ERROR! " << currentPointClouds[cameraID]->width << " x " << currentPointClouds[cameraID]->height << std::endl;
                continue;
            }
            std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];

            if(currentPC->positions != nullptr){
                glBindTexture(GL_TEXTURE_2D, texture2D_inputVertices[cameraID]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CAMERA_IMAGE_WIDTH, CAMERA_IMAGE_HEIGHT, GL_RGBA, GL_FLOAT, currentPC->positions);
            }
        }
        endTimeMeasure("1b) Positions");

        startTimeMeasure("1c) Colors");
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            if(currentPointClouds[cameraID]->width != CAMERA_IMAGE_WIDTH || currentPointClouds[cameraID]->height != CAMERA_IMAGE_HEIGHT){
                std::cout << "SIZE ERROR! " << currentPointClouds[cameraID]->width << " x " << currentPointClouds[cameraID]->height << std::endl;
                continue;
            }

            std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];
            if(currentPC->colors != nullptr){
                glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CAMERA_IMAGE_WIDTH, CAMERA_IMAGE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, currentPC->colors);
            }
        }
        endTimeMeasure("1c) Colors");

        startTimeMeasure("1d) Lookup");
        if(!lookupTablesUploaded){
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(currentPointClouds[cameraID]->width != CAMERA_IMAGE_WIDTH || currentPointClouds[cameraID]->height != CAMERA_IMAGE_HEIGHT){
                    std::cout << "SIZE ERROR! " << currentPointClouds[cameraID]->width << " x " << currentPointClouds[cameraID]->height << std::endl;
                    continue;
                }

                std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];
                if(currentPC->lookup3DToImage != nullptr){
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputLookup3DToImage[cameraID]);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, currentPointClouds[cameraID]->lookup3DToImageSize, currentPointClouds[cameraID]->lookup3DToImageSize, GL_RG, GL_FLOAT, currentPC->lookup3DToImage);
                }
            }
            lookupTablesUploaded = true;
        }
        endTimeMeasure("1d) Lookup");
        //endTimeMeasure("1) UploadTextures");



        startTimeMeasure("2a) RejectedPass", true);
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            // Rejected PASS:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_rejection[cameraID]);
                rejectionShader.bind();

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture2D_inputVertices[cameraID]);
                rejectionShader.setUniform("pointCloud", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
                rejectionShader.setUniform("colorTexture", 2);

                rejectionShader.setUniform("model", currentPointClouds[cameraID]->modelMatrix);

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        endTimeMeasure("2a) RejectedPass", true);

        startTimeMeasure("2b) EdgeProximity", true);
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            // Edge Distance PASS:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_edgeProximity[cameraID]);
                edgeProximityShader.bind();

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture2D_rejection[cameraID]);
                edgeProximityShader.setUniform("rejectedTexture", 1);
                edgeProximityShader.setUniform("kernelRadius", 10);

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        endTimeMeasure("2b) EdgeProximity", true);

        startTimeMeasure("2c) MLS", true);
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            // Texture a(x) PASS:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[cameraID]);
                mlsShader.bind();

                mlsShader.setUniform("kernelRadius", kernelRadius);
                mlsShader.setUniform("kernelSpread", kernelSpread);
                mlsShader.setUniform("p_h", implicitH);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture2D_inputVertices[cameraID]);
                mlsShader.setUniform("pointCloud", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                mlsShader.setUniform("edgeNearness", 2);

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        endTimeMeasure("2c) MLS", true);

        startTimeMeasure("2d) Normal", true);
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            // Texture n(x) PASS:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[cameraID]);
                //gl.glDisable(GL_DEPTH_TEST);
                normalsShader.bind();

                //float implicitH = GlobalStateHandler::instance().rightBarViewModel()->implicitH();

                normalsShader.setUniform("kernelRadius", kernelRadius);
                normalsShader.setUniform("kernelSpread", kernelSpread);
                //normalsShader.setUniform(gl, "p_h", 0.025f);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
                normalsShader.setUniform("texture2D_mlsVertices", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                normalsShader.setUniform("texture2D_edgeProximity", 2);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, texture2D_inputVertices[cameraID]);
                normalsShader.setUniform("texture2D_inputVertices", 3);

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        endTimeMeasure("2d) Normal", true);

        startTimeMeasure("2e) Influence", true);
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            // OVERLAP PASS:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[cameraID]);
                qualityEstimateShader.bind();

                unsigned int currentTexture = 1;
                for(unsigned int cameraIDOfTexture : cameraIDsThatCanBeRendered){
                    glActiveTexture(GL_TEXTURE0 + currentTexture);
                    glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraIDOfTexture]);
                    qualityEstimateShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                    ++currentTexture;

                    glActiveTexture(GL_TEXTURE0 + currentTexture);
                    glBindTexture(GL_TEXTURE_2D, texture2D_normals[cameraIDOfTexture]);
                    qualityEstimateShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                    ++currentTexture;

                    glActiveTexture(GL_TEXTURE0 + currentTexture);
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputLookup3DToImage[cameraIDOfTexture]);
                    qualityEstimateShader.setUniform("lookup["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                    ++currentTexture;

                    glActiveTexture(GL_TEXTURE0 + currentTexture);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraIDOfTexture]);
                    qualityEstimateShader.setUniform("edgeDistances["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                    ++currentTexture;


                    Mat4f fromCurrentCam = currentPointClouds[cameraIDOfTexture]->modelMatrix.inverse() * currentPointClouds[cameraID]->modelMatrix;

                    qualityEstimateShader.setUniform("fromCurrentCam["+std::to_string(cameraIDOfTexture)+"]", fromCurrentCam);
                    qualityEstimateShader.setUniform("toCurrentCam["+std::to_string(cameraIDOfTexture)+"]", fromCurrentCam.inverse());
                }

                for(int i=0; i < CAMERA_COUNT; ++i){
                    qualityEstimateShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
                }

                qualityEstimateShader.setUniform("useFusion", useFusion);
                qualityEstimateShader.setUniform("cameraID", int(cameraID));

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        endTimeMeasure("2e) Influence", true);

        glFlush();
        auto time2 = high_resolution_clock::now();
        uploadTime = duration_cast<microseconds>(time2 - time).count() / 1000.f;


        startTimeMeasure("3a) RenderMesh", true);
        // Restore viewport for screen rendering:
        glViewport(0, 0, mainViewport[2], mainViewport[3]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Just for switching for paper images:
        // bool merge = GlobalStateHandler::instance().rightBarViewModel()->debug_showRays();

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Now we render all meshes of each depth camera to a framebuffer:
        for(unsigned int cameraID : cameraIDsThatCanBeRendered){
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_screen[cameraID]);

            // Draw out point cloud and color texture:
            unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
            glDrawBuffers(3, attachments);

            float clearColor[4] = {0.0, 0.0, 0.0, 0.0};
            glClear(GL_DEPTH_BUFFER_BIT);
            glClearBufferfv(GL_COLOR, 0, clearColor);
            glClearBufferfv(GL_COLOR, 1, clearColor);

            renderShader.bind();
            renderShader.setUniform("model", currentPointClouds[cameraID]->modelMatrix);
            renderShader.setUniform("view", view);
            renderShader.setUniform("projection", projection);
            renderShader.setUniform("useColorIndices", useColorIndices);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
            renderShader.setUniform("texture2D_colors", 2);

            glActiveTexture(GL_TEXTURE3);
            // pointCloudTexture->bind(gl);
            glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
            renderShader.setUniform("texture2D_vertices", 3);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
            renderShader.setUniform("texture2D_edgeProximity", 4);

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, texture2D_normals[cameraID]);
            renderShader.setUniform("texture2D_normals", 5);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, texture2D_qualityEstimate[cameraID]);
            renderShader.setUniform("texture2D_qualityEstimate", 6);

            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, highres_colors[cameraID]);
            renderShader.setUniform("highResTexture", 7);

            //gl.glEnable(GL_CULL_FACE);
            //gl.glPolygonMode( GL_FRONT_AND_BACK, GL_LINE  );
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indicesSize,  GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
            //gl.glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

            glDrawBuffers(1, attachments);
            //gl.glDisable(GL_CULL_FACE);
        }
        endTimeMeasure("3a) RenderMesh", true);

        startTimeMeasure("3b) MajorCam", true);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // MiniScreen:
        {
            glViewport(0, 0, fbo_mini_screen_width, fbo_mini_screen_height);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_majorCam);
            majorCamShader.bind();

            unsigned int currentTexture = 1;
            for(unsigned int cameraIDOfTexture : cameraIDsThatCanBeRendered){
                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenColor[cameraIDOfTexture]);
                majorCamShader.setUniform("color["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenVertices[cameraIDOfTexture]);
                majorCamShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenNormals[cameraIDOfTexture]);
                majorCamShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenDepth[cameraIDOfTexture]);
                majorCamShader.setUniform("depth["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;
            }

            for(int i=0; i < CAMERA_COUNT; ++i){
                majorCamShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            majorCamShader.setUniform("view", view);

            majorCamShader.setUniform("useFusion", useFusion);
            majorCamShader.setUniform("cameraVector", view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

            glBindVertexArray(VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        endTimeMeasure("3b) MajorCam", true);

        startTimeMeasure("3c) CamWeights", true);
        {
            glViewport(0, 0, fbo_mini_screen_width, fbo_mini_screen_height);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_cameraWeights);
            cameraWeightsShader.bind();

            unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glDrawBuffers(2, attachments);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2D_majorCam);
            cameraWeightsShader.setUniform("dominanceTexture", 1);

            for(int i=0; i < CAMERA_COUNT; ++i){
                cameraWeightsShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            glBindVertexArray(VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        endTimeMeasure("3c) CamWeights", true);

        startTimeMeasure("3d) ScreenMerging", true);
        // Screen Merging:
        {
            glViewport(mainViewport[0], mainViewport[1], mainViewport[2], mainViewport[3]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0};
            glDrawBuffers(1, attachments);

            blendingShader.bind();

            unsigned int currentTexture = 1;
            for(unsigned int cameraIDOfTexture : cameraIDsThatCanBeRendered){
                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenColor[cameraIDOfTexture]);
                blendingShader.setUniform("color["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenVertices[cameraIDOfTexture]);
                blendingShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenNormals[cameraIDOfTexture]);
                blendingShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenDepth[cameraIDOfTexture]);
                blendingShader.setUniform("depth["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;
            }

            glActiveTexture(GL_TEXTURE0 + currentTexture);
            glBindTexture(GL_TEXTURE_2D, texture2D_cameraWeightsA);
            blendingShader.setUniform("miniWeightsA", int(currentTexture));
            ++currentTexture;

            glActiveTexture(GL_TEXTURE0 + currentTexture);
            glBindTexture(GL_TEXTURE_2D, texture2D_cameraWeightsB);
            blendingShader.setUniform("miniWeightsB", int(currentTexture));
            ++currentTexture;

            for(int i=0; i < CAMERA_COUNT; ++i){
                blendingShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            blendingShader.setUniform("view", view);

            // Lighting
            Vec4f lightPos[4] = {Vec4f(1.5f, 1.5f, 1.5f), Vec4f(-1.5f, 1.5f, 1.5f), Vec4f(-1.5f, 1.5f, -1.5f), Vec4f(1.5f, 1.5f, -1.5f)};
            Vec4f lightColors[4] = {Vec4f(0.5f, 0.75f, 1.0f), Vec4f(1.0f, 0.75f, 0.5f), Vec4f(0.5f, 0.75f, 1.0f), Vec4f(1.0f, 0.75f, 0.5f)};

            for(int i=0; i < 4; ++i){
                blendingShader.setUniform("lights["+std::to_string(i)+"].position", view * lightPos[i]);
                blendingShader.setUniform("lights["+std::to_string(i)+"].intensity", lightColors[i]);
                blendingShader.setUniform("lights["+std::to_string(i)+"].range", 10.f);
            }

            blendingShader.setUniform("useFusion", useFusion);
            blendingShader.setUniform("cameraVector", view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

            glBindVertexArray(VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        endTimeMeasure("3d) ScreenMerging", true);


        /*
        if(!timeLabelsPrinted){
            std::cout << "ScreenWidth,ScreenHeight,";

            // Iteration mit einem for-each-Loop
            for (const auto& pair : timeMeasureMap) {
                std::cout << pair.first << ",";
            }
            timeLabelsPrinted = true;
            std::cout << std::endl;
        }

        std::cout << fbo_screen_width << "," << fbo_screen_height << ",";
        // Iteration mit einem for-each-Loop
        for (const auto& pair : timeMeasureMap) {
            std::cout << pair.second << ",";
        }
        std::cout << std::endl;
*/
        GLint maxCombinedTextureImageUnits;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureImageUnits);

        //std::cout << "maxCombinedTextureImageUnits: " << maxCombinedTextureImageUnits << std::endl;

        GLint maxTextureImageUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);
        //std::cout << "maxTextureImageUnits: " << maxTextureImageUnits << std::endl;

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_CULL_FACE);

        glEnable(GL_BLEND);
    }

};
