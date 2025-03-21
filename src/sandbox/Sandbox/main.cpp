#include "Sandbox.h"

int main()
{
    IE::CreationParams creationParams;
    creationParams.WindowParams.title  = "Sandbox";
    creationParams.WindowParams.width  = 1920;
    creationParams.WindowParams.height = 1080;
    creationParams.EnableVSync         = false;
    creationParams.SimulationFrequency = 60;
    // creationParams.AssetDirectory      = "..\\..\\..\\..\\sandbox_environment\\assets";
    creationParams.AssetDirectory      = "../assets";
    creationParams.RunAsync            = false;

    Sandbox sandbox;
    sandbox.init( creationParams );
    sandbox.enable_debugui( true );
    return static_cast<int>( sandbox.run() );
}
