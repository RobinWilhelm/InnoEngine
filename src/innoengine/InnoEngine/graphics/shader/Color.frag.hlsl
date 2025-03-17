struct Input
{
    float4 Color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{   
    return input.Color;
}