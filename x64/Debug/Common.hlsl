struct Light
{
    float3 radiance; // Strength
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;

    matrix viewProj;
};

cbuffer GLOBAL_CONSTANT : register(b0)
{
    matrix ViewProj;
    float4 eyePos;
    
    Light light;
};

cbuffer MODEL_CONSTANT : register(b1)
{
    matrix Model;
    matrix NormalModel;
}; 

