#include "Sandbox.h"


int main()
{        
    IE::CreationParams creationParams;
    creationParams.WindowParams.title = "Sandbox";
    creationParams.WindowParams.width = 1280;
    creationParams.WindowParams.height = 720;
    creationParams.EnableVSync = true;
    creationParams.SimulationFrequency = 60;
    creationParams.AssetDirectory = "..\\..\\..\\assets";

    Sandbox sandbox;
    sandbox.init(creationParams);
    sandbox.run();
}