Texture2D<float4> msdf_texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);


float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}


struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{   
/*
    float3 msd = msdf_texture.Sample(Sampler, input.TexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    color = mix(bgColor, fgColor, opacity);
*/
}