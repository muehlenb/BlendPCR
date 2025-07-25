# BlendPCR: Seamless and Efficient Rendering of Dynamic Point Clouds captured by Multiple RGB-D Cameras
#### [Paper](https://diglib.eg.org/handle/10.2312/egve20241366) | [Video](https://www.youtube.com/watch?v=KBcpvIa-oNc) | [Slides](https://cgvr.cs.uni-bremen.de/papers/icategve24/slides/blendpcr_slides.pdf) | [Supplementary](https://cgvr.cs.uni-bremen.de/papers/icategve24/paper/blendpcr_supplementary.pdf) 

C++/OpenGL implementation of our real-time renderer BlendPCR for dynamic point clouds derived from multiple RGB-D cameras. It combines efficiency with high-quality rendering while effectively preventing common z-fighting-like seam flickering. The software is equipped to load and stream the CWIPC-SXR dataset for test purposes and comes with a GUI.

[Andre Mühlenbrock¹](https://orcid.org/0000-0002-7836-3341), [Rene Weller¹](https://orcid.org/0009-0002-2544-4153), [Gabriel Zachmann¹](https://orcid.org/0000-0001-8155-1127)\
¹Computer Graphics and Virtual Reality Research Lab ([CGVR](https://cgvr.cs.uni-bremen.de/)), University of Bremen

Presented at ICAT-EGVE 2024 **(Best Paper Award)**

![image](images/teaser.jpg)

## Updates
- CUDA filter replaced by full OpenGL 3.3 implementation
- WiP of Unreal Engine 5 VR integration (see `unreal_engine_5_streamer` branch)
- Performance Optimization
  - Bottleneck of uploading point clouds to GPU were solved by uploading `uint16_t*` depth image and generate the point cloud on the GPU



## Pre-built Binaries
If you only want to test the BlendPCR renderer, without editing the implementation, we also offer pre-built binaries:
- [Download Windows (64-Bit), main branch](https://cgvr.cs.uni-bremen.de/papers/icategve24/builds/blendpcr_win64_main.html), *(fixed shader paths)*, OpenGL 3.3 Only Version 
  
**Current Benchmark**, rendering at **3580 x 2066** while fusing **7** Microsoft Azure Kinects @ 30 Hz simultaneously on NVIDIA GeForce 4090 RTX:
- Single Person: approx. **240 fps**
- Whole Scene: approx. **140 fps**

## Build Requirements

### Required:
 - **CMake** ≥ 3.11
 - **OpenGL** ≥ 3.3
 - **C++ Compiler**, e.g. MSVC v143
 - **Azure Kinect SDK 1.4.1**: Required to load and stream the CWIPC-SXR dataset.

*Note: As the C++ compiler, we have currently only tested MSVC, but other compilers that support the Azure Kinect SDK 1.4.1 are likely to work as well.*

### Optional:
 - **CUDA Toolkit 12.1:** CUDA Kernels are currently only used for a *SpatialHoleFiller*, *ErosionFilter* and *ClippingFilter*. We have reimplemented these filters as GLSL passes in case of *BlendPCR*, so even without CUDA almost the same visual quality is achieved as presented in the paper. To enable CUDA compilation, you need to explicitly set the CMake variable `USE_CUDA` to `ON`.
 

*Additionally, this project uses small open-source libraries that we have directly integrated into our source code, so no installation is required. You can find them in the `lib` folder. 
A big thank you to the developers of
[Dear ImGui 1.88](https://github.com/ocornut/imgui),
[nlohmann/json](https://github.com/nlohmann/json),
[GLFW 3.3](https://www.glfw.org/),
[stb_image.h](https://github.com/nothings/stb),
[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader),
[imfilebrowser](https://github.com/AirGuanZ/imgui-filebrowser), and
[GLAD](https://gen.glad.sh/).*
## Build from Source
### Azure Kinect SDK 1.4.1
This project has been tested with Azure Kinect SDKs version 1.4.1, although other SDK versions may also be compatible. 

On Windows, you can install `Azure Kinect SDK 1.4.1.exe` from the official website using the default paths. Once installed, the program should be buildable and executable, since the *CMakeLists.txt* is configured to search at default paths.

If you use custom paths or are operating on Linux, please set the following CMAKE-variables:
 - `K4A_INCLUDE_DIR` to the directory containing the `k4a` and `k4arecord` folders with the include files. 
 - `K4A_LIB` to the file path of `k4a.lib` 
 - `K4A_RECORD_LIB` to the file path of `k4arecord.lib`

**Note:** Current usage of the *k4a* and *k4arecord* libraries included with **vcpkg** might lead to errors, as both libraries seem to be configured to create an spdlog instance with the same name.

### BlendPCR

 1) After installing Azure Kinect SDK 1.4.1, simply clone the BlendPCR repository and run CMake. 
 2) If you don't use Windows or installed the Azure Kinect SDK to a custom path, configure the variables above. 
 3) Build & Run.

## Run
![image](images/screenshot.jpg)

### CWIPC-SXR Dataset
To use the renderer out-of-the-box, RGB-D recordings from seven Azure Kinect sensors are required, and these recordings must conform to the format of the CWIPC-SXR dataset.

You can download the CWIPC-SXR dataset here: **[CWIPC-SXR Dataset](https://www.dis.cwi.nl/cwipc-sxr-dataset/downloads/)**. 

It is recommended to download only the `dataset_hierarchy.tgz`, which provides metadata for all scenes, as the entire dataset is very large (1.6TB). To download a specific scene, such as the *S3 Flight Attendant* scene, navigate to the `s3_flight_attendant/r1_t1/` directory and run the `download_raw.sh` file, which downloads the `.mkv` recordings from all seven cameras. After downloading, ensure that the `.mkv` recordings are located in the `raw_files` folder. The scene is now ready to be opened in this software project.

### Source Mode
When loading the CWIPC-SXR dataset, you have two options:

- **CWIPC-SXR (Streamed):** This mode streams the RGB-D camera recordings directly from the hard drive. Operations such as reading from the hard drive, color conversion (MJPEG to BGRA32), and point cloud generation are performed on the fly. Real-time streaming is usually not feasible when using seven cameras.
- **CWIPC-SXR (Buffered):** This mode initially reads the complete RGB-D recordings, performs color conversion, and generates point clouds. While this process can be time-consuming and requires significant RAM, it enables subsequent real-time streaming of the recordings. *(**Note:** (1) Due to memory requirements, no high resolution color textures are loaded and (2) don't use this buffered version with CUDA filters, since the CUDA filters alter the buffer during playback)*

After choosing your preferred mode, a file dialog will appear, prompting you to select the `cameraconfig.json` file for the scene you wish to load. Playback will commence a few seconds or minutes after the selection, depending on the chosen Source Mode.

### Filters
The filter section is intended for the implemented CUDA filters. If compiled without CUDA, no filters can be selected.

When compiled with CUDA, the used filter configuration which was used in the paper is activated by default.

### Rendering Technique
You can switch between following rendering techniques:

- **Splats (Uniform):** Using uniform splats with a fixed (configurable) size.
- **Naive Mesh:** Separate Meshes reconstructed for each camera, which are not blended.
- **BlendPCR:** The BlendPCR implementation, which we described in our paper. Note that you can activate the 'Reimpl. Filters' option, which enables a GLSL reimplementation of the CUDA filters we used in the evaluation of our paper. 

*Note: For High Resolution Color Textures - named **BlendPCR (HR)** in the paper -, enable **High Resolution Encoding** both in the Source Mode **and** in the BlendPCR renderer.*

## Remarks
### Point Cloud Filters via CUDA

In our research paper, we conducted visual comparisons among the SplatRenderer, Simple Mesh Renderer, TSDF, and BlendPCR. Each renderer used the same set of CUDA filters (*ErosionFilter*, *SpatialHoleFilter*, and *ClippingFilter*), located in the `src/pcfilter/` folder. To use these CUDA filters instead of the reimplemented GLSL filters, enable them in the "Filter" section and disable "Reimpl. Filters" in the BlendPCR section. Note that CUDA filters can only be used if `USE_CUDA` was set to `ON` in the CMAKE configuration during compilation.

Please note that *SpatialHoleFiller* and *ErosionFilter* have also been implemented as an initial GLSL pass in the `BlendPCR` renderer, which is enabled by default. This allows us to achieve similar visual quality even without CUDA.

**Note on visual quality**: Without CUDA, the SplatRenderer and Simple Mesh Renderer may display lower visual quality than shown in our paper, though the BlendPCR renderer will maintain almost the same quality. Nevertheless, there are further possibilities for improving quality, for example by improving the reimplemented erosion and hole filling filters (try to activate and deactive the reimpl. filters).


## Results
### Visual comparison in CWIPC-SXR, S3 Flight Attentant Scene
![image](https://cgvr.cs.uni-bremen.de/papers/icategve24/images/compare_flight_att.jpg)

<sub>1: Note that both Pointersect and P2ENet are rendered from slightly different perspectives and use slightly different preprossesing filters (in terms of erosion & hole filling). Both renderings are taken from the Supplemental Material of *HU Y., GONG R., SUN Q., WANG Y.: Low latency point cloud rendering with learned splatting. In Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (CVPR) Workshops (June 2024), pp. 5752–5761*.</sub>

### Visual comparison in CWIPC-SXR, S7 Scarf Dressing Scene
![image](https://cgvr.cs.uni-bremen.de/papers/icategve24/images/compare_scarf.jpg)

<sub>1: Note that both Pointersect and P2ENet are rendered from slightly different perspectives and use slightly different preprossesing filters (in terms of erosion & hole filling). Both renderings are taken from the Supplemental Material of *HU Y., GONG R., SUN Q., WANG Y.: Low latency point cloud rendering with learned splatting. In Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (CVPR) Workshops (June 2024), pp. 5752–5761*.</sub>

### Performance
The performance in default configuration for different numbers of cameras, divided by point cloud passes and screen passes on an NVIDIA GeForce RTX 4090 using a resolution of 3580 x 2066.

![image](https://cgvr.cs.uni-bremen.de/papers/icategve24/images/performance_cameras.jpg)

For further details, see [our paper](https://diglib.eg.org/handle/10.2312/egve20241366) .

## Cite
```
@inproceedings{10.2312:egve.20241366,
    booktitle = {ICAT-EGVE 2024 - International Conference on Artificial Reality and Telexistence and Eurographics Symposium on Virtual Environments},
    editor = {Hasegawa, Shoichi and Sakata, Nobuchika and Sundstedt, Veronica},
    title = {{BlendPCR: Seamless and Efficient Rendering of Dynamic Point Clouds captured by Multiple RGB-D Cameras}},
    author = {Mühlenbrock, Andre and Weller, Rene and Zachmann, Gabriel},
    year = {2024},
    publisher = {The Eurographics Association},
    ISSN = {1727-530X},
    ISBN = {978-3-03868-245-5},
    DOI = {10.2312/egve.20241366}
}
```
