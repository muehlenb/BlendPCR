// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#include "src/pcstreamer/Streamer.h"

#include "src/pcstreamer/AzureKinectMKVStreamer.h"

const char* Streamer::availableStreamerNames[] = { "- No streamer selected -", "CWIPC-SXR (Streamed)", "CWIPC-SXR (Buffered)"};

const unsigned int Streamer::availableStreamerNum = 3;

std::shared_ptr<Streamer> Streamer::constructStreamerInstance(int type, std::string filepath){
    if(type == 1){
        return std::make_shared<AzureKinectMKVStreamer>(filepath, false);
    } else if(type == 2){
        return std::make_shared<AzureKinectMKVStreamer>(filepath, true);
    }

    return nullptr;
};


int Streamer::BufferedMaxFrameCount = 200;
// Config for buffered loader, should be placed in AzureKinectMKVStreamer:
int Streamer::BufferedStartFrameOffset = 90;
