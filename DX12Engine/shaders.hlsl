// Constant buffer holding the pass constants
cbuffer cb : register(b0)
{
    float4x4 viewMtx;
    float4x4 projMtx;
    float4x4 viewProjMtx;
};

// Define the input structure
struct VertexIn
{
    float3 position : POSITION; // POSITION match the semantic name "POSITION" in the vertex descriptor
    float4 color : COLOR;       // COLOR match the semantic name "COLOR" in the vertex descriptor
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// The vertex shader 
VertexOut VSMain(VertexIn vIn)
{
    VertexOut vOut;
    vOut.position = mul(float4(vIn.position, 1.0f), viewProjMtx);
    vOut.color = vIn.color; 
    return vOut;
}

// The pixel shader
float4 PSMain(VertexOut vIn) : SV_Target // SV_Target means that the output should match the rendering target format
{
    return vIn.color;
}