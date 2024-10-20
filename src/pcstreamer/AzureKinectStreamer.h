// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/pcstreamer/Streamer.h"

/**
 * A streamer when using a single or multiple Azure Kinect devices:
 */

class AzureKinectStreamer : public Streamer {
public:
    AzureKinectStreamer();
};
