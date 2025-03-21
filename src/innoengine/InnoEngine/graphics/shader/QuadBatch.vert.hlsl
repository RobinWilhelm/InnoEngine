#include "GPUPipelineBase.verti.hlsl"

struct QuadData
{
    float2 Position;
    float2 Size;
    float4 Color;    
    float Rotation;
    float Depth;
    uint CameraIndex;
    float pad;
};

StructuredBuffer<QuadData> DataBuffer : register(t1, space0);

struct Output
{
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

Output main(uint id : SV_VertexID)
{
    uint quad_index = id / 6;
    uint vert = QuadIndices[id % 6];
    QuadData quad = DataBuffer[quad_index];
    float2 coord = QuadVertices[vert];
    
    coord -= float2(0.5f, 0.5f);
    coord *= quad.Size;    
    
    if (quad.Rotation != 0.0f)
    {
        float c = cos(quad.Rotation);
        float s = sin(quad.Rotation);
        
        float2x2 rotation = { c, s, -s, c };
        coord = mul(coord, rotation);
    }
    
    float4 coord_with_depth = float4(coord + quad.Position, float(quad.Depth), 1.0f);
                
    Output output;
    output.Position = transform_coordinates_2D(coord_with_depth, quad.CameraIndex);
    output.Color = quad.Color;
    return output;
}