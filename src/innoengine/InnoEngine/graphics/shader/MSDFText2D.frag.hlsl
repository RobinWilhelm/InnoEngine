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
    float ScreenPixRange : TEXCOORD2;
};

// needed for 3d rendering
/*
float screenPxRange(Input input)
{
    const float pxRange = 2.0; // set to distance field's pixel range
    float2 unitRange = float2(pxRange, pxRange) / float2(atlas_size);
    float2 screenTexSize = float2(1.0f, 1.0f) / fwidth(input.TexCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}
*/

float4 main(Input input) : SV_Target0
{  
    float3 msd = msdf_texture.Sample(Sampler, input.TexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = input.ScreenPixRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    if (opacity == 0.0f)
        discard;
    
    return lerp(float4(input.Color.rgb, 0.0f), input.Color, opacity);
}