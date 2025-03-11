#include "Sandbox.h"

int main()
{
    IE::CreationParams creationParams;
    creationParams.WindowParams.title  = "Sandbox";
    creationParams.WindowParams.width  = 1280;
    creationParams.WindowParams.height = 720;
    creationParams.EnableVSync         = true;
    creationParams.SimulationFrequency = 0;
    creationParams.AssetDirectory      = "..\\..\\..\\..\\sandbox_environment\\assets";
    creationParams.RunAsync            = true;
                                                           
    Sandbox sandbox;
    sandbox.init( creationParams );
    sandbox.enable_debugui( false );
    return static_cast<int>( sandbox.run() );
}
