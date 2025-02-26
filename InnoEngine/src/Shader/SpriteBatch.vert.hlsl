#include "GPUPipelineBase.vertincl.hlsl"

struct SpriteData
{
    float X, Y, Z, Rotation;
    float2 Scale;
    float2 Padding;
    float4 SourceRect;
    float4 Color;
};

StructuredBuffer<SpriteData> DataBuffer : register(t0, space0);


static const uint QuadIndices[6] = { 0, 1, 2, 3, 2, 1 };
static const float2 QuadVertices[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };


struct Output
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

Output main(uint id : SV_VertexID)
{
    uint spriteIndex = id / 6;
    uint vert = QuadIndices[id % 6];
    SpriteData sprite = DataBuffer[spriteIndex];

    float c = cos(sprite.Rotation);
    float s = sin(sprite.Rotation);

    float2 coord = QuadVertices[vert];
    coord *= sprite.Scale;
    float2x2 rotation = { c, s, -s, c };
    coord = mul(coord, rotation);

    float4 coordWithDepth = float4(coord.x + sprite.X, coord.y + sprite.Y, sprite.Z, 1.0f);
    
    float2 texcoord[4] =
    {
        { sprite.SourceRect.x, sprite.SourceRect.y },
        { sprite.SourceRect.x + sprite.SourceRect.w, sprite.SourceRect.y },
        { sprite.SourceRect.x, sprite.SourceRect.y + sprite.SourceRect.z },
        { sprite.SourceRect.x + sprite.SourceRect.w, sprite.SourceRect.y + sprite.SourceRect.z }
    };
    
    
    Output output;    
    output.Position = mul(ViewProjectionMatrix, coordWithDepth);
    output.TexCoord = texcoord[vert];
    output.Color = sprite.Color;
    return output;
}