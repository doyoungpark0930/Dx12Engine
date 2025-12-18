#include "Common.hlsl"

Texture2D albedoTex : register(t0);
Texture2D aoTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D metallicTex : register(t3);
Texture2D roughnessTex : register(t4);

Texture2D shadowMaps : register(t8);

static const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0

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
    if (useNormalTex)
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

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;

    return alphaSq / (3.141592 * denom * denom);
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float SchlickGGX(float NdotI, float NdotO, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH); //계산을 좀더 빠르게 하기 위함. 차이는 별로 없음
    //return F0 + (1.0 - F0) * pow(1.0 - NdotH, 5.0);
}

//roughness를 고려한 Fresnel공식
float3 fresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye,
                  float metallic, float roughness)//kd
{
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = fresnelSchlickRoughness(F0, max(0.0, dot(normalWorld, pixelToEye)), roughness);
    float3 kd = lerp(1.0 - F, 0.0, metallic); //즉 반사를 뺀 나머지 에너지가 디퓨즈로 간다는 뜻, 1- ks
    float3 irradiance = irradianceIBLTex.Sample(wrapSampler, normalWorld).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye,
                   float metallic, float roughness)//ks
{
    float2 specularBRDF = brdfTex.Sample(wrapSampler, float2(max(0.0, dot(normalWorld, pixelToEye)), roughness)).rg;
    float3 specularIrradiance = specularIBLTex.SampleLevel(wrapSampler, reflect(-pixelToEye, normalWorld),
                                                             2 + roughness * 5.0f).rgb; //원래 2 + roughenss*5.0f
    const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    float3 F = fresnelSchlickRoughness(F0, max(0.0, dot(normalWorld, pixelToEye)), roughness);

    return (F * specularBRDF.x + specularBRDF.y) * specularIrradiance; //(F0 * specularBRDF.x + specularBRDF.y) 이 계산애 ks포함. 따라서 ks 안곱함
    //F*D*G를 다 계산하면 느리기 때문에 BRDF에서 가져오는 것
}

float3 AmbientLightingByIBL(float3 albedo, float3 normalW, float3 pixelToEye, float ao,
                            float metallic, float roughness)
{
    float3 diffuseIBL = DiffuseIBL(albedo, normalW, pixelToEye, metallic, roughness);
    float3 specularIBL = SpecularIBL(albedo, normalW, pixelToEye, metallic, roughness);
    
    return (diffuseIBL + specularIBL) * ao;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float3 pixelToEye = normalize(eyePos.xyz - input.worldPos);
    float3 normalWorld = GetNormal(input);
    
    
    float albedoFactor = 1.0f;
    float aoFactor = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float3 albedo = useAlbedoTex ? albedoTex.Sample(wrapSampler, input.uv).rgb * albedoFactor
                                 : albedoFactor;
    float ao = useAoTex ? aoTex.Sample(wrapSampler, input.uv).rgb * aoFactor : aoFactor;
    float metallic = useMetallicTex ? metallicTex.Sample(wrapSampler, input.uv).rgb * metallicFactor
                                    : metallicFactor;
    metallic = 0.0f;
    float roughness = useRoughnessTex ? roughnessTex.Sample(wrapSampler, input.uv).rgb * roughnessFactor :
                       useGlossinessTex ? 1 - roughnessTex.Sample(wrapSampler, input.uv).rgb * roughnessFactor :
                        1.0f;

    
    float3 ambientLighting = AmbientLightingByIBL(albedo, normalWorld, pixelToEye, ao, 0.0, roughness) * strengthIBL;
    ambientLighting += albedo * 0.3f; //너무어두워서 좀 넣어줌
    float3 directLighting = 0.0;
    
    [unroll] 
    for (int i = 0; i < 1; ++i) //적분대신 모든 광원에 대하여 한번 씩 돌음
    {
 
        float3 lightVec = lightPos.xyz - input.worldPos; //wi = L
           
            
        float lightDist = length(lightVec);
        lightVec /= lightDist;
        
        
        float3 halfway = normalize(pixelToEye + lightVec);
        
        float NdotI = max(0.0, dot(normalWorld, lightVec));
        float NdotH = max(0.0, dot(normalWorld, halfway));
        float NdotO = max(0.0, dot(normalWorld, pixelToEye));
        
        const float3 Fdielectric = 0.04; // 비금속(Dielectric) 재질의 F0
        float3 F0 = lerp(Fdielectric, albedo, metallic);
        float3 F = SchlickFresnel(F0, max(0.0, dot(halfway, pixelToEye)));
        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
        float3 diffuseBRDF = kd * albedo / 3.1415;

        float D = NdfGGX(NdotH, roughness);
        float3 G = SchlickGGX(NdotI, NdotO, roughness);
        float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO); //cook torrance
            //1e-5는 나눗셈 오차를 피하기 위함

            
        float shadowFactor = useShadowMap ? ComputeShadow(input.worldPos) : 1.0f;
                
        directLighting += (diffuseBRDF + specularBRDF) * shadowFactor * NdotI; //NdotI는 cos theta 즉, N과 빛과의 각도
        
    }
    
    
    
    float4 color = float4(ambientLighting * 0.4 + directLighting * 2.0, 1.0);
    
    //color.rgb = pow(color.rgb, 1.0 / 2.2);
    return color;
}