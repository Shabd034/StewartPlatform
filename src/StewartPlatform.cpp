#include <chrono>
#include <cmath>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <mutex>
#include <random>
#include <vector>

#include "Shader.h"
#include "StewartPlatform.h"

// OpenGL controls
int gNumVertices = 0;
bool gIsDragging = false;
double gLastX = 0;
double gLastY = 0;
glm::mat4 gRotationMatrix = glm::mat4(1.0f);
Shader* pShaderProgram = nullptr;
GLFWwindow* gWindow = nullptr;
GLuint legVAO, legVBO;
GLuint plateVAO, plateVBO;
GLuint ballVAO, ballVBO;
std::mutex targetLegEndsMutex;
std::mutex ballMutex;

const float speedActuators = 3;
 
// Platform control
const int NUM_LEGS = 6;
int numSegments = 100;

// scale
const float baseDiameter = 1.5;
const float gBaseRadius = 0.6f;
const float initHeight = 1.4;

const float worldToGl = (gBaseRadius * 2) / baseDiameter;

//ball
const int numLatitudeSegments = 50;
const int numLongitudeSegments = 50;
const float ballRadius = 0.075f;
glm::vec3 ballPosition = glm::vec3(0.0f, initHeight + ballRadius, 0.0f);
glm::vec3 ballVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ballAcc = glm::vec3(0.0f, 0.0f, 0.0f);
auto lastTime = std::chrono::system_clock::now();

bool onTarget = true;
bool firstTimeChange = true;

// leg speeds
float legSpeeds[NUM_LEGS] = {speedActuators, speedActuators, speedActuators, speedActuators, speedActuators, speedActuators};

const glm::vec3 legStarts[NUM_LEGS] = 
{
    glm::vec3(gBaseRadius * cos(M_PI / 18), 0.0f, gBaseRadius * sin(M_PI / 18)),
    glm::vec3(gBaseRadius * cos(-M_PI / 18), 0.0f, gBaseRadius * sin(-M_PI / 18)),
    glm::vec3(gBaseRadius * cos((2 * M_PI / 3) + M_PI / 18), 0.0f, gBaseRadius * sin((2 * M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos((2 * M_PI / 3) - M_PI / 18), 0.0f, gBaseRadius * sin((2 * M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos((4 * M_PI / 3) + M_PI / 18), 0.0f, gBaseRadius * sin((4 * M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos((4 * M_PI / 3) - M_PI / 18), 0.0f, gBaseRadius * sin((4 * M_PI / 3) - M_PI / 18))
};

glm::vec3 legEnds[NUM_LEGS] = 
{
    glm::vec3(gBaseRadius * cos((M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(- (M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin(- (M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI - M_PI / 18), initHeight, gBaseRadius * sin(M_PI - M_PI / 18)),
    glm::vec3(gBaseRadius * cos((M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos((5 * M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((5 * M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI + M_PI / 18), initHeight, gBaseRadius * sin(M_PI + M_PI / 18))
};

glm::vec3 initialPositions[NUM_LEGS] = 
{
    glm::vec3(gBaseRadius * cos((M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(- (M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin(- (M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI - M_PI / 18), initHeight, gBaseRadius * sin(M_PI - M_PI / 18)),
    glm::vec3(gBaseRadius * cos((M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos((5 * M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((5 * M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI + M_PI / 18), initHeight, gBaseRadius * sin(M_PI + M_PI / 18))
};

glm::vec3 targetLegEnds[NUM_LEGS] = 
{
    glm::vec3(gBaseRadius * cos((M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(- (M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin(- (M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI - M_PI / 18), initHeight, gBaseRadius * sin(M_PI - M_PI / 18)),
    glm::vec3(gBaseRadius * cos((M_PI / 3) + M_PI / 18), initHeight, gBaseRadius * sin((M_PI / 3) + M_PI / 18)),
    glm::vec3(gBaseRadius * cos((5 * M_PI / 3) - M_PI / 18), initHeight, gBaseRadius * sin((5 * M_PI / 3) - M_PI / 18)),
    glm::vec3(gBaseRadius * cos(M_PI + M_PI / 18), initHeight, gBaseRadius * sin(M_PI + M_PI / 18))
};

// Mouse button callback
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) 
    {
        if (action == GLFW_PRESS)
        {
            gIsDragging = true;
            glfwGetCursorPos(window, &gLastX, &gLastY);
        } 
        else if (action == GLFW_RELEASE) 
        {
            gIsDragging = false;
        }
    }
}

// Cursor position callback
static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) 
{
    static float lastX = xpos;
    float xOffset = xpos - lastX;
    lastX = xpos;

    // Sensitivity factor
    float sensitivity = 0.3f;
    xOffset *= sensitivity;

    // Update rotation angles
    if (gIsDragging) 
    {
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(xOffset), glm::vec3(0.0f, 1.0f, 0.0f));
        gRotationMatrix = rotY * gRotationMatrix;
    }
}

// Window resize callback
static void Reshape(GLFWwindow* window, int width, int height) 
{
    glViewport(0, 0, width, height);
}

StewartPlatform::StewartPlatform() 
{
    if (!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void StewartPlatform::SetPlatformNormal(glm::vec3 normal)
{
    if (normal == glm::vec3(0.0f)) 
    {
        return;
    }

    if (normal.y <= 0.1f) 
    {
        return;
    }

    UpdateTargetLegEnds(normal);
    onTarget = false;
    firstTimeChange = true;
}

void StewartPlatform::SetupBuffers() 
{
    // Legs
    glGenVertexArrays(1, &legVAO);
    glGenBuffers(1, &legVBO);
    glBindVertexArray(legVAO);

    glBindBuffer(GL_ARRAY_BUFFER, legVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Plate
    std::vector<float> vertices;

    float radius = baseDiameter / 2.0f; // 15 cm
    int numSegments = 100; // Define the number of segments for the circle
    for (int i = 0; i <= numSegments; ++i) 
    {
        float angle = 2.0f * M_PI * float(i) / float(numSegments);  // Calculate angle
        float x = radius * cos(angle);  // Calculate x position
        float z = radius * sin(angle);  // Calculate z position (changed from y)
        vertices.push_back(x); // X position
        vertices.push_back(initHeight); // Y position
        vertices.push_back(z); // Z position
    }

    glGenVertexArrays(1, &plateVAO);
    glGenBuffers(1, &plateVBO);

    glBindVertexArray(plateVAO);

    glBindBuffer(GL_ARRAY_BUFFER, plateVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ball
    vertices.clear();

    for (int lat = 0; lat <= numLatitudeSegments; ++lat)
    {
        float phi = M_PI * float(lat) / float(numLatitudeSegments);
        float y = ballRadius * cos(phi);

        for (int lon = 0; lon <= numLongitudeSegments; ++lon) 
        {
            float theta = 2.0f * M_PI * float(lon) / float(numLongitudeSegments);
            float x = ballRadius * sin(phi) * cos(theta);
            float z = ballRadius * sin(phi) * sin(theta);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    glGenVertexArrays(1, &ballVAO);
    glGenBuffers(1, &ballVBO);

    glBindVertexArray(ballVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Draw each leg between the specified start and end points
void StewartPlatform::DrawLeg(const glm::vec3& start, const glm::vec3& end) 
{
    // Bind the VAO containing the line vertices
    glBindVertexArray(legVAO);

    // Create a model matrix to transform the line from start to end
    glm::mat4 model = glm::mat4(1.0f);

    model = model * gRotationMatrix;

    // Send the model matrix to the shader
    pShaderProgram->setMat4("model", model);

    // Update the line vertices (assuming you use glBufferSubData or a similar method to update the line VAO dynamically)
    glm::vec3 vertices[2] = { start, end };
    glBindBuffer(GL_ARRAY_BUFFER, legVBO);  // Assuming legVBO is the VBO associated with legVAO
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    // Draw the line representing the leg
    glDrawArrays(GL_LINES, 0, 2);

    // Unbind the VAO
    glBindVertexArray(0);
}

void StewartPlatform::UpdateTargetLegEnds(glm::vec3 normal)
{
    std::lock_guard<std::mutex> lock(targetLegEndsMutex);

    // Compute centroid of current leg ends
    glm::vec3 centroid(0.0f);
    for (int i = 0; i < NUM_LEGS; ++i)
        centroid += legEnds[i];
    centroid /= static_cast<float>(NUM_LEGS);

    // Compute current plane normal
    glm::vec4 plane = CalculatePlane();
    glm::vec3 planeNormal = glm::vec3(plane.x, plane.y, plane.z);

    // Compute rotation axis and angle
    glm::vec3 rotationAxis = glm::cross(planeNormal, normal);
    float angle = CalculateRotationAngle(planeNormal, normal);

    // Handle degenerate axis
    if (glm::length(rotationAxis) < 1e-6f)
    {
        if (glm::dot(planeNormal, normal) > 0.0f)
        {
            rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            angle = 0.0f;
        }
        else
        {
            rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            angle = glm::pi<float>();
        }
    }
    else
    {
        rotationAxis = glm::normalize(rotationAxis);
    }

    // Build transformation: move to origin, rotate, move back
    glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -centroid);
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
    glm::mat4 back = glm::translate(glm::mat4(1.0f), centroid);

    glm::mat4 transform = back * rotate * toOrigin;

    // Apply transformation to each leg end
    for (int i = 0; i < NUM_LEGS; ++i)
    {
        glm::vec4 leg = glm::vec4(legEnds[i], 1.0f);
        targetLegEnds[i] = glm::vec3(transform * leg);
    }
}

// Function to calculate the plane from 6 points
glm::vec4 StewartPlatform::CalculatePlane() 
{
    glm::vec3 v1 = legEnds[1] - legEnds[0];
    glm::vec3 v2 = legEnds[2] - legEnds[0];
    glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

    float D = -glm::dot(normal, legEnds[0]);

    return glm::vec4(normal, D);
}

float StewartPlatform::CalculateRotationAngle(const glm::vec3& u, const glm::vec3& v)
{
    // Normalize both vectors
    glm::vec3 normU = glm::normalize(u);
    glm::vec3 normV = glm::normalize(v);

    // Calculate the dot product
    float dotProduct = glm::dot(normU, normV);

    // Calculate the angle using acos
    float angle = acos(dotProduct);
    return angle;
}

glm::mat4 StewartPlatform::FindRotationAndTranslationToPlane()
{
    glm::vec3 initialNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec4 plane = CalculatePlane();
    glm::vec3 planeNormal = glm::vec3(plane.x, plane.y, plane.z);
    glm::vec3 rotationAxis = glm::cross(initialNormal, planeNormal);

    glm::vec3 Q = glm::vec3(0.0f, initHeight, 0.0f);
    glm::vec3 targetCentroid(0.0f);
    for (int i = 0; i < 6; ++i)
    {
        targetCentroid += legEnds[i];
    }
    targetCentroid /= 6.0f;

    glm::vec3 translation = targetCentroid - Q;

    // Calculate the rotation angle from initial normal to plane normal
    float angle = CalculateRotationAngle(initialNormal, planeNormal);

    glm::mat4 translationToOriginMatrix = glm::translate(glm::mat4(1.0f), -targetCentroid);
    glm::mat4 translationBackMatrix = glm::translate(glm::mat4(1.0f),targetCentroid);

    glm::mat4 rotationMatrix;
    if (glm::length(rotationAxis) < 1e-6f) 
    {
        if (glm::dot(initialNormal, planeNormal) > 0.0f) 
        {
            rotationMatrix = glm::mat4(1.0f);
        } 
        else 
        {
            rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            angle = glm::pi<float>();
            rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
        }
    } 
    else 
    {
        rotationAxis = glm::normalize(rotationAxis);
        rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
    }

    // Translate object to origin, rotate and translate back
    // Apply the actual translation to the plane
    glm::mat4 actualTranslationMatrix = glm::translate(glm::mat4(1.0f), translation);

    glm::mat4 transformationMatrix = translationBackMatrix * rotationMatrix * translationToOriginMatrix * actualTranslationMatrix;

    return transformationMatrix;
}

void StewartPlatform::DrawPlate()
{
    glBindVertexArray(plateVAO);

    glm::mat4 endsPlane = FindRotationAndTranslationToPlane();
    glm::mat4 model = gRotationMatrix * endsPlane;
    pShaderProgram->setMat4("model", model);

    glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments);
    glBindVertexArray(0);
}

void StewartPlatform::DrawBall()
{
    glBindVertexArray(ballVAO);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), ballPosition);
    pShaderProgram->setMat4("model", model);

    glDrawArrays(GL_TRIANGLE_FAN, 0, (numLatitudeSegments) * (numLongitudeSegments));
    glBindVertexArray(0);
}

void StewartPlatform::UpdateLegEnds()
{
    if (firstTimeChange)
    {
        int maxChangeindex = -1;
        float legDistanceChanges[NUM_LEGS];
        targetLegEndsMutex.lock();
        for (int i = 0; i < NUM_LEGS; i++) 
        {
            glm::vec3 diff = targetLegEnds[i] - legEnds[i];
            float length = glm::length(diff);
            legDistanceChanges[i] = length;
        }
        targetLegEndsMutex.unlock();

        float maxDistanceLength = -1.0f;
        for (int i = 0; i < NUM_LEGS; i++) 
        {
            if (legDistanceChanges[i] > maxDistanceLength) 
            {
                maxDistanceLength = legDistanceChanges[i];
                maxChangeindex = i;
            }
        }

        float time = maxDistanceLength / speedActuators;
        for (int i = 0; i < NUM_LEGS; i++) 
        {
            legSpeeds[i] = legDistanceChanges[i] / time;
        }

        firstTimeChange = false;
    }

    if (!onTarget)
    {
        glm::vec3 legEndsNewTarget[NUM_LEGS];

        targetLegEndsMutex.lock();
        for (int i = 0; i < NUM_LEGS; i++) 
        {
            glm::vec3 diff = targetLegEnds[i] - legEnds[i];
            float length = glm::length(diff);
            float change = legSpeeds[i] * 0.001f;

            if (length < change) 
            {
                legEnds[i] = targetLegEnds[i];
            } 
            else 
            {
                legEnds[i] += change * glm::normalize(diff);
            }

            legEndsNewTarget[i] = targetLegEnds[i];
        }
        targetLegEndsMutex.unlock();

        for (int i = 0; i < NUM_LEGS; i++) 
        {
            if (glm::distance(legEnds[i], legEndsNewTarget[i]) > 1e-5f) 
            {
                return;
            }
        }

        onTarget = true;
    }
}

void StewartPlatform::GetBallPosition(glm::vec3& position)
{
    ballMutex.lock();
    position = ballPosition;
    ballMutex.unlock();
}

void StewartPlatform::RenderScene() 
{
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int width, height;
    glfwGetFramebufferSize(gWindow, &width, &height);

    Shader shaderProgram = *pShaderProgram;
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    shaderProgram.Use();
    shaderProgram.setMat4("view", view);
    shaderProgram.setMat4("projection", projection);
    shaderProgram.setVec3("color", 1.0f, 0.5f, 0.2f);

    //Update ball position
    auto now = std::chrono::system_clock::now();
    auto diff = now - lastTime;
    float seconds = std::chrono::duration<float>(diff).count();

    glm::vec3 distance = ballVelocity * (float)seconds + 0.5f * ballAcc * (seconds * seconds);
    distance *= worldToGl;
    ballMutex.lock();
    ballPosition += distance;
    ballMutex.unlock();
    ballVelocity += ballAcc * (float)seconds;

    glm::vec4 plane = CalculatePlane();
    glm::vec3 normal = glm::vec3(plane.x, plane.y, plane.z);
    
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    // normalize
    glm::vec3 unitNormal = glm::normalize(normal);

    float plateRadius = baseDiameter / 2.0f;
    float ballHeightOnPlane = glm::dot(glm::vec3(ballPosition), unitNormal) + plane.w;
    // Project ball position onto plate plane
    glm::vec3 plateCenter = glm::vec3(0.0f, initHeight, 0.0f);
    // Find closest point on plane to ball
    glm::vec3 ballToPlane = ballPosition - unitNormal * ballHeightOnPlane;
    float distFromCenter = glm::length(glm::vec2(ballToPlane.x - plateCenter.x, ballToPlane.z - plateCenter.z));

    // Ground collision: prevent ball from going below y = 0 + ballRadius
    if (ballPosition.y < ballRadius) 
    {
        ballPosition.y = ballRadius;
        if (ballVelocity.y < 0.0f) ballVelocity.y = 0.0f;
        if (ballAcc.y < 0.0f) ballAcc.y = 0.0f;
    }

    if (ballHeightOnPlane >= -ballRadius && distFromCenter <= plateRadius + 1e-5f)
    {
        glm::vec3 acc = glm::dot(gravity, unitNormal) * unitNormal;
        ballAcc = gravity - acc;
    }
    else
    {
        ballAcc = gravity;
    }

    lastTime = now;

    glm::vec4 oldPlane = CalculatePlane();
    glm::vec3 oldNormal = glm::normalize(glm::vec3(oldPlane.x, oldPlane.y, oldPlane.z));
    float oldBallHeightOnPlane = glm::dot(ballPosition, oldNormal) + oldPlane.w;
    glm::vec3 ballOnOldPlane = ballPosition - oldNormal * oldBallHeightOnPlane;

    UpdateLegEnds();

    glm::vec4 newPlane = CalculatePlane();
    glm::vec3 newNormal = glm::normalize(glm::vec3(newPlane.x, newPlane.y, newPlane.z));

    float newBallHeightOnPlane = glm::dot(ballOnOldPlane, newNormal) + newPlane.w;
    glm::vec3 ballOnNewPlane = ballOnOldPlane - newNormal * newBallHeightOnPlane;

    ballMutex.lock();
    ballPosition = ballOnNewPlane + newNormal * ballRadius;
    ballMutex.unlock();
    for (int i = 0; i < NUM_LEGS; i++) 
    {
        DrawLeg(legStarts[i], legEnds[i]);
    }

    shaderProgram.setVec3("color", 0.0f, 0.2f, 0.7f);
    DrawPlate();

    shaderProgram.setVec3("color", 1.0f, 0.5f, 0.2f);
    DrawBall();
    glfwSwapBuffers(gWindow);
}


void StewartPlatform::ProcessInput() 
{
    if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(gWindow, true);

    // Reset platform rotation
    if (glfwGetKey(gWindow, GLFW_KEY_R) == GLFW_PRESS) 
    {
        gRotationMatrix = glm::mat4(1.0f);

        ballMutex.lock();
        ballPosition = glm::vec3(0.0f, initHeight + ballRadius, 0.0f);
        ballVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        ballAcc = glm::vec3(0.0f, 0.0f, 0.0f);
        ballMutex.unlock();

        targetLegEndsMutex.lock();
        for (int i = 0; i < NUM_LEGS; i++) 
        {
            legEnds[i] = initialPositions[i];
            targetLegEnds[i] = initialPositions[i];
        }
        targetLegEndsMutex.unlock();

        onTarget = true;
        firstTimeChange = true;
    }

    glm::vec3 direction(0.0f);

    // Arrow keys: Up/Down/Left/Right to tilt the platform
    if (glfwGetKey(gWindow, GLFW_KEY_UP) == GLFW_PRESS) {
        direction.z -= 1.0f;
    }
    if (glfwGetKey(gWindow, GLFW_KEY_DOWN) == GLFW_PRESS) {
        direction.z += 1.0f;
    }
    if (glfwGetKey(gWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
        direction.x -= 1.0f;
    }
    if (glfwGetKey(gWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        direction.x += 1.0f;
    }

    // If any arrow key is pressed, set new platform normal relative to current orientation
    if (glm::length(direction) > 0.0f) {
        glm::vec4 plane = CalculatePlane();
        glm::vec3 currentNormal = glm::normalize(glm::vec3(plane.x, plane.y, plane.z));
        float tiltAmount = 0.05f; // Adjust tilt sensitivity
        glm::vec3 newNormal = glm::normalize(currentNormal + tiltAmount * direction);
        SetPlatformNormal(newNormal);
    }
}

void StewartPlatform::Start(int width, int height) 
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    gWindow = glfwCreateWindow(width, height, "Stewart Platform Simulation", NULL, NULL);
    if (!gWindow) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(gWindow);
    glfwSetFramebufferSizeCallback(gWindow, Reshape);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return;
    }

    // Set the mouse button callback
    glfwSetMouseButtonCallback(gWindow, MouseButtonCallback);

    // Set the cursor position callback
    glfwSetCursorPosCallback(gWindow, CursorPositionCallback);

    Shader shaderProgram = Shader("vertex_shader.glsl", "fragment_shader.glsl");

    pShaderProgram = &shaderProgram;
    SetupBuffers();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    lastTime = std::chrono::system_clock::now();

    std::random_device rd;  // Seed the random number generator
    std::mt19937 gen(rd()); // Mersenne Twister engine

    // Define a distribution range (e.g., 0.0 to 1.5)
    std::uniform_real_distribution<float> dist(0.0f, 0.0f);

    // Generate a random float
    float velX = dist(gen);
    float velZ = dist(gen);

    ballMutex.lock();
    ballVelocity = glm::vec3(velX, 0.0f, velZ);
    ballMutex.unlock();

    while (!glfwWindowShouldClose(gWindow)) 
    {
        ProcessInput();
        RenderScene();
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &legVAO);
    glDeleteBuffers(1, &legVBO);
    glDeleteVertexArrays(1, &plateVAO);
    glDeleteBuffers(1, &plateVBO);
    glDeleteVertexArrays(1, &ballVAO);
    glDeleteBuffers(1, &ballVBO);
    shaderProgram.Delete();

    const GLubyte* renderer = glGetString(GL_RENDERER); // Get renderer string
    const GLubyte* version = glGetString(GL_VERSION);   // Get OpenGL version string
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION); // Get GLSL version string

    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;
    std::cout << "GLSL version supported: " << glslVersion << std::endl;

    glfwTerminate();
}

bool StewartPlatform::PlatformOnTarget()
{
    return onTarget;
}