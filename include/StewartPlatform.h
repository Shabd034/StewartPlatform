#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class StewartPlatform 
{
public:
    StewartPlatform();
    void Start(int width, int height);
    void SetPlatformPosition(glm::vec3 normal, glm::vec3 position);
    bool PlatformOnTarget();

private:
    void SetupBuffers();
    void RenderScene();
    void ProcessInput();
    void DrawLeg(const glm::vec3& start, const glm::vec3& end);
    void DrawPlate();
    glm::vec4 CalculatePlane();
    float CalculateRotationAngle(const glm::vec3& u, const glm::vec3& v);
    glm::mat4 FindRotationAndTranslationToPlane();
    void UpdateTargetLegEnds(glm::vec3 normal, glm::vec3 position);
    void UpdateLegEnds();
};