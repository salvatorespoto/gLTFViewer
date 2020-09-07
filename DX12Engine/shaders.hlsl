static const float PI = 3.14159265f;

static const uint MESH_CONSTANTS_N_DESCRIPTORS = 15;
static const uint MATERIALS_N_DESCRIPTORS = 15;
static const uint TEXTURES_N_DESCRIPTORS = 15;
static const uint SAMPLERS_N_DESCRIPTORS = 15;
static const uint MAX_LIGHT_NUMBER = 7;
static const uint MAX_MESH_INSTANCES = 10;

static const float3 dielectricSpecular = { 0.04f, 0.04f, 0.04f };
static const float3 black = { 0.0f, 0.0f, 0.0f };

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
    Light lights[MAX_LIGHT_NUMBER];
};

struct MeshConstants
{
    float4x4 modelMtx;
    float4x4 nodeMtx;
    float4 rotationXYZ; // rotations about X, Y and Z axes. Fourth component unused;
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

FrameConstants frameConstants : register(b0, space0);
StructuredBuffer<MeshConstants> meshConstants : register(t0, space0);
ConstantBuffer<RoughMetallicMaterial> materials[MATERIALS_N_DESCRIPTORS]  : register(b0, space1);
Texture2D textures[TEXTURES_N_DESCRIPTORS] : register(t0, space1);
TextureCube cubeMap : register(t0, space2);
SamplerState samplers[SAMPLERS_N_DESCRIPTORS] : register(s0);

//StructuredBuffer<InstanceData> gInstanceData : register(t0, space2);

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
VertexOut VSMain(VertexIn vIn, uint instanceID : SV_InstanceID)
{
    VertexOut vOut;
    float4x4 modelMtx = mul(meshConstants[instanceID].modelMtx, meshConstants[instanceID].nodeMtx);
    vOut.shadingLocation = mul(float4(vIn.position, 1.0f), modelMtx).xyz;
    vOut.normal = mul(float4(vIn.normal, 1.0f), modelMtx).xyz;
    vOut.position = mul(float4(vIn.position, 1.0f), mul(modelMtx, frameConstants.viewProjMtx));
    vOut.textCoord = float2(vIn.textCoord.x, vIn.textCoord.y);
    vOut.tangent.xyz = mul(vIn.tangent.xyz, (float3x3)modelMtx);
    vOut.tangent.w = vIn.tangent.w;
    return vOut;
}

float3 diffuse(float3 albedo, float3 lightColor, float NdotL); // Lambertian diffuse
float3 fresnel(float m, float3 lightColor, float3 F0, float NdotH, float NdotL, float LdotH); // Specular fresnel 

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    float4 baseColor;
    if (materials[0].baseColorTA.textureId != -1) baseColor = textures[materials[0].baseColorTA.textureId].Sample(samplers[0], vIn.textCoord);

    float3 V = normalize(frameConstants.eyePosition.xyz - vIn.shadingLocation);      // V is the normalized vector from the shading location to the eye
    float3 N = vIn.normal;
    float3 T = vIn.tangent.xyz;

    float3 normal;
    if (materials[0].normalTA.textureId != -1) normal = textures[materials[0].normalTA.textureId].Sample(samplers[0], vIn.textCoord);

    normal = 2.0f * normal - 1.0f;
    float3 B = cross(T,N) * vIn.tangent.w;
    float3x3 M_tw = float3x3(T,B,N);
    N = mul(normal, M_tw);
    T = normalize(T - dot(T, N) * N);
    float3 R = reflect(-V, N);

    float4 cubeMapSample = cubeMap.Sample(samplers[0], R);

    float3 c = cubeMapSample.xyz * baseColor.xyz;
    return float4(c.xyz, 1.0f);
}

float3 diffuse(float3 albedo, float3 lightColor, float NdotL)
{
    return albedo * (lightColor * max(NdotL, 0.0f));
}

float3 fresnel(float m, float3 lightColor, float3 F0, float NdotH, float NdotL, float LdotH)
{
    float3 R = F0 + (1.0f - F0) * (1.0f - max(LdotH, 0.0f));
    R = R * ((m + 8) / 8) * pow(NdotH, m);
    return saturate(lightColor * max(NdotL, 0.0f) * R);
}