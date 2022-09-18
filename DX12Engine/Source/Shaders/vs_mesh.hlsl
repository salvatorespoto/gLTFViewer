#include "mesh_common.hlsli"

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
