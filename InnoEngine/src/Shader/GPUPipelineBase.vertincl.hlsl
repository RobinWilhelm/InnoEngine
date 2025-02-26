
cbuffer FrameMatrices : register(b0, space1)
{
    float4x4 ViewProjectionMatrix : packoffset(c0);
};
