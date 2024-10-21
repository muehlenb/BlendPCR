// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#include "src/pcstreamer/Streamer.h"

#include "src/pcstreamer/AzureKinectMKVStreamer.h"

const char* Streamer::availableStreamerNames[] = { "- No streamer selected -", "CWIPC-SXR (Streamed)", "CWIPC-SXR (Buffered)"};

const unsigned int Streamer::availableStreamerNum = 2;

std::shared_ptr<Streamer> Streamer::constructStreamerInstance(int type, std::string filepath){
    if(type == 1){
        return std::make_shared<AzureKinectMKVStreamer>(filepath);
    }

    return nullptr;
};
