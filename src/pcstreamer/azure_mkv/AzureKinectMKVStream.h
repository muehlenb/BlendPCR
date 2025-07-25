// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include "src/util/math/Mat4.h"
#include "src/util/OrganizedPointCloud.h"

#include <k4a/k4a.h>
#include <k4arecord/playback.h>

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

#define LOOKUP_TABLE_SIZE 1024

/**
 * A streamer when using a single or multiple Azure Kinect devices:
 */

class AzureKinectMKVStream {
    std::vector<double> allTimestamps;

    uint64_t totalFrameCount = 0;
    int currentFrame = 0;

    Mat4f transformation;
    Mat4f depthToColorTransform;

    k4a_playback_t playback_handle = nullptr;
    k4a_record_configuration_t record_config;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation_handle;

    bool successfullyOpened = false;

    float* DFToCS = nullptr;
    float* lookupTable3DToImage = nullptr;

    int width = 2048;
    int height = 1536;
    k4a_image_t colorIndexImage = nullptr;

    bool& useColorIndices;

    bool useBuffer;
    int bufferedMaxFrameCount;
    int bufferedStartFrameOffset;

    std::vector<std::shared_ptr<OrganizedPointCloud>> preloadedBuffer;

    void createColorIndexImage(){
        k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, width, height, width * 4, &colorIndexImage);
        uint8_t* buffer = k4a_image_get_buffer(colorIndexImage);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = (y * width + x) * 4;

                int r = (x*4) % 256;
                int g = (y*4) % 256;
                int b = int(x / 64) + int(y / 64) * 32;

                if (b % 2 == 1)
                    r = 255 - r;

                if (int(b / 32) % 2 == 1)
                    g = 255 - g;

                buffer[index + 0] = b & 0xFF;
                buffer[index + 1] = g;
                buffer[index + 2] = r;
                buffer[index + 3] = (b >> 8) & 0xFF;
            }
        }
    }

public:
    AzureKinectMKVStream(std::string filepath, Mat4f transformation, bool& useColorIndices, bool useBuffer, int maxFrameCount, int startFrameOffset)
        : transformation(transformation)
        , useColorIndices(useColorIndices)
        , useBuffer(useBuffer)
        , bufferedMaxFrameCount(maxFrameCount)
        , bufferedStartFrameOffset(startFrameOffset)
    {
        if (k4a_playback_open(filepath.c_str(), &playback_handle) != K4A_RESULT_SUCCEEDED)
        {
            std::cerr << "Failed to open recording\n";
            return;
        }

        if (k4a_playback_get_record_configuration(playback_handle, &record_config) != K4A_RESULT_SUCCEEDED)
        {
            std::cerr << "Failed to get record configuration\n";
            k4a_playback_close(playback_handle);
            return;
        }

        if (k4a_playback_get_calibration(playback_handle, &calibration) != K4A_RESULT_SUCCEEDED)
        {
            std::cerr << "Failed to get calibration\n";
            k4a_playback_close(playback_handle);
            return;
        }
    }

    void init(){
        // Set automatic color conversion when loading the recordings:
        k4a_playback_set_color_conversion(playback_handle, K4A_IMAGE_FORMAT_COLOR_BGRA32);

        // Ensure look up tables are available:
        createLookupTables();
        createColorIndexImage();

        // Get depth to color transform:
        {
            k4a_calibration_extrinsics_t extr = calibration.extrinsics[0][1];
            float *rotation = extr.rotation;
            float *translation = extr.translation;
            float extrMat[16] = {
                rotation[0], rotation[3], rotation[6], 0,
                rotation[1], rotation[4], rotation[7], 0,
                rotation[2], rotation[5], rotation[8], 0,
                translation[0]/1000.f, translation[1]/1000.f, translation[2]/1000.f, 1
            };
            depthToColorTransform = Mat4f(extrMat);
        }

        transformation_handle = k4a_transformation_create(&calibration);

        std::vector<double> timestamps;
        k4a_capture_t capture = NULL;

        int counter = -1;
        while (k4a_playback_get_next_capture(playback_handle, &capture) == K4A_STREAM_RESULT_SUCCEEDED)
        {
            ++counter;
            if(useBuffer && counter < bufferedStartFrameOffset){
                k4a_capture_release(capture);
                continue;
            }


            k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
            if (depth_image != NULL) {
                // Erhalte den Timestamp des Tiefenbildes
                uint64_t timestamp_usec = k4a_image_get_device_timestamp_usec(depth_image);

                timestamps.push_back(timestamp_usec / 1000000.0);

                // If the buffer should be used, already generate the point cloud:
                if(useBuffer){
                    preloadedBuffer.push_back(generatePointCloudFromCapture(capture));

                    if(preloadedBuffer.size() >= bufferedMaxFrameCount){
                        break;
                    }
                } else {
                    k4a_image_release(depth_image);
                }
            } else {
                //std::cerr << "No depth image available in this capture." << std::endl;
            }

            k4a_capture_release(capture);
        }

        allTimestamps = timestamps;
        totalFrameCount = timestamps.size();

        std::cout << "Total Frame Count: " << totalFrameCount << " | Start TS: " << allTimestamps[0] << std::endl;

        k4a_result_t seek_result = k4a_playback_seek_timestamp(playback_handle, 0, K4A_PLAYBACK_SEEK_BEGIN);
        if (seek_result != K4A_RESULT_SUCCEEDED)
        {
            std::cerr << "Failed to seek to beginning of the recording\n";
        }

        successfullyOpened = true;
    }

    ~AzureKinectMKVStream(){
        k4a_transformation_destroy(transformation_handle);
        k4a_playback_close(playback_handle);
        k4a_image_release(colorIndexImage);

        delete[] DFToCS;
        delete[] lookupTable3DToImage;
    }

    int getCurrentFrame(){
        return currentFrame;
    }

    int getTotalFrameCount(){
        return int(totalFrameCount);
    }

    std::vector<double>& getAllTimestamps(){
        return allTimestamps;
    }

    float getTotalTime(){
        return float(allTimestamps[totalFrameCount-1] - allTimestamps[0]);
    }

    double getTimeDeltaToNextFrame() {
        if(currentFrame + 1 >= int(totalFrameCount))
            return 0.f;

        return allTimestamps[currentFrame + 1] - allTimestamps[currentFrame];
    }

    std::shared_ptr<OrganizedPointCloud> readImage(unsigned int frame){
        if(frame < 0 || frame >= totalFrameCount)
            return nullptr;

        return syncImage(allTimestamps[frame]);
    }

    double getTimeStampAtFrame(int frame){
        return allTimestamps[frame];
    }

    void createLookupTables(){
        lookupTable3DToImage = new float[LOOKUP_TABLE_SIZE * LOOKUP_TABLE_SIZE * 2];

        for(unsigned int y = 0; y < LOOKUP_TABLE_SIZE; ++y){
            for(unsigned int x = 0; x < LOOKUP_TABLE_SIZE; ++x){
                k4a_float3_t p;
                k4a_float2_t img;

                p.xyz.x = ((x / float(LOOKUP_TABLE_SIZE)) * 2 - 1) * 1000;
                p.xyz.y = ((y / float(LOOKUP_TABLE_SIZE)) * 2 - 1) * 1000;
                p.xyz.z = 1000;

                int valid;
                k4a_calibration_3d_to_2d(&calibration, &p, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &img, &valid);
                if (valid == 1) {
                    float relImgX = img.xy.x / float(640);
                    float relImgY = img.xy.y / float(576);

                    if(relImgX >= 0 && relImgX <= 1 && relImgY >= 0 && relImgY <= 1){
                        lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2] = relImgX;
                        lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2 + 1] = relImgY;
                    } else {
                        lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2] = -1;
                        lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2 + 1] = -1;
                    }
                } else {
                    lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2] = -1;
                    lookupTable3DToImage[(x + y * LOOKUP_TABLE_SIZE) * 2 + 1] = -1;
                }
            }
        }

        k4a_float2_t p;
        k4a_float3_t ray;

        DFToCS = new float[640 * 576 * 2];

        for (unsigned int y = 0, idx = 0; y < 576; y++)
        {
            p.xy.y = (float)y;

            for (unsigned int x = 0; x < 640; x++, idx++)
            {
                p.xy.x = (float)x;

                int valid;
                k4a_calibration_2d_to_3d(&calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid);
                if (valid) {
                    DFToCS[idx * 2] = ray.xyz.x;
                    DFToCS[idx * 2 + 1] = ray.xyz.y;
                }
                else {
                    DFToCS[idx * 2] = nanf("");
                    DFToCS[idx * 2 + 1] = nanf("");
                }
            }
        }
    }

    std::shared_ptr<OrganizedPointCloud> generatePointCloudFromCapture(k4a_capture_t& capture){
        k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
        k4a_image_t color_image = k4a_capture_get_color_image(capture);

        k4a_image_t transformed_color_image;

        if (depth_image != nullptr && color_image != nullptr) {
            int width = k4a_image_get_width_pixels(depth_image);
            int height = k4a_image_get_height_pixels(depth_image);

            std::shared_ptr<OrganizedPointCloud> pc = std::make_shared<OrganizedPointCloud>(width, height);

            int highres_width = k4a_image_get_width_pixels(color_image);
            int highres_height = k4a_image_get_height_pixels(color_image);

            // Create highres texture:
            if(highres_width == 2048 && highres_height == 1536 && useColorIndices){
                uint8_t* buffer = k4a_image_get_buffer(color_image);

                pc->highResColors = new Vec4b[highres_width * highres_height];
                pc->highResWidth = highres_width;
                pc->highResHeight = highres_height;
                std::memcpy(pc->highResColors, buffer, highres_width * highres_height * sizeof(Vec4b));
            }

            k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, width, height, width * 4 * sizeof(uint8_t), &transformed_color_image);
            if (k4a_transformation_color_image_to_depth_camera(transformation_handle, depth_image, useColorIndices ? colorIndexImage : color_image, transformed_color_image) != K4A_RESULT_SUCCEEDED)
            {
                std::cout << "A color image could not be transformed to the depth image." << std::endl;

                k4a_image_release(depth_image);
                k4a_image_release(transformed_color_image);
                k4a_image_release(color_image);
                k4a_capture_release(capture);
                return nullptr;
            }

            pc->depth = new uint16_t[width * height];
            pc->colors = new Vec4b[width * height];
            pc->modelMatrix = transformation * depthToColorTransform;
            pc->lookupImageTo3D = DFToCS;
            pc->lookup3DToImage = lookupTable3DToImage;
            pc->lookup3DToImageSize = LOOKUP_TABLE_SIZE;
            pc->frameID = currentFrame;
            pc->width = width;
            pc->height = height;

            uint16_t* pcdata = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);
            uint8_t* bgradata = k4a_image_get_buffer(transformed_color_image);
            std::memcpy(pc->colors, bgradata, width * height * sizeof(int));
            std::memcpy(pc->depth, pcdata, width * height * sizeof(uint16_t));

            k4a_image_release(depth_image);
            k4a_image_release(transformed_color_image);
            k4a_image_release(color_image);
            return pc;
        }

        return nullptr;
    }

    std::shared_ptr<OrganizedPointCloud> syncImage(double timeStamp){
        double specificTimestep = 0.f;
        for(int i=0; i < allTimestamps.size(); ++i){
            if(timeStamp - 0.0166f < allTimestamps[i]){
                specificTimestep = allTimestamps[i];
                currentFrame = i;
                break;
            }
        }

        if(useBuffer){
            if(currentFrame >= 0 && currentFrame < preloadedBuffer.size())
                return preloadedBuffer[currentFrame];
        } else {
            // Seek to timestamp and load:
            k4a_result_t seek_result = k4a_playback_seek_timestamp(playback_handle, uint64_t(specificTimestep * 1000000), K4A_PLAYBACK_SEEK_BEGIN);
            if (seek_result != K4A_RESULT_SUCCEEDED)
            {
                return nullptr;
            }

            // Read capture:
            k4a_capture_t capture = NULL;
            if (k4a_playback_get_next_capture(playback_handle, &capture) == K4A_STREAM_RESULT_SUCCEEDED) {
                std::shared_ptr<OrganizedPointCloud> pc = generatePointCloudFromCapture(capture);
                k4a_capture_release(capture);
                return pc;
            }
            return nullptr;
        }
        return nullptr;
    }
};
