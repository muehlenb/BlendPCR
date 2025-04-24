# Define the minimum CMAKE version required:
cmake_minimum_required(VERSION 3.5)

option(USE_CUDA "Enable CUDA" OFF)
option(USE_BOOST "Enable BOOST" ON)

if(USE_CUDA)
    project(BlendPCR CUDA CXX)
else()
    project(BlendPCR CXX)
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set C++ stardard:
set(CMAKE_CXX_STANDARD 17)

# Enable automatic include of generated files (like paths.h):
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include IMGUI (which already includes GLFW):
add_subdirectory(lib/imgui-1.88/)

# Include GLAD for OpenGL 3.3 Core functionalities:
add_subdirectory(lib/glad)

# Definieren der Cache-Variablen zuerst
set(K4A_INCLUDE_DIR "" CACHE PATH "Path to directory which contains k4a and k4arecord includes")
set(K4A_LIB "" CACHE FILEPATH "Path to k4a.lib")
set(K4A_RECORD_LIB "" CACHE FILEPATH "Path to k4arecord.lib")

# Plattformspezifische Default-Werte setzen
if(WIN32)
    # Setze nur die Default-Werte, wenn die Benutzervariablen nicht bereits gesetzt wurden
    if("${K4A_INCLUDE_DIR}" STREQUAL "")
        set(K4A_INCLUDE_DIR "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\include" CACHE PATH "Path to directory which contains k4a and k4arecord includes" FORCE)
    endif()
    if("${K4A_LIB}" STREQUAL "")
        set(K4A_LIB "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\windows-desktop\\amd64\\release\\lib\\k4a.lib" CACHE FILEPATH "Path to k4a.lib" FORCE)
    endif()
    if("${K4A_RECORD_LIB}" STREQUAL "")
        set(K4A_RECORD_LIB "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\windows-desktop\\amd64\\release\\lib\\k4arecord.lib" CACHE FILEPATH "Path to k4arecord.lib" FORCE)
    endif()
endif()

# OpenMP (normally installed):
find_package(OpenMP)

# Default headers (which are usable without external libraries):
set(DEFAULT_HEADERS
    # PC Filter
    src/pcfilter/Filter.h

    # PC Renderer
    src/pcrenderer/Renderer.h
    src/pcrenderer/SplatRenderer.h
    src/pcrenderer/SimpleMeshRenderer.h
    src/pcrenderer/BlendPCR.h

    # PC Streamer
    src/pcstreamer/Streamer.h

    src/pcstreamer/AzureKinectMKVStreamer.h
    src/pcstreamer/azure_mkv/AzureKinectMKVStream.h

    # Point Cloud & other utils
    src/util/OrganizedPointCloud.h
    src/util/Semaphore.h

    src/util/TimeMeasurement.h

    src/util/opc/OPCAttachment.h

    # GL stuff
    src/util/gl/Shader.h
    src/util/gl/Texture2D.h
    src/util/gl/TextureFBO.h
    src/util/gl/GLMesh.h
    src/util/gl/GLRenderable.h

    # Primitives
    src/util/gl/primitive/Triangle.h
    src/util/gl/primitive/Vertex.h
    src/util/gl/primitive/TexCoord.h

    # Coordinate System
    src/util/gl/objects/GLCoordinateSystem.h

    # Math classes
    src/util/math/Vec4.h
    src/util/math/Mat4.h
)

# Default sources (which are usable without external libraries):
set(DEFAULT_SOURCES
    # PC Filter:
    src/pcfilter/Filter.cpp

    # PC Renderer:
    src/pcrenderer/Renderer.cpp

    # PC Streamer:
    src/pcstreamer/Streamer.cpp

    # GL stuff
    src/util/gl/Shader.cpp
    src/util/gl/Texture2D.cpp
    src/util/gl/TextureFBO.cpp
    src/util/gl/GLMesh.cpp

    # Coordinate System
    src/util/gl/objects/GLCoordinateSystem.cpp

    # Organized point cloud
    src/util/OrganizedPointCloud.cpp
)

# When CUDA 12.1 is enabled:
if(USE_CUDA)
    if(NOT USE_CUDA)
        # CUDA Lambda device functions:
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --extended-lambda")
    endif()

    # CUDA:
    find_package(CUDAToolkit 12.1 REQUIRED)

    # CUDA headers:
    set(CUDA_HEADERS
        src/pcfilter/SpatialHoleFiller.h
        src/pcfilter/ErosionFilter.h
        src/pcfilter/ClippingFilter.h
    )

    set(CUDA_SOURCES
        src/pcfilter/SpatialHoleFiller.cu
        src/pcfilter/ErosionFilter.cu
        src/pcfilter/ClippingFilter.cu
    )
endif()


if(USE_BOOST)
    # Boost asio to send image data (Unreal Engine Streamer):
    find_package(Boost REQUIRED COMPONENTS system thread)

    set(BOOST_HEADERS
        src/util/network/NetworkImageSender.h
    )
endif()

# Define shader & resources which should be listed in IDE:
set(RESOURCES
    shader/singleColorShader.vert
    shader/singleColorShader.frag

    shader/splats/splats.vert
    shader/splats/splats.frag

    shader/simple_mesh/simpleMesh.vert
    shader/simple_mesh/simpleMesh.geo
    shader/simple_mesh/simpleMesh.frag

    shader/blendpcr/filter/erosion.vert
    shader/blendpcr/filter/erosion.frag

    shader/blendpcr/filter/holeFilling.vert
    shader/blendpcr/filter/holeFilling.frag

    shader/blendpcr/pointcloud/rejection.vert
    shader/blendpcr/pointcloud/rejection.frag

    shader/blendpcr/pointcloud/edgeProximity.vert
    shader/blendpcr/pointcloud/edgeProximity.frag

    shader/blendpcr/pointcloud/mls.vert
    shader/blendpcr/pointcloud/mls.frag

    shader/blendpcr/pointcloud/normals.vert
    shader/blendpcr/pointcloud/normals.frag

    shader/blendpcr/pointcloud/qualityEstimate.vert
    shader/blendpcr/pointcloud/qualityEstimate.frag

    shader/blendpcr/screen/separateRendering.vert
    shader/blendpcr/screen/separateRendering.geo
    shader/blendpcr/screen/separateRendering.frag

    shader/blendpcr/screen/majorCam.vert
    shader/blendpcr/screen/majorCam.frag

    shader/blendpcr/screen/cameraWeights.vert
    shader/blendpcr/screen/cameraWeights.frag

    shader/blendpcr/screen/blending.vert
    shader/blendpcr/screen/blending.frag
)
# Allow to include files directly in this paths (without the need to specify folders):
include_directories(src src/util/math lib lib/stb_image lib/tiny_obj_loader)

set(HEADERS ${DEFAULT_HEADERS})
set(SOURCES ${DEFAULT_SOURCES})

if(USE_CUDA)
    list(APPEND HEADERS ${CUDA_HEADERS})
    list(APPEND SOURCES ${CUDA_SOURCES})
endif()

if(USE_BOOST)
    list(APPEND HEADERS ${BOOST_HEADERS})
    list(APPEND SOURCES ${BOOST_SOURCES})
endif()

list(APPEND SOURCES src/main.cpp)

# Define executables with source files and resources:
add_executable(BlendPCR ${SOURCES} ${HEADERS} ${RESOURCES})

# IMGUI specific compile definition:
target_compile_definitions(BlendPCR PUBLIC -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Define the libraries to link against:
target_link_libraries(BlendPCR PUBLIC imgui glad)

if(USE_CUDA)
    target_compile_definitions(BlendPCR PRIVATE USE_CUDA)
    target_include_directories(BlendPCR PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
    target_link_libraries(BlendPCR PRIVATE ${CUDA_LIBRARIES})
endif()

target_compile_definitions(BlendPCR PRIVATE USE_KINECT)

# Azure Kinect libraries
target_include_directories(BlendPCR PRIVATE ${K4A_INCLUDE_DIR})
target_link_libraries(BlendPCR PRIVATE ${K4A_LIB})
target_link_libraries(BlendPCR PRIVATE ${K4A_RECORD_LIB})

# OpenMP
target_link_libraries(BlendPCR PUBLIC OpenMP::OpenMP_CXX)

if(USE_BOOST)
    target_compile_definitions(BlendPCR PRIVATE USE_BOOST)
    target_link_libraries(BlendPCR PRIVATE Boost::boost Boost::system Boost::thread Boost::headers)
    #target_link_libraries(BlendPCR PRIVATE ${Boost_LIBRARIES})
    #target_include_directories(BlendPCR PRIVATE ${Boost_INCLUDE_DIRS})
endif()
