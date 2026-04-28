// ============================================================================
// vignette.hlsl — radial darkening towards edges
// Constant registers:
//   c8: Intensity, Radius, Softness, 0
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c8 : register(c8);

#define INTENSITY  c8.x
#define RADIUS     c8.y
#define SOFTNESS   c8.z

float4 PS_Vignette(float2 uv : TEXCOORD0) : COLOR
{
    float3 col = tex2D(ScreenTex, uv).rgb;

    // Distance from centre (0 = centre, ~0.7 = corner at 1.0)
    float2 centre = uv - 0.5f;
    float dist    = length(centre) * 1.41421356f; // normalise corner = 1.0

    // Smooth vignette mask
    float vig = smoothstep(RADIUS, RADIUS - SOFTNESS, dist);

    // Lerp from darkened to original based on intensity
    float3 result = lerp(col * (1.0f - INTENSITY), col, vig);

    return float4(result, 1.0f);
}
