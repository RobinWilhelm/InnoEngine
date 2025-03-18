
cbuffer FrameMatrices : register(b0, space1)
{
    float4x4 ViewProjectionMatrix : packoffset(c0);
};

static const uint QuadIndices[6] = { 0, 1, 2, 3, 2, 1 };
static const float2 QuadVertices[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };


float4 transform_coordinates_2D(float4 coordinates)
{
    float4 pos = mul(ViewProjectionMatrix, coordinates);
    pos.z = coordinates.z; // linear depth
    return pos;
}