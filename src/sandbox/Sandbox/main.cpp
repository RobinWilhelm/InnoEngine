#include "Sandbox.h"

int main()
{
    IE::CreationParams creationParams;
    creationParams.WindowParams.title  = "Sandbox";
    creationParams.WindowParams.width  = 1920;
    creationParams.WindowParams.height = 1080;
    creationParams.EnableVSync         = true;
    creationParams.SimulationFrequency = 0;
    // creationParams.AssetDirectory      = "..\\..\\..\\..\\sandbox_environment\\assets";
    creationParams.AssetDirectory      = "../assets";
    creationParams.RunAsync            = true;

    Sandbox sandbox;
    sandbox.init( creationParams );
    sandbox.enable_debugui(false);
    return static_cast<int>( sandbox.run() );
}
