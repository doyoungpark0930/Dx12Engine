#include "Common.hlsl"

cbuffer SkinnedConstants : register(b2)
{
    Matrix boneTransforms[128];
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    
     
    float4 boneWeights0 : BLENDWEIGHT0;
    uint4 boneIndices0 : BLENDINDICES0;
};

float4 VSMain(VS_INPUT input) : SV_POSITION
{
    
    float weights[4];
    weights[0] = input.boneWeights0.x;
    weights[1] = input.boneWeights0.y;
    weights[2] = input.boneWeights0.z;
    weights[3] = input.boneWeights0.w;
    
    uint indices[4];
    indices[0] = input.boneIndices0.x;
    indices[1] = input.boneIndices0.y;
    indices[2] = input.boneIndices0.z;
    indices[3] = input.boneIndices0.w;
    
    float3 posModel = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < 4; ++i)
    {
        if (indices[i] != 255)
        {
            float4x4 boneMatrix = boneTransforms[indices[i]];

            posModel += mul(float4(input.pos, 1.0), boneMatrix).xyz * weights[i];
    
        }
    }
    
    float4 pos = mul(float4(posModel, 1.0f), Model);
    return mul(pos, shadowFrustum.viewProj);
}
