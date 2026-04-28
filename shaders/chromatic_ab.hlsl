// ============================================================================
// chromatic_ab.hlsl — Chromatic Aberration + Film Grain (PS 3.0)
//
// SAFE version: UVs always clamped to [0,1] — no out-of-bounds sampling.
// CA strength is very subtle (0.001-0.004 range).
//
// c11: x=CA_Strength (0.001-0.005), y=Grain_Strength (0-0.03), z=Time
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c11 : register(c11);

#define CA_STRENGTH    c11.x
#define GRAIN_STR      c11.y
#define TIME           c11.z

// Gold noise — fast, no texture needed
float Gold_Noise(float2 xy, float seed)
{
    return frac(tan(distance(xy * 1.61803398874989484820459f,
                             float2(1.61803398874989484820459f, seed))) * 1.41421356237f);
}

float2 SafeUV(float2 uv)
{
    return clamp(uv, 0.001f, 0.999f);
}

float4 PS_ChromaticAB(float2 uv : TEXCOORD0) : COLOR
{
    // Guard: if CA is effectively off, passthrough
    if (CA_STRENGTH < 0.0001f && GRAIN_STR < 0.001f)
        return tex2D(ScreenTex, uv);

    // ---- Lens distortion offset: stronger towards screen edges ----
    float2 center = uv - 0.5f;
    float  dist2  = dot(center, center);   // 0=center, ~0.25=corners

    // Use a fixed direction per-pixel, safely handle center area
    float2 dir = (dist2 > 0.0001f) ? normalize(center) : float2(0, 0);

    // Scale strength — max offset at corners ~CA_STRENGTH pixels in UV space
    // dist2 max is 0.5, so strength*dist2*2 = strength at corner (very small)
    float strength = CA_STRENGTH * dist2 * 2.0f;

    // Sample R shifted outward, B shifted inward — ALL UVs clamped
    float r = tex2D(ScreenTex, SafeUV(uv + dir * strength)        ).r;
    float g = tex2D(ScreenTex, uv                                  ).g;
    float b = tex2D(ScreenTex, SafeUV(uv - dir * strength * 0.5f) ).b;

    float3 col = float3(r, g, b);

    // ---- Film grain (temporal — changes each frame via TIME) ----
    if (GRAIN_STR > 0.001f) {
        float grain = Gold_Noise(uv, fmod(TIME, 97.0f)) * 2.0f - 1.0f;  // [-1, 1]
        col = saturate(col + grain * GRAIN_STR);
    }

    return float4(saturate(col), 1.0f);
}
