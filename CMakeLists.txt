# Define the minimum CMAKE version required:
cmake_minimum_required(VERSION 3.5)

project(BlendPCR CXX)

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

# OpenMP (normally installed):
find_package(OpenMP)

find_package(nlohmann_json CONFIG REQUIRED)

# Default headers (which are usable without external libraries):
set(DEFAULT_HEADERS
    # PC Filter
    src/pcfilter/Filter.h

    # PC Renderer
    src/pcrenderer/Renderer.h
    src/pcrenderer/SplatRenderer.h
    src/pcrenderer/SimpleMeshRenderer.h
    src/pcrenderer/BlendedMeshRenderer.h

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

# Define shader & resources which should be listed in IDE:
set(RESOURCES
)
# Allow to include files directly in this paths (without the need to specify folders):
include_directories(src src/util/math lib lib/stb_image lib/tiny_obj_loader)

set(HEADERS ${DEFAULT_HEADERS})
set(SOURCES ${DEFAULT_SOURCES})

list(APPEND SOURCES src/main.cpp)

# Define executables with source files and resources:
add_executable(BlendPCR ${SOURCES} ${HEADERS} ${RESOURCES})

# IMGUI specific compile definition:
target_compile_definitions(BlendPCR PUBLIC -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Define the libraries to link against:

target_link_libraries(BlendPCR PUBLIC imgui glad)

target_compile_definitions(BlendPCR PRIVATE USE_KINECT)

# Azure Kinect libraries
target_include_directories(BlendPCR PRIVATE "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\include\\")
target_link_libraries(BlendPCR PRIVATE "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\windows-desktop\\amd64\\release\\lib\\k4a.lib")
target_link_libraries(BlendPCR PRIVATE "C:\\Program Files\\Azure Kinect SDK v1.4.1\\sdk\\windows-desktop\\amd64\\release\\lib\\k4arecord.lib")

target_link_libraries(BlendPCR PRIVATE nlohmann_json::nlohmann_json)

# OpenMP
target_link_libraries(BlendPCR PUBLIC OpenMP::OpenMP_CXX)

# Precompiled Header konfigurieren
target_precompile_headers(BlendPCR PRIVATE)
