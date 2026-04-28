// ============================================================================
// bloom_composite.hlsl — screen-blend scene + bloom layer
// Constant registers:
//   c5: Threshold, Intensity, Radius, SoftKnee
// Samplers:
//   s0 = scene texture (full-res)
//   s1 = bloom texture (half-res, bilinear upsample)
// ============================================================================

sampler2D SceneTex : register(s0);
sampler2D BloomTex : register(s1);

float4 c5 : register(c5);

#define INTENSITY c5.y
#define RADIUS    c5.z

float4 PS_BloomComposite(float2 uv : TEXCOORD0) : COLOR
{
    float3 scene = tex2D(SceneTex, uv).rgb;
    float3 bloom = tex2D(BloomTex, uv).rgb;

    // Screen blend: 1-(1-A)*(1-B) — avoids clipping unlike additive
    float3 result = 1.0f - (1.0f - scene) * (1.0f - bloom * INTENSITY * RADIUS);

    return float4(saturate(result), 1.0f);
}
