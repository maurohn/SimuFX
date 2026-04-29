// ============================================================================
// bloom_upsample.hlsl — 9-tap tent filter upsample
// Constant registers:
//   c6: RcpW, RcpH, 0, 0
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c6 : register(c6);

#define RCP_W c6.x
#define RCP_H c6.y

float4 PS_BloomUp(float2 uv : TEXCOORD0) : COLOR
{
    float2 px = float2(RCP_W, RCP_H);

    // 9-tap tent filter
    float3 col =
        tex2D(ScreenTex, uv + float2(-1,-1)*px).rgb * 1.0f +
        tex2D(ScreenTex, uv + float2( 0,-1)*px).rgb * 2.0f +
        tex2D(ScreenTex, uv + float2( 1,-1)*px).rgb * 1.0f +
        tex2D(ScreenTex, uv + float2(-1, 0)*px).rgb * 2.0f +
        tex2D(ScreenTex, uv).rgb                   * 4.0f +
        tex2D(ScreenTex, uv + float2( 1, 0)*px).rgb * 2.0f +
        tex2D(ScreenTex, uv + float2(-1, 1)*px).rgb * 1.0f +
        tex2D(ScreenTex, uv + float2( 0, 1)*px).rgb * 2.0f +
        tex2D(ScreenTex, uv + float2( 1, 1)*px).rgb * 1.0f;

    col /= 16.0f;

    return float4(col, 1.0f);
}
