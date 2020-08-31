static const float PI = 3.14159265f;

static const uint MESH_CONSTANTS_N_DESCRIPTORS = 15;
static const uint MATERIALS_N_DESCRIPTORS = 15;
static const uint TEXTURES_N_DESCRIPTORS = 15;
static const uint SAMPLERS_N_DESCRIPTORS = 15;
static const uint MAX_LIGHT_NUMBER = 7;

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
ConstantBuffer<MeshConstants> meshConstants : register(b1, space0);
ConstantBuffer<RoughMetallicMaterial> materials[MATERIALS_N_DESCRIPTORS]  : register(b0, space1);
Texture2D textures[TEXTURES_N_DESCRIPTORS] : register(t0, space0);
TextureCube cubeMap : register(t0, space1);
SamplerState samplers[SAMPLERS_N_DESCRIPTORS] : register(s0);

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
    float4x4 modelMtx = mul(meshConstants.modelMtx, meshConstants.nodeMtx);
    vOut.shadingLocation = mul(float4(vIn.position, 1.0f), modelMtx).xyz;
    vOut.normal = mul(float4(vIn.normal, 1.0f), modelMtx).xyz;
    vOut.position = mul(float4(vIn.position, 1.0f), mul(modelMtx, frameConstants.viewProjMtx));
    vOut.textCoord = float2(vIn.textCoord.x, vIn.textCoord.y);
    vOut.tangent = mul(float4(vIn.tangent.xyz, 1.0f), modelMtx);
    return vOut;
}

float3 diffuse(float3 albedo, float3 lightColor, float NdotL); // Lambertian diffuse
float3 fresnel(float m, float3 lightColor, float3 F0, float NdotH, float NdotL, float LdotH); // Specular fresnel 

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    Light light0 = frameConstants.lights[0];
    float3 V = normalize(frameConstants.eyePosition.xyz - vIn.shadingLocation);      // V is the normalized vector from the shading location to the eye
    float3 L = normalize(light0.position.xyz - vIn.shadingLocation);    // L is the normalized vector from the shading location to the light
    float3 N = vIn.normal;                                 // N is the surface normal in the same space as the above values
    float3 H = normalize(L + V);                           // H is the half vector, where H = normalize(L + V)
    float3 R = reflect(-V, N);                              // R 
    float VdotH = dot(V, H);
    float LdotH = dot(L, H);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float NdotH = dot(N, H);

    float3 dieletricSpecular = { 0.04f, 0.04f, 0.04f };
    float3 black = { 0.0f, 0.0f, 0.0f };
    float3 ambientLight = { 0.2f, 0.2f, 0.2f };

    float4 baseColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 roughMetallic = { 0.0f, 0.0f, 0.0f, 1.0f };;
    float4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 occlusion = { 0.0f, 0.0f, 0.0f, 1.0f };

    if (materials[0].baseColorTA.textureId != -1) baseColor = textures[materials[0].baseColorTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].roughMetallicTA.textureId != -1) roughMetallic = textures[materials[0].roughMetallicTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].emissiveTA.textureId != -1) emissive = textures[materials[0].emissiveTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].occlusionTA.textureId != -1) occlusion = textures[materials[0].occlusionTA.textureId].Sample(samplers[0], vIn.textCoord);

    float4 cubeMapSample = cubeMap.Sample(samplers[0], R);

    float roughness = roughMetallic.g;
    float metallic = roughMetallic.b;
    float3 albedo = baseColor.xyz;

    float3 C_diff = lerp(baseColor.rgb * (1.0f - dielectricSpecular.r), black, metallic);
    float3 F0 = lerp(dieletricSpecular, baseColor.rgb, metallic);

    float3 C_ambient = ambientLight * albedo * occlusion.xyz;
    float3 C_diffuse = diffuse(C_diff, light0.color.xyz * cubeMapSample.xyz, NdotL);

    float m = metallic * 256.0f;	// Exponent in the model for the roughness. Higher the value, more the material is shine.
    float3 C_specular = fresnel(m, cubeMapSample.xyz, F0, NdotH, NdotL, LdotH);
    float3 f = C_ambient + C_diffuse + C_specular + emissive.xyz + cubeMapSample.xyz;
    return float4(baseColor.xyz, 1.0f);
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