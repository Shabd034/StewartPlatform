#include <iostream>
#include <thread>
#include <cmath>

#include "StewartPlatform.h"

void changePoition(StewartPlatform& platform)
{
    //platform.SetPlatformNormal(glm::vec3(0.0, 1.0, 0.4));
    while (!platform.PlatformOnTarget())
    {
        // wait
    }

    //platform.SetPlatformNormal(glm::vec3(0.0, 1.0, 0.0));
}

int main()
{
    StewartPlatform platform;
    std::thread workerThread(changePoition, std::ref(platform));

    platform.Start(800, 600);

    // Wait for the thread to finish
    workerThread.join();

    return 0;
}