// SPDX-FileCopyrightText: 2022 Salvatore Spoto <salvatore.spoto@gmail.com> 
// SPDX-License-Identifier: MIT

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
    float4 color;
    float4 position;
};

struct FrameConstants
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
    float4 eyePosition;
    int renderMode; // Bitmask that store the current render mode: 0 rendering, 1 wireframe, 2 base color, 3 rough map, 4 occlusion map, 5 emissive map
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