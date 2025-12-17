SamplerState wrapSampler : register(s0);
SamplerState shadowPointSampler : register(s1);
SamplerComparisonState shadowCompareSampler : register(s2);

struct ShadowFrustum
{
    float4 radiance; // Strength
    float4 direction;
    float4 position;

    matrix viewProj;
};

cbuffer GLOBAL_CONSTANT : register(b0)
{
    matrix ViewProj;
    float4 eyePos;
    float4 lightPos;
    
    ShadowFrustum shadowFrustum;
};

cbuffer MODEL_CONSTANT : register(b1)
{
    matrix Model;
    matrix NormalModel;
}; 

