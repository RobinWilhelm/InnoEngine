#include "GPUPipelineBase.verti.hlsl"

struct MSDFSpriteData
{
    float2 Position;
    float2 Size;
    float4 SourceRect;
    float4 ForegroundColor; // text color
    float Depth;
    float ScreenPixRange;
    float2 pad;
};

StructuredBuffer<MSDFSpriteData> DataBuffer : register(t0, space0);

struct Output
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float ScreenPixRange : TEXCOORD2;
    float4 Position : SV_Position;
};

Output main(uint id : SV_VertexID)
{
    uint spriteIndex    = id / 6;
    uint vert           = QuadIndices[id % 6];
    float2 coord        = QuadVertices[vert];
    
    MSDFSpriteData sprite = DataBuffer[spriteIndex];    
    coord *= sprite.Size;
    float4 coordWithDepth = float4(coord + sprite.Position, sprite.Depth, 1.0f);
    
    float2 texcoord[4] =
    {
        { sprite.SourceRect.x, sprite.SourceRect.y },
        { sprite.SourceRect.z, sprite.SourceRect.y },
        { sprite.SourceRect.x, sprite.SourceRect.w },
        { sprite.SourceRect.z, sprite.SourceRect.w }
    };
            
    Output output;
    output.Position         = transform_coordinates_2D(coordWithDepth);
    output.TexCoord         = texcoord[vert];
    output.Color            = sprite.ForegroundColor;
    output.ScreenPixRange   = sprite.ScreenPixRange;
    return output;
}