#include "GPUPipelineBase.verti.hlsl"

struct SpriteData
{
    float4 SourceRect;    
    float4 Color;
    float2 Position;
    float2 Size;
    float2 OriginOffset;
    float Depth;
    float Rotation;
    uint CameraIndex;
    float3 pad;
};

StructuredBuffer<SpriteData> DataBuffer : register(t1, space0);


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
    float2 coord = QuadVertices[vert];
    coord *= sprite.Size;
    coord -= sprite.OriginOffset;
    
    if (sprite.Rotation != 0.0f)
    {
        float c = cos(sprite.Rotation);
        float s = sin(sprite.Rotation);    
        
        float2x2 rotation = { c, s, -s, c };
        coord = mul(coord , rotation) ;
    }
    
    float4 coord_with_depth = float4(coord + sprite.Position,sprite.Depth, 1.0f);
    
    
    float2 texcoord[4] =
    {
        { sprite.SourceRect.x, sprite.SourceRect.y },
        {  sprite.SourceRect.z, sprite.SourceRect.y },
        { sprite.SourceRect.x,  sprite.SourceRect.w },
        {  sprite.SourceRect.z,  sprite.SourceRect.w }
    };
            
    Output output;
    output.Position = transform_coordinates_2D(coord_with_depth, sprite.CameraIndex);
    output.TexCoord = texcoord[vert];
    output.Color = sprite.Color;
    return output;
}