#include "Common.hlsl"

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
};
 
PSInput VSMain(VS_INPUT input)
{
    PSInput Output;

    float3 posModel = input.pos;

    float4 worldPos = mul(float4(posModel, 1.0f), Model);
    Output.worldPos = worldPos.xyz;
    Output.position = mul(float4(worldPos.xyz, 0.0), View); //이동은 제거
    Output.position = mul(float4(Output.position.xyz, 1.0), Proj);

    return Output;
}

