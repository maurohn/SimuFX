// ============================================================================
// shadow_depth.hlsl — Shadow Depth Enhancement (PS 3.0)
//
// Deepens existing shadows by applying a power-curve to dark areas.
// Leaves midtones and highlights completely untouched.
// Adds richness and realism to shadows in the scene.
//
// c0: x=depth(0-1), y=threshold(luma cutoff), z=feather(blend range), w=unused
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c0 : register(c0);   // depth, threshold, feather, unused

static const float3 kLuma = float3(0.2126f, 0.7152f, 0.0722f);

float4 PS_ShadowDepth(float2 uv : TEXCOORD0) : COLOR
{
    float3 col   = tex2D(ScreenTex, uv).rgb;
    float  luma  = dot(col, kLuma);

    float depth     = c0.x;   // 0 = no change, 1 = max darkening
    float threshold = c0.y;   // luma above which effect fades out
    float feather   = c0.z;   // softness of transition

    // Shadow mask: 1.0 in pure black, fades to 0 at 'threshold' luma
    float shadowMask = saturate((threshold - luma) / max(feather, 0.001f));

    // Darken using a scaled multiply — preserves hue, just reduces brightness
    // depth=0.5 → dark pixels get multiplied by ~0.75 at threshold
    float darken = 1.0f - shadowMask * depth * 0.5f;
    float3 result = col * darken;

    return float4(saturate(result), 1.0f);
}
