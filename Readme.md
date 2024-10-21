# BlendPCR: Seamless and Efficient Rendering of Dynamic Point Clouds captured by Multiple RGB-D Cameras

#### [Project Page](https://cgvr.cs.uni-bremen.de/projects/blendpcr) |  [Video](https://cgvr.cs.uni-bremen.de/projects/blendpcr/video.mp4) | [Paper](https://link_to_eg_digital_library)
[Andre Mühlenbrock](https://orcid.org/0000-0002-7836-3341), [Rene Weller](https://orcid.org/0009-0002-2544-4153), [Gabriel Zachmann](https://orcid.org/0000-0001-8155-1127)\
Computer Graphics and Virtual Reality Research Lab ([CGVR](https://cgvr.cs.uni-bremen.de/)), University of Bremen

This is our C++/OpenGL implementation of a real-time renderer BlendPCR, optimized for dynamic point clouds derived from multiple RGB-D cameras. It combines efficiency with high-quality rendering while effectively preventing common z-fighting-like seam flickering. The software is equipped to load and stream the CWIPC-SXR dataset for test purposes and comes with a GUI.

![image](images/teaser.jpg)


*Conditionally Accepted at ICAT-EGVE 2024*
 


## Requirements
 - **CMake** ≥ 3.5
 - **OpenGL** ≥ 3.3
 - **Azure Kinect SDK** 1.41
 - **nlohmann/json**
 
## Installation
TODO

## Run


### Source Mode
When loading the CWIPC-SXR dataset, you have two options:

- **CWIPC-SXR (Streamed):** This mode streams the RGB-D camera recordings directly from the hard drive. Operations such as reading from the hard drive, color conversion (MJPEG to BGRA32), and point cloud generation are performed on the fly. Real-time streaming is usually not feasible when using seven cameras.
- **CWIPC-SXR (Buffered):** This mode initially reads the complete RGB-D recordings, performs color conversion, and generates point clouds. While this process can be time-consuming and requires significant RAM, it enables subsequent real-time streaming of the recordings.

After choosing your preferred mode, a file dialog will appear, prompting you to select the `cameraconfig.json` file for the scene you wish to load. Playback will commence a few seconds or minutes after the selection, depending on the chosen Source Mode.


## Cite
Will be added as soon the paper is published.
