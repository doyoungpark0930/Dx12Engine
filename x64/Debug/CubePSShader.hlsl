#include "Common.hlsl"



struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
};


float4 PSMain(PS_INPUT input) : SV_TARGET
{ 
    float3 color = envIBLTex.Sample(wrapSampler, input.worldPos).xyz;
    
    float exposure = 1.0f;
    color *= strengthIBL;
    color = clamp(color * exposure, 0.0, 1.0);
    color = pow(color, 1.0 / 2.2);

    return float4(color, 1.0);
}