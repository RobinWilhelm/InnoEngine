#include "Sandbox.h"


int main()
{        
    IE::CreationParams creationParams;
    creationParams.WindowParams.title = "Sandbox";
    creationParams.WindowParams.width = 1280;
    creationParams.WindowParams.height = 720;
    creationParams.EnableVSync = false;
    creationParams.SimulationFrequency = 0;
    creationParams.AssetDirectory = "..\\..\\..\\assets";
    creationParams.RunAsync = true;

    Sandbox sandbox;
    sandbox.init(creationParams);
    sandbox.enable_debugui(true);
    return static_cast<int>( sandbox.run() );
}