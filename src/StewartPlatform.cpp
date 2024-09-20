#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
std::mutex targetLegEndsMutex;

const float speedActuators = 4;
const float maxLegLength = 1.4;
const float minLegLength = 0.6;
 
// Platform control
const int NUM_LEGS = 6;
int numSegments = 100;

const float gBaseRadius = 0.6f;
const float initHeight = 0.8;

bool onTarget = true;
bool firstTimeChange = true;

// leg speeds
float legSpeeds[NUM_LEGS] = {4, 4, 4, 4, 4, 4};

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

void StewartPlatform::SetPlatformPosition(glm::vec3 position)
{
    targetLegEndsMutex.lock();
    glm::vec3 vec_one = targetLegEnds[1] - targetLegEnds[0];
    glm::vec3 vec_two = targetLegEnds[2] - targetLegEnds[0];
    targetLegEndsMutex.unlock();

    glm::vec3 currNormal = glm::normalize(glm::cross(vec_one, vec_two));

    UpdateTargetLegEnds(currNormal, position);
    onTarget = false;
    firstTimeChange = true;
}

void StewartPlatform::SetPlatformNormal(glm::vec3 normal)
{
    if (normal == glm::vec3(0.0f)) 
    {
        return;
    }

    if (normal.y <= 0.0f) 
    {
        return;
    }

    glm::vec3 currTargetCentroid(0.0f);
    for (int i = 0; i < 6; ++i) 
    {
        currTargetCentroid += targetLegEnds[i];
    }
    currTargetCentroid /= 6.0f;

    UpdateTargetLegEnds(normal, currTargetCentroid);
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

    float radius = 0.75f;
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

void StewartPlatform::UpdateTargetLegEnds(glm::vec3 normal, glm::vec3 position)
{
    glm::vec4 plane = CalculatePlane();
    glm::vec3 planeNormal = glm::vec3(plane.x, plane.y, plane.z);
    glm::vec3 rotationAxis = glm::cross(planeNormal, normal);

    glm::vec3 Q = glm::vec3(0.0f);
    for (int i = 0; i < 6; ++i) 
    {
        Q += legEnds[i];
    }

    Q /= NUM_LEGS;

    glm::vec3 translation = position - Q;
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);

    float angle = CalculateRotationAngle(planeNormal, normal);

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
    glm::mat4 transformationMatrix;

    if (glm::length(rotationAxis) < 1e-6f) 
    {
        if (glm::dot(planeNormal, normal) > 0.0f) 
        {
            transformationMatrix = translationMatrix;
        } 
        else 
        {
            rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            angle = glm::pi<float>();
            transformationMatrix = rotationMatrix * translationMatrix;
        }
    } 
    else 
    {
        rotationAxis = glm::normalize(rotationAxis);
        transformationMatrix = rotationMatrix * translationMatrix;
    }

    targetLegEndsMutex.lock();
    for (int i = 0; i < NUM_LEGS; i++) 
    {
        glm::vec3 leg = legEnds[i];
        glm::vec3 diff = leg - Q;
        glm::vec3 rotatedLeg = glm::vec3(transformationMatrix * glm::vec4(diff, 1.0f));
        glm::vec3 newLeg = rotatedLeg + position;
        targetLegEnds[i] = newLeg;
    }
    targetLegEndsMutex.unlock();
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
            if (legEnds[i] != legEndsNewTarget[i]) 
            {
                return;
            }
        }

        onTarget = true;
    }
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

    UpdateLegEnds();
    for (int i = 0; i < NUM_LEGS; i++) 
    {
        // Render each leg with its specified start and end points
        DrawLeg(legStarts[i], legEnds[i]);
    }

    shaderProgram.setVec3("color", 0.0f, 0.2f, 0.7f);
    DrawPlate();
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

    float expectedLength_one = glm::length(legEnds[0] - legEnds[1]);
    float expectedLength_two = glm::length(legEnds[0] - legEnds[2]);
    float expectedLength_three = glm::length(legEnds[0] - legEnds[3]);
    float expectedLength_four = glm::length(legEnds[0] - legEnds[4]);
    float expectedLength_five = glm::length(legEnds[0] - legEnds[5]);

    while (!glfwWindowShouldClose(gWindow)) 
    {
        ProcessInput();
        RenderScene();
        glfwPollEvents();

        float currentLength_one = glm::length(legEnds[0] - legEnds[1]);
        float currentLength_two = glm::length(legEnds[0] - legEnds[2]);
        float currentLength_three = glm::length(legEnds[0] - legEnds[3]);
        float currentLength_four = glm::length(legEnds[0] - legEnds[4]);
        float currentLength_five = glm::length(legEnds[0] - legEnds[5]);
    }

    glDeleteVertexArrays(1, &legVAO);
    glDeleteBuffers(1, &legVBO);
    glDeleteVertexArrays(1, &plateVAO);
    glDeleteBuffers(1, &plateVBO);
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