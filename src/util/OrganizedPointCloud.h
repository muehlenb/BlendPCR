// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <map>

struct float2 {float x, y;};
struct float3 {float x, y, z;};
struct float4 {float x, y, z, w;};
struct uchar4 {unsigned char x, y, z, w;};

#include "src/util/math/Mat4.h"
#include "src/util/gl/primitive/TexCoord.h"

#include "src/util/opc/OPCAttachment.h"

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

/**
 * Represents an organized point cloud ordered by the original camera image
 * (Row major order).
 *
 * Note that this organized point cloud
 */
struct OrganizedPointCloud {
public:
    OrganizedPointCloud(unsigned int width, unsigned int height)
        : width(width)
        , height(height){};

    /** Stores the xyzw coordiantes of all points */
    uint16_t* depth = nullptr;

    /** Stores the rgba coordinates of all points */
    Vec4b* colors = nullptr;

    /** Stores the normal of all points */
    Vec4f* normals = nullptr;

    /** Texture coordinates */
    TexCoord* texCoords = nullptr;

    /** */
    int frameID = -1;

    /**
     * 3D-to-image Lookup Table (memory is managed by READER to avoid copying for every point cloud
     * since the values doesn't change between different images!)
     */
    float* lookup3DToImage = nullptr;

    /**
     * 3D-to-image Lookup Table (memory is managed by READER to avoid copying for every point cloud
     * since the values doesn't change between different images!)
     */
    float* lookupImageTo3D = nullptr;

    /** High resolution colors */
    unsigned int highResWidth = 0;
    unsigned int highResHeight = 0;
    Vec4b* highResColors = 0;

    /**
     * 3D-to-image Lookup Table Size for one dimension (which results in a square
     * resolution)
     */
    unsigned int lookup3DToImageSize = 0;

    /** Transforms the point cloud positions from camera space into world space */
    Mat4f modelMatrix;

    /** Width of the original depth image */
    unsigned int width = 0;

    /** Height of the original depth iamge */
    unsigned int height = 0;

    /**
     * Usually not used / needed for processing, implemented for writing synchronized
     * training data
     */
    float timestamp = -1;

    /**
     * Attachments to this point cloud (e.g. ground truth data for training).
     */
    std::map<std::string, std::shared_ptr<OPCAttachment>> attachments;

    ~OrganizedPointCloud(){
        if(depth != nullptr){
            delete[] depth;
            depth = nullptr;
        }

        if(colors != nullptr){
            delete[] colors;
            colors = nullptr;
        }

        if(normals != nullptr){
            delete[] normals;
            normals = nullptr;
        }

        if(texCoords != nullptr){
            delete[] texCoords;
            texCoords = nullptr;
        }

        if(highResColors != nullptr){
            delete[] highResColors;
            highResColors = nullptr;
        }
    }
};
