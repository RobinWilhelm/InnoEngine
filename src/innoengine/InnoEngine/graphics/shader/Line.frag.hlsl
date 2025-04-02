#include "FragmentBase.fragi.hlsl"

struct Input
{
    float4 Color : TEXCOORD0;
    float LocalDistance : TEXCOORD1;
    float Fade : TEXCOORD2;
};

float4 main(Input input) : SV_Target0
{    
    float4 color = input.Color;
    color.a *= 1 - smoothstep(1 - input.Fade, 1, abs(input.LocalDistance));
    return calc_final_color(color);
}