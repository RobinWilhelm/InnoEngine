#include "Sample.h"

int main()
{
    IE::CreationParams creationParams;
    creationParams.WindowParams.title  = "Sample game";
    creationParams.WindowParams.width  = 1920;
    creationParams.WindowParams.height = 1080;
    creationParams.EnableVSync         = false;
    creationParams.SimulationFrequency = 60;
    // creationParams.AssetDirectory      = "..\\..\\..\\..\\sandbox_environment\\assets";
    creationParams.AssetDirectory      = "../assets";
    creationParams.RunAsync            = true;

    SampleProject sample;
    sample.init( creationParams );
    sample.enable_debugui(true);
    return static_cast<int>(sample.run() );
}
