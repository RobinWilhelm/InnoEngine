#include "GPUPipelineBase.verti.hlsl"

struct CircleData
{
    float4 Color;
    float2 Position;
    float Radius;
    float Thickness;
    float Fade;  
    float Depth;
    uint CameraIndex;
    float pad;
};

StructuredBuffer<CircleData> DataBuffer : register(t1, space0);

struct Output
{
    float4 Position : SV_Position;
    float4 Color : TEXCOORD1;
    float2 Local : TEXCOORD2;
    float Thickness : TEXCOORD3;
    float Fade : TEXCOORD4;
};

Output main(uint id : SV_VertexID)
{
    const uint circle_index = id / 6;
    const uint vert = QuadIndices[id % 6];
    const CircleData circle_data = DataBuffer[circle_index];
    const float2 vertex_base_coords = QuadVertices[vert];
     
    float4 position = transform_coordinates_2D(float4(vertex_base_coords * circle_data.Radius * 2 + circle_data.Position, circle_data.Depth, 1.0f), circle_data.CameraIndex);
     
    Output output;    
    output.Position = position;
    output.Color    = circle_data.Color;
    output.Local = vertex_base_coords * 2 - 1.0f;
    output.Thickness = circle_data.Thickness;
    output.Fade     = circle_data.Fade;
    return output;
}