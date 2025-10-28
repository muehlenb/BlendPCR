#include "util/gl/Shader.h"

// Loading files in C++:
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <regex>

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

Shader::Shader(std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath)
    : numOfCopies(new int(1)){

    createShaderProgram(vertexShaderPath, fragmentShaderPath, geometryShaderPath);
}

/**
 * This is an explicit copy constructor which is automatically
 * called every time an existing Shader is assigned to a variable
 * via the = Operator. Not important for CG1 and C++ specific,
 * but it is needed here to avoid problems because of early
 * deletion of the shaderProgram when the destructor is called.
 */
Shader::Shader(const Shader& shader){
    shaderProgram = shader.shaderProgram;
    numOfCopies = shader.numOfCopies;
    initialized = shader.initialized;
    uniformLocationMap = shader.uniformLocationMap;
    ++(*numOfCopies);
}

Shader::~Shader(){
    // We only want the shaderProgram to be destroyed
    // when the last copy of the shader is deleted:
    if(--(*numOfCopies) > 0)
        return;

    // If there is a compiled & linked shaderProgram, delete it from the GPU:
    if(shaderProgram != 0)
        glDeleteProgram(shaderProgram);

    if(uniformLocationMap != nullptr)
        delete uniformLocationMap;

    // Delete also the numOfCopies variable:
    delete numOfCopies;
}

void Shader::createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath) {
    shaderFiles.clear();

    // Get folder path to vertex shader for include reasons:
    std::filesystem::path fullPath(vertexShaderPath);
    folderPath = fullPath.parent_path().string();

    // Load vertex shader source as string:
    std::ifstream ifs(vertexShaderPath);
    std::string vertexShaderSourceString(std::istreambuf_iterator<char>{ifs}, {});
    processIncludes(vertexShaderSourceString);
    const char* vertexShaderSource = vertexShaderSourceString.c_str();
    ifs.close();

    shaderFiles.push_back(ShaderFile(vertexShaderPath, std::filesystem::last_write_time(vertexShaderPath)));

    // Load fragment shader source as string:
    ifs = std::ifstream(fragmentShaderPath);
    std::string fragmentShaderSourceString(std::istreambuf_iterator<char>{ifs}, {});
    processIncludes(fragmentShaderSourceString);
    const char* fragmentShaderSource = fragmentShaderSourceString.c_str();
    ifs.close();

    shaderFiles.push_back(ShaderFile(fragmentShaderPath, std::filesystem::last_write_time(fragmentShaderPath)));

    // Should the geometry shader be loaded?
    bool useGeometryShader = geometryShaderPath != "";

    // Create temporary pointers to the compiled vertex and fragment shader:
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int geometryShader;

    // Upload VERTEX SHADER source code to GPU and compile it:
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Check if compilation was successful, otherwise print error:
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "Vertex Shader Compilation failed:\n" << "File: " << vertexShaderPath << infoLog << std::endl;
    }

    // Upload FRAGMENT SHADER source code to GPU and compile it:
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Check if compilation was successful, otherwise print error to console:
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "Fragment Shader Compilation failed:\n" << "File: " << fragmentShaderPath << infoLog << std::endl;
    }

    if (useGeometryShader) {
        // Load fragment shader source as string:
        ifs = std::ifstream(geometryShaderPath);
        std::string geometryShaderSourceString(std::istreambuf_iterator<char>{ifs}, {});
        processIncludes(geometryShaderSourceString);
        const char* geometryShaderSource = geometryShaderSourceString.c_str();
        ifs.close();

        geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometryShader, 1, &geometryShaderSource, nullptr);
        glCompileShader(geometryShader);
        // print compile errors if any
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(geometryShader, 512, nullptr, infoLog);
            std::cout << "Geometry Shader Compilation failed:\n" << "File: " << geometryShaderPath << infoLog << std::endl;
        }
        shaderFiles.push_back(ShaderFile(geometryShaderPath, std::filesystem::last_write_time(geometryShaderPath)));
    }

    // Finally create the shader program which contains both vertex and fragment shader:
    shaderProgram = glCreateProgram();

    if(shaderProgram == 0)
        std::cout << "Error creating shader program " << shaderProgram << " (bug?): " << vertexShaderPath << std::endl;

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    if (useGeometryShader)
        glAttachShader(shaderProgram, geometryShader);

    glLinkProgram(shaderProgram);

    // Check if linking of shaders was successful, otherwise print error to console:
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << vertexShaderPath << std::endl;
    }

    // After shaders were linked to a shader program, we don't need the
    // compiled shaders anymore to run the shader program, so we can delete
    // it on the GPU:
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (useGeometryShader)
        glDeleteShader(geometryShader);

    if (success)
        initialized = true;
}

void Shader::processIncludes(std::string& sourceCode) {
    std::regex includeRegex(R"x(#include\s*"([^"]*)")x");
    std::smatch match;

    // Parse the source code for all instances of '#include ...':
    while (std::regex_search(sourceCode, match, includeRegex)) {
        std::string includeDirective = match[0];
        std::string includeFilePath = match[1];
        includeFilePath = folderPath + "/" + includeFilePath;

        // Read the content of the include file:
        std::ifstream includeFile(includeFilePath);
        if (!includeFile.is_open()) {
            std::cout << "ERROR::SHADER: Failed to open included file '" << includeFilePath << "'" << std::endl;
            break;
        }
        std::string includedContent((std::istreambuf_iterator<char>(includeFile)), {});
        includeFile.close();

        // Replace the #include directive with the content of the file:
        sourceCode.replace(match.position(0), includeDirective.length(), includedContent);
    }
}

void Shader::hotReloadCheck() {
    // Check all shader files for changes:
    for (ShaderFile& shaderFile : shaderFiles) {
        std::filesystem::file_time_type currentLastWriteTime = std::filesystem::last_write_time(shaderFile.path);

        // Reload if the shader file was modified:
        if (currentLastWriteTime > shaderFile.lastModifiedTime) {
            // If any change is found, all the files will be reloaded and a new shader program will be created:
            if(shaderFiles.size() > 2)
                createShaderProgram(shaderFiles[0].path, shaderFiles[1].path, shaderFiles[2].path);
            else
                createShaderProgram(shaderFiles[0].path, shaderFiles[1].path);
            std::cout << "Shader recompiled successfully." << std::endl;
            break;
        }
    }
}

void Shader::bind(){
    //hotReloadCheck();
    glUseProgram(shaderProgram);
}

void Shader::setUniform(std::string name, Mat4f value){
    if(!initialized)
        return;

    // Gets the location ID where the uniform variable called <name> is stored
    // in GPU memory:
    int loc = glGetUniformLocation(shaderProgram, name.c_str());

    // If the uniform variable exists, upload the values of the matrix to the GPU:
    if(loc != -1)
        glUniformMatrix4fv(loc, 1, GL_FALSE, value.data);
}

void Shader::setUniform(std::string name, Vec4f value, int num){
    if(!initialized)
        return;

    // Gets the location ID where the uniform variable called <name> is stored
    // in GPU memory:
    int loc = glGetUniformLocation(shaderProgram, name.c_str());

    // If the uniform variable exists, upload the values of the vector to the GPU:
    if(loc != -1){
        if(num == 4)
            glUniform4f(loc, value.x, value.y, value.z, value.w);
        else if(num == 3)
            glUniform3f(loc, value.x, value.y, value.z);
        else if(num == 2)
            glUniform2f(loc, value.x, value.y);
    }
}

void Shader::setUniform(std::string name, float value){
    if(!initialized)
        return;

    // Gets the location ID where the uniform variable called <name> is stored
    // in GPU memory:
    int loc = glGetUniformLocation(shaderProgram, name.c_str());

    // If the uniform variable exists, upload the float value to the GPU:
    if(loc != -1)
        glUniform1f(loc, value);
}

void Shader::setUniform(std::string name, int value){
    if(!initialized)
        return;

    // Gets the location ID where the uniform variable called <name> is stored
    // in GPU memory:
    int loc = glGetUniformLocation(shaderProgram, name.c_str());

    // If the uniform variable exists, upload the integer value to the GPU:
    if(loc != -1)
        glUniform1i(loc, value);
}
