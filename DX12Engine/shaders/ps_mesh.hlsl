#include "mesh_common.hlsli"

float3 diffuse(float3 albedo, float3 lightColor, float NdotL); // Lambertian diffuse
float3 fresnel(float m, float3 lightColor, float3 F0, float NdotH, float NdotL, float LdotH); // Specular fresnel 

// Pixel shader entry point
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    float4 baseColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 normal = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 roughMetallic = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 occlusion = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f };

    if (materials[0].baseColorTA.textureId != -1)          baseColor = textures[materials[0].baseColorTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].normalTA.textureId != -1)		       normal = textures[materials[0].normalTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].roughMetallicTA.textureId != -1)   roughMetallic = textures[materials[0].roughMetallicTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].occlusionTA.textureId != -1)           occlusion = textures[materials[0].occlusionTA.textureId].Sample(samplers[0], vIn.textCoord);
    if (materials[0].emissiveTA.textureId != -1)            emissive = textures[materials[0].emissiveTA.textureId].Sample(samplers[0], vIn.textCoord);

    if (frameConstants.renderMode & (0x1 << 1)) { return baseColor; }
    if (frameConstants.renderMode & (0x1 << 2)) { return normal; }
    if (frameConstants.renderMode & (0x1 << 3)) { return roughMetallic; }
    if (frameConstants.renderMode & (0x1 << 4)) { return occlusion; }
    if (frameConstants.renderMode & (0x1 << 5)) { return emissive; }

    Light light0 = frameConstants.lights[0];

    float3 V = normalize(frameConstants.eyePosition.xyz - vIn.shadingLocation); // From the shading location to the EYE
    float3 L = normalize(light0.position.xyz - vIn.shadingLocation);    // From the shading location to the LIGHT
    float3 N = vIn.normal; // Interpolated vertex normal
    float3 H = normalize(L + V); // Half vector between light and viewer
    float3 T = vIn.tangent.xyz; // Interpolated tangent
    T = normalize(T - dot(T, N) * N);

    normal = 2.0f * normal - 1.0f; // [0,1] ->[-1,1]
    float3 B = cross(T,N) * vIn.tangent.w; // Binormal
    float3x3 M_tw = float3x3(T,B,N); // Tangent space to world space
    N = mul(normal, M_tw); // Normal in world space
    float3 R = reflect(-V, N); // Reflected view vector

    float VdotH = dot(V, H);
    float LdotH = dot(L, H);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float NdotH = dot(N, H);

    float3 dieletricSpecular = { 0.04f, 0.04f, 0.04f };
    float3 black = { 0.0f, 0.0f, 0.0f };
    float3 ambientLight = { 0.2f, 0.2f, 0.2f };
    float4 cubeMapSample = cubeMap.Sample(samplers[0], R);
    float roughness = roughMetallic.g;
    float metallic = roughMetallic.b;
    float3 albedo = baseColor.xyz;

    float3 C_diff = lerp(baseColor.rgb * (1.0f - dielectricSpecular.r), black, metallic);
    float3 F0 = lerp(dieletricSpecular, baseColor.rgb, metallic);

    float3 C_ambient = ambientLight * albedo * occlusion.xyz;
    float3 C_diffuse = diffuse(C_diff, light0.color.xyz, NdotL);

    float m = metallic * 256.0f;	// Exponent in the model for the roughness. Higher the value, more the material is shine.
    float3 C_specular = fresnel(m, cubeMapSample.xyz , F0, NdotH, NdotL, LdotH);
    float3 f = C_ambient + C_diffuse + C_specular + emissive.xyz + cubeMapSample.xyz;

    return float4(f.xyz, 1.0f);

    //float3 c = cubeMapSample.xyz * baseColor.xyz;
      //  return float4(f.xyz, 1.0f);
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