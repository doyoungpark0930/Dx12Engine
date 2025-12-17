#include "Common.hlsl"


struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

float4 VSMain(VS_INPUT input) : SV_POSITION
{
    float4 pos = mul(float4(input.pos, 1.0f), Model);
    return mul(pos, shadowFrustum.viewProj);
}
