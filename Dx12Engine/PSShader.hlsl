#include "Common.hlsl"

Texture2D albedoTex : register(t0);
Texture2D aoTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);

Texture2D shadowMaps : register(t8);


cbuffer MATERIAL_CONSTANT : register(b3)
{
    int useNormalMap;
    int useShadowMap;
}

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 tangent : TANGENT;
};

float3 GetNormal(PS_INPUT input)
{
    float3 normalWorld = normalize(input.normal);
    
    if (useNormalMap)
    {
        float3 normal = normalTex.Sample(wrapSampler, input.uv).rgb;
        normal = 2.0 * normal - 1.0; // 범위 조절 [-1.0, 1.0]
            
        float3 N = normalWorld;
        float3 T = normalize(input.tangent - dot(input.tangent, N) * N);
        float3 B = cross(N, T);
        
        // matrix는 float4x4, 여기서는 벡터 변환용이라서 3x3 사용
        float3x3 TBN = float3x3(T, B, N);
        normalWorld = normalize(mul(normal, TBN));
    }
    
    return normalWorld;
}

float ComputeShadow(float3 worldPos)
{
    
    float shadowFactor = 1.0;
    
    // 1. World → Light clip space
    float4 lightClip = mul(float4(worldPos, 1.0), shadowFrustum.viewProj);

    // 2. Perspective divide
    lightClip.xyz /= lightClip.w;
    
     // Light NDC 범위 체크 (-1 ~ 1)
    if (lightClip.x < -1.0 || lightClip.x > 1.0 ||
        lightClip.y < -1.0 || lightClip.y > 1.0 ||
        lightClip.z < 0.0 || lightClip.z > 1.0)
    {
        // light projection 범위 밖 → 일반 렌더링
        return 1.0;
    }

    lightClip.y *= -1;
    float2 lightTexcoord = (lightClip.xy + 1.0) / 2.0;

    
     // 4. texel size
    uint width, height, mip;
    shadowMaps.GetDimensions(0, width, height, mip);
    float2 texelSize = 1.0 / float2(width, height);
    
    // 5. PCF
    float shadow = 0.0;
    const float bias = 0.00001;
    
    const float2 poissonDisk[16] =
    {
        float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
            float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
            float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
            float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
            float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
            float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
            float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
            float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
    };

    [unroll]
    for (int i = 0; i < 16; i++)
    {
        if (lightTexcoord.x < 0.05 || lightTexcoord.x > 0.95 || lightTexcoord.y < 0.05 || lightTexcoord.y > 0.95)
        {
            shadow += 1.0; //shadow frustum에 경계선 부분은 pcf하지 않는다
        }
        else
        {
            shadow += shadowMaps.SampleCmpLevelZero(shadowCompareSampler, lightTexcoord + poissonDisk[i] * texelSize, lightClip.z - bias).r;
        }
        //shadow += shadowMaps.SampleCmpLevelZero(shadowCompareSampler, lightTexcoord + poissonDisk[i] * texelSize, lightClip.z - bias).r;
        //ndc판정 주석하고 이 주석shadow로 하면 어디에 frustum 맺히는지 보임

    }

    // 평균
    shadow /= 16.0;

    return shadow;
    
    
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    
    float3 normalWorld = GetNormal(input);
    
    float3 lightDir = normalize(lightPos.xyz - input.worldPos);
    float3 viewDir = normalize(eyePos.xyz - input.worldPos);
    float3 reflectDir = reflect(-lightDir, normalWorld);

    // Ambient
    float3 ambient = float3(0.7f, 0.7f, 0.7f);
      
    // Diffuse
    float diff = max(dot(normalWorld, lightDir), 0.0f);
    float3 diffuse = diff * float3(1.0f, 1.0f, 1.0f);
    // Specular
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 5.0f); // shininess
    float3 specular = spec * float3(1.0f, 1.0f, 1.0f);
    
    float shadow = useShadowMap ? ComputeShadow(input.worldPos) : 1.0f;

    float3 lighting = ambient + (diffuse + specular) * shadow * 0.7f;
    float4 color = albedoTex.Sample(wrapSampler, input.uv);

    return float4(color.rgb * lighting, color.a);
}