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
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 tangent : TANGENT;
};
 
PSInput VSMain(VS_INPUT input)
{
    PSInput Output;
    
    float3 posModel = input.pos;
    float3 normalModel = input.normal;
    float3 tangentModel = input.tangent;

    float4 worldPos = mul(float4(posModel, 1.0f), Model);
    Output.worldPos = worldPos.xyz;
    Output.position = mul(worldPos, View);
    Output.position = mul(Output.position, Proj);
    Output.normal = normalize(mul(float4(normalModel, 0.0f), NormalModel).xyz);
    Output.tangent = mul(float4(tangentModel, 0.0f), Model).xyz;
    Output.uv = input.uv;

    return Output;
}

