// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/pcrenderer/Renderer.h"

#include "src/util/gl/Shader.h"

using namespace std::chrono;

// Uncomment if you want to print timings:
//#define PRINT_TIMINGS

#define CAMERA_COUNT 7
#define CAMERA_IMAGE_WIDTH 640
#define CAMERA_IMAGE_HEIGHT 576
#define LOOKUP_IMAGE_SIZE 1024

class BlendPCR : public Renderer {
public:
    int result_width = 1920;
    int result_height = 1080;
    int screensNumber = 1;
    bool makeScreenShot = false;
    int screenshotID = 0;

private:
    std::vector<std::shared_ptr<OrganizedPointCloud>> currentPointClouds;

    bool isInitialized = false;

    /**
     * Declares all FBOs and textures which we need.
     */
    unsigned int highres_colors[CAMERA_COUNT];

    // Reimplemented point cloud filter (Hole Filling):
    unsigned int fbo_pcf_holeFilling[CAMERA_COUNT];
    unsigned int texture2D_pcf_holeFilledVertices[CAMERA_COUNT];
    unsigned int texture2D_pcf_holeFilledRGB[CAMERA_COUNT];

    // Reimplemented point cloud filter (Erosion):
    unsigned int fbo_pcf_erosion[CAMERA_COUNT];
    unsigned int texture2D_pcf_erosion[CAMERA_COUNT];

    // FBO for generating 3D vertices from depth image:
    unsigned int fbo_genVertices[CAMERA_COUNT];

    // The textures for the input point clouds:
    unsigned int texture2D_inputGenVertices[CAMERA_COUNT];
    unsigned int texture2D_inputDepth[CAMERA_COUNT];
    unsigned int texture2D_inputRGB[CAMERA_COUNT];
    unsigned int texture2D_inputLookupImageTo3D[CAMERA_COUNT];
    unsigned int texture2D_inputLookup3DToImage[CAMERA_COUNT];

    // The fbo and texture for the rejection pass:
    unsigned int fbo_rejection[CAMERA_COUNT];
    unsigned int texture2D_rejection[CAMERA_COUNT];

    // The fbo and texture for the edge proximity pass:
    unsigned int fbo_edgeProximity[CAMERA_COUNT];
    unsigned int texture2D_edgeProximity[CAMERA_COUNT];

    // The fbo and texture for the mls pass:
    unsigned int fbo_mls[CAMERA_COUNT];
    unsigned int texture2D_mlsVertices[CAMERA_COUNT];

    // The fbo and texture for the normal estimation pass:
    unsigned int fbo_normals[CAMERA_COUNT];
    unsigned int texture2D_normals[CAMERA_COUNT];

    // The fbo and texture for the quality estimation pass:
    unsigned int fbo_qualityEstimate[CAMERA_COUNT];
    unsigned int texture2D_qualityEstimate[CAMERA_COUNT];

    // The fbo and textures for the separate screen rendering passes:
    unsigned int fbo_screen[CAMERA_COUNT];
    unsigned int texture2D_screenColor[CAMERA_COUNT];
    unsigned int texture2D_screenVertices[CAMERA_COUNT];
    unsigned int texture2D_screenNormals[CAMERA_COUNT];
    unsigned int texture2D_screenDepth[CAMERA_COUNT];

    // The fbo and texture for the major cam pass:
    unsigned int fbo_majorCam;
    unsigned int texture2D_majorCam;

    // The fbo and texture for the camera weights:
    unsigned int fbo_cameraWeights;
    unsigned int texture2D_cameraWeightsA; // Camera 0-3
    unsigned int texture2D_cameraWeightsB; // Camera 4-7

    unsigned int fbo_result[10];
    unsigned int texture2D_resultColor[10];
    unsigned int texture2D_resultDepth[10];


    int fbo_screen_width = -1;
    int fbo_screen_height = -1;

    int fbo_mini_screen_width = -1;
    int fbo_mini_screen_height = -1;

    bool lookupTablesUploaded = false;

    std::vector<unsigned int> usedCameraIDs;

    /**
     * Define all the shaders for the reimplemented point cloud filters
     * (originally CUDA implemented):
     */
    Shader pcfHoleFillingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/filter/holeFilling.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/filter/holeFilling.frag");
    Shader pcfErosionShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/filter/erosion.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/filter/erosion.frag");

    /** Generates 3D vertices (in m) from depth image (in mm) */
    Shader vertexGenShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/vertexGenerator.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/vertexGenerator.frag");

    /**
     * Define all the shaders for the point cloud processing passes:
     */
    Shader rejectionShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.frag");
    Shader edgeProximityShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.frag");
    Shader mlsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.frag");
    Shader normalsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.frag");
    Shader qualityEstimateShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.frag");

    /**
     * Define all the shaders for the screen passes:
     */
    Shader renderShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.frag");
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
#ifdef PRINT_TIMINGS
        auto time = high_resolution_clock::now();
        startTimeMeasureMap[str] = time;

        if(gpu){
            GLuint query;
            glGenQueries(1, &query);
            timeQueries[str] = query;
            glBeginQuery(GL_TIME_ELAPSED, timeQueries[str]);
        }
#endif
    }

    void endTimeMeasure(std::string str, bool gpu = false){
#ifdef PRINT_TIMINGS
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
        }
#endif
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
    }

    void init(){
        int mainViewport[4];
        glGetIntegerv(GL_VIEWPORT, mainViewport);

        // If screen size changed:
        if(result_width != fbo_screen_width || result_height != fbo_screen_height){
            for(unsigned int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
                // Delete old frame buffer + texture:
                if(fbo_screen_width != -1){
                    glDeleteFramebuffers(1, &fbo_screen[cameraID]);
                    glDeleteTextures(1, &texture2D_screenColor[cameraID]);
                    glDeleteTextures(1, &texture2D_screenVertices[cameraID]);
                    glDeleteTextures(1, &texture2D_screenNormals[cameraID]);
                    glDeleteTextures(1, &texture2D_screenDepth[cameraID]);
                }

                // Generate resources for SCREEN SPACE PASS:
                {
                    std::cout << "Generate FBO Screen" << std::endl;

                    glGenFramebuffers(1, &fbo_screen[cameraID]);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_screen[cameraID]);

                    generateAndBind2DTexture(texture2D_screenColor[cameraID], result_width, result_height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_screenColor[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenVertices[cameraID], result_width, result_height, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_screenVertices[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenNormals[cameraID], result_width, result_height, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texture2D_screenNormals[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenDepth[cameraID], result_width, result_height, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture2D_screenDepth[cameraID], 0);
                }
            }

            for(unsigned int screenID = 0; screenID < screensNumber; ++screenID){
                glGenFramebuffers(1, &fbo_result[screenID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_result[screenID]);

                generateAndBind2DTexture(texture2D_resultColor[screenID], result_width, result_height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_resultColor[screenID], 0);

                generateAndBind2DTexture(texture2D_resultDepth[screenID], result_width, result_height, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture2D_resultDepth[screenID], 0);
            }

            fbo_screen_width = result_width;
            fbo_screen_height = result_height;

            // Delete old frame buffer + texture:
            if(fbo_mini_screen_width != -1){
                glDeleteFramebuffers(1, &fbo_majorCam);
                glDeleteTextures(1, &texture2D_majorCam);

                glDeleteFramebuffers(1, &fbo_cameraWeights);
                glDeleteTextures(1, &texture2D_cameraWeightsA);
                glDeleteTextures(1, &texture2D_cameraWeightsB);
            }

            int requestedMiniScreenWidth = result_width / 4;
            int requestedMiniScreenHeight =result_height / 4;


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
                generateAndBind2DTexture(texture2D_inputDepth[cameraID], imageWidth, imageHeight, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
                generateAndBind2DTexture(texture2D_inputRGB[cameraID], imageWidth, imageHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);

                // Input lookup table
                generateAndBind2DTexture(texture2D_inputLookupImageTo3D[cameraID], imageWidth, imageHeight, GL_RG32F, GL_RG, GL_FLOAT, GL_NEAREST);
                generateAndBind2DTexture(texture2D_inputLookup3DToImage[cameraID], LOOKUP_IMAGE_SIZE, LOOKUP_IMAGE_SIZE, GL_RG32F, GL_RG, GL_FLOAT, GL_LINEAR);
            }

            {
                glGenFramebuffers(1, &fbo_genVertices[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_genVertices[cameraID]);

                generateAndBind2DTexture(texture2D_inputGenVertices[cameraID], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_inputGenVertices[cameraID], 0);
            }

            /**
             * Generate resources for reimplemented EROSION FILTER (orig. CUDA implemented).
             */
            {
                glGenFramebuffers(1, &fbo_pcf_erosion[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_erosion[cameraID]);

                generateAndBind2DTexture(texture2D_pcf_erosion[cameraID], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_erosion[cameraID], 0);
            }

            /**
             * Generate resources for reimplemented HOLE FILLING FILTER (orig. CUDA implemented).
             */
            {
                glGenFramebuffers(1, &fbo_pcf_holeFilling[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_holeFilling[cameraID]);

                generateAndBind2DTexture(texture2D_pcf_holeFilledVertices[cameraID], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_holeFilledVertices[cameraID], 0);

                generateAndBind2DTexture(texture2D_pcf_holeFilledRGB[cameraID], imageWidth, imageHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_pcf_holeFilledRGB[cameraID], 0);
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
            }

            /**
             * Generate resources for EDGE PROXIMITY PASS.
             *
             * This pass calculates the proximity to invalid pixels.
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
             * Generate resources for QUALITY ESTIMATE PASS.
             *
             * This pass calcluates the estimated quality for each pixel of
             * this camera. The first output value is the quality estimate,
             * the second output is the edge proximity.
             */
            {
                glGenFramebuffers(1, &fbo_qualityEstimate[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[cameraID]);

                generateAndBind2DTexture(texture2D_qualityEstimate[cameraID], imageWidth, imageHeight, GL_RG32F, GL_RG, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_qualityEstimate[cameraID], 0);
            }

            /**
             * MLS PASS: Generate frame buffer and render texture.
             *
             * This pass smoothes the vertices with a weighted moving least
             * squares kernel while being weighted with the edgeProximity
             * (to smooth the edges).
             */
            {
                glGenFramebuffers(1, &fbo_mls[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[cameraID]);

                generateAndBind2DTexture(texture2D_mlsVertices[cameraID], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_mlsVertices[cameraID], 0);
            }

            /**
             * NORMAL ESTIMATION PASS: Generate frame buffer and render texture.
             *
             * Calculates normals for the vertices using cholesky, eigenvalues,
             * and so on.
             */
            {
                glGenFramebuffers(1, &fbo_normals[cameraID]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[cameraID]);

                generateAndBind2DTexture(texture2D_normals[cameraID], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_normals[cameraID], 0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        std::cout << "Initialized FrameBuffers for BlendPCR" << std::endl;
        isInitialized = true;
    }

public:
    bool useReimplementedFilters = true;
    bool shouldClip = true;

    Vec4f clipMin = Vec4f(-1.0f, 0.05f, -1.0, 0.0);
    Vec4f clipMax = Vec4f(1.0f, 2.0f, 1.0, 0.0);

    int stride = 1;

    float implicitH = 0.08f;
    float kernelRadius = 4.f;
    float kernelSpread = 1.f;

    bool useColorIndices = false;

    float uploadTime = 0;

    bool newPointCloudsAvailable = false;


    ~BlendPCR(){
        if(fbo_mini_screen_width != -1){
            glDeleteFramebuffers(1, &fbo_majorCam);
            glDeleteTextures(1, &texture2D_majorCam);

            glDeleteFramebuffers(1, &fbo_cameraWeights);
            glDeleteTextures(1, &texture2D_cameraWeightsA);
            glDeleteTextures(1, &texture2D_cameraWeightsB);
        }

        for(int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
            if(fbo_screen_width != -1){
                glDeleteFramebuffers(1, &fbo_screen[cameraID]);
                glDeleteTextures(1, &texture2D_screenColor[cameraID]);
                glDeleteTextures(1, &texture2D_screenVertices[cameraID]);
                glDeleteTextures(1, &texture2D_screenDepth[cameraID]);
            }


            glDeleteFramebuffers(1, &fbo_pcf_holeFilling[cameraID]);
            glDeleteTextures(1, &texture2D_pcf_holeFilledVertices[cameraID]);
            glDeleteTextures(1, &texture2D_pcf_holeFilledRGB[cameraID]);

            glDeleteFramebuffers(1, &fbo_pcf_erosion[cameraID]);
            glDeleteTextures(1, &texture2D_pcf_erosion[cameraID]);

            glDeleteTextures(1, &texture2D_inputDepth[cameraID]);
            glDeleteTextures(1, &texture2D_inputRGB[cameraID]);
            glDeleteTextures(1, &texture2D_inputLookupImageTo3D[cameraID]);
            glDeleteTextures(1, &highres_colors[cameraID]);

            glDeleteFramebuffers(1, &fbo_rejection[cameraID]);
            glDeleteTextures(1, &texture2D_rejection[cameraID]);

            glDeleteFramebuffers(1, &fbo_edgeProximity[cameraID]);
            glDeleteTextures(1, &texture2D_edgeProximity[cameraID]);

            glDeleteFramebuffers(1, &fbo_mls[cameraID]);
            glDeleteTextures(1, &texture2D_mlsVertices[cameraID]);

            glDeleteFramebuffers(1, &fbo_normals[cameraID]);
            glDeleteTextures(1, &texture2D_normals[cameraID]);

            glDeleteFramebuffers(1, &fbo_qualityEstimate[cameraID]);
            glDeleteTextures(1, &texture2D_qualityEstimate[cameraID]);
        }

        for(int screenID = 0; screenID < screensNumber; ++screenID){
            glDeleteFramebuffers(1, &fbo_result[screenID]);
            glDeleteTextures(1, &texture2D_resultColor[screenID]);
            glDeleteTextures(1, &texture2D_resultDepth[screenID]);
        }

        delete[] indices;

        delete[] gridData;

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO_pc);
        glDeleteBuffers(1, &VBO_indices);

        glDeleteVertexArrays(1, &VAO_quad);
        glDeleteBuffers(1, &VBO_quad);
    }


    /**
     * Integrate new RGB XYZ images.
     */
    virtual void integratePointClouds(std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds) override {
        currentPointClouds = pointClouds;
        newPointCloudsAvailable = true;
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

        if(newPointCloudsAvailable){
            newPointCloudsAvailable = false;
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
    
                if(currentPC->depth != nullptr){
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputDepth[cameraID]);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CAMERA_IMAGE_WIDTH, CAMERA_IMAGE_HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_SHORT, currentPC->depth);
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
                    if(currentPC->lookupImageTo3D != nullptr){
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, currentPointClouds[cameraID]->width, currentPointClouds[cameraID]->height, GL_RG, GL_FLOAT, currentPC->lookupImageTo3D);
                        lookupTablesUploaded = true;
                    }
                }
            }
            endTimeMeasure("1d) Lookup");
            //endTimeMeasure("1) UploadTextures");

            // Generate vertices from depth images:
            {
                startTimeMeasure("1e) Vertex Generation", true);
                for(unsigned int cameraID : cameraIDsThatCanBeRendered){

                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_genVertices[cameraID]);
                    vertexGenShader.bind();

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputDepth[cameraID]);
                    vertexGenShader.setUniform("depthTexture", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                    vertexGenShader.setUniform("lookupTexture", 2);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                endTimeMeasure("1e) Vertex Generation", true);
            }
    
            if(useReimplementedFilters){
                startTimeMeasure("2a) Hole Filling Pass", true);
                for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                    // Hole Filling Pass:
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_holeFilling[cameraID]);
                        pcfHoleFillingShader.bind();
    
                        unsigned int hfattachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                        glDrawBuffers(2, hfattachments);
    
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputGenVertices[cameraID]);
                        pcfHoleFillingShader.setUniform("inputVertices", 1);
    
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
                        pcfHoleFillingShader.setUniform("inputColors", 2);
    
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                        pcfHoleFillingShader.setUniform("lookupImageTo3D", 3);
    
                        glBindVertexArray(VAO_quad);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                }
                endTimeMeasure("2a) Hole Filling Pass", true);
            }
    
            startTimeMeasure("3a) RejectedPass", true);
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                // Rejected PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_rejection[cameraID]);
                    rejectionShader.bind();
    
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ? texture2D_pcf_holeFilledVertices[cameraID] : texture2D_inputGenVertices[cameraID]);
                    rejectionShader.setUniform("pointCloud", 1);
    
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ? texture2D_pcf_holeFilledRGB[cameraID] : texture2D_inputRGB[cameraID]);
                    rejectionShader.setUniform("colorTexture", 2);
    
                    rejectionShader.setUniform("model", currentPointClouds[cameraID]->modelMatrix);
                    rejectionShader.setUniform("shouldClip", shouldClip);
                    rejectionShader.setUniform("clipMin", clipMin);
                    rejectionShader.setUniform("clipMax", clipMax);
    
                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            endTimeMeasure("3a) RejectedPass", true);
    
            startTimeMeasure("3b) EdgeProximity", true);
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
            endTimeMeasure("3b) EdgeProximity", true);
    
            startTimeMeasure("3c) MLS", true);
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                // Texture a(x) PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[cameraID]);
                    mlsShader.bind();

                    mlsShader.setUniform("kernelRadius", int(kernelRadius));
                    mlsShader.setUniform("kernelSpread", kernelSpread);
                    mlsShader.setUniform("p_h", implicitH);
    
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ?  texture2D_pcf_holeFilledVertices[cameraID] : texture2D_inputGenVertices[cameraID]);
                    mlsShader.setUniform("pointCloud", 1);
    
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    mlsShader.setUniform("edgeProximity", 2);
    
                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            endTimeMeasure("3c) MLS", true);
    
            startTimeMeasure("3d) Normal", true);
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                // Texture n(x) PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[cameraID]);
                    //gl.glDisable(GL_DEPTH_TEST);
                    normalsShader.bind();
    
                    normalsShader.setUniform("kernelRadius", kernelRadius);
                    normalsShader.setUniform("kernelSpread", kernelSpread);
    
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
                    normalsShader.setUniform("texture2D_mlsVertices", 1);
    
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    normalsShader.setUniform("texture2D_edgeProximity", 2);
    
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ?  texture2D_pcf_holeFilledVertices[cameraID] : texture2D_inputGenVertices[cameraID]);
                    normalsShader.setUniform("texture2D_inputVertices", 3);
    
                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            endTimeMeasure("3d) Normal", true);
    
            startTimeMeasure("3e) Influence", true);
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                // OVERLAP PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[cameraID]);
                    qualityEstimateShader.bind();
    
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
                    qualityEstimateShader.setUniform("vertices", 0);
    
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_normals[cameraID]);
                    qualityEstimateShader.setUniform("normals", 1);
    
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    qualityEstimateShader.setUniform("edgeDistances", 2);
    
                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            endTimeMeasure("3e) Influence", true);
        }

        //glFlush();
        auto time2 = high_resolution_clock::now();
        uploadTime = duration_cast<microseconds>(time2 - time).count() / 1000.f;


        for(int screenID = 0; screenID < screensNumber; ++screenID){
            // Restore viewport for screen rendering:
            glViewport(0, 0, result_width, result_height);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Just for switching for paper images:
            // bool merge = GlobalStateHandler::instance().rightBarViewModel()->debug_showRays();

            glDisable(GL_CULL_FACE);
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

                if(screenID == 0){
                    renderShader.setUniform("view", Mat4f::translation(-0.03f,0.f,0.f) * view);
                } else {
                    renderShader.setUniform("view", Mat4f::translation(0.03f,0.f,0.f) * view);
                }
                renderShader.setUniform("projection", projection);
                renderShader.setUniform("useColorIndices", useColorIndices);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ?  texture2D_pcf_holeFilledRGB[cameraID] : texture2D_inputRGB[cameraID]);
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

                //glBindVertexArray(VAO);
                //glDrawElements(GL_TRIANGLES, indicesSize,  GL_UNSIGNED_INT, NULL);
                //glBindVertexArray(0);

                /*
                glBindVertexArray(VAO);
                int instances = (CAMERA_IMAGE_WIDTH - 1) * (CAMERA_IMAGE_HEIGHT - 1); // Anzahl der Zellen
                glDrawArraysInstanced(
                    GL_TRIANGLES, // Primitive
                    0,            // Startindex
                    6,            // 6 Vertices pro Zelle (2 Dreiecke)
                    instances     // Anzahl Zellen
                    );
                glBindVertexArray(0);
    */

                glBindVertexArray(VAO);
                int gridW = CAMERA_IMAGE_WIDTH;
                int gridH = CAMERA_IMAGE_HEIGHT;
                int cellsX = (gridW - 1) / stride;
                int cellsY = (gridH - 1) / stride;


                renderShader.setUniform("stride", stride);

                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cellsX * cellsY);
                glBindVertexArray(0);

                glDrawBuffers(1, attachments);
            }
            endTimeMeasure("4a) RenderMesh", true);

            startTimeMeasure("4b) MajorCam", true);
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
            endTimeMeasure("4b) MajorCam", true);

            startTimeMeasure("4c) CamWeights", true);
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
            endTimeMeasure("4c) CamWeights", true);

            startTimeMeasure("4d) ScreenMerging", true);
            // Screen Merging:
            {
                glViewport(0, 0, result_width, result_height);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo_result[screenID]);

                glClearColor(0.5f,0.5f,0.5f,0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

                blendingShader.setUniform("useFusion", useFusion);
                blendingShader.setUniform("cameraVector", view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

                glBindVertexArray(VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                int mainVPWidth = mainViewport[2];

                glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_result[screenID]);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(0, 0, result_width, result_height, mainViewport[0] + (mainVPWidth / screensNumber) * screenID, mainViewport[1], mainViewport[0] + (mainVPWidth / screensNumber) * screenID + (mainVPWidth / screensNumber), mainViewport[3],  GL_COLOR_BUFFER_BIT, GL_LINEAR);

            }
        }


        glViewport(mainViewport[0], mainViewport[1], mainViewport[2], mainViewport[3]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        endTimeMeasure("4d) ScreenMerging", true);

        GLint maxCombinedTextureImageUnits;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureImageUnits);

        //std::cout << "maxCombinedTextureImageUnits: " << maxCombinedTextureImageUnits << std::endl;

        GLint maxTextureImageUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);
        //std::cout << "maxTextureImageUnits: " << maxTextureImageUnits << std::endl;

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_CULL_FACE);

        glEnable(GL_BLEND);

#ifdef PRINT_TIMINGS
        for(auto pair : timeMeasureMap){
            std::cout << pair.first << ": " << pair.second << std::endl;
        }
        std::cout << std::endl;
#endif
    }
};
