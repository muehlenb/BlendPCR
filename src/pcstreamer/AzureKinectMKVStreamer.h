// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
//
// Adapted to load the CWIPC-SXR dataset for BlendPCR evaluation
#pragma once

#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <chrono>
using namespace std::chrono;

#include "src/pcstreamer/Streamer.h"
#include "src/pcstreamer/azure_mkv/AzureKinectMKVStream.h"

/**
 * A streamer when using a single or multiple Azure Kinect devices:
 */

class AzureKinectMKVStreamer : public FileStreamer {
public:
    std::vector<std::shared_ptr<AzureKinectMKVStream>> streams;

    std::thread readingThread;
    bool shouldStop = false;

    float processingTime = 0.f;
    float lastTimeWhileStopped = -1.f;

    float debugOffsetX = 0.f;
    float debugOffsetY = 0.f;
    float debugOffsetZ = 0.f;

    bool useColorIndices = false;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;

    AzureKinectMKVStreamer(std::string cameraConfigPath, bool useBuffer = true){
        allowFrameSkipping = useBuffer;

        std::string rootPath = std::filesystem::path(cameraConfigPath).parent_path().string() + "/";

        std::ifstream f(cameraConfigPath);
        json jfile = json::parse(f);

        int numCameras = int(jfile["camera"].size());
        numCameras = 7;

        streams = std::vector<std::shared_ptr<AzureKinectMKVStream>>(numCameras);

        for(int i = 0; i < numCameras; ++i){
            json jcamera = jfile["camera"][i];
            std::string filename = jcamera["filename"].get<std::string>();

            float rawMat[16];
            int index = 0;
            for (const auto& row : jcamera["trafo"]) {
                for (float value : row) {
                    int rowMajorIdx = index++;
                    int colMajorIdx = (rowMajorIdx % 4) * 4 + (rowMajorIdx / 4);
                    rawMat[colMajorIdx] = value;
                }
            }

            Mat4f matrix(rawMat);

            streams[i] = std::make_shared<AzureKinectMKVStream>(rootPath + filename, matrix, useColorIndices, useBuffer, BufferedMaxFrameCount, BufferedStartFrameOffset);
        }

        #pragma omp parallel for
        for(int i = 0; i < numCameras; ++i){
            streams[i]->init();
        }

        readingThread = std::thread([this](){
            lastFrameTime = high_resolution_clock::now();
            while(!shouldStop){
                if(!isPlaying){
                    std::this_thread::sleep_for(50ms);

                    // If currentTime was changed manually, update the point cloud even when paused:
                    if(lastTimeWhileStopped != currentTime){
                        std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds;

                        double searchedTimestamp = currentTime + streams[0]->getTimeStampAtFrame(0);

                        for(int i = 0; i < int(streams.size()); ++i){
                            std::shared_ptr<OrganizedPointCloud> pc = streams[i]->syncImage(searchedTimestamp);
                            if(pc != nullptr){
                                pointClouds.push_back(pc);
                            }
                        }

                        if(callback)
                            callback(pointClouds);

                        lastTimeWhileStopped = currentTime;
                    }

                    lastFrameTime = high_resolution_clock::now();
                    continue;
                }

                auto startTime = high_resolution_clock::now();

                // Point Clouds of this frame:
                std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds;

                auto now = high_resolution_clock::now();
                if(allowFrameSkipping){
                    currentTime += float(duration_cast<nanoseconds>(now - lastFrameTime).count() * 0.000000001);
                } else {
                    currentTime += float(streams[0]->getTimeDeltaToNextFrame()) + 0.000001f;
                }
                lastFrameTime = now;

                // If playtime exceeds the total time:
                if(currentTime + 0.1f >= streams[0]->getTotalTime()){
                    currentTime = 0;
                    streams[0]->readImage(0);

                    if(!loop){
                        isPlaying = false;
                        continue;
                    }
                }
                lastTimeWhileStopped = currentTime;

                double searchedTimestamp = currentTime + streams[0]->getTimeStampAtFrame(0);

                bool noUpdate = false;
                for(unsigned int i = 0; i < streams.size(); ++i){
                    std::shared_ptr<OrganizedPointCloud> pc = streams[i]->syncImage(searchedTimestamp);

                    if(pc != nullptr){
                        pointClouds.push_back(pc);
                    } else {
                        noUpdate = true;
                    }
                }

                processingTime = (duration_cast<microseconds>(high_resolution_clock::now() - startTime).count()*0.001f) * 0.1f + processingTime * 0.9f;

                if(callback && !noUpdate)
                    callback(pointClouds);

                // Just relax a little bit (especially for buffer reader):
                if(allowFrameSkipping)
                    std::this_thread::sleep_for(25ms);
                else
                    std::this_thread::sleep_for(1ms);
            }
        });
    }

    /**
     * Join and destroy reading thread on deconstruction.
     */
    ~AzureKinectMKVStreamer(){
        shouldStop = true;
        if(readingThread.joinable())
            readingThread.join();
    }

    /**
     * Steps a frame forward. If the parameter 'frameDelta' is given, it steps
     * the number of frames forward (or backward, when negative).
     */
    virtual void step(int frameDelta = 1) override {
        int newFrameID = streams[0]->getCurrentFrame() + frameDelta;

        if(newFrameID >= 0 && newFrameID < int(streams[0]->getTotalFrameCount()))
            currentTime = float(streams[0]->getAllTimestamps()[newFrameID] - streams[0]->getAllTimestamps()[0]);
    };

    /**
     * Returns the number of frames of the point cloud recording for the
     * master depth sensor (idx 0).
     */
    virtual float getTotalTime() override {
        return float(streams[0]->getTotalTime());
    };

    /**
     * Returns the CPU processing time in milliseconds per read frame of
     * sensor 0).
     */
    virtual float getProcessingTime() override {
        return processingTime;
    };
};
