// ============================================================================
// tonemap.hlsl — SimuFX Tonemap (PS 3.0)
//
// Works in gamma space — no linearization. Hue-safe operations.
//
// c3: x=Exposure,  y=Gamma,    z=Contrast,   w=Highlights
// c4: x=Shadows,   y=Filmic(0/1)
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c3 : register(c3);
float4 c4 : register(c4);

#define EXPOSURE   c3.x   // EV stops (-1 to +1)
#define GAMMA_OUT  c3.y   // output gamma (1.0 = neutral)
#define CONTRAST   c3.z   // S-curve strength (1.0 = neutral)
#define HIGHLIGHTS c3.w   // highlight compression (1.0 = neutral)
#define SHADOWS    c4.x   // shadow lift multiplier (1.0 = neutral, >1 = brighter blacks)
#define FILMIC     c4.y   // 0=off, 1=gentle Reinhard on highlights

float4 PS_Tonemap(float2 uv : TEXCOORD0) : COLOR
{
    float3 col = tex2D(ScreenTex, uv).rgb;

    // 1. Exposure — EV stops, hue neutral
    if (abs(EXPOSURE) > 0.005f)
        col *= pow(2.0f, EXPOSURE);

    // 2. Shadow lift — raises the black point without affecting mids/highlights
    //    SHADOWS > 1 lifts blacks; SHADOWS = 1 is neutral.
    if (SHADOWS > 1.005f) {
        float luma = dot(col, float3(0.2126f, 0.7152f, 0.0722f));
        // shadowMask: 1 in pure black, fades to 0 at luma=0.5+
        float shadowMask = saturate(1.0f - luma * 2.0f);
        float lift = (SHADOWS - 1.0f) * 0.08f;   // convert to additive offset
        col = saturate(col + lift * shadowMask);
    }

    // 3. Highlight compression — reduces overexposed areas
    //    HIGHLIGHTS < 1 compresses highlights; 1.0 = neutral
    if (abs(HIGHLIGHTS - 1.0f) > 0.005f) {
        float luma = dot(col, float3(0.2126f, 0.7152f, 0.0722f));
        float hiMask = saturate((luma - 0.5f) * 2.0f);  // 0 in mids, 1 in bright
        col = lerp(col, col * HIGHLIGHTS, hiMask);
    }

    // 4. Filmic curve — Reinhard applied to LUMA only (hue-safe, no color shifts)
    if (FILMIC > 0.5f) {
        float luma = dot(col, float3(0.2126f, 0.7152f, 0.0722f));
        if (luma > 0.001f) {
            // Shifted Reinhard: luma_out = luma/(luma+0.85)*1.85
            // This curve only compresses the top 15-20% — mids are mostly untouched
            float luma_out = (luma / (luma + 0.85f)) * 1.85f;
            col *= (luma_out / luma);
        }
    }

    col = saturate(col);

    // 5. Contrast S-curve (centred at 0.5)
    if (abs(CONTRAST - 1.0f) > 0.005f)
        col = saturate((col - 0.5f) * CONTRAST + 0.5f);

    // 6. Gamma output tweak
    if (abs(GAMMA_OUT - 1.0f) > 0.005f)
        col = pow(col, 1.0f / GAMMA_OUT);

    return float4(col, 1.0f);
}
