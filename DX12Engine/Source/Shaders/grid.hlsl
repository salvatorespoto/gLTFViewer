struct Light
{
    float4 position;
    float4 color;
};

struct FrameConstants
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
    float4 eyePosition;
    Light lights[7];
};

struct GridConstants
{
    float4x4 modelMtx;
};

struct VertexIn
{
    float3 position : POSITION;
};

struct VertexOut
{
    float4 position : SV_POSITION;
};

FrameConstants frameConstants : register(b0, space0);
ConstantBuffer<GridConstants> gridConstants: register(b1, space0);

VertexOut VSMain(VertexIn vIn)
{
    VertexOut vOut;
    vOut.position = mul(float4(vIn.position, 1.0f), mul(gridConstants.modelMtx, frameConstants.viewProjMtx)).xyzw;
    return vOut;
}

float4 PSMain(VertexOut vIn) : SV_Target
{
    return float4(0.5f, 0.5f, 0.5f, 1.0f);
}