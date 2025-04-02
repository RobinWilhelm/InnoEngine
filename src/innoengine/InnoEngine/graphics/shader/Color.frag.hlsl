#include "FragmentBase.fragi.hlsl"

struct Input
{
    float4 Color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{   
    return calc_final_color(input.Color);
}