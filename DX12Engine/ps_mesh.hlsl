#include "mesh_common.hlsli"

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