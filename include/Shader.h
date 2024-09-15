#pragma once

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>


class Shader
{
    public:
        uint ID;
        Shader(std::string vertexPath, std::string fragmentPath);
        Shader(std::string geometryPath, std::string vertexPath, std::string fragmentPath);
        void Use();
        void Delete();

        // utility uniform functions
        void setUniformValue(const std::string &name, bool value);  
        void setUniformValue(const std::string &name, int value);   
        void setUniformValue(const std::string &name, float value);

        void setVec2(const std::string &name, const glm::vec2 &value);
        void setVec2(const std::string &name, float x, float y);
        void setVec3(const std::string &name, const glm::vec3 &value);
        void setVec3(const std::string &name, float x, float y, float z);
        void setVec4(const std::string &name, const glm::vec4 &value);
        void setVec4(const std::string &name, float x, float y, float z, float w);
        void setMat2(const std::string &name, const glm::mat2 &mat);
        void setMat3(const std::string &name, const glm::mat3 &mat);
        void setMat4(const std::string &name, const glm::mat4 &mat);
        void setFloat(const std::string &name, float value);

    private:
        std::string ReadShaderSource(const std::string& filePath);
};