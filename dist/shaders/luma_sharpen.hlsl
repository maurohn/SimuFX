// ============================================================================
// luma_sharpen.hlsl — Unsharp-mask based luma sharpening (fallback)
// Constant registers:
//   c7: Strength, Clamp, RcpW, RcpH
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c7 : register(c7);

#define STRENGTH c7.x
#define CLAMP_V  c7.y
#define RCP_W    c7.z
#define RCP_H    c7.w

float3 Blur5(float2 uv)
{
    float2 px = float2(RCP_W, RCP_H);
    float3 sum =
        tex2D(ScreenTex, uv + float2(-1,-1)*px).rgb * 0.0625f +
        tex2D(ScreenTex, uv + float2( 0,-1)*px).rgb * 0.125f  +
        tex2D(ScreenTex, uv + float2( 1,-1)*px).rgb * 0.0625f +
        tex2D(ScreenTex, uv + float2(-1, 0)*px).rgb * 0.125f  +
        tex2D(ScreenTex, uv).rgb                   * 0.25f   +
        tex2D(ScreenTex, uv + float2( 1, 0)*px).rgb * 0.125f  +
        tex2D(ScreenTex, uv + float2(-1, 1)*px).rgb * 0.0625f +
        tex2D(ScreenTex, uv + float2( 0, 1)*px).rgb * 0.125f  +
        tex2D(ScreenTex, uv + float2( 1, 1)*px).rgb * 0.0625f;
    return sum;
}

float4 PS_LumaSharpen(float2 uv : TEXCOORD0) : COLOR
{
    float3 col  = tex2D(ScreenTex, uv).rgb;
    float3 blur = Blur5(uv);
    float3 diff = col - blur;

    // Clamp halos
    diff = clamp(diff, -CLAMP_V, CLAMP_V);

    float3 result = col + diff * STRENGTH;
    return float4(saturate(result), 1.0f);
}
