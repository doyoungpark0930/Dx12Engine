TextureCube g_textureCube : register(t0);
SamplerState g_sampler : register(s0);


struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
};


float4 PSMain(PS_INPUT input) : SV_TARGET
{ 
    float4 color = g_textureCube.Sample(g_sampler, input.worldPos);
    return color;
}