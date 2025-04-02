struct CameraData
{
    float4x4 ViewProjectionMatrix;
};

StructuredBuffer<CameraData> CameraDataBuffer : register(t0, space0);

static const uint QuadIndices[6] = { 0, 1, 2, 3, 2, 1 };
static const float2 QuadVertices[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };


float4 transform_coordinates_2D(float4 coordinates, uint camera_index)
{
    float4 pos = mul(CameraDataBuffer[camera_index].ViewProjectionMatrix, coordinates);
    pos.z = coordinates.z; // linear depth
    return pos;
}