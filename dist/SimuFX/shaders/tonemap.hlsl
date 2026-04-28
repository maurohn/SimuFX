// ============================================================================
// tonemap.hlsl — Exposure + Gamma + Contrast + Highlights/Shadows + Filmic
// Constant registers:
//   c3: Exposure, Gamma, Contrast, Highlights
//   c4: Shadows, Filmic(0/1), 0, 0
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c3 : register(c3);
float4 c4 : register(c4);

#define EXPOSURE   c3.x
#define GAMMA      c3.y
#define CONTRAST   c3.z
#define HIGHLIGHTS c3.w
#define SHADOWS    c4.x
#define USE_FILMIC c4.y

// ACES Filmic approximation (Krzysztof Narkowicz)
float3 ACES(float3 x)
{
    float a = 2.51f, b = 0.03f, c2 = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c2*x+d)+e));
}

// Smooth S-curve contrast
float3 Contrast(float3 col, float contrast)
{
    return saturate((col - 0.5f) * contrast + 0.5f);
}

// Highlight compression (rolls off bright areas gently)
float3 CompressHighlights(float3 col, float hl)
{
    return col / (col + (1.0f - hl) + 0.001f) * (1.0f + (1.0f - hl));
}

// Shadow lift
float3 LiftShadows(float3 col, float sh)
{
    return col * sh + max(col - 0.5f, 0) * (1.0f - sh);
}

float4 PS_Tonemap(float2 uv : TEXCOORD0) : COLOR
{
    float3 col = tex2D(ScreenTex, uv).rgb;

    // 1. Exposure
    col *= exp2(EXPOSURE);

    // 2. Shadow lift
    col = LiftShadows(col, SHADOWS);

    // 3. Highlight compression
    col = CompressHighlights(col, HIGHLIGHTS);

    // 4. Filmic / linear tonemap
    if (USE_FILMIC > 0.5f)
        col = ACES(col);
    else
        col = saturate(col);

    // 5. Gamma
    col = pow(max(col, 0.0001f), 1.0f / max(GAMMA, 0.01f));

    // 6. Contrast
    col = Contrast(col, CONTRAST);

    return float4(col, 1.0f);
}
