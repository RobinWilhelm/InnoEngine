#include "GPUPipelineBase.verti.hlsl"

struct Input
{
    float4 Color : TEXCOORD1;
};

struct Output
{
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

Output main(Input input)
{
    Output output;
    return output;
}