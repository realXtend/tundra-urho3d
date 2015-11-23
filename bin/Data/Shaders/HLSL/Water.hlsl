#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Fog.hlsl"

uniform float2 cNoiseSpeed;
uniform float cNoiseTiling;
uniform float cNoiseStrength;
uniform float cFresnelPower;
uniform float3 cWaterTint;

void VS(float4 iPos : POSITION,
    float3 iNormal: NORMAL,
    float2 iTexCoord : TEXCOORD0,
    out float4 oScreenPos : TEXCOORD0,
    out float2 oWaterUV : TEXCOORD2,
    out float3 oNormal : TEXCOORD3,
    out float4 oEyeVec : TEXCOORD4,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);

    oScreenPos = GetScreenPos(oPos);
    
    oWaterUV = iTexCoord * cNoiseTiling + cElapsedTime * cNoiseSpeed;
    oNormal = GetWorldNormal(modelMatrix);
    oEyeVec = float4(cCameraPos - worldPos, GetDepth(oPos));
}

void PS(
    float4 iScreenPos : TEXCOORD0,
    float2 iWaterUV : TEXCOORD2,
    float3 iNormal : TEXCOORD3,
    float4 iEyeVec : TEXCOORD4,
    out float4 oColor : OUTCOLOR0)
{
    float2 refractUV = iScreenPos.xy / iScreenPos.w;

    float2 noise = (Sample2D(NormalMap, iWaterUV).rg - 0.5) * cNoiseStrength;
    refractUV += noise;

    float3 refractColor = Sample2D(EnvMap, refractUV).rgb * cWaterTint;
    oColor = float4(GetFog(refractColor, GetFogFactor(iEyeVec.w)), 1.0);
}