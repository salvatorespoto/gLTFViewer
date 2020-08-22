// Constant buffer holding the pass constants
cbuffer cb0 : register(b0)
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
};

cbuffer cb1 : register(b1)
{
    float4x4 meshModelMtx;
};


struct TextureAccessor
{
    unsigned int textureId;
    unsigned int texCoordId;
};

struct RoughMetallicMaterial
{
    float4 baseColorFactor;
    float metallicFactor; 
    float roughnessFactor;
    TextureAccessor baseColorTA;
    TextureAccessor roughMetallicTA;
    TextureAccessor normalTA;
    TextureAccessor emissiveTA;
    TextureAccessor occlusionTA;
};

ConstantBuffer<RoughMetallicMaterial> materials : register(b2);

Texture2D textures[5] : register(t0);

SamplerState texSampler : register(s0);

struct VertexIn
{
    float3 position : POSITION; // POSITION match the semantic name "POSITION" in the vertex descriptor
    float3 normals : NORMAL;       // COLOR match the semantic name "COLOR" in the vertex descriptor
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
    vIn.position = float4(vIn.position.x, vIn.position.y, vIn.position.z, 1.0f);
    vOut.position = mul(float4(vIn.position, 1.0f), mul(meshModelMtx, viewProjMtx));
    vOut.textCoord = vIn.textCoord;
    return vOut;
}

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    float4 albedo = textures[materials.baseColorTA.textureId].Sample(texSampler, vIn.textCoord);
    return albedo;
}