// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#include "src/pcstreamer/Streamer.h"

#include "src/pcstreamer/AzureKinectMKVStreamer.h"

const char* Streamer::availableStreamerNames[] = { "Azure Kinect (MKV)"};

const unsigned int Streamer::availableStreamerNum = 1;

std::shared_ptr<Streamer> Streamer::constructStreamerInstance(int type){
    if(type == 0){
        return std::make_shared<AzureKinectMKVStreamer>();
    }

    if(type >= 0 && type < int(sizeof(availableStreamerNames)))
        std::cout << "Created " << availableStreamerNames[type] << std::endl;

    return nullptr;
};
