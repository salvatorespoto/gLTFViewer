static const float PI = 3.14159265f;

cbuffer cb0 : register(b0)
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
    float4 eyePosition;
    float4 lightPosition;
};

cbuffer cb1 : register(b1)
{
    float4x4 meshModelMtx;
};

struct TextureAccessor
{
    uint textureId; float3 _pad0;
    uint texCoordId; float3 _pad1;
};

struct RoughMetallicMaterial
{
    float4 baseColorFactor;
    float metallicFactor; float3 _pad0;
    float roughnessFactor; float3 _pad1;
    TextureAccessor baseColorTA;
    TextureAccessor roughMetallicTA;
    TextureAccessor normalTA;
    TextureAccessor emissiveTA;
    TextureAccessor occlusionTA;
};

//const float3 dielectricSpecular = { 0.04f, 0.04f, 0.04f };
const float3 black = { 0.0f, 0.0f, 0.0f };

ConstantBuffer<RoughMetallicMaterial> materials : register(b2);

Texture2D textures[5] : register(t0);

SamplerState texSampler : register(s0);

struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 textCoord : TEXCOORD;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 textCoord : TEXCOORD0;
    float3 shadingLocation: TEXCOORD1;
    float4 tangent : TEXCOORD2;
    float3 normal : TEXCOORD3;
};

// The vertex shader 
VertexOut VSMain(VertexIn vIn)
{
    VertexOut vOut;
    vOut.shadingLocation = mul(float4(vIn.position, 1.0f), meshModelMtx).xyz;
    vOut.normal = mul(float4(vIn.normal, 1.0f), meshModelMtx);
    vOut.position = mul(float4(vIn.position, 1.0f), mul(meshModelMtx, viewProjMtx));
    vOut.textCoord = vIn.textCoord;
    vOut.tangent = mul(float4(vIn.tangent.xyz, 1.0f), meshModelMtx);;
    return vOut;
}

float3 fresnel(float3 F0, float3 F90, float VdotH);                         // Fresnel
float V_GGX(float NdotL, float NdotV, float ALPHA_roughness);               // Geometric occlusion
float D_GGX(float NdotH, float ALPHA_roughness);                            // Microfaced distribution
float3 diffuseLambert(float3 C_diff);                                       // Diffuse term
float diffuseDisney(float3 C_diff, float F90, float NdotL, float NdotV);    // Diffuse term according to Disney model

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    float3 V = normalize(eyePosition.xyz - vIn.shadingLocation);      // V is the normalized vector from the shading location to the eye
    float3 L = normalize(lightPosition.xyz - vIn.shadingLocation);    // L is the normalized vector from the shading location to the light
    float3 N = vIn.normal;                                 // N is the surface normal in the same space as the above values
    float3 H = normalize(L + V);                           // H is the half vector, where H = normalize(L + V)
    float VdotH = dot(V, H);
    float LdotH = dot(L, H);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float NdotH = dot(N, H);

    float3 dielectricSpecular = { 0.04f, 0.04f, 0.04f };
    float3 black = { 0.0f, 0.0f, 0.0f };

    float4 baseColor = textures[materials.baseColorTA.textureId].Sample(texSampler, vIn.textCoord);
    float4 roughMetallic = textures[materials.roughMetallicTA.textureId].Sample(texSampler, vIn.textCoord);
    float4 emissive = textures[materials.emissiveTA.textureId].Sample(texSampler, vIn.textCoord);
    float4 occlusion = textures[materials.occlusionTA.textureId].Sample(texSampler, vIn.textCoord);


    float roughness = roughMetallic.g;
    float metallic = roughMetallic.b;

    float3 C_diff = lerp(baseColor.rgb * (1.0f - dielectricSpecular.r), black, metallic);
    float3 F0 = lerp(dielectricSpecular, baseColor.rgb, metallic); // F0 is Freshnel reflectance at 0 degrees
    //float3 F90 = { 1.0f, 1.0f, 1.0f };                           // F90 is Freshnel reflectance at 90 degrees
    float3 F90 = 2.0f * LdotH * LdotH * roughness + 0.5f;          // F90 accordind to Disney model 
    float ALPHA_roughness = roughness * roughness;


    float3 F = fresnel(F0, F90, VdotH);
    float G = V_GGX(NdotL, NdotV, ALPHA_roughness);             // G is Geometric Occlusion 
    float Vis = G / (4.0f * NdotL * NdotV);                     // Vis
    float D = D_GGX(NdotH, ALPHA_roughness);                    // D is Microfacet Distribution
    //float diffuse = diffuseLambert(C_diff);                     // Diffuse term
    float diffuse = diffuseDisney(C_diff, F90, NdotL, NdotV);   // Diffuse term according to Disney model

    float3 f_diffuse = (1.0f - F) * diffuse;
    float3 f_specular = D * Vis * F; // D * Vis * F;
    float3 f = float3(10.1f, 10.1f, 10.1f) * (f_diffuse + f_specular) * occlusion.xyz * 0.2f;
    f = f + (emissive.xyz * 1.5f);
    return float4(f, 1.0f);
}

float3 fresnel(float3 F0, float3 F90, float VdotH)
{
    return F0 + (F90 - F0) * pow(clamp(1.0f - VdotH, 0.0f, 1.0f), 5.0f);
}

float V_GGX(float NdotL, float NdotV, float ALPHA_roughness)
{
    float ALPHA_roughness_sq = ALPHA_roughness * ALPHA_roughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - ALPHA_roughness_sq) + ALPHA_roughness_sq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - ALPHA_roughness_sq) + ALPHA_roughness_sq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

float D_GGX(float NdotH, float ALPHA_roughness)
{
    float ALPHA_roughness_sq = ALPHA_roughness * ALPHA_roughness;
    float f = (NdotH * NdotH) * (ALPHA_roughness_sq - 1.0) + 1.0;
    return ALPHA_roughness_sq / (PI * f * f);
}

float3 diffuseLambert(float3 C_diff)
{
    return C_diff / PI;
}

float diffuseDisney(float3 C_diff, float F90, float NdotL, float NdotV)
{
    return (C_diff / PI) * (1.0f + (F90 - 1.0f) * pow(1.0f - NdotL, 5.0f)) * (1.0f + (F90 - 1.0f) * pow(1.0f - NdotV, 5.0f));
}
