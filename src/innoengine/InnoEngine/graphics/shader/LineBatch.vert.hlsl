#include "GPUPipelineBase.verti.hlsl"

struct LineData
{
    float2 Start;
    float2 End;
    float4 Color;    
    float Thickness;
    float Fade;
    float Depth;
    uint CameraIndex;
};

StructuredBuffer<LineData> DataBuffer : register(t1, space0);

struct Output
{
    float4 Position : SV_Position;
    float4 Color : TEXCOORD0;
    float LocalDistance : TEXCOORD1;
    float Fade : TEXCOORD2;
};

Output main(uint id : SV_VertexID)
{
    const uint line_index = id / 6;
    const uint vert = QuadIndices[id % 6];
    const LineData line_data = DataBuffer[line_index];
     
    const float3 up = float3(0.0f, 0.0f, 1.0f);
    const float3 se = float3(line_data.End - line_data.Start, 0.0f);
    
    const float half_thickness = line_data.Thickness * 0.5;
    const float2 perpendicular = float2(normalize(cross(se, up).xy));
      
    
    Output output;
    output.Color = line_data.Color;
    output.Fade = line_data.Fade;
    
    float4 position = float4(0, 0, 0, 0);
    switch (vert)
    {
        case 0:
            output.LocalDistance = 1;
            position = float4(line_data.Start + half_thickness * perpendicular, line_data.Depth, 1.0f);
            break;
        case 1:
            output.LocalDistance = 1;
            position = float4(line_data.End + half_thickness * perpendicular, line_data.Depth, 1.0f);
            break;
        case 2:
            output.LocalDistance = -1;
            position = float4(line_data.Start - half_thickness * perpendicular, line_data.Depth, 1.0f);
            break;
        case 3:
            output.LocalDistance = -1;
            position = float4(line_data.End - half_thickness * perpendicular, line_data.Depth, 1.0f);
            break;
    }                
    
    output.Position = transform_coordinates_2D(position, line_data.CameraIndex);   
    return output;
}