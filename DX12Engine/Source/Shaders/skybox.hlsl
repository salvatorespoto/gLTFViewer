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

struct SkyBoxConstants
{
    float4x4 modelMtx;
};

struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 textCoord : TEXCOORD;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 textCoord : TEXCOORD0;
};

FrameConstants frameConstants : register(b0, space0);
ConstantBuffer<SkyBoxConstants> skyBoxConstants: register(b1, space0);
TextureCube cubeMap : register(t0, space0);
SamplerState linearWrapSampler : register(s0, space0);

VertexOut VSMain(VertexIn vIn)
{
    VertexOut vOut;
    vOut.textCoord = vIn.position;                  // The local position will be used as lookup in the cubemap
    vIn.position += frameConstants.eyePosition.xyz; // Center the sphere on the camera
    vOut.position = mul(float4(vIn.position, 1.0f), mul(skyBoxConstants.modelMtx, frameConstants.viewProjMtx)).xyww;
    return vOut;
}

float4 PSMain(VertexOut vIn) : SV_Target
{   
    return cubeMap.Sample(linearWrapSampler, vIn.textCoord);
}