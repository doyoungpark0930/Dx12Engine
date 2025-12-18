SamplerState wrapSampler : register(s0);
SamplerState shadowPointSampler : register(s1);
SamplerComparisonState shadowCompareSampler : register(s2);


TextureCube envIBLTex : register(t15);
TextureCube specularIBLTex : register(t16);
TextureCube irradianceIBLTex : register(t17);
Texture2D brdfTex : register(t18);

struct ShadowFrustum
{
    float4 radiance; // Strength
    float4 direction;
    float4 position;

    matrix viewProj;
};

cbuffer GLOBAL_CONSTANT : register(b0)
{
    matrix View;
    matrix Proj;
    float4 eyePos;
    float4 lightPos;
    
    ShadowFrustum shadowFrustum;
    float strengthIBL;
};

cbuffer MODEL_CONSTANT : register(b1)
{
    matrix Model;
    matrix NormalModel;
}; 

cbuffer MATERIAL_CONSTANT : register(b3)
{
    int useAlbedoTex;
    int useAoTex;
    int useNormalTex;
    int useMetallicTex;
    int useRoughnessTex;
    int useGlossinessTex;
    
    int useShadowMap;

}