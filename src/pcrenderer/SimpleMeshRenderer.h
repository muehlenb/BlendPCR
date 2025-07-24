// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/pcrenderer/Renderer.h"

#include "src/util/gl/Shader.h"
#include "src/util/gl/GLMesh.h"

using namespace std::chrono;

class SimpleMeshRenderer : public Renderer {
    std::vector<std::shared_ptr<OrganizedPointCloud>> currentPointClouds;

    // Splat shader:
    Shader simpleMeshShader = Shader(CMAKE_SOURCE_DIR "/shader/simple_mesh/simpleMesh.vert", CMAKE_SOURCE_DIR "/shader/simple_mesh/simpleMesh.frag", CMAKE_SOURCE_DIR "/shader/simple_mesh/simpleMesh.geo");

    // Pointer to Vertex Array Object (on GPU):
    GLuint vao = 0;

    // Pointer to index buffer:
    GLuint vbo_indices = 0;

    // Pointer to vertex buffer of points:
    GLuint vbo_positions = 0;

    unsigned int bufferWidth = -1;
    unsigned int bufferHeight = -1;

    GLuint texture_depth = 0;
    GLuint texture_colors = 0;
    GLuint texture_lookup = 0;

    unsigned int* indices = nullptr;

public:
    float uploadTime = 0;

    /**
     * Max length of an edge before the triangle that is part of it gets discarded
     * This value is multipicated by the average distance of both vertices.
     */
    float maxEdgeLength = 0.05f;

    // Should black pixels be discarded:
    bool discardBlackPixels = true;

    SimpleMeshRenderer(){
        // Generate buffers for locations, colors:
        glGenBuffers(1, &vbo_indices);
        glGenBuffers(1, &vbo_positions);
        glGenVertexArrays(1, &vao);

        glGenTextures(1, &texture_depth);
        glGenTextures(1, &texture_colors);
        glGenTextures(1, &texture_lookup);
    }

    ~SimpleMeshRenderer(){
        delete[] indices;
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo_indices);
        glDeleteBuffers(1, &vbo_positions);

        glDeleteTextures(1, &texture_depth);
        glDeleteTextures(1, &texture_colors);
        glDeleteTextures(1, &texture_lookup);
    }

    /**
     * Integrate new RGB XYZ images.
     */
    virtual void integratePointClouds(std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds) override {
        currentPointClouds = pointClouds;
    };

    /**
     * Renders the point cloud
     */
    virtual void render(Mat4f projection, Mat4f view) override {
        for(std::shared_ptr<OrganizedPointCloud> pc : currentPointClouds){
            if(pc == nullptr || pc->width == 0 || pc->height == 0)
                continue;

            int pointNum = pc->width * pc->height;


            glBindVertexArray(vao);

            // Fill index buffer:
            if(bufferWidth != pc->width || bufferHeight != pc->height){
                if(indices != nullptr)
                    delete[] indices;

                int c = 0;
                int indicesSize = pointNum * 6;
                indices = new unsigned int[indicesSize * 6];

                for(unsigned int h=0; h < pc->height - 1; ++h){
                    for(unsigned int w=0; w < pc->width - 1; ++w){

                        indices[c++] = w + h * pc->width;
                        indices[c++] = w + (h+1) * pc->width;
                        indices[c++] = (w+1) + (h+1) * pc->width;

                        indices[c++] = (w+1) + (h+1) * pc->width;
                        indices[c++] = (w+1) + h * pc->width;
                        indices[c++] = w + h * pc->width;
                    }
                }

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);

                // Fill vertex buffer:
                glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
                glBufferData(GL_ARRAY_BUFFER, pointNum * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
                glBufferData(GL_ARRAY_BUFFER, pointNum * sizeof(uint16_t), &pc->depth[0], GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(Vec4f), 0);
                glEnableVertexAttribArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_depth);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, pc->width, pc->height, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &pc->depth[0]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texture_colors);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pc->width, pc->height, 0, GL_RGBA,  GL_UNSIGNED_BYTE, &pc->colors[0]);


                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texture_lookup);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, pc->width, pc->height, 0, GL_RG,  GL_FLOAT, &pc->lookupImageTo3D[0]);

                bufferWidth = pc->width;
                bufferHeight = pc->height;
            }

            auto time = high_resolution_clock::now();

            glCullFace(GL_FRONT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_depth);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pc->width, pc->height, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &pc->depth[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture_colors);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pc->width, pc->height, GL_RGBA, GL_UNSIGNED_BYTE, &pc->colors[0]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pc->width, pc->height, GL_RG, GL_FLOAT, &pc->lookupImageTo3D[0]);

            glFlush();
            auto time2 = high_resolution_clock::now();
            uploadTime = duration_cast<microseconds>(time2 - time).count() / 1000.f;


            // Settings & bind shaders:
            simpleMeshShader.bind();
            simpleMeshShader.setUniform("projection", projection);
            simpleMeshShader.setUniform("view", view);
            simpleMeshShader.setUniform("model", pc->modelMatrix);
            simpleMeshShader.setUniform("maxEdgeLength", maxEdgeLength);
            simpleMeshShader.setUniform("discardBlackPixels", discardBlackPixels);
            simpleMeshShader.setUniform("depthTexture", 0);
            simpleMeshShader.setUniform("colorTexture", 1);
            simpleMeshShader.setUniform("lookupTexture", 2);

            // Draw triangles:
            glDrawElements(GL_TRIANGLES, bufferWidth * bufferHeight * 6, GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);

            glCullFace(GL_FRONT);
        }
    };
};
