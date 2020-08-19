// Constant buffer holding the pass constants
cbuffer cb : register(b0)
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
};

Texture2D gDiffuseMap : register(t0);
SamplerState gsamPointWrap : register(s0);

// Define the input structure
struct VertexIn
{
    float3 position : POSITION; // POSITION match the semantic name "POSITION" in the vertex descriptor
    float2 textCoord : TEXCOORD;       // COLOR match the semantic name "COLOR" in the vertex descriptor
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 textCoord : TEXCOORD;       // COLOR match the semantic name "COLOR" in the vertex descriptor
};

// The vertex shader 
VertexOut VSMain(VertexIn vIn)
{
    VertexOut vOut;
    vOut.position = mul(float4(vIn.position, 1.0f), viewProjMtx);
    vOut.textCoord = vIn.textCoord;
    return vOut;
}

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    float4 albedo = gDiffuseMap.Sample(gsamPointWrap, vIn.textCoord);
    return albedo;
}