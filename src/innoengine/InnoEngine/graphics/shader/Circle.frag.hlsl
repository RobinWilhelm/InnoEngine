struct Input
{
    float4 Color : TEXCOORD1;
    float2 Local : TEXCOORD2;
    float Thickness : TEXCOORD3;
    float Fade : TEXCOORD4;
};

float4 main(Input input) : SV_Target0
{
    float4 color = input.Color;
    float distance = 1 - length(input.Local);
    float alpha = smoothstep(0, input.Fade, distance);
    alpha *= smoothstep(input.Thickness + input.Fade, input.Thickness, distance);
    if (alpha == 0.0f)
        discard;
    
    color.a *= alpha;
    return color;
}